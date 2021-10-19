/*
    ===  Lighting Utilities  ===
    This file contains functions useful for coding lighting profiles
*/
#include "light_utils.h"
#include "string.h"

/*
    #define Directives declaration
*/
#define LEN(x)                                                                 \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

/*
    Static Declarations
*/

// Array with Modifier keys IDs (Esc, Tab, Ctrl, Enter etc)
static const uint8_t modKeyIDs[] = {0,  13, 14, 28, 40, 41, 42, 54,
                                    55, 56, 57, 58, 59, 60, 61, 62,
                                    63, 64, 65, 66, 67, 68, 69};

// Array with Letter keys IDs (Q, W, E, R etc)
static const uint8_t letterKeyIDs[] = {15, 16, 17, 18, 19, 20, 21, 22, 23,
                                       24, 29, 30, 31, 32, 33, 34, 35, 36,
                                       37, 43, 44, 45, 46, 47, 48, 49, 50};

/*
    Function declarations
*/

// Set all keys to blank
void setAllKeysToBlank(led_t *ledColors) {
  memset(ledColors, 0, NUM_COLUMN * NUM_ROW * sizeof(*ledColors));
}

// Set all keys lighting to a specific color
void setAllKeysColor(led_t *ledColors, uint32_t color) {
  for (uint16_t i = 0; i < NUM_COLUMN * NUM_ROW; ++i) {
    ledColors[i].rgb = color;
  }
}

// Set modifier keys lighting to a specific color
void setModKeysColor(led_t *ledColors, uint32_t color) {
  for (uint16_t i = 0; i < LEN(modKeyIDs); ++i) {
    ledColors[modKeyIDs[i]].rgb = color;
  }
}

// Set letters keys lighting to a specific color
void setLetterKeysColor(led_t *ledColors, uint32_t color) {
  for (uint16_t i = 0; i < LEN(letterKeyIDs); ++i) {
    ledColors[letterKeyIDs[i]].rgb = color;
  }
}

// Set specific key color
inline void setKeyColor(led_t *key, uint32_t color) { key->rgb = color; }
