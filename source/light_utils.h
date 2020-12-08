#ifndef LIGHT_UTILS_INCLUDED
#define LIGHT_UTILS_INCLUDED

#include "board.h"
#include "hal.h"

/*
    Structs
*/
// Struct defining an LED and its RGB color components
typedef struct {
  uint8_t red, green, blue;
} led_t;

/*
    Function Signatures
*/
void setAllKeysColor(led_t *ledColors, uint32_t color, uint8_t intensity);
void setModKeysColor(led_t *ledColors, uint32_t color, uint8_t intensity);
void setKeyColor(led_t *key, uint32_t color, uint8_t intensity);

#endif
