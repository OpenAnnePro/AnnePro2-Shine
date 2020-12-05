#include "light_utils.h"

/*
 * STATIC
 */
void red(led_t* currentKeyLedColors, uint8_t intensity);
void green(led_t* currentKeyLedColors, uint8_t intensity);
void blue(led_t* currentKeyLedColors, uint8_t intensity);
void rainbowHorizontal(led_t* currentKeyLedColors, uint8_t intensity);
void rainbowVertical(led_t* currentKeyLedColors, uint8_t intensity);
void miamiNights(led_t* currentKeyLedColors, uint8_t intensity);

/*
 * ANIMATED
 */
void animatedRainbowVertical(led_t* currentKeyLedColors, uint8_t intensity);
void animatedRainbowFlow(led_t* currentKeyLedColors, uint8_t intensity);
void animatedRainbowWaterfall(led_t* currentKeyLedColors, uint8_t intensity);
void animatedBreathing(led_t* currentKeyLedColors, uint8_t intensity);
void animatedSpectrum(led_t* currentKeyLedColors, uint8_t intensity);
void animatedWave(led_t* currentKeyLedColors, uint8_t intensity);
