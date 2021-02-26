/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ap2_qmk_led.h"
#include "board.h"
#include "ch.h"
#include "hal.h"
#include "light_utils.h"
#include "miniFastLED.h"
#include "profiles.h"
#include "string.h"

static void animationCallback(void);
static void executeMsg(msg_t msg);
static void executeProfile(bool init);
static void disableLeds(void);
static void enableLeds(void);
static void ledSet(void);
static void ledSetRow(void);
static void setProfile(void);
static void changeMask(uint8_t mask);
static void nextIntensity(void);
static void nextSpeed(void);
static void setForegroundColor(void);
static void clearForegroundColor(void);
static void handleKeypress(msg_t msg);
static void setIAP(void);
static void mainCallback(GPTDriver *_driver);

ioline_t ledColumns[NUM_COLUMN] = {
    LINE_LED_COL_1,  LINE_LED_COL_2,  LINE_LED_COL_3,  LINE_LED_COL_4,
    LINE_LED_COL_5,  LINE_LED_COL_6,  LINE_LED_COL_7,  LINE_LED_COL_8,
    LINE_LED_COL_9,  LINE_LED_COL_10, LINE_LED_COL_11, LINE_LED_COL_12,
    LINE_LED_COL_13, LINE_LED_COL_14};

ioline_t ledRows[NUM_ROW * 4] = {
    LINE_LED_ROW_1_R, LINE_LED_ROW_1_G, LINE_LED_ROW_1_B, 0,
    LINE_LED_ROW_2_R, LINE_LED_ROW_2_G, LINE_LED_ROW_2_B, 0,
    LINE_LED_ROW_3_R, LINE_LED_ROW_3_G, LINE_LED_ROW_3_B, 0,
    LINE_LED_ROW_4_R, LINE_LED_ROW_4_G, LINE_LED_ROW_4_B, 0,
    LINE_LED_ROW_5_R, LINE_LED_ROW_5_G, LINE_LED_ROW_5_B, 0,
};

#define REFRESH_FREQUENCY 200

#define KEY_COUNT 70

#define LEN(a) (sizeof(a) / sizeof(*a))

/*
 * Active profiles
 * Add profiles from source/profiles.h in the profile array
 */
typedef void (*lighting_callback)(led_t *, uint8_t);

/*
 * keypress handler
 */
typedef void (*keypress_handler)(led_t *colors, uint8_t row, uint8_t col,
                                 uint8_t intensity);

typedef void (*profile_init)(led_t *colors);

typedef struct {
  // callback function implementing the lighting effect
  lighting_callback callback;
  // For static effects, their `callback` is called only once.
  // For dynamic effects, their `callback` is called in a loop.
  // This field controls the animation speed by specifying
  // how many ticks to skip before calling the callback again.
  // For example, 8000 in the array means that `callback` is called on every
  // 8000th tick. Different 4 values can be specified to allow different
  // speeds of the same effect. For static effects, the array must contain {0,
  // 0, 0, 0}.
  uint16_t animationSpeed[4];
  // In case the profile is reactive, it responds to each keypress.
  // This callback is called with the locations of the pressed keys.
  keypress_handler keypressCallback;
  // Some profiles might need additional setup when just enabled.
  // This callback defines such logic if needed.
  profile_init profileInit;
} profile;

profile profiles[] = {
    {red, {0, 0, 0, 0}, NULL, NULL},
    {green, {0, 0, 0, 0}, NULL, NULL},
    {blue, {0, 0, 0, 0}, NULL, NULL},
    {rainbowHorizontal, {0, 0, 0, 0}, NULL, NULL},
    {rainbowVertical, {0, 0, 0, 0}, NULL, NULL},
    {animatedRainbowVertical, {3000, 2000, 1200, 600}, NULL, NULL},
    {animatedRainbowFlow, {1000, 400, 200, 140}, NULL, NULL},
    {animatedRainbowWaterfall, {1600, 1200, 800, 400}, NULL, NULL},
    {animatedBreathing, {1600, 1200, 800, 400}, NULL, NULL},
    {animatedWave, {1200, 800, 400, 200}, NULL, NULL},
    {animatedSpectrum, {1600, 1200, 800, 400}, NULL, NULL},
    {reactiveFade,
     {400, 1600, 1200, 800},
     reactiveFadeKeypress,
     reactiveFadeInit},
    {reactivePulse,
     {400, 1600, 1200, 800},
     reactivePulseKeypress,
     reactivePulseInit}};

static uint8_t currentProfile = 0;
static const uint8_t amountOfProfiles = sizeof(profiles) / sizeof(profile);
static volatile uint8_t currentSpeed = 0;
static volatile uint16_t animationSkipTicks = 0;
static uint32_t animationLastCallTime = 0;

static const GPTConfig bftm0Config = {.frequency = 25000,
                                      .callback = mainCallback};

static mutex_t mtx;

// each color from RGB is rightshifted by this amount
// default zero corresponds to full intensity, max 3 correponds to 1/8 of color
static uint8_t ledIntensity = 0;

static volatile bool ledEnabled = false;

// Flag to check if there is a foreground color currently active
static bool foregroundColorSet = false;
static uint32_t foregroundColor = 0;

uint8_t ledMasks[KEY_COUNT];
led_t ledColors[KEY_COUNT];
static uint16_t currentCol = 0;

static const SerialConfig usart1Config = {.speed = 115200};

static uint8_t commandBuffer[64];

void updateAnimationSpeed(void) {
  animationSkipTicks = profiles[currentProfile].animationSpeed[currentSpeed];
  animationLastCallTime = 0;
}

/*
 * Reactive profiles are profiles which react to keypresses.
 * This helper is used to notify the main controller that
 * the current profile is reactive and coordinates of pressed
 * keys should be sent to LED controller.
 */
void forwardReactiveFlag(void) {
  uint8_t isReactive = 0;
  if (profiles[currentProfile].keypressCallback != NULL) {
    isReactive = 1;
  }
  sdWrite(&SD1, &isReactive, 1);
}

/*
 * Execute action based on a message
 */
static inline void executeMsg(msg_t msg) {
  switch (msg) {
  case CMD_LED_ON:
    enableLeds();
    break;
  case CMD_LED_OFF:
    disableLeds();
    break;
  case CMD_LED_SET:
    ledSet();
    break;
  case CMD_LED_SET_ROW:
    ledSetRow();
    break;
  case CMD_LED_SET_PROFILE:
    setProfile();
    forwardReactiveFlag();
    break;
  case CMD_LED_NEXT_PROFILE:
    currentProfile = (currentProfile + 1) % amountOfProfiles;
    executeProfile(true);
    forwardReactiveFlag();
    break;
  case CMD_LED_PREV_PROFILE:
    currentProfile =
        (currentProfile + (amountOfProfiles - 1u)) % amountOfProfiles;
    executeProfile(true);
    forwardReactiveFlag();
    break;
  case CMD_LED_GET_PROFILE:
    sdWrite(&SD1, &currentProfile, 1);
    break;
  case CMD_LED_GET_NUM_PROFILES:
    sdWrite(&SD1, &amountOfProfiles, 1);
    break;
  case CMD_LED_SET_MASK:
    changeMask(0xFF);
    break;
  case CMD_LED_CLEAR_MASK:
    changeMask(0x00);
    break;
  case CMD_LED_NEXT_INTENSITY:
    nextIntensity();
    break;
  case CMD_LED_NEXT_ANIMATION_SPEED:
    nextSpeed();
    break;
  case CMD_LED_SET_FOREGROUND_COLOR:
    setForegroundColor();
    break;
  case CMD_LED_CLEAR_FOREGROUND_COLOR:
    clearForegroundColor();
    forwardReactiveFlag();
    break;
  case CMD_LED_IAP:
    setIAP();
    break;
  default:
    if (msg & 0b10000000) {
      handleKeypress(msg);
    }
    break;
  }
}

void setIAP() {

  // Magic key to set keyboard to IAP
  *((uint32_t *)0x20001ffc) = 0x0000fab2;

  __disable_irq();
  NVIC_SystemReset();
}

void changeMask(uint8_t mask) {
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, 1, 10000);

  if (bytesRead == 1) {
    if (commandBuffer[0] < KEY_COUNT) {
      ledMasks[commandBuffer[0]] = mask;
    }
  }
}

