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
#include "matrix.h"
#include "miniFastLED.h"
#include "profiles.h"
#include "settings.h"
#include "string.h"

static void executeMsg(msg_t msg);
static void executeProfile(bool init);
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

#define LEN(a) (sizeof(a) / sizeof(*a))

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
    executeProfile(true);
    matrixEnable();
    break;
  case CMD_LED_OFF:
    matrixDisable();
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
  /* FIXME: Unify with the main chip or remove */
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

  matrixInit();

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
