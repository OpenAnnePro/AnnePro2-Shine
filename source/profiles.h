#include "light_utils.h"

/*
 * STATIC
 */
bool red(led_t *currentKeyLedColors, uint8_t intensity);
bool green(led_t *currentKeyLedColors, uint8_t intensity);
bool blue(led_t *currentKeyLedColors, uint8_t intensity);
bool rainbowHorizontal(led_t *currentKeyLedColors, uint8_t intensity);
bool rainbowVertical(led_t *currentKeyLedColors, uint8_t intensity);
bool miamiNights(led_t *currentKeyLedColors, uint8_t intensity);

/*
 * ANIMATED
 */
bool animatedRainbowVertical(led_t *currentKeyLedColors, uint8_t intensity);
bool animatedRainbowFlow(led_t *currentKeyLedColors, uint8_t intensity);
bool animatedRainbowWaterfall(led_t *currentKeyLedColors, uint8_t intensity);
bool animatedBreathing(led_t *currentKeyLedColors, uint8_t intensity);
bool animatedSpectrum(led_t *currentKeyLedColors, uint8_t intensity);
bool animatedWave(led_t *currentKeyLedColors, uint8_t intensity);

/*
 * ANIMATED - responding to key presses
 */
bool reactiveFade(led_t *ledColors, uint8_t intensity);
void reactiveFadeKeypress(led_t *ledColors, uint8_t row, uint8_t col,
                          uint8_t intensity);
void reactiveFadeInit(led_t *ledColors);
