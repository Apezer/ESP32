/**
 * rgb_led.h - WS2812B RGB strip effects.
 */

#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>

#define RGB_LED_PIN         17
#define RGB_LED_COUNT       8
#define RGB_LED_BRIGHTNESS  40

enum RgbLedEffect : uint8_t {
    RGB_EFFECT_RAINBOW = 0,
    RGB_EFFECT_BREATHING,
    RGB_EFFECT_COMET,
    RGB_EFFECT_COLOR_WIPE,
    RGB_EFFECT_FIRE,
    RGB_EFFECT_SOLID_FADE,
    RGB_EFFECT_COUNT,
};

void rgbLedInit();
void rgbLedUpdate();
void rgbLedNextEffect();
void rgbLedSetEffect(uint8_t effect);
uint8_t rgbLedGetEffect();
const char *rgbLedGetEffectName();

#endif /* RGB_LED_H */
