#ifndef LIGHT_UTILS_INCLUDED
#define LIGHT_UTILS_INCLUDED

#include "board.h"
#include "hal.h"

/*
    Structs
*/
// Struct defining an LED and its RGB color components
typedef union {
  struct {
    /* Little endian ordering to match uint32_t */
    uint8_t blue, green, red;
    uint8_t align;
  } p;
  /* Index access: 0 - blue, 1 - green, 2 - red */
  uint8_t pv[4];
  /* 0xrgb in mem is b g r X */
  uint32_t rgb;
} led_t;

/*
    Function Signatures
*/
void setAllKeysColor(led_t *ledColors, uint32_t color);
void setModKeysColor(led_t *ledColors, uint32_t color);
void setKeyColor(led_t *key, uint32_t color);

#endif
