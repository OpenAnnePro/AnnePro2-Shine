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

#include "board.h"
#include "hal.h"
#include "ch.h"
#include "string.h"
#include "ap2_qmk_led.h"
#include "light_utils.h"
#include "profiles.h"
#include "miniFastLED.h"

static void columnCallback(GPTDriver* driver);
static void animationCallback(GPTDriver* driver);
static void executeMsg(msg_t msg);
static void executeProfile(void);
static void disableLeds(void);
static void enableLeds(void);
static void ledSet(void);
static void ledSetRow(void);
static void setProfile(void);
static void changeMask(uint8_t mask);
static void nextIntensity(void);
static void nextSpeed(void);
static void setForegroundColor(void);
static void handleKeypress(msg_t msg);

ioline_t ledColumns[NUM_COLUMN] = {
  LINE_LED_COL_1,
  LINE_LED_COL_2,
  LINE_LED_COL_3,
  LINE_LED_COL_4,
  LINE_LED_COL_5,
  LINE_LED_COL_6,
  LINE_LED_COL_7,
  LINE_LED_COL_8,
  LINE_LED_COL_9,
  LINE_LED_COL_10,
  LINE_LED_COL_11,
  LINE_LED_COL_12,
  LINE_LED_COL_13,
  LINE_LED_COL_14
};

ioline_t ledRows[NUM_ROW * 4] = {
  LINE_LED_ROW_1_R,
  LINE_LED_ROW_1_G,
  LINE_LED_ROW_1_B,
  0,
  LINE_LED_ROW_2_R,
  LINE_LED_ROW_2_G,
  LINE_LED_ROW_2_B,
  0,
  LINE_LED_ROW_3_R,
  LINE_LED_ROW_3_G,
  LINE_LED_ROW_3_B,
  0,
  LINE_LED_ROW_4_R,
  LINE_LED_ROW_4_G,
  LINE_LED_ROW_4_B,
  0,
  LINE_LED_ROW_5_R,
  LINE_LED_ROW_5_G,
  LINE_LED_ROW_5_B,
  0,
};

#define REFRESH_FREQUENCY           200

#define ANIMATION_TIMER_FREQUENCY   60

#define KEY_COUNT                   70

#define LEN(a) (sizeof(a)/sizeof(*a))

/*
 * Active profiles
 * Add profiles from source/profiles.h in the profile array
 */
typedef void (*lighting_callback)( led_t*, uint8_t );

/*
 * keypress handler
 */
typedef void (*keypress_handler)( led_t* colors, uint8_t row, uint8_t col, uint8_t intensity );

typedef void (*profile_init)( led_t* colors );

typedef struct {
  // callback function implementing the lighting effect
  lighting_callback callback;
  // In case the lighting effect is animated, this contains 4 different frequencies
  // which are passed in gptStartContinuous() and determine how often the function is called.
  // 1 represents function being called every tick, and n represents being called only on
  // every n-th tick (skipping other ticks).
  // We do support 4 different speeds, and generally they go from slowest to fastest.
  // For static effects, the array should contain {0, 0, 0, 0} which stops the GPT timer.
  uint8_t animationSpeed[4];
  // In case the profile is reactive, it responds to each keypress.
  // This callback is called with the locations of the pressed keys.
  keypress_handler keypressCallback;
  // Some profiles might need additional setup when just enabled.
  // This callback defines such logic if needed.
  profile_init profileInit;
} profile;

profile profiles[12] = {
    { red, {0, 0, 0, 0}, NULL, NULL },
    { green, {0, 0, 0, 0}, NULL, NULL },
    { blue, {0, 0, 0, 0}, NULL, NULL },
    { rainbowHorizontal, {0, 0, 0, 0}, NULL, NULL },
    { rainbowVertical, {0, 0, 0, 0}, NULL, NULL },
    { animatedRainbowVertical, {20, 10, 8, 5}, NULL, NULL },
    { animatedRainbowFlow, {12, 6, 3, 1}, NULL, NULL },
    { animatedRainbowWaterfall, {12, 6, 3, 1}, NULL, NULL },
    { animatedBreathing, {12, 6, 3, 1}, NULL, NULL },
    { animatedWave, {12, 6, 3, 1}, NULL, NULL },
    { animatedSpectrum, {12, 6, 3, 1}, NULL, NULL },
    { reactiveFade, {12, 6, 3, 1}, reactiveFadeKeypress, reactiveFadeInit },
};

