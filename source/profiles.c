#include "profiles.h"
#include "miniFastLED.h"
#include "string.h"

// An array of basic colors used accross different lighting profiles
// static const uint32_t colorPalette[] = {0xFF0000, 0xF0F00, 0x00F00, 0x00F0F,
// 0x0000F, 0xF000F, 0x50F0F};
static const uint32_t colorPalette[] = {0xcc0000, 0xcccc00, 0x5fcc00, 0x00c7cc,
                                        0x006ecc, 0x0033ff, 0x6900cc, 0xcc00bf};

#define LEN(a) (sizeof(a) / sizeof(*a))

void red(led_t *currentKeyLedColors) {
  setAllKeysColor(currentKeyLedColors, 0xFF0000);
}

void green(led_t *currentKeyLedColors) {
  setAllKeysColor(currentKeyLedColors, 0x00FF00);
}

void blue(led_t *currentKeyLedColors) {
  setAllKeysColor(currentKeyLedColors, 0x0000FF);
}

/* Color bleed test pattern */
void colorBleed(led_t *currentKeyLedColors) {
  for (uint16_t c = 0; c < NUM_COLUMN; ++c) {
    for (uint16_t r = 0; r < NUM_ROW; ++r) {
      const uint16_t i = c + NUM_COLUMN * r;
      if (c == 0) {
        if (r <= 3)
          currentKeyLedColors[i].rgb = 0x0000FF;
        else
          currentKeyLedColors[i].rgb = 0xFF0000;
        continue;
      }
      if (r == 0) {
        if (c <= 10)
          currentKeyLedColors[i].rgb = 0x0000FF;
        else
          currentKeyLedColors[i].rgb = 0xFF0000;
        continue;
      }

      switch (i % 3) {
      case 0:
        currentKeyLedColors[i].rgb = 0x0000FF;
        break;
      case 1:
        currentKeyLedColors[i].rgb = 0x00FF00;
        break;
      case 2:
        currentKeyLedColors[i].rgb = 0xFF0000;
        break;
      }
    }
  }
}

void white(led_t *currentKeyLedColors) {
  /* To get "white" you need to compensate for red/blue switches on board */
  setAllKeysColor(currentKeyLedColors, 0x80ff99);
}

void miamiNights(led_t *currentKeyLedColors) {
  setAllKeysColor(currentKeyLedColors, 0x00979c);
  setModKeysColor(currentKeyLedColors, 0x9c008f);
}

void rainbowHorizontal(led_t *currentKeyLedColors) {
  for (uint16_t i = 0; i < NUM_ROW; ++i) {
    for (uint16_t j = 0; j < NUM_COLUMN; ++j) {
      setKeyColor(&currentKeyLedColors[i * NUM_COLUMN + j], colorPalette[i]);
    }
  }
}

void rainbowVertical(led_t *currentKeyLedColors) {
  for (uint16_t i = 0; i < NUM_COLUMN; ++i) {
    for (uint16_t j = 0; j < NUM_ROW; ++j) {
      setKeyColor(&currentKeyLedColors[j * NUM_COLUMN + i],
                  colorPalette[i % LEN(colorPalette)]);
    }
  }
}

static uint8_t colAnimOffset = 0;
void animatedRainbowVertical(led_t *currentKeyLedColors) {
  for (uint16_t i = 0; i < NUM_COLUMN; ++i) {
    for (uint16_t j = 0; j < NUM_ROW; ++j) {
      setKeyColor(&currentKeyLedColors[j * NUM_COLUMN + i],
                  colorPalette[(i + colAnimOffset) % LEN(colorPalette)]);
    }
  }
  colAnimOffset = (colAnimOffset + 1) % LEN(colorPalette);
}

static uint8_t flowValue[NUM_COLUMN] = {0,  11, 22, 33,  44,  55,  66,
                                        77, 88, 99, 110, 121, 132, 143};
void animatedRainbowFlow(led_t *currentKeyLedColors) {
  for (int i = 0; i < NUM_COLUMN; i++) {
    setColumnColorHSV(currentKeyLedColors, i, flowValue[i], 255, 255);
    if (flowValue[i] >= 179 && flowValue[i] < 240) {
      flowValue[i] = 240;
    }
    flowValue[i] += 3;
  }
}

static uint8_t waterfallValue[NUM_COLUMN] = {0,  10, 20, 30,  40,  50,  60,
                                             70, 80, 90, 100, 110, 120, 130};
