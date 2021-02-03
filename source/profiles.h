#include "light_utils.h"

/*
 * STATIC
 */
void red(led_t *currentKeyLedColors, uint8_t intensity);
void green(led_t *currentKeyLedColors, uint8_t intensity);
void blue(led_t *currentKeyLedColors, uint8_t intensity);
void rainbowHorizontal(led_t *currentKeyLedColors, uint8_t intensity);
void rainbowVertical(led_t *currentKeyLedColors, uint8_t intensity);
void miamiNights(led_t *currentKeyLedColors, uint8_t intensity);

/*
 * ANIMATED
 */
void animatedRainbowVertical(led_t *currentKeyLedColors, uint8_t intensity);
void animatedRainbowFlow(led_t *currentKeyLedColors, uint8_t intensity);
void animatedRainbowWaterfall(led_t *currentKeyLedColors, uint8_t intensity);
void animatedBreathing(led_t *currentKeyLedColors, uint8_t intensity);
void animatedSpectrum(led_t *currentKeyLedColors, uint8_t intensity);
void animatedWave(led_t *currentKeyLedColors, uint8_t intensity);

/*
 * ANIMATED - responding to key presses
 */
void reactiveFade(led_t *ledColors, uint8_t intensity);
void reactiveFadeKeypress(led_t *ledColors, uint8_t row, uint8_t col,
                          uint8_t intensity);
void reactiveFadeInit(led_t *ledColors);

void reactivePulse(led_t *ledColors, uint8_t intensity);
void reactivePulseKeypress(led_t *ledColors, uint8_t row, uint8_t col,
                           uint8_t intensity);
void reactivePulseInit(led_t *ledColors);