void nextIntensity() {
  ledIntensity = (ledIntensity + 1) % 4;
  executeProfile(false);
}

void nextSpeed() {
  currentSpeed = (currentSpeed + 1) % 4;
  foregroundColorSet = false;
  updateAnimationSpeed();
}

/*
 * The message contains 1 flag bit which is always set
 * and then 3 bits of row and 4 bits of col.
 * Because this callback is called on every keypress,
 * the data is packed into a single byte to decrease the data traffic.
 */
inline void handleKeypress(msg_t msg) {
  uint8_t row = (msg >> 4) & 0b111;
  uint8_t col = msg & 0b1111;
  keypress_handler handler = profiles[currentProfile].keypressCallback;
  if (handler != NULL && row < NUM_ROW && col < NUM_COLUMN) {
    handler(ledColors, row, col, ledIntensity);
  }
}

/*
 * Set all the leds to the specified color
 */
void setForegroundColor() {
  size_t bytesRead;
  bytesRead = sdRead(&SD1, commandBuffer, 3);

  if (bytesRead >= 3) {
    uint8_t colorBytes[4] = {commandBuffer[2], commandBuffer[1],
                             commandBuffer[0], 0x00};
    foregroundColor = *(uint32_t *)&colorBytes;
    foregroundColorSet = true;

    setAllKeysColor(ledColors, foregroundColor, ledIntensity);
  }
}

// In case we switched to a new profile, the mainCallback
// should call the profile handler initially when this flag is set to true.
bool needToCallbackProfile = false;

void clearForegroundColor() {
  foregroundColorSet = false;

  if (animationSkipTicks == 0) {
    // If the current profile is static, we need to reset its colors
    // to what it was before the background color was activated.
    memset(ledColors, 0, sizeof(ledColors));
    needToCallbackProfile = true;
  } else if (profiles[currentProfile].keypressCallback != NULL) {
    /* Check if current profile is reactive. If it is, clear the colors. Not
     * doing it will keep the foreground color if it is a reactive profile This
     * might cause a split blackout with reactive profiles in the future if they
     * have also have static colors/animation.
     */
    memset(ledColors, 0, sizeof(ledColors));
  }
}

/*
 * Set profile and execute it
 */
void setProfile() {
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, 1, 10000);

  if (bytesRead == 1) {
    if (commandBuffer[0] < amountOfProfiles) {
      foregroundColorSet = false;
      currentProfile = commandBuffer[0];
      executeProfile(true);
    }
  }
}

/*
 * Execute current profile
 */
void executeProfile(bool init) {
  // Here we disable the foreground to ensure the animation will run
  foregroundColorSet = false;

  profile_init pinit = profiles[currentProfile].profileInit;
  if (init && pinit != NULL) {
    pinit(ledColors);
  }

  updateAnimationSpeed();

  needToCallbackProfile = true;
}

/*
 * Turn off all leds
 */
static inline void disableLeds() {

  chMtxLock(&mtx);

  ledEnabled = false;

  // stop timer, clock is still enabled
  if (GPTD_BFTM0.state == GPT_CONTINUOUS) {
    gptStopTimer(&GPTD_BFTM0);
  }
  // enter low power mode
  if (GPTD_BFTM0.state == GPT_READY) {
    gptStop(&GPTD_BFTM0);
  }

  palClearLine(LINE_LED_PWR);

  for (int i = 0; i < NUM_ROW * 4; i++) {
    if (i % 4 != 3) {
      palClearLine(ledRows[i]);
    }
  }
  for (int i = 0; i < NUM_COLUMN; i++) {
    palClearLine(ledColumns[i]);
  }

  chMtxUnlock(&mtx);
}

/*
 * Turn on all leds
 */
static inline void enableLeds(void) {
  chMtxLock(&mtx);
  ledEnabled = true;

  executeProfile(true);
  palSetLine(LINE_LED_PWR);

  // start PWM handling interval
  gptStart(&GPTD_BFTM0, &bftm0Config);
  gptStartContinuous(&GPTD_BFTM0, 1);

  chMtxUnlock(&mtx);
}

/*
 * Set a led based on qmk communication
 */
