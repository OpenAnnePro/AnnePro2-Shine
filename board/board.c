/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio
                        (C) 2018 Charlie Waters

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

#include "ch.h"
#include "hal.h"

#include <string.h>
/* ============ Private Defines ===================== */

/* ============ Function Prototypes ================== */
#define PBIT(PORT, LINE) ((PAL_PORT(LINE) == PORT) ? (1 << PAL_PAD(LINE)) : 0)
#define P2BVAL(PORT, LINE, VAL) ((PAL_PORT(LINE) == PORT) ? (VAL << (PAL_PAD(LINE) << 1)) : 0)
#define PAFIO_L(PORT, LINE, AF) (((PAL_PORT(LINE) == PORT) && (PAL_PAD(LINE) < 8)) ? (AF << (PAL_PAD(LINE) << 2)) : 0)
#define PAFIO_H(PORT, LINE, AF) (((PAL_PORT(LINE) == PORT) && (PAL_PAD(LINE) >= 8)) ? (AF << ((PAL_PAD(LINE) - 8) << 2)) : 0)
#define PAFIO(PORT, N, LINE, AF) ((N) ? PAFIO_H(PORT, LINE, AF) : PAFIO_L(PORT, LINE, AF))

#define OUT_BITS(PORT) (\
    PBIT(PORT, LINE_LED_PWR)     |\
    PBIT(PORT, LINE_LED_COL_1  ) |\
    PBIT(PORT, LINE_LED_COL_2  ) |\
    PBIT(PORT, LINE_LED_COL_3  ) |\
    PBIT(PORT, LINE_LED_COL_4  ) |\
    PBIT(PORT, LINE_LED_COL_5  ) |\
    PBIT(PORT, LINE_LED_COL_6  ) |\
    PBIT(PORT, LINE_LED_COL_7  ) |\
    PBIT(PORT, LINE_LED_COL_8  ) |\
    PBIT(PORT, LINE_LED_COL_9  ) |\
    PBIT(PORT, LINE_LED_COL_10 ) |\
    PBIT(PORT, LINE_LED_COL_11 ) |\
    PBIT(PORT, LINE_LED_COL_12 ) |\
    PBIT(PORT, LINE_LED_COL_13 ) |\
    PBIT(PORT, LINE_LED_COL_14 ) |\
    PBIT(PORT, LINE_LED_ROW_1_R) |\
    PBIT(PORT, LINE_LED_ROW_1_G) |\
    PBIT(PORT, LINE_LED_ROW_1_B) |\
    PBIT(PORT, LINE_LED_ROW_2_R) |\
    PBIT(PORT, LINE_LED_ROW_2_G) |\
    PBIT(PORT, LINE_LED_ROW_2_B) |\
    PBIT(PORT, LINE_LED_ROW_3_R) |\
    PBIT(PORT, LINE_LED_ROW_3_G) |\
    PBIT(PORT, LINE_LED_ROW_3_B) |\
    PBIT(PORT, LINE_LED_ROW_4_R) |\
    PBIT(PORT, LINE_LED_ROW_4_G) |\
    PBIT(PORT, LINE_LED_ROW_4_B) |\
    PBIT(PORT, LINE_LED_ROW_5_R) |\
    PBIT(PORT, LINE_LED_ROW_5_G) |\
    PBIT(PORT, LINE_LED_ROW_5_B) |\
0)

#define IN_BITS(PORT) (\
0)

#define HIGH_CURRENT_BITS(PORT) (\
    P2BVAL(PORT, LINE_LED_COL_1 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_2 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_3 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_4 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_5 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_6 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_7 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_8 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_9 , 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_10, 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_11, 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_12, 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_13, 0b11) |\
    P2BVAL(PORT, LINE_LED_COL_14, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_1_R, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_1_G, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_1_B, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_2_R, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_2_G, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_2_B, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_3_R, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_3_G, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_3_B, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_4_R, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_4_G, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_4_B, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_5_R, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_5_G, 0b11) |\
    P2BVAL(PORT, LINE_LED_ROW_5_B, 0b11) |\
0U)

// Alternate Functions
#define AF_BITS(PORT, N) (\
    PAFIO(PORT, N, LINE_LED_PWR,        AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_1,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_2,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_3,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_4,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_5,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_6,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_7,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_8,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_9,      AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_10,     AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_11,     AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_12,     AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_13,     AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_COL_14,     AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_1_R,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_1_G,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_1_B,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_2_R,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_2_G,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_2_B,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_3_R,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_3_G,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_3_B,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_4_R,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_4_G,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_4_B,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_5_R,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_5_G,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_LED_ROW_5_B,    AFIO_GPIO) |\
    PAFIO(PORT, N, LINE_USART_TX,       AFIO_USART)|\
    PAFIO(PORT, N, LINE_USART_RX,       AFIO_USART)|\
0)



/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
const PALConfig pal_default_config = {
    // GPIO A
    .setup[0] = {
        .DIR = OUT_BITS(IOPORTA),
        .INE = IN_BITS(IOPORTA),
        .PU = IN_BITS(IOPORTA),
        .PD = 0x0000,
        .OD = 0x0000,
        .DRV = HIGH_CURRENT_BITS(IOPORTA),
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTA, 0),
        .CFG[1] = AF_BITS(IOPORTA, 1),
    },
    // GPIO B
    .setup[1] = {
        .DIR = OUT_BITS(IOPORTB),
        .INE = IN_BITS(IOPORTB),
        .PU = IN_BITS(IOPORTB),
        .PD = 0x0000,
        .OD = 0x0000,
        .DRV = HIGH_CURRENT_BITS(IOPORTB),
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTB, 0),
        .CFG[1] = AF_BITS(IOPORTB, 1),
    },
    // GPIO C
    .setup[2] = {
        .DIR = OUT_BITS(IOPORTC),
        .INE = IN_BITS(IOPORTC),
        .PU = IN_BITS(IOPORTC),
        .PD = 0x0000,
        .OD = 0x0000,
        .DRV = HIGH_CURRENT_BITS(IOPORTC),
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTC, 0),
        .CFG[1] = AF_BITS(IOPORTC, 1),
    },
    // GPIO D
    .setup[3] = {
        .DIR = OUT_BITS(IOPORTD),
        .INE = IN_BITS(IOPORTD),
        .PU = IN_BITS(IOPORTD),
        .PD = 0x0000,
        .OD = 0x0000,
        .DRV = HIGH_CURRENT_BITS(IOPORTD),
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTD, 0),
        .CFG[1] = AF_BITS(IOPORTD, 1),
    },
    .ESSR[0] = 0x00000000,
    .ESSR[1] = 0x00000000,
};

void __early_init(void) {
    ht32_clock_init();
}

/**
 * @brief   Board-specific initialization code.
 * @todo    Add your board-specific code, if any.
 */
void boardInit(void) {
}
