/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

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

#ifndef BOARD_H
#define BOARD_H
/*
 * Board identifier.
 */

#ifdef C18
#define ANNEPRO2_C18
#define BOARD_NAME "Anne Pro 2 C18"
#define HT32F52352
#else
#define ANNEPRO2_C15
#define BOARD_NAME "Anne Pro 2 C15"
#define HT32F52342
#endif

#define FLASH_SIZE 0x10000 - 0x4000 // 64kB - 16kB

// clang-format off
/*
 * I/O
 */


#define NUM_COLUMN                                         14
#define NUM_ROW                                            5

#ifdef C18
#define LINE_LED_PWR                                       PAL_LINE(IOPORTA, 10)

#define LINE_USART_TX                                      PAL_LINE(IOPORTA, 4 )
#define LINE_USART_RX                                      PAL_LINE(IOPORTA, 5 )

// PORTA 12,13 conflict SWD
#define LINE_LED_COL_1                                     PAL_LINE(IOPORTB,  1)
#define LINE_LED_COL_2                                     PAL_LINE(IOPORTD,  1)
#define LINE_LED_COL_3                                     PAL_LINE(IOPORTD,  2)
#define LINE_LED_COL_4                                     PAL_LINE(IOPORTD,  3)
#define LINE_LED_COL_5                                     PAL_LINE(IOPORTC, 15)
#define LINE_LED_COL_6                                     PAL_LINE(IOPORTC,  1)
#define LINE_LED_COL_7                                     PAL_LINE(IOPORTC,  2)
#define LINE_LED_COL_8                                     PAL_LINE(IOPORTC,  3)
#define LINE_LED_COL_9                                     PAL_LINE(IOPORTB,  6)
#define LINE_LED_COL_10                                    PAL_LINE(IOPORTB,  7)
#define LINE_LED_COL_11                                    PAL_LINE(IOPORTB,  8)
#define LINE_LED_COL_12                                    PAL_LINE(IOPORTA,  0)
#define LINE_LED_COL_13                                    PAL_LINE(IOPORTA,  1)
#define LINE_LED_COL_14                                    PAL_LINE(IOPORTA,  2)

#define LINE_LED_ROW_1_R                                   PAL_LINE(IOPORTA, 11)
#define LINE_LED_ROW_1_G                                   PAL_LINE(IOPORTB,  2)
#define LINE_LED_ROW_1_B                                   PAL_LINE(IOPORTC, 14)
#define LINE_LED_ROW_2_R                                   PAL_LINE(IOPORTA, 14)
#define LINE_LED_ROW_2_G                                   PAL_LINE(IOPORTB,  3)
#define LINE_LED_ROW_2_B                                   PAL_LINE(IOPORTA,  6)
#define LINE_LED_ROW_3_R                                   PAL_LINE(IOPORTA, 15)
#define LINE_LED_ROW_3_G                                   PAL_LINE(IOPORTB,  4)
#define LINE_LED_ROW_3_B                                   PAL_LINE(IOPORTA,  7)
#define LINE_LED_ROW_4_R                                   PAL_LINE(IOPORTB,  0)
#define LINE_LED_ROW_4_G                                   PAL_LINE(IOPORTB,  5)
#define LINE_LED_ROW_4_B                                   PAL_LINE(IOPORTC,  4)
#define LINE_LED_ROW_5_R                                   PAL_LINE(IOPORTC,  5)
#define LINE_LED_ROW_5_G                                   PAL_LINE(IOPORTC,  7)
#define LINE_LED_ROW_5_B                                   PAL_LINE(IOPORTC,  6)
#else
#define LINE_LED_PWR                                       PAL_LINE(IOPORTB, 15)

#define LINE_USART_TX                                      PAL_LINE(IOPORTA, 4 )
#define LINE_USART_RX                                      PAL_LINE(IOPORTA, 5 )

// PORTA 12,13 conflict SWD
#define LINE_LED_COL_1                                     PAL_LINE(IOPORTA, 12)
#define LINE_LED_COL_2                                     PAL_LINE(IOPORTA, 13)
#define LINE_LED_COL_3                                     PAL_LINE(IOPORTA, 14)
#define LINE_LED_COL_4                                     PAL_LINE(IOPORTA, 15)
#define LINE_LED_COL_5                                     PAL_LINE(IOPORTB,  5)
#define LINE_LED_COL_6                                     PAL_LINE(IOPORTC,  1)
#define LINE_LED_COL_7                                     PAL_LINE(IOPORTC,  2)
#define LINE_LED_COL_8                                     PAL_LINE(IOPORTC,  3)
#define LINE_LED_COL_9                                     PAL_LINE(IOPORTB,  6)
#define LINE_LED_COL_10                                    PAL_LINE(IOPORTB,  7)
#define LINE_LED_COL_11                                    PAL_LINE(IOPORTB,  8)
#define LINE_LED_COL_12                                    PAL_LINE(IOPORTA,  1)
#define LINE_LED_COL_13                                    PAL_LINE(IOPORTA,  2)
#define LINE_LED_COL_14                                    PAL_LINE(IOPORTA,  3)

#define LINE_LED_ROW_1_R                                   PAL_LINE(IOPORTB,  0)
#define LINE_LED_ROW_1_G                                   PAL_LINE(IOPORTC,  0)
#define LINE_LED_ROW_1_B                                   PAL_LINE(IOPORTB,  4)
#define LINE_LED_ROW_2_R                                   PAL_LINE(IOPORTB,  1)
#define LINE_LED_ROW_2_G                                   PAL_LINE(IOPORTA,  8)
#define LINE_LED_ROW_2_B                                   PAL_LINE(IOPORTA,  6)
#define LINE_LED_ROW_3_R                                   PAL_LINE(IOPORTB,  2)
#define LINE_LED_ROW_3_G                                   PAL_LINE(IOPORTA, 10)
#define LINE_LED_ROW_3_B                                   PAL_LINE(IOPORTA,  7)
#define LINE_LED_ROW_4_R                                   PAL_LINE(IOPORTB,  3)
#define LINE_LED_ROW_4_G                                   PAL_LINE(IOPORTA, 11)
#define LINE_LED_ROW_4_B                                   PAL_LINE(IOPORTC,  4)
#define LINE_LED_ROW_5_R                                   PAL_LINE(IOPORTC,  7)
#define LINE_LED_ROW_5_G                                   PAL_LINE(IOPORTC,  5)
#define LINE_LED_ROW_5_B                                   PAL_LINE(IOPORTC,  6)
#endif

// clang-format on

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* BOARD_H */
