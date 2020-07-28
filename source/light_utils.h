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
void setAllKeysColor(led* ledColors, uint32_t color);
void setModKeysColor(led* ledColors, uint32_t color);
void setKeyColor(led *key, uint32_t color);
