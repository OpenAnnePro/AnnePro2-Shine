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
static void pwmRowDimmer(void);
static void pwmNextColumn(void);
static void mainCallback(GPTDriver *_driver);

ioline_t ledColumns[NUM_COLUMN] = {
    LINE_LED_COL_1,  LINE_LED_COL_2,  LINE_LED_COL_3,  LINE_LED_COL_4,
    LINE_LED_COL_5,  LINE_LED_COL_6,  LINE_LED_COL_7,  LINE_LED_COL_8,
    LINE_LED_COL_9,  LINE_LED_COL_10, LINE_LED_COL_11, LINE_LED_COL_12,
    LINE_LED_COL_13, LINE_LED_COL_14};

ioline_t ledRows[NUM_ROW * 3] = {
    LINE_LED_ROW_1_R, LINE_LED_ROW_1_G, LINE_LED_ROW_1_B,

    LINE_LED_ROW_2_R, LINE_LED_ROW_2_G, LINE_LED_ROW_2_B,

    LINE_LED_ROW_3_R, LINE_LED_ROW_3_G, LINE_LED_ROW_3_B,

    LINE_LED_ROW_4_R, LINE_LED_ROW_4_G, LINE_LED_ROW_4_B,

    LINE_LED_ROW_5_R, LINE_LED_ROW_5_G, LINE_LED_ROW_5_B,
};

#define KEY_COUNT 70

#define LEN(a) (sizeof(a) / sizeof(*a))

/*
 * Active profiles
 * Add profiles from source/profiles.h in the profile array
 */
typedef void (*lighting_callback)(led_t *);

/*
 * keypress handler
 */
typedef void (*keypress_handler)(led_t *colors, uint8_t row, uint8_t col);

typedef void (*profile_init)(led_t *colors);

typedef struct {
  // callback function implementing the lighting effect
  lighting_callback callback;
  // For static effects, their `callback` is called only once.
  // For dynamic effects, their `callback` is called in a loop.
  //
  // This field controls the animation speed by specifying how many LED refresh
  // cycles to skip before calling the callback again. For example, 1 in the
  // array means that `callback` is called on every cycle, ie. ~71Hz on default
  // settings.
  //
  // Different 4 values can be specified to allow different speeds of the same
  // effect. For static effects, the array must contain {0, 0, 0, 0}.
  uint16_t animationSpeed[4];
  // In case the profile is reactive, it responds to each keypress.
  // This callback is called with the locations of the pressed keys.
  keypress_handler keypressCallback;
  // Some profiles might need additional setup when just enabled.
  // This callback defines such logic if needed.
  profile_init profileInit;
} profile;

profile profiles[] = {
    /* {colorBleed, {0, 0, 0, 0}, NULL, NULL}, */
    {red, {0, 0, 0, 0}, NULL, NULL},
    {green, {0, 0, 0, 0}, NULL, NULL},
    {blue, {0, 0, 0, 0}, NULL, NULL},
    {rainbowHorizontal, {0, 0, 0, 0}, NULL, NULL},
    {rainbowVertical, {0, 0, 0, 0}, NULL, NULL},
    {animatedRainbowVertical, {35, 28, 21, 14}, NULL, NULL},
    {animatedRainbowFlow, {7, 5, 2, 1}, NULL, NULL},
    {animatedRainbowWaterfall, {7, 5, 2, 1}, NULL, NULL},
    {animatedBreathing, {5, 3, 2, 1}, NULL, NULL},
    {animatedWave, {5, 3, 2, 1}, NULL, NULL},
    {animatedSpectrum, {11, 6, 4, 1}, NULL, NULL},
    {reactiveFade, {4, 3, 2, 1}, reactiveFadeKeypress, reactiveFadeInit},
    {reactivePulse, {4, 3, 2, 1}, reactivePulseKeypress, reactivePulseInit}};

static uint8_t currentProfile = 0;
static const uint8_t amountOfProfiles = sizeof(profiles) / sizeof(profile);
static volatile uint8_t currentSpeed = 0;

/* If non-zero the animation is enabled; 1 is full speed */
static volatile uint16_t animationSkipTicks = 0;

/* Animation tick counter used to slow down animations */
static uint16_t animationTicks = 0;

/*
   Original firmware reference:
   ~9.81ms column cycle measured -> 100Hz.

   During each column cycle the row is "strobed" 3 - 32 times (measured on white
   with different intensity settings). This wastes some shining time, but limits
   the current.

   The row strobing is done at around 70kHz.

   We would like to achieve more colors than the original firmware.

   80kHz with pwmCounterLimit=80 will scan 14 columns at 71.4Hz - enough for
   smooth animations, and allows for 6bit LED brightness control.

   Tests on oscilloscope suggest it can reach 100kHz.
 */
static const GPTConfig bftm0Config = {.frequency = 80000,
                                      .callback = mainCallback};

static mutex_t mtx;

// Default zero corresponds to full intensity. Each color has the intensity
// decreased linearly with this setting by the PWM loop.
static uint8_t ledIntensity = 0;

static volatile bool ledEnabled = false;

