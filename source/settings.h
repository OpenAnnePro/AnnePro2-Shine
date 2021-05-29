#ifndef SETTINGS_INCLUDED
#define SETTINGS_INCLUDED

#include "light_utils.h"

/*
 * profile handlers
 */
typedef void (*keypress_handler)(led_t *colors, uint8_t row, uint8_t col);
typedef void (*profile_init)(led_t *colors);
typedef void (*lighting_callback)(led_t *);

typedef struct {
  // callback function implementing the lighting effect
  lighting_callback callback;
  // For static effects, their `callback` is called only once.
  // For dynamic effects, their `callback` is called in a loop.
  //
  // This field controls the animation speed by specifying how many LED refresh
  // cycles to skip before calling the callback again. For example, 1 in the
  // array means that `callback` is called on every cycle, ie. ~71Hz on default
  // settings.
  //
  // Different 4 values can be specified to allow different speeds of the same
  // effect. For static effects, the array must contain {0, 0, 0, 0}.
  uint16_t animationSpeed[4];
  // In case the profile is reactive, it responds to each keypress.
  // This callback is called with the locations of the pressed keys.
  keypress_handler keypressCallback;
  // Some profiles might need additional setup when just enabled.
  // This callback defines such logic if needed.
  profile_init profileInit;
} profile;

/* You can select your defaults in settings.c */
extern profile profiles[];
extern uint8_t currentProfile;
extern const uint8_t amountOfProfiles;
extern volatile uint8_t currentSpeed;

/* 0 - 7: Zero corresponds to the full intensity. */
extern uint8_t ledIntensity;

#endif
