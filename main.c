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
void executeMsg(msg_t msg);
void switchProfile(void);
void executeProfile(void);
void disableLeds(void);
void ledSet(void);
void ledRowSet(void);


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

ioline_t ledRows[NUM_ROW * 3] = {
  LINE_LED_ROW_1_R,
  LINE_LED_ROW_1_G,
  LINE_LED_ROW_1_B,
  LINE_LED_ROW_2_R,
  LINE_LED_ROW_2_G,
  LINE_LED_ROW_2_B,
  LINE_LED_ROW_3_R,
  LINE_LED_ROW_3_G,
  LINE_LED_ROW_3_B,
  LINE_LED_ROW_4_R,
  LINE_LED_ROW_4_G,
  LINE_LED_ROW_4_B,
  LINE_LED_ROW_5_R,
  LINE_LED_ROW_5_G,
  LINE_LED_ROW_5_B,
};

#define REFRESH_FREQUENCY           200

#define ANIMATION_TIMER_FREQUENCY   60

#define LEN(a) (sizeof(a)/sizeof(*a))

/*
 * Active profiles
 * Add profiles from source/profiles.h in the profile array
 */
typedef void (*profile)( led_t* );
profile profiles[9] = {
  red, green, blue, rainbowHorizontal, rainbowVertical, 
  animatedRainbowVertical, animatedRainbowWaterfall, 
  animatedBreathing, animatedSpectrum
};
static uint8_t currentProfile = 0;
static uint8_t amountOfProfiles = sizeof(profiles)/sizeof(profile);

led_t ledColors[70];
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

/*
 * Thread 1.
 */
THD_WORKING_AREA(waThread1, 128);
THD_FUNCTION(Thread1, arg) {
  (void)arg;
    
  while(true){
    msg_t msg;
    msg = sdGet(&SD1);
    if(msg >= MSG_OK){
      executeMsg(msg);
    }
  }
}

/*
 * Execute action based on a message
 */
void executeMsg(msg_t msg){
  switch (msg) {
    case CMD_LED_ON:
      switchProfile();
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
    default:
      break;
  }
}

/*
 * Switch to next profile and execute it
 */
void switchProfile(){
  currentProfile = (currentProfile+1)%amountOfProfiles;
  executeProfile();
}

/*
 * Execute current profile
 */
void executeProfile(){
  chSysLock();
  profiles[currentProfile](ledColors);
  palSetLine(LINE_LED_PWR);
  chSysUnlock();
}

/*
 * Turn off all leds
 */
void disableLeds(){
  currentProfile = (currentProfile+amountOfProfiles-1)%amountOfProfiles;
  palClearLine(LINE_LED_PWR);
}

/*
 * Set a led based on qmk communication
 */
void ledSet(){
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, 4, 10000);

  if(bytesRead > 4){
      if(commandBuffer[0] < NUM_ROW || commandBuffer[1] < NUM_COLUMN){
          setKeyColor(&ledColors[commandBuffer[0] * NUM_COLUMN + commandBuffer[1]], ((uint16_t)commandBuffer[3] << 8 | commandBuffer[2]));
      }
  }
}

/*
 * Set a row of leds based on qmk communication
 */
void ledSetRow(){
  size_t bytesRead;
  bytesRead = sdReadTimeout(&SD1, commandBuffer, sizeof(uint16_t) * NUM_COLUMN + 1, 1000);
  if(bytesRead >= sizeof(uint16_t) * NUM_COLUMN + 1){
    if(commandBuffer[0] < NUM_ROW){
      memcpy(&ledColors[commandBuffer[0] * NUM_COLUMN],&commandBuffer[1], sizeof(uint16_t) * NUM_COLUMN); 
    }
  }
}

inline uint8_t min(uint8_t a, uint8_t b){
  return a<=b?a:b;
}

/*
 * Update lighting table as per animation
 */
void animationCallback(GPTDriver* _driver){
  profile currentFunction = profiles[currentProfile];
  if(currentFunction == animatedRainbowVertical){
    gptChangeInterval(_driver, ANIMATION_TIMER_FREQUENCY/5);
    currentFunction(ledColors);
  }else if(currentFunction == animatedRainbowWaterfall){
    gptChangeInterval(_driver, ANIMATION_TIMER_FREQUENCY/20);
    currentFunction(ledColors);
  }else if(currentFunction == animatedRainbowFlow){
    gptChangeInterval(_driver, ANIMATION_TIMER_FREQUENCY/30);
    currentFunction(ledColors);
  }else if(currentFunction == animatedSpectrum){
    gptChangeInterval(_driver, ANIMATION_TIMER_FREQUENCY/15);
    currentFunction(ledColors);
  }else if(currentFunction == animatedBreathing){
    gptChangeInterval(_driver, ANIMATION_TIMER_FREQUENCY/30);
    currentFunction(ledColors);
  }
}

inline void sPWM(uint8_t cycle, uint8_t currentCount, uint8_t start, ioline_t port){
  if (start+cycle>0xFF) start = 0xFF - cycle;
  if (start <= currentCount && currentCount < start+cycle)
    palSetLine(port);
  else
    palClearLine(port);
}

void columnCallback(GPTDriver* _driver)
{
  (void)_driver;
  palClearLine(ledColumns[currentColumn]);
  currentColumn = (currentColumn+1) % NUM_COLUMN;
  palSetLine(ledColumns[currentColumn]);
  if (columnPWMCount < 255)
  {
    for (size_t row = 0; row < NUM_ROW; row++)
    {
    const led_t keyLED = ledColors[currentColumn + (NUM_COLUMN * row)];
    const uint8_t red = keyLED.red;
    const uint8_t green = keyLED.green;
    const uint8_t blue = keyLED.blue;

    sPWM(red, columnPWMCount, 0, ledRows[row * 3]);
    sPWM(green, columnPWMCount, red, ledRows[row * 3+1]);
    sPWM(blue, columnPWMCount, red+green, ledRows[row * 3+2]);
    }
    columnPWMCount++;
  }
  else
  {
    columnPWMCount = 0;
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

  // Setup UART1
  sdStart(&SD1, &usart1Config);
  palSetLine(LINE_LED_PWR);

  // Setup Column Multiplex Timer
  gptStart(&GPTD_BFTM0, &bftm0Config);
  gptStartContinuous(&GPTD_BFTM0, 1);

  // Setup Animation Timer
  gptStart(&GPTD_BFTM1, &lightAnimationConfig);
  gptStartContinuous(&GPTD_BFTM1, 1);

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  /* This is now the idle thread loop, you may perform here a low priority
     task but you must never try to sleep or wait in this loop. Note that
     this tasks runs at the lowest priority level so any instruction added
     here will be executed after all other tasks have been started.*/
  while (true) {
  }
}