// Flag to check if there is a foreground color currently active
static bool foregroundColorSet = false;
static uint32_t foregroundColor = 0;

uint8_t ledMasks[KEY_COUNT];
led_t ledColors[KEY_COUNT];

static uint8_t currentColumn = 0;

static const SerialConfig usart1Config = {.speed = 115200};

static uint8_t commandBuffer[64];

void updateAnimationSpeed(void) {
  animationSkipTicks = profiles[currentProfile].animationSpeed[currentSpeed];
  animationTicks = 0;
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
  ledIntensity = (ledIntensity + 1) % 8;
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
    handler(ledColors, row, col);
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

    setAllKeysColor(ledColors, foregroundColor);
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

  for (int ledRow = 0; ledRow < NUM_ROW * 3; ledRow++) {
    palClearLine(ledRows[ledRow]);
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
      setKeyColor(&ledColors[ROWCOL2IDX(commandBuffer[0], commandBuffer[1])],
                  ((uint16_t)commandBuffer[3] << 8 | commandBuffer[2]));
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
      /* FIXME: Don't use direct access */
      memcpy(&ledColors[ROWCOL2IDX(commandBuffer[0], 0)], &commandBuffer[1],
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
  profiles[currentProfile].callback(ledColors);
}

/*
 * Time each row has left to shine within the current column cycle.
 * Row1 R-G-B, Row2 R-G-B, Row3 R-G-B, ...
 */
uint8_t rowTimes[NUM_ROW * 3];
uint8_t rowsEnabled;

/*
 * pwmCounter which counts time of lit rows within each column cycle.
 */
uint16_t pwmCounter;

/*
 * pwmCounter goes over 64 a bit to limit the current of completely white
 * board (0.5A) vs <0.3A for original firmware.
 *
 * You can get brighter LEDs if you set this to 64. And possibly burn the
 * board in the longer period of time.
 */
const uint16_t pwmCounterLimit = 80;

/* Disable timeouted LEDs */
static inline void pwmRowDimmer() {
  for (size_t ledRow = 0; ledRow < NUM_ROW * 3; ledRow++) {
    const uint8_t time = rowTimes[ledRow];
    if (pwmCounter == time) {
      palClearLine(ledRows[ledRow]);
      rowsEnabled--;
    }
  }
  if (rowsEnabled == 0) {
    /* Limit color bleed by disabling the column as early as possible */
    palClearLine(ledColumns[currentColumn]);
  }
}

/* Start new PWM cycle */
static inline void pwmNextColumn() {
  /* Disable previously lit column */
  palClearLine(ledColumns[currentColumn]);

  currentColumn = (currentColumn + 1) % NUM_COLUMN;

  /* TODO: Minimum intensity per animation? For some, the darkness doesn't work
     well (rainbow) */
  const uint8_t intensityDecrease = ledIntensity * 8;

  /* Prepare the PWM data and enable leds for non-zero colors */
  rowsEnabled = 0;
  for (size_t keyRow = 0; keyRow < NUM_ROW; keyRow++) {
    const uint8_t ledIndex = ROWCOL2IDX(keyRow, currentColumn);
    const led_t cl = ledColors[ledIndex];

    for (size_t colorIdx = 0; colorIdx < 3; colorIdx++) {
      const uint8_t ledRow = 3 * keyRow + colorIdx;
      /* >>2 to decrease the color resolution from 0-255 to 0-63 */
      uint8_t color = cl.pv[2 - colorIdx] >> 2;
      if (intensityDecrease >= color) {
        color = 0;
      } else {
        color -= intensityDecrease;
        /* Each led is enabled for color>0 even for a short while. */
        palSetLine(ledRows[ledRow]);
        rowsEnabled++;
      }
      rowTimes[ledRow] = color;
    }
  }

  /* Enable the current LED column if at least one row needs this. Limit bleed
     and maybe power consumption on reactive profiles. */
  if (rowsEnabled) {
    palSetLine(ledColumns[currentColumn]);
  }
}

// mainCallback is responsible for 2 things:
// * software PWM
// * calling animation callback for animated profiles
void mainCallback(GPTDriver *_driver) {
  (void)_driver;

  if (!ledEnabled) {
    return;
  }

  pwmCounter += 1;
  if (pwmCounter < pwmCounterLimit) {
    pwmRowDimmer();
    return;
  }

  /* Update profile if required before starting new cycle */
  if (needToCallbackProfile) {
    needToCallbackProfile = false;
    profiles[currentProfile].callback(ledColors);
  }

  /* Animation can be updated after each full column cycle. On
   * pwmCounterLimit=80 + 80kHz timer this refreshes at 80kHz/80/14 = 71Hz and
   * should be a sensible maximum speed for a fluent smooth animation.
   */
  if (animationSkipTicks > 0 && currentColumn == 13) {
    animationTicks++;
    if (animationTicks >= animationSkipTicks) {
      animationTicks = 0;
      animationCallback();
    }
  }

  /* We start a new PWM column cycle. */
  pwmCounter = 0;

  pwmNextColumn();
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
