#include "commands.h"
#include "board.h"
#include "matrix.h"
#include "miniFastLED.h"
#include "profiles.h"
#include "protocol.h"
#include <string.h>

/*
 * Reactive profiles are profiles which react to keypresses.
 * This helper is used to notify the main controller that
 * the current profile is reactive and coordinates of pressed
 * keys should be sent to LED controller.
 */
static inline void sendStatus(void) {
  uint8_t isReactive = profiles[currentProfile].keypressCallback != NULL;

  uint8_t payload[] = {
      amountOfProfiles, currentProfile, matrixEnabled,
      isReactive,       ledIntensity,   proto.errors,
  };
  protoTx(CMD_LED_STATUS, payload, sizeof(payload), 3);
}

void sendDebug(const char *payload, uint8_t size) {
  protoTx(CMD_LED_DEBUG, (unsigned char *)payload, size, 1);
}

static inline void setIAP(void) {
  // Magic key to set keyboard to IAP
  *((uint32_t *)0x20001ffc) = 0x0000fab2;

  __disable_irq();
  NVIC_SystemReset();
}

/*
 * Execute current profile
 */
static inline void executeProfile(bool init) {
  profile_init pinit = profiles[currentProfile].profileInit;
  if (init && pinit != NULL) {
    pinit(ledColors);
  }

  updateAnimationSpeed();

  needToCallbackProfile = true;
}

static inline void nextIntensity(void) {
  ledIntensity = (ledIntensity + 1) % 8;
  executeProfile(false);
}

static inline void nextSpeed(void) {
  currentSpeed = (currentSpeed + 1) % 4;
  updateAnimationSpeed();
}

/*
 * The message contains 1 flag bit which is always set
 * and then 3 bits of row and 4 bits of col.
 * Because this callback is called on every keypress,
 * the data is packed into a single byte to decrease the data traffic.
 */
static inline void handleKeypress(uint8_t command) {
  uint8_t row = (command >> 4) & 0b111;
  uint8_t col = command & 0b1111;
  keypress_handler handler = profiles[currentProfile].keypressCallback;
  if (handler != NULL && row < NUM_ROW && col < NUM_COLUMN) {
    handler(ledColors, row, col);
  }
}

/*
 * Set profile and execute it
 */
static inline void setProfile(uint8_t profile) {
  if (profile < amountOfProfiles) {
    currentProfile = profile;
    executeProfile(true);
  }
}

/*
 * Handle masking
 */

/* Override one key with a given color */
static inline void setMaskKey(const message_t *msg) {
  uint8_t row = msg->payload[0];
  uint8_t col = msg->payload[1];
  led_t color = {.p.blue = msg->payload[2],
                 .p.green = msg->payload[3],
                 .p.red = msg->payload[4],
                 .p.alpha = msg->payload[5]};
  naiveDimLed(&color);
  if (row < NUM_ROW && col <= NUM_COLUMN)
    setKeyColor(&ledMask[ROWCOL2IDX(row, col)], color.rgb);
}

/* Override all keys with given color */
static inline void setMaskRow(const message_t *msg) {
  uint8_t row = msg->payload[0];
  if (row > NUM_ROW)
    return;

  const uint8_t *payloadPtr = &msg->payload[1];

  for (int col = 0; col < NUM_COLUMN; col++) {
    led_t color;
    color.p.blue = *(payloadPtr++);
    color.p.green = *(payloadPtr++);
    color.p.red = *(payloadPtr++);
    color.p.alpha = *(payloadPtr++);

    naiveDimLed(&color);
    ledMask[ROWCOL2IDX(row, col)] = color;
  }
}

/* Override all keys with given color */
static inline void setMaskMono(const message_t *msg) {
  led_t color = {.p.red = msg->payload[2],
                 .p.green = msg->payload[1],
                 .p.blue = msg->payload[0],
                 .p.alpha = msg->payload[3]};

  naiveDimLed(&color);
  setAllKeysColor(ledMask, color.rgb);
}

/* Thread status */
struct {
  volatile uint8_t running;
  uint8_t row;
  uint8_t col;
  led_t color;
  uint8_t times;
  uint32_t hundredths;
  thread_t *thread;
} blinker;

/* Blinker thread - one active at a time. */
static THD_FUNCTION(blinkerFun, arg) {
  (void)arg;
  uint8_t on = 1;

  for (; blinker.times > 0 && blinker.running; blinker.times--) {
    if (on)
      setKeyColor(&ledMask[ROWCOL2IDX(blinker.row, blinker.col)],
                  blinker.color.rgb);
    else
      setKeyColor(&ledMask[ROWCOL2IDX(blinker.row, blinker.col)], 0xff000000);
    on ^= 0x01;

    for (uint32_t i = 0; i < blinker.hundredths && blinker.running; i++) {
      /* Doing this in chunks of 10ms allows for a faster quit */
      chThdSleepMilliseconds(10);
    }
  }
  setKeyColor(&ledMask[ROWCOL2IDX(blinker.row, blinker.col)], 0x00);
}

/* Prepare thread data and schedule a blinking thread */
static inline void blinkKey(const message_t *msg) {
  if (blinker.thread != NULL) {
    blinker.running = 0;
    chThdWait(blinker.thread);
    blinker.thread = NULL;
  }

  blinker.row = msg->payload[0];
  blinker.col = msg->payload[1];
  blinker.color.p.blue = msg->payload[2];
  blinker.color.p.green = msg->payload[3];
  blinker.color.p.red = msg->payload[4];
  blinker.color.p.alpha = msg->payload[5];

  blinker.times = msg->payload[6] * 2;
  blinker.hundredths = msg->payload[7];
  blinker.running = 1;

  blinker.thread = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(32),
                                       "blinker", NORMALPRIO, blinkerFun, NULL);
}

/*
 * Execute action based on a message. This runs in a separate thread than LED
 * refreshing algorithm. Keep it simple, fast, mark something in a variable
 * and do the rest in another thread.
 */
void commandCallback(const message_t *msg) {
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
    setProfile(msg->payload[0]);
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

  /* Handle masking */
  case CMD_LED_MASK_SET_KEY:
    setMaskKey(msg);
    break;
  case CMD_LED_MASK_SET_ROW:
    setMaskRow(msg);
    break;
  case CMD_LED_MASK_SET_MONO:
    setMaskMono(msg);
    break;
  case CMD_LED_KEY_BLINK:
    blinkKey(msg);
    break;

  default:
    proto.errors++;
    break;
  }
}
