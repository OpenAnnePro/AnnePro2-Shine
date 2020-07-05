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

/*
 * Thread 1.
 */
THD_WORKING_AREA(waThread1, 128);
THD_FUNCTION(Thread1, arg) {

  (void)arg;

  while (true)
  {
    for (size_t i = 0; i < NUM_COLUMN; i++)
    {
      palSetLine(ledColumns[i]);
      chThdSleepMicroseconds(200);
      palClearLine(ledColumns[i]);
    }
    
  }
}

#define REFRESH_FREQUENCY           100

// BFTM0 Configuration, this runs at 15 * REFRESH_FREQUENCY Hz
const GPTConfig bftm0Config = {
  .frequency = NUM_COLUMN * REFRESH_FREQUENCY * 10,
  .callback = columnCallback
};

void columnCallback(GPTDriver* driver)
{
  static size_t currentColumn = 0;

  (void)driver;
  palClearLine(ledColumns[currentColumn]);
  if (currentColumn < NUM_COLUMN)
    currentColumn++;
  else
    currentColumn = 0;
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

  // Powerup LED
  // palSetLine(LINE_LED_PWR);
  // palSetLine(LINE_LED_ROW_1_G);

  // Setup Column Multiplex Timer
  gptStart(&GPTD_BFTM0, &bftm0Config);
  gptStartContinuous(&GPTD_BFTM0, 5);

  // chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  /* This is now the idle thread loop, you may perform here a low priority
     task but you must never try to sleep or wait in this loop. Note that
     this tasks runs at the lowest priority level so any instruction added
     here will be executed after all other tasks have been started.*/
  while (true) {
  }
}
