/*
    ===  Lighting Utilities  ===
    This file contains functions useful for coding lighting profiles
*/
#include "light_utils.h"

/*
    #define Directives declaration
*/
#define LEN(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))


/*
    Static Declarations
*/

// An array of basic colors used accross different lighting profiles
static const uint32_t colorPalette[] = {0xF0000, 0xF0F00, 0x00F00, 0x00F0F, 0x0000F, 0xF000F, 0x50F0F};


// Array with Modifier keys IDs (Esc, Tab, Ctrl, Enter etc) 
static const uint8_t modKeyIDs[] = {0, 13, 14, 28, 40, 41, 42, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69};

/*
    Function declarations
*/

// Set all keys lighting to a specific color
void setAllKeysColor(led_t* ledColors, uint32_t color){
    const uint8_t red = (color >> 16) & 0xFF;
    const uint8_t green = (color >> 8) & 0xFF;
    const uint8_t blue = color & 0xFF;

    for (uint16_t i=0; i<NUM_COLUMN * NUM_ROW; ++i){
        ledColors[i].red = red;
        ledColors[i].green = green;
        ledColors[i].blue = blue;
    }
}

// Set modifier keys lighting to a specific color
void setModKeysColor(led_t* ledColors, uint32_t color){
    const uint8_t red = (color >> 16) & 0xFF;
    const uint8_t green = (color >> 8) & 0xFF;
    const uint8_t blue = color & 0xFF;
    
    for (uint16_t i=0; i<LEN(modKeyIDs); ++i){
        ledColors[modKeyIDs[i]].red = red;
        ledColors[modKeyIDs[i]].green = green;
        ledColors[modKeyIDs[i]].blue = blue;
    }
}

// Set specific key color
void setKeyColor(led_t *key, uint32_t color){
    key->red = (color >> 16) & 0xFF;
    key->green = (color >> 8) & 0xFF;
    key->blue = color & 0xFF;
}