static uint8_t currentProfile = 0;
static uint8_t amountOfProfiles = sizeof(profiles)/sizeof(profile);
static uint8_t currentSpeed = 0;

// each color from RGB is rightshifted by this amount
// default zero corresponds to full intensity, max 3 correponds to 1/8 of color
static uint8_t ledIntensity = 0;

// Flag to check if there is a foreground color currently active
static bool is_foregroundColor_set = false;

uint8_t ledMasks[KEY_COUNT];
led_t ledColors[KEY_COUNT];
static uint32_t currentColumn = 0;
static uint32_t columnPWMCount = 0;

// BFTM0 Configuration, this runs at 15 * REFRESH_FREQUENCY Hz
static const GPTConfig bftm0Config = {
  .frequency = NUM_COLUMN * REFRESH_FREQUENCY * 2 * 16,
  .callback = columnCallback
};

// Lighting animation refresh timer
static const GPTConfig lightAnimationConfig = {
  .frequency = ANIMATION_TIMER_FREQUENCY,
  .callback = animationCallback
};

static const SerialConfig usart1Config = {
  .speed = 115200
};

static uint8_t commandBuffer[64];

void updateLightningTimer(void) {
    uint8_t freq = profiles[currentProfile].animationSpeed[currentSpeed];
    if (freq > 0) {
      if (GPTD_BFTM1.state == GPT_CONTINUOUS) {
        if (gptGetIntervalX(&GPTD_BFTM1) != freq) {
          gptStopTimer(&GPTD_BFTM1);
          gptStartContinuous(&GPTD_BFTM1, freq);
        }
      } else {
        gptStartContinuous(&GPTD_BFTM1, freq);
      }

    } else if (GPTD_BFTM1.state == GPT_CONTINUOUS) {
      gptStopTimer(&GPTD_BFTM1);
    }
    // Here we disable the foreground so the speed change is visible
    is_foregroundColor_set = false;
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
void executeMsg(msg_t msg) {
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
      currentProfile = (currentProfile+1)%amountOfProfiles;
      if (profiles[currentProfile].profileInit != NULL) {
        profiles[currentProfile].profileInit(ledColors);
      }
      executeProfile();
      updateLightningTimer();
      forwardReactiveFlag();
      break;
    case CMD_LED_PREV_PROFILE:
      currentProfile = (currentProfile+(amountOfProfiles-1u))%amountOfProfiles;
      executeProfile();
      updateLightningTimer();
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
    default:
      if (msg & 0b10000000) {
        handleKeypress(msg);
      }
      break;
  }
}

void changeMask(uint8_t mask) {
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, 1, 10000);

  if (bytesRead == 1) {
    if(commandBuffer[0] < KEY_COUNT) {
      ledMasks[commandBuffer[0]] = mask;
    }
  }
}

void nextIntensity() {
    ledIntensity = (ledIntensity + 1) % 4;
    executeProfile();
}



void nextSpeed() {
    currentSpeed = (currentSpeed + 1) % 4;
    updateLightningTimer();
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
  if (handler != NULL) {
    handler(ledColors, row, col, ledIntensity);
  }
}

/*
 * Set all the leds to the specified color
 */
void setForegroundColor(){
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, 3, 10000);

  if(bytesRead >= 3)
  {
    uint8_t colorBytes[4] = {commandBuffer[2], commandBuffer[1], commandBuffer[0], 0x00};
    uint32_t ForegroundColor = *(uint32_t*)&colorBytes;
    is_foregroundColor_set = true;

    chSysLock();
    setAllKeysColor(ledColors, ForegroundColor, ledIntensity);
    chSysUnlock();
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
      currentProfile = commandBuffer[0];
      if (profiles[currentProfile].profileInit != NULL) {
        profiles[currentProfile].profileInit(ledColors);
      }
      executeProfile();
      updateLightningTimer();
    }
  }
}

/*
 * Execute current profile
 */
void executeProfile() {
  chSysLock();
  // Here we disable the foreground to ensure the animation will run
  is_foregroundColor_set = false;

  if (profiles[currentProfile].profileInit != NULL) {
    profiles[currentProfile].profileInit(ledColors);
  }
  profiles[currentProfile].callback(ledColors, ledIntensity);
  chSysUnlock();
}

