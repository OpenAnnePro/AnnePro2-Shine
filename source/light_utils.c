/*
    ===  Lighting Utilities  ===
    This file contains functions useful for coding lighting profiles
*/
#include "light_utils.h"

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

/*
    Function declarations
*/

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

// Set specific key color
inline void setKeyColor(led_t *key, uint32_t color) { key->rgb = color; }
