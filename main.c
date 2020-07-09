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

static void columnCallback(GPTDriver* driver);


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

#define LEN(a) (sizeof(a)/sizeof(*a))

static const uint16_t colorCycle[] = {0x000, 0xF00, 0xFF0, 0x0F0, 0x0FF, 0x00F, 0xF0F, 0x5FF, 0x000, 0x001};
static uint16_t cycleIndex = 0;

uint16_t ledColors[NUM_COLUMN * NUM_ROW] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
static uint32_t currentColumn = 0;
static uint32_t columnPWMCount = 0;

// BFTM0 Configuration, this runs at 15 * REFRESH_FREQUENCY Hz
static const GPTConfig bftm0Config = {
  .frequency = NUM_COLUMN * REFRESH_FREQUENCY * 2 * 16,
  .callback = columnCallback
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

  while (true)
  {
    msg_t msg;
    size_t bytesRead;
    msg = sdGet(&SD1);
    if (msg >= MSG_OK) {
      switch (msg) {
        case CMD_LED_ON:
          if (colorCycle[cycleIndex]==0){
            for (uint16_t i=0; i<NUM_ROW; ++i){
              for (uint16_t j=0; j<NUM_COLUMN; ++j){
                ledColors[i*NUM_COLUMN+j] = colorCycle[i%LEN(colorCycle)];
              }     
            }
          }
          else if (colorCycle[cycleIndex]==1){
            for (uint16_t i=0; i<NUM_COLUMN; ++i){
              for (uint16_t j=0; j<NUM_ROW; ++j){
                ledColors[j*NUM_COLUMN+i] = colorCycle[i%LEN(colorCycle)];
              }     
            }
          }
          else {
            for (uint16_t i=0; i<LEN(ledColors); ++i){
              ledColors[i] = colorCycle[cycleIndex];
            }
          }
          palSetLine(LINE_LED_PWR);
          cycleIndex = (cycleIndex+1)%LEN(colorCycle);
          break;
        case CMD_LED_OFF:
          cycleIndex = (cycleIndex+LEN(colorCycle)-1)%LEN(colorCycle);
          palClearLine(LINE_LED_PWR);
          break;
        case CMD_LED_SET:
          bytesRead = sdReadTimeout(&SD1, commandBuffer, 4, 10000);
          if (bytesRead < 4)
            continue;
          if (commandBuffer[0] >= NUM_ROW || commandBuffer[1] >= NUM_COLUMN)
            continue;
          ledColors[commandBuffer[0] * NUM_COLUMN + commandBuffer[1]] = 
            ((uint16_t)commandBuffer[3] << 8 | commandBuffer[2]) ;
          break;
        case CMD_LED_SET_ROW:
          bytesRead = sdReadTimeout(&SD1, commandBuffer, sizeof(uint16_t) * NUM_COLUMN + 1, 1000);
          if (bytesRead < sizeof(uint16_t) * NUM_COLUMN + 1)
            continue;
          if (commandBuffer[0] >= NUM_ROW)
            continue;
          memcpy(&ledColors[commandBuffer[0] * NUM_COLUMN],&commandBuffer[1], sizeof(uint16_t) * NUM_COLUMN);
          break;
        default:
          break;
      }
    }
  }
}

inline uint8_t min(uint8_t a, uint8_t b){
  return a<=b?a:b;
}

inline void sPWM(uint8_t cycle, uint8_t currentCount, uint8_t start, ioline_t port){
  if (start+cycle>0xF) start = 0xF - cycle;
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
  if (columnPWMCount < 16)
  {
    for (size_t row = 0; row < NUM_ROW; row++)
    {
    const uint16_t row_color = ledColors[currentColumn + (NUM_COLUMN * row)];
    const uint8_t red = (row_color >> 8) & 0xF;
    const uint8_t green = (row_color >> 4) & 0xF;
    const uint8_t blue = (row_color >> 0) & 0xF;

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

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  /* This is now the idle thread loop, you may perform here a low priority
     task but you must never try to sleep or wait in this loop. Note that
     this tasks runs at the lowest priority level so any instruction added
     here will be executed after all other tasks have been started.*/
  while (true) {
  }
}
