#include "profiles.h"
#include "miniFastLED.h"

// An array of basic colors used accross different lighting profiles
// static const uint32_t colorPalette[] = {0xFF0000, 0xF0F00, 0x00F00, 0x00F0F, 0x0000F, 0xF000F, 0x50F0F};
static const uint32_t colorPalette[] = {
  0x9c0000, 0x9c9900, 0x1f9c00, 
  0x00979c, 0x003e9c, 0x39009c, 
  0x9c008f
};

#define LEN(a) (sizeof(a)/sizeof(*a))

void red(led_t* currentKeyLedColors){
  setAllKeysColor(currentKeyLedColors, 0xFF0000);
}

void green(led_t* currentKeyLedColors){
  setAllKeysColor(currentKeyLedColors, 0x00FF00);
}

void blue(led_t* currentKeyLedColors){
  setAllKeysColor(currentKeyLedColors, 0x0000FF);
}

void miamiNights(led_t* currentKeyLedColors){
  setAllKeysColor(currentKeyLedColors, 0x00979c);
  setModKeysColor(currentKeyLedColors, 0x9c008f);
}

void rainbowHorizontal(led_t* currentKeyLedColors){
  for (uint16_t i=0; i<NUM_ROW; ++i){
    for (uint16_t j=0; j<NUM_COLUMN; ++j){
      setKeyColor(&currentKeyLedColors[i*NUM_COLUMN+j], colorPalette[i%LEN(colorPalette)]);
    }     
  }
}

void rainbowVertical(led_t* currentKeyLedColors){
  for (uint16_t i=0; i<NUM_COLUMN; ++i){
    for (uint16_t j=0; j<NUM_ROW; ++j){
      setKeyColor(&currentKeyLedColors[j*NUM_COLUMN+i], colorPalette[i%LEN(colorPalette)]);
    }     
  }
}

static uint8_t colAnimOffset = 0;
void animatedRainbowVertical(led_t* currentKeyLedColors){
  for (uint16_t i=0; i<NUM_COLUMN; ++i){
    for (uint16_t j=0; j<NUM_ROW; ++j){
      setKeyColor(&currentKeyLedColors[j*NUM_COLUMN+i], colorPalette[(i + colAnimOffset)%LEN(colorPalette)]);
    }     
  }
  colAnimOffset = (colAnimOffset + 1)%LEN(colorPalette);
}

static uint8_t flowValue[NUM_COLUMN] = {
  0, 11, 22, 33, 44, 55, 66, 77,
  88, 99, 110, 121, 132, 143
};
void animatedRainbowFlow(led_t* currentKeyLedColors){
  for(int i = 0; i < NUM_COLUMN; i++){
    setColumnColorHSV(currentKeyLedColors, i, flowValue[i], 255, 125);
    if(flowValue[i] == 179){
      flowValue[i] = 240;
    }
    flowValue[i]++;
  }
}

static uint8_t waterfallValue[NUM_COLUMN] = {
  0, 10, 20, 30, 40, 50, 60, 70,
  80, 90, 100, 110, 120, 130
};
void animatedRainbowWaterfall(led_t* currentKeyLedColors){
  for(int i = 0; i < NUM_ROW; i++){
    setRowColorHSV(currentKeyLedColors, i, waterfallValue[i], 255, 125);
    if(waterfallValue[i] == 179){
      waterfallValue[i] = 240;
    }
    waterfallValue[i]++;
  }
}

static uint8_t breathingValue = 180;
static int breathingDirection = -1;
void animatedBreathing(led_t* currentKeyLedColors){
  setAllKeysColorHSV(currentKeyLedColors, 85, 255, breathingValue);
  if(breathingValue >= 180){
    breathingDirection = -1;
  }else if(breathingValue <= 0){
    breathingDirection = 1;
  }
  breathingValue += breathingDirection;
}

static uint8_t spectrumValue = 2;
static int spectrumDirection = 1;
void animatedSpectrum(led_t* currentKeyLedColors){
  setAllKeysColorHSV(currentKeyLedColors, spectrumValue, 255, 125);
  if(spectrumValue >= 179){
    spectrumDirection = -1;
  }else if(spectrumValue <= 1){
    spectrumDirection = 1;
  }
  spectrumValue += spectrumDirection;
}