#include "profiles.h"
#include "miniFastLED.h"
#include "string.h"

// An array of basic colors used accross different lighting profiles
// static const uint32_t colorPalette[] = {0xFF0000, 0xF0F00, 0x00F00, 0x00F0F,
// 0x0000F, 0xF000F, 0x50F0F};
static const uint32_t colorPalette[] = {0x9c0000, 0x9c9900, 0x1f9c00, 0x00979c,
                                        0x003e9c, 0x39009c, 0x9c008f};

#define LEN(a) (sizeof(a) / sizeof(*a))

void red(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColor(currentKeyLedColors, 0xFF0000, intensity);
}

void green(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColor(currentKeyLedColors, 0x00FF00, intensity);
}

void blue(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColor(currentKeyLedColors, 0x0000FF, intensity);
}

void miamiNights(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColor(currentKeyLedColors, 0x00979c, intensity);
  setModKeysColor(currentKeyLedColors, 0x9c008f, intensity);
}

void rainbowHorizontal(led_t *currentKeyLedColors, uint8_t intensity) {
  for (uint16_t i = 0; i < NUM_ROW; ++i) {
    for (uint16_t j = 0; j < NUM_COLUMN; ++j) {
      setKeyColor(&currentKeyLedColors[i * NUM_COLUMN + j],
                  colorPalette[i % LEN(colorPalette)], intensity);
    }
  }
}

void rainbowVertical(led_t *currentKeyLedColors, uint8_t intensity) {
  for (uint16_t i = 0; i < NUM_COLUMN; ++i) {
    for (uint16_t j = 0; j < NUM_ROW; ++j) {
      setKeyColor(&currentKeyLedColors[j * NUM_COLUMN + i],
                  colorPalette[i % LEN(colorPalette)], intensity);
    }
  }
}

static uint8_t colAnimOffset = 0;
void animatedRainbowVertical(led_t *currentKeyLedColors, uint8_t intensity) {
  for (uint16_t i = 0; i < NUM_COLUMN; ++i) {
    for (uint16_t j = 0; j < NUM_ROW; ++j) {
      setKeyColor(&currentKeyLedColors[j * NUM_COLUMN + i],
                  colorPalette[(i + colAnimOffset) % LEN(colorPalette)],
                  intensity);
    }
  }
  colAnimOffset = (colAnimOffset + 1) % LEN(colorPalette);
}

void christmasLights(led_t *currentKeyLedColors, uint8_t intensity) {
  //sets of red and green to be alternated
  uint32_t christmasCol[] = {0xFF0000, 0xFF0000, 0xFF0000, 0x008000,
    0x008000, 0x008000, 0x008000};
  for (uint16_t i = 0; i < NUM_COLUMN; ++i) {
    for (uint16_t j = 0; j < NUM_ROW; ++j) {
      setKeyColor(&currentKeyLedColors[j * NUM_COLUMN + i],
                  christmasCol[(i + colAnimOffset) % LEN(christmasCol)],
                  intensity);
    }
  }
  colAnimOffset = (colAnimOffset + 1) % LEN(christmasCol);
}


static uint8_t flowValue[NUM_COLUMN] = {0,  11, 22, 33,  44,  55,  66,
                                        77, 88, 99, 110, 121, 132, 143};
void animatedRainbowFlow(led_t *currentKeyLedColors, uint8_t intensity) {
  for (int i = 0; i < NUM_COLUMN; i++) {
    setColumnColorHSV(currentKeyLedColors, i, flowValue[i], 255, 125,
                      intensity);
    if (flowValue[i] >= 179 && flowValue[i] < 240) {
      flowValue[i] = 240;
    }
    flowValue[i] += 3;
  }
}

static uint8_t waterfallValue[NUM_COLUMN] = {0,  10, 20, 30,  40,  50,  60,
                                             70, 80, 90, 100, 110, 120, 130};
void animatedRainbowWaterfall(led_t *currentKeyLedColors, uint8_t intensity) {
  for (int i = 0; i < NUM_ROW; i++) {
    setRowColorHSV(currentKeyLedColors, i, waterfallValue[i], 255, 125,
                   intensity);
    if (waterfallValue[i] >= 179 && waterfallValue[i] < 240) {
      waterfallValue[i] = 240;
    }
    waterfallValue[i] += 3;
  }
}

static uint8_t breathingValue = 180;
static int breathingDirection = -1;
void animatedBreathing(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColorHSV(currentKeyLedColors, 85, 255, breathingValue, intensity);
  if (breathingValue >= 180) {
    breathingDirection = -3;
  } else if (breathingValue <= 2) {
    breathingDirection = 3;
  }
  breathingValue += breathingDirection;
}

static uint8_t spectrumValue = 2;
static int spectrumDirection = 1;
void animatedSpectrum(led_t *currentKeyLedColors, uint8_t intensity) {
  setAllKeysColorHSV(currentKeyLedColors, spectrumValue, 255, 125, intensity);
  if (spectrumValue >= 177) {
    spectrumDirection = -3;
  } else if (spectrumValue <= 2) {
    spectrumDirection = 3;
  }
  spectrumValue += spectrumDirection;
}

static uint8_t waveValue[NUM_COLUMN] = {0,  0,  0,  10,  15,  20,  25,
                                        40, 55, 75, 100, 115, 135, 140};
static int waveDirection[NUM_COLUMN] = {3, 3, 3, 3, 3, 3, 3,
                                        3, 3, 3, 3, 3, 3, 3};
void animatedWave(led_t *currentKeyLedColors, uint8_t intensity) {
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
}

uint8_t animatedPressedFadeBuf[NUM_ROW * NUM_COLUMN] = {0};

void reactiveFade(led_t *ledColors, uint8_t intensity) {
  for (int i = 0; i < NUM_ROW * NUM_COLUMN; i++) {
    if (animatedPressedFadeBuf[i] > 5) {
      animatedPressedFadeBuf[i] -= 5;
      hsv2rgb(100 - animatedPressedFadeBuf[i], 255, 125,
              (uint8_t *)&ledColors[i]);
      ledColors[i].red >>= intensity;
      ledColors[i].green >>= intensity;
      ledColors[i].blue >>= intensity;
    } else if (animatedPressedFadeBuf[i] > 0) {
      ledColors[i].blue = 0;
      ledColors[i].red = 0;
      ledColors[i].green = 0;
      animatedPressedFadeBuf[i] = 0;
    }
  }
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
  memset(animatedPressedFadeBuf, 0, sizeof(animatedPressedFadeBuf));
  memset(ledColors, 0, NUM_ROW * NUM_COLUMN * 3);
}