/*
 * Turn off all leds
 */
void disableLeds() {
  palClearLine(LINE_LED_PWR);
  if (GPTD_BFTM1.state == GPT_CONTINUOUS) {
    gptStopTimer(&GPTD_BFTM1);
  }
}

/*
 * Turn on all leds
 */
void enableLeds() {
  if (profiles[currentProfile].profileInit != NULL) {
    profiles[currentProfile].profileInit(ledColors);
  }
  palSetLine(LINE_LED_PWR);
  executeProfile();
  updateLightningTimer();
}

/*
 * Set a led based on qmk communication
 */
void ledSet() {
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, 4, 10000);

  if (bytesRead >= 4) {
    if (commandBuffer[0] < NUM_ROW || commandBuffer[1] < NUM_COLUMN) {
      setKeyColor(&ledColors[commandBuffer[0] * NUM_COLUMN + commandBuffer[1]], ((uint16_t)commandBuffer[3] << 8 | commandBuffer[2]), ledIntensity);
    }
  }
}

/*
 * Set a row of leds based on qmk communication
 */
void ledSetRow(){
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, sizeof(uint16_t) * NUM_COLUMN + 1, 1000);
  if (bytesRead >= sizeof(uint16_t) * NUM_COLUMN + 1) {
    if (commandBuffer[0] < NUM_ROW) {
      memcpy(&ledColors[commandBuffer[0] * NUM_COLUMN],&commandBuffer[1], sizeof(uint16_t) * NUM_COLUMN);
    }
  }
}

inline uint8_t min(uint8_t a, uint8_t b) {
  return a<=b?a:b;
}

/*
 * Update lighting table as per animation
 */
void animationCallback(GPTDriver* _driver) {
  (void)_driver;

  // If the foreground is set we skip the animation as a way to avoid it overrides the foreground
  if(!is_foregroundColor_set) {
    profiles[currentProfile].callback(ledColors, ledIntensity);
  }
}

inline void sPWM(uint8_t cycle, uint8_t currentCount, uint8_t start, ioline_t port) {
  if (start+cycle>0xFF) start = 0xFF - cycle;
  if (start <= currentCount && currentCount < start+cycle)
    palSetLine(port);
  else
    palClearLine(port);
}

void columnCallback(GPTDriver* _driver) {
  (void)_driver;
  palClearLine(ledColumns[currentColumn]);
  currentColumn = (currentColumn+1) % NUM_COLUMN;
  if (columnPWMCount < 255) {
    for (size_t row = 0; row < NUM_ROW; row++) {
      const size_t ledIndex = currentColumn + (NUM_COLUMN * row);
      const led_t keyLED = ledColors[ledIndex];
      const uint8_t ledMask = ledMasks[ledIndex];
      const uint8_t red = keyLED.red & ledMask;
      const uint8_t green = keyLED.green & ledMask;
      const uint8_t blue = keyLED.blue & ledMask;

      sPWM(red, columnPWMCount, 0, ledRows[row << 2]);
      sPWM(green, columnPWMCount, red, ledRows[(row << 2) | 1]);
      sPWM(blue, columnPWMCount, red+green, ledRows[(row << 2) | 2]);
    }
    columnPWMCount++;
  } else {
    columnPWMCount = 0;
  }
  palSetLine(ledColumns[currentColumn]);
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

  profiles[currentProfile].callback(ledColors, ledIntensity);

  // Setup masks to all be 0xFF at the start
  for (size_t i = 0; i < KEY_COUNT; ++i)
  {
    ledMasks[i] = 0xFF;
  }

  palClearLine(LINE_LED_PWR);
  sdStart(&SD1, &usart1Config);

  // Setup Column Multiplex Timer
  gptStart(&GPTD_BFTM0, &bftm0Config);
  gptStartContinuous(&GPTD_BFTM0, 1);

  // Setup Animation Timer
  gptStart(&GPTD_BFTM1, &lightAnimationConfig);

  while (true) {
    msg_t msg;
    msg = sdGet(&SD1);
    if (msg >= MSG_OK) {
      executeMsg(msg);
    }
  }
}