void ledSet() {
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, 4, 10000);

  if (bytesRead >= 4) {
    if (commandBuffer[0] < NUM_ROW && commandBuffer[1] < NUM_COLUMN) {
      setKeyColor(&ledColors[commandBuffer[0] * NUM_COLUMN + commandBuffer[1]],
                  ((uint16_t)commandBuffer[3] << 8 | commandBuffer[2]),
                  ledIntensity);
    }
  }
}

/*
 * Set a row of leds based on qmk communication
 */
void ledSetRow() {
  size_t bytesRead;
  bytesRead =
      sdReadTimeout(&SD1, commandBuffer, sizeof(led_t) * NUM_COLUMN + 1, 1000);
  if (bytesRead >= sizeof(led_t) * NUM_COLUMN + 1) {
    if (commandBuffer[0] < NUM_ROW) {
      memcpy(&ledColors[commandBuffer[0] * NUM_COLUMN], &commandBuffer[1],
             sizeof(led_t) * NUM_COLUMN);
    }
  }
}

inline uint8_t min(uint8_t a, uint8_t b) { return a <= b ? a : b; }

/*
 * Update lighting table as per animation
 */
static inline void animationCallback() {

  // If the foreground is set we skip the animation as a way to avoid it
  // overrides the foreground
  if (foregroundColorSet) {
    return;
  }

  profiles[currentProfile].callback(ledColors, ledIntensity);
}

static inline void sPWM(uint8_t cycle, uint8_t currentCount, ioline_t port) {
  if (cycle > currentCount) {
    palSetLine(port);
  }
}

uint8_t rowPWMCount = 0;

// mainCallback is responsible for 2 things:
// * software PWM
// * calling animation callback for animated profiles
void mainCallback(GPTDriver *_driver) {
  (void)_driver;

  if (ledEnabled) {

    palClearLine(ledColumns[currentCol]);
    for (int i = 0; i < NUM_ROW * 4; i++) {
      if (i % 4 != 3) {
        palClearLine(ledRows[i]);
      }
    }

    if (needToCallbackProfile) {
      needToCallbackProfile = false;
      profiles[currentProfile].callback(ledColors, ledIntensity);
    } else {
      bool animationCalled = false;
      if (animationSkipTicks > 0) {
        // animation update logic
        uint32_t curTime = chVTGetSystemTimeX();
        // curTime wraps around when overflows, hence the check for "less"
        if (curTime < animationLastCallTime ||
            curTime - animationLastCallTime >= animationSkipTicks) {
          animationCalled = true;
          animationCallback();
          animationLastCallTime = curTime;
        }
      }
      if (!animationCalled) {
        currentCol = (currentCol + 1) % NUM_COLUMN;

        rowPWMCount += 63;
        if (rowPWMCount == 255) {
          rowPWMCount += 63;
        }

        for (size_t row = 0; row < NUM_ROW * 4 - 1; row++) {
          if (row % 4 != 3) {
            const size_t ledIndex = currentCol + (NUM_COLUMN * (row / 4));

            const led_t keyLED = ledColors[ledIndex];
            uint8_t cl;
            uint8_t delta = 0;

            if (row % 4 == 0) {
              cl = keyLED.red;
              delta = 0;
            } else if (row % 4 == 1) {
              cl = keyLED.green;
              delta = 85;
            } else {
              cl = keyLED.blue;
              delta = 170;
            }
            sPWM(cl, rowPWMCount + delta, ledRows[row]);
          }

          palSetLine(ledColumns[currentCol]);
        }
      }
    }
  }
}

/*
 * Application entry point.
 */
int main(void) {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /* profiles[currentProfile].callback(ledColors, ledIntensity); */
  updateAnimationSpeed();

  // Setup masks to all be 0xFF at the start
  memset(ledMasks, 0xFF, sizeof(ledMasks));

  chMtxObjectInit(&mtx);

  palClearLine(LINE_LED_PWR);
  sdStart(&SD1, &usart1Config);

  chThdSetPriority(HIGHPRIO);

  // start the handler for commands coming from the main MCU
  while (true) {
    msg_t msg;
    msg = sdGet(&SD1);
    if (msg >= MSG_OK) {
      executeMsg(msg);
    }
  }
}
