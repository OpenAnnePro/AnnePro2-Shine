#include "board.h"
#include "hal.h"

/*
    Structs
*/
// Struct defining an LED and its RGB color components
typedef struct led{
    uint8_t red, green, blue;
}led;

/*
    Function Signatures
*/
void setAllKeysColor(led* ledColors, uint32_t color);
void setModKeysColor(led* ledColors, uint32_t color);
void setKeyColor(led *key, uint32_t color);