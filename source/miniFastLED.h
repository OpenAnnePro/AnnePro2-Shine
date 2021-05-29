/*
    License & Copyright notice:
    The code included in this file and miniFastLED.c have been adapted from the
   FastLED project which is licensed under the MIT License.
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
#include "settings.h"

/*
    HSV Constants
*/
#define HUE_RED 0
#define HUE_YELLOW 32
#define HUE_GREEN 64
#define HUE_CYAN 96
#define HUE_BLUE 128
#define HUE_MAGENTA 160

/*
    Helpers for dimming. There're used by hsv2rgb internally, but you can use
    them if you're using direct RGB colors and want to dim them naively.
 */

/* Dim a single value: R, G, B or a saturation component of the HSV color */
static inline int8_t dimValue(uint8_t value) {
  const uint8_t dimBy = ledIntensity * 30;
  if (dimBy > value)
    return 0;
  return value - dimBy;
}

/* Naively dim all values within the led_t variable */
static inline void naiveDimLed(led_t *color) {
  color->p.blue = dimValue(color->p.blue);
  color->p.red = dimValue(color->p.red);
  color->p.green = dimValue(color->p.green);
}

/* Returns a naively "dimmed" RGB color */
static inline uint32_t naiveDimRGB(uint32_t color) {
  led_t c;
  c.rgb = color;
  naiveDimLed(&c);
  return c.rgb;
}

/*
    Function Signatures
*/
void hsv2rgb(uint8_t hue, uint8_t sat, uint8_t val, led_t *rgbResults);
void setAllKeysColorHSV(led_t *ledColors, uint8_t hue, uint8_t sat,
                        uint8_t val);
void setColumnColorHSV(led_t *ledColors, uint8_t column, uint8_t hue,
                       uint8_t sat, uint8_t val);
void setRowColorHSV(led_t *ledColors, uint8_t column, uint8_t hue, uint8_t sat,
                    uint8_t val);

#endif
