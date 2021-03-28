#include "settings.h"
#include "profiles.h"

/*
 * Active profiles
 * Add profiles from source/profiles.h in the profile array
 */
profile profiles[] = {
    /* {colorBleed, {0, 0, 0, 0}, NULL, NULL}, */
    {white, {0, 0, 0, 0}, NULL, NULL},
    {red, {0, 0, 0, 0}, NULL, NULL},
    {green, {0, 0, 0, 0}, NULL, NULL},
    {blue, {0, 0, 0, 0}, NULL, NULL},
    {rainbowHorizontal, {0, 0, 0, 0}, NULL, NULL},
    {rainbowVertical, {0, 0, 0, 0}, NULL, NULL},
    {animatedRainbowVertical, {35, 28, 21, 14}, NULL, NULL},
    {animatedRainbowFlow, {7, 5, 2, 1}, NULL, NULL},
    {animatedRainbowWaterfall, {7, 5, 2, 1}, NULL, NULL},
    {animatedBreathing, {5, 3, 2, 1}, NULL, NULL},
    {animatedWave, {5, 3, 2, 1}, NULL, NULL},
    {animatedSpectrum, {11, 6, 4, 1}, NULL, NULL},
    {reactiveFade, {4, 3, 2, 1}, reactiveFadeKeypress, reactiveFadeInit},
    {reactivePulse, {4, 3, 2, 1}, reactivePulseKeypress, reactivePulseInit}};

/* Set your defaults here */
uint8_t currentProfile = 0;
const uint8_t amountOfProfiles = sizeof(profiles) / sizeof(profile);
volatile uint8_t currentSpeed = 0;
uint8_t ledIntensity = 0;
