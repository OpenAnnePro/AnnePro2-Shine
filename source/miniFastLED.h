/*
    License & Copyright notice: 
    The code included in this file and miniFastLED.c have been adapted from the FastLED project which is licensed under the MIT License.
    https://github.com/FastLED/FastLED/blob/master/LICENSE
*/

/*
The MIT License (MIT)

Copyright (c) 2013 FastLED

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef MINIFASTLED_INCLUDED 
#define MINIFASTLED_INCLUDED 

#include "board.h"
#include "hal.h"
#include "light_utils.h"

/*
    Function Signatures
*/
void hsv2rgb(uint8_t hue, uint8_t sat, uint8_t val, uint8_t* rgbResults);
void setAllKeysColorHSV(led_t* ledColors, uint8_t hue, uint8_t sat, uint8_t val);
void setColumnColorHSV(led_t* ledColors, uint8_t column, uint8_t hue, uint8_t sat, uint8_t val);
void setRowColorHSV(led_t* ledColors, uint8_t column, uint8_t hue, uint8_t sat, uint8_t val);

#endif