void animatedRainbowWaterfall(led_t *currentKeyLedColors) {
  for (int i = 0; i < NUM_ROW; i++) {
    setRowColorHSV(currentKeyLedColors, i, waterfallValue[i], 255, 125);
    if (waterfallValue[i] >= 179 && waterfallValue[i] < 240) {
      waterfallValue[i] = 240;
    }
    waterfallValue[i] += 3;
  }
}

static uint8_t breathingValue = 180;
static int breathingDirection = -1;
void animatedBreathing(led_t *currentKeyLedColors) {
  setAllKeysColorHSV(currentKeyLedColors, 85, 255, breathingValue);
  if (breathingValue >= 180) {
    breathingDirection = -2;
  } else if (breathingValue <= 2) {
    breathingDirection = 2;
  }
  breathingValue += breathingDirection;
}

static uint8_t spectrumValue = 2;
static int spectrumDirection = 1;
void animatedSpectrum(led_t *currentKeyLedColors) {
  setAllKeysColorHSV(currentKeyLedColors, spectrumValue, 255, 125);
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
void animatedWave(led_t *currentKeyLedColors) {
  for (int i = 0; i < NUM_COLUMN; i++) {
    if (waveValue[i] >= 140) {
      waveDirection[i] = -3;
    } else if (waveValue[i] <= 10) {
      waveDirection[i] = 3;
    }
    setColumnColorHSV(currentKeyLedColors, i, 190, 255, waveValue[i]);
    waveValue[i] += waveDirection[i];
  }
}

uint8_t animatedPressedBuf[NUM_ROW * NUM_COLUMN] = {0};

void reactiveFade(led_t *ledColors) {
  for (int i = 0; i < NUM_ROW * NUM_COLUMN; i++) {
    if (animatedPressedBuf[i] > 5) {
      animatedPressedBuf[i] -= 5;
      hsv2rgb(100 - animatedPressedBuf[i], 255, 225, &ledColors[i]);
    } else if (animatedPressedBuf[i] > 0) {
      ledColors[i].p.blue = 0;
      ledColors[i].p.red = 0;
      ledColors[i].p.green = 0;
      animatedPressedBuf[i] = 0;
    }
  }
}

void reactiveFadeKeypress(led_t *ledColors, uint8_t row, uint8_t col) {
  int i = row * NUM_COLUMN + col;
  animatedPressedBuf[i] = 100;
  ledColors[i].p.green = 0;
  ledColors[i].p.red = 0xFF;
  ledColors[i].p.blue = 0;
}

void reactiveFadeInit(led_t *ledColors) {
  // create a quick "falling" animation to make it easier to see
  // that this profile is activated
  for (int i = 0; i < NUM_ROW; i++) {
    for (int j = 0; j < NUM_COLUMN; j++) {
      animatedPressedBuf[i * NUM_COLUMN + j] = i * 15 + 25;
    }
  }
  memset(ledColors, 0, NUM_ROW * NUM_COLUMN * sizeof(*ledColors));
}

uint8_t pulseBuf[NUM_ROW];

void reactivePulse(led_t *ledColors) {
  uint8_t pulseSpeed = 16;

  for (int i = 0; i < NUM_ROW; i++) {
    if (pulseBuf[i] > 80) {
      pulseBuf[i] -= pulseSpeed;
    } else if (pulseBuf[i] > pulseSpeed) {
      for (int j = 0; j < NUM_COLUMN; j++) {
        ledColors[i * NUM_COLUMN + j].p.blue = (175 + pulseBuf[i]);
        ledColors[i * NUM_COLUMN + j].p.red = 0;
        ledColors[i * NUM_COLUMN + j].p.green = 0;
      }
      pulseBuf[i] -= pulseSpeed;
    } else if (pulseBuf[i] > 0) {
      pulseBuf[i] = 0;
      for (int j = 0; j < NUM_COLUMN; j++) {
        ledColors[i * NUM_COLUMN + j].p.blue = 0;
        ledColors[i * NUM_COLUMN + j].p.red = 0;
        ledColors[i * NUM_COLUMN + j].p.green = 0;
      }
    }
  }
}

void reactivePulseKeypress(led_t *ledColors, uint8_t row, uint8_t col) {
  (void)ledColors;
  (void)col;

  pulseBuf[row] = 80;
}

void reactivePulseInit(led_t *ledColors) {
  for (int i = 0; i < NUM_ROW; i++) {
    pulseBuf[i] = 80 + i * 5;
  }
  memset(ledColors, 0, NUM_ROW * NUM_COLUMN * 3);
}
