#include "profiles.h"
#include "miniFastLED.h"
#include "string.h"

// An array of basic colors used accross different lighting profiles
// static const uint32_t colorPalette[] = {0xFF0000, 0xF0F00, 0x00F00, 0x00F0F,
// 0x0000F, 0xF000F, 0x50F0F};
static const uint32_t colorPalette[] = {0xcc0000, 0xccc900, 0x5fcc00, 0x00c7cc,
                                        0x006ecc, 0x0033ff, 0x6900cc, 0xcc00bf};

#define LEN(a) (sizeof(a) / sizeof(*a))

bool red(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColor(currentKeyLedColors, 0xFF0000, intensity);
  return true;
}

bool green(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColor(currentKeyLedColors, 0x00FF00, intensity);
  return true;
}

bool blue(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColor(currentKeyLedColors, 0x0000FF, intensity);
  return true;
}

bool miamiNights(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColor(currentKeyLedColors, 0x00979c, intensity);
  setModKeysColor(currentKeyLedColors, 0x9c008f, intensity);
  return true;
}

bool rainbowHorizontal(led_t *currentKeyLedColors, uint8_t intensity) {
  for (uint16_t i = 0; i < NUM_ROW; ++i) {
    for (uint16_t j = 0; j < NUM_COLUMN; ++j) {
      setKeyColor(&currentKeyLedColors[i * NUM_COLUMN + j],
                  colorPalette[i % LEN(colorPalette)], intensity);
    }
  }
  return true;
}

bool rainbowVertical(led_t *currentKeyLedColors, uint8_t intensity) {
  for (uint16_t i = 0; i < NUM_COLUMN; ++i) {
    for (uint16_t j = 0; j < NUM_ROW; ++j) {
      setKeyColor(&currentKeyLedColors[j * NUM_COLUMN + i],
                  colorPalette[i % LEN(colorPalette)], intensity);
    }
  }
  return true;
}

static uint8_t colAnimOffset = 0;
bool animatedRainbowVertical(led_t *currentKeyLedColors, uint8_t intensity) {
  for (uint16_t i = 0; i < NUM_COLUMN; ++i) {
    for (uint16_t j = 0; j < NUM_ROW; ++j) {
      setKeyColor(&currentKeyLedColors[j * NUM_COLUMN + i],
                  colorPalette[(i + colAnimOffset) % LEN(colorPalette)],
                  intensity);
    }
  }
  colAnimOffset = (colAnimOffset + 1) % LEN(colorPalette);
  return true;
}

static uint8_t flowValue[NUM_COLUMN] = {0,  11, 22, 33,  44,  55,  66,
                                        77, 88, 99, 110, 121, 132, 143};
bool animatedRainbowFlow(led_t *currentKeyLedColors, uint8_t intensity) {
  for (int i = 0; i < NUM_COLUMN; i++) {
    setColumnColorHSV(currentKeyLedColors, i, flowValue[i], 255, 255,
                      intensity);
    if (flowValue[i] >= 179 && flowValue[i] < 240) {
      flowValue[i] = 240;
    }
    flowValue[i] += 3;
  }
  return true;
}

static uint8_t waterfallValue[NUM_COLUMN] = {0,  10, 20, 30,  40,  50,  60,
                                             70, 80, 90, 100, 110, 120, 130};
bool animatedRainbowWaterfall(led_t *currentKeyLedColors, uint8_t intensity) {
  for (int i = 0; i < NUM_ROW; i++) {
    setRowColorHSV(currentKeyLedColors, i, waterfallValue[i], 255, 125,
                   intensity);
    if (waterfallValue[i] >= 179 && waterfallValue[i] < 240) {
      waterfallValue[i] = 240;
    }
    waterfallValue[i] += 3;
  }
  return true;
}

static uint8_t breathingValue = 180;
static int breathingDirection = -1;
bool animatedBreathing(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColorHSV(currentKeyLedColors, 85, 255, breathingValue, intensity);
  if (breathingValue >= 180) {
    breathingDirection = -3;
  } else if (breathingValue <= 2) {
    breathingDirection = 3;
  }
  breathingValue += breathingDirection;
  return true;
}

static uint8_t spectrumValue = 2;
static int spectrumDirection = 1;
bool animatedSpectrum(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColorHSV(currentKeyLedColors, spectrumValue, 255, 125, intensity);
  if (spectrumValue >= 177) {
    spectrumDirection = -3;
  } else if (spectrumValue <= 2) {
    spectrumDirection = 3;
  }
  spectrumValue += spectrumDirection;
  return true;
}

static uint8_t waveValue[NUM_COLUMN] = {0,  0,  0,  10,  15,  20,  25,
                                        40, 55, 75, 100, 115, 135, 140};
static int waveDirection[NUM_COLUMN] = {3, 3, 3, 3, 3, 3, 3,
                                        3, 3, 3, 3, 3, 3, 3};
bool animatedWave(led_t *currentKeyLedColors, uint8_t intensity) {
  for (int i = 0; i < NUM_COLUMN; i++) {
    if (waveValue[i] >= 140) {
      waveDirection[i] = -3;
    } else if (waveValue[i] <= 10) {
      waveDirection[i] = 3;
    }
    setColumnColorHSV(currentKeyLedColors, i, 190, 255, waveValue[i],
                      intensity);
    waveValue[i] += waveDirection[i];
  }
  return true;
}

uint8_t animatedPressedFadeBuf[NUM_ROW * NUM_COLUMN] = {0};

bool reactiveFade(led_t *ledColors, uint8_t intensity) {
  bool shouldUpdate = false;
  for (int i = 0; i < NUM_ROW * NUM_COLUMN; i++) {
    if (animatedPressedFadeBuf[i] > 5) {
      shouldUpdate = true;
      animatedPressedFadeBuf[i] -= 5;
      hsv2rgb(100 - animatedPressedFadeBuf[i], 255, 225,
              (uint8_t *)&ledColors[i]);
      ledColors[i].red >>= intensity;
      ledColors[i].green >>= intensity;
      ledColors[i].blue >>= intensity;
    } else if (animatedPressedFadeBuf[i] > 0) {
      shouldUpdate = true;
      ledColors[i].blue = 0;
      ledColors[i].red = 0;
      ledColors[i].green = 0;
      animatedPressedFadeBuf[i] = 0;
    }
  }
  return shouldUpdate;
}

void reactiveFadeKeypress(led_t *ledColors, uint8_t row, uint8_t col,
                          uint8_t intensity) {
  int i = row * NUM_COLUMN + col;
  animatedPressedFadeBuf[i] = 100;
  ledColors[i].green = 0;
  ledColors[i].red = 0xFF >> intensity;
  ledColors[i].blue = 0;
}

void reactiveFadeInit(led_t *ledColors) {
  // create a quick "falling" animation to make it easier to see
  // that this profile is activated
  for (int i = 0; i < NUM_ROW; i++) {
    for (int j = 0; j < NUM_COLUMN; j++) {
      animatedPressedFadeBuf[i * NUM_COLUMN + j] = i * 15 + 25;
    }
  }
  memset(ledColors, 0, NUM_ROW * NUM_COLUMN * 3);
}
