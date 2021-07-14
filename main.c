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
#include "ch.h"
#include "commands.h"
#include "hal.h"
#include "light_utils.h"
#include "matrix.h"
#include "miniFastLED.h"
#include "profiles.h"
#include "protocol.h"
#include "settings.h"
#include "string.h"

static const SerialConfig usart1Config = {.speed = 115200};

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

  matrixInit();

  palClearLine(LINE_LED_PWR);
  sdStart(&SD1, &usart1Config);

  /* Make sure the SD is empty */
  while (!sdGetWouldBlock(&SD1))
    sdGet(&SD1);

  chThdSetPriority(HIGHPRIO);

  // start the handler for commands coming from the main MCU
  protoInit(&proto, commandCallback);
  msg_t msg;
  while (true) {
    msg = sdGetTimeout(&SD1, TIME_MS2I(100));
    if (msg == MSG_TIMEOUT) {
      protoSilence(&proto);
    } else {
      protoConsume(&proto, msg);
    }
  }
}
