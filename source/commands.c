#include "commands.h"
#include "matrix.h"
#include "profiles.h"
#include "protocol.h"
#include <string.h>

static void executeProfile(bool init);
static void setProfile(message_t *msg);
static void nextIntensity(void);
static void nextSpeed(void);
static void handleKeypress(uint8_t command);
static void setIAP(void);

/*
 * Reactive profiles are profiles which react to keypresses.
 * This helper is used to notify the main controller that
 * the current profile is reactive and coordinates of pressed
 * keys should be sent to LED controller.
 */
void sendStatus(void) {
  uint8_t isReactive = profiles[currentProfile].keypressCallback != NULL;

  uint8_t payload[] = {
      amountOfProfiles, currentProfile, matrixEnabled,
      isReactive,       ledIntensity,   proto.errors,
  };
  protoTx(CMD_LED_STATUS, payload, sizeof(payload), 3);
}

void setIAP() {

  // Magic key to set keyboard to IAP
  *((uint32_t *)0x20001ffc) = 0x0000fab2;

  __disable_irq();
  NVIC_SystemReset();
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
inline void handleKeypress(uint8_t command) {
  uint8_t row = (command >> 4) & 0b111;
  uint8_t col = command & 0b1111;
  keypress_handler handler = profiles[currentProfile].keypressCallback;
  if (handler != NULL && row < NUM_ROW && col < NUM_COLUMN) {
    handler(ledColors, row, col);
  }
}

/*
 * Set all the leds to the specified color
 */
#if 0
void setForegroundColor(led_t color) {
  foregroundColor = color.rgb;

  foregroundColorSet = true;

  setAllKeysColor(ledColors, color.rgb);
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
#endif

/*
 * Set profile and execute it
 */
void setProfile(message_t *msg) {
  uint8_t profile = msg->payload[0];
  if (profile < amountOfProfiles) {
    foregroundColorSet = false;
    currentProfile = profile;
    executeProfile(true);
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
/*
void ledSet(message_t *msg) {
  const uint8_t row = msg->payload[0];
  const uint8_t col = msg->payload[1];
  const led_t color = {
      .p.red = msg->payload[2],
      .p.green = msg->payload[3],
      .p.blue = msg->payload[4],
  };

  if (row < NUM_ROW && col < NUM_COLUMN) {
    setKeyColor(&ledColors[ROWCOL2IDX(row, col)], color.rgb);
  }
}
*/

/*
 * Set a row of leds based on qmk communication
 * TODO: Do it better with masks.
 */
/*
void ledSetRow(message_t *msg) {
  // FIXME: Unify with the main chip or remove
  uint8_t row = msg->payload[0];
  if (row >= NUM_ROW)
    return;

  led_t color;

  for (int c = 0; c < NUM_COLUMN; c++) {
    color.p.red = msg->payload[1 + c * 3 + 0];
    color.p.green = msg->payload[1 + c * 3 + 1];
    color.p.blue = msg->payload[1 + c * 3 + 2];

    ledColors[ROWCOL2IDX(row, c)] = color;
  }
}
*/

/*
 * Execute action based on a message. This runs in a separate thread than LED
 * refreshing algorithm. Keep it simple, fast, mark something in a variable and
 * do the rest in another thread.
 */
void commandCallback(message_t *msg) {
  switch (msg->command) {
  case CMD_LED_ON:
    executeProfile(true);
    matrixEnable();
    sendStatus();
    break;
  case CMD_LED_OFF:
    matrixDisable();
    sendStatus();
    break;
  case CMD_LED_SET_PROFILE:
    setProfile(msg);
    sendStatus();
    break;
  case CMD_LED_NEXT_PROFILE:
    currentProfile = (currentProfile + 1) % amountOfProfiles;
    executeProfile(true);
    sendStatus();
    break;
  case CMD_LED_PREV_PROFILE:
    currentProfile =
        (currentProfile + (amountOfProfiles - 1u)) % amountOfProfiles;
    executeProfile(true);
    sendStatus();
    break;
  case CMD_LED_NEXT_INTENSITY:
    nextIntensity();
    sendStatus();
    break;
  case CMD_LED_NEXT_ANIMATION_SPEED:
    nextSpeed();
    sendStatus();
    break;
  case CMD_LED_IAP:
    setIAP();
    break;
  case CMD_LED_KEY_DOWN:
    handleKeypress(msg->payload[0]);
    break;
  default:
    proto.errors++;
    break;
  }
}
