/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/**
 * leds.cpp - Marlin RGB LED general support
 */

#include "../../inc/MarlinConfig.h"

#if HAS_COLOR_LEDS

#include "leds.h"

#if ENABLED(BLINKM)
  #include "blinkm.h"
#endif

#if ENABLED(PCA9632)
  #include "pca9632.h"
#endif

#if ENABLED(PCA9533)
  #include "pca9533.h"
#endif

#if ENABLED(CASE_LIGHT_USE_RGB_LED)
  #include "../../feature/caselight.h"
#endif

#if ENABLED(LED_COLOR_PRESETS)
  const LEDColor LEDLights::defaultLEDColor = MakeLEDColor(
    LED_USER_PRESET_RED, LED_USER_PRESET_GREEN, LED_USER_PRESET_BLUE,
    LED_USER_PRESET_WHITE, LED_USER_PRESET_BRIGHTNESS
  );
#endif

#if EITHER(LED_CONTROL_MENU, PRINTER_EVENT_LEDS)
  LEDColor LEDLights::color;
  bool LEDLights::lights_on;
#endif

LEDLights leds;

void LEDLights::setup() {
  #if EITHER(RGB_LED, RGBW_LED)
    if (PWM_PIN(RGB_LED_R_PIN)) SET_PWM(RGB_LED_R_PIN); else SET_OUTPUT(RGB_LED_R_PIN);
    if (PWM_PIN(RGB_LED_G_PIN)) SET_PWM(RGB_LED_G_PIN); else SET_OUTPUT(RGB_LED_G_PIN);
    if (PWM_PIN(RGB_LED_B_PIN)) SET_PWM(RGB_LED_B_PIN); else SET_OUTPUT(RGB_LED_B_PIN);
    #if ENABLED(RGBW_LED)
      if (PWM_PIN(RGB_LED_W_PIN)) SET_PWM(RGB_LED_W_PIN); else SET_OUTPUT(RGB_LED_W_PIN);
    #endif
    #ifdef RGB_HW_PWM_FEATURE
      if (PWM_PIN(RGB_LED_R_PIN)) set_pwm_frequency(pin_t(RGB_LED_R_PIN), RGB_LED_FREQUENCY);
      if (PWM_PIN(RGB_LED_G_PIN)) set_pwm_frequency(pin_t(RGB_LED_G_PIN), RGB_LED_FREQUENCY);
      if (PWM_PIN(RGB_LED_B_PIN)) set_pwm_frequency(pin_t(RGB_LED_B_PIN), RGB_LED_FREQUENCY);
      #define LED_PINS_COUNT 3
      #if ENABLED(RGBW_LED)
        if (PWM_PIN(RGB_LED_W_PIN)) set_pwm_frequency(pin_t(RGB_LED_W_PIN), RGB_LED_FREQUENCY);
        #undef LED_PINS_COUNT
        #define LED_PINS_COUNT 4
      #endif
      #ifdef RGB_LED_STARTUP_ANIM
        //No optimisation to create natural delay
        uint16_t led_pwm;
        //Fading loop
        for (uint8_t i = 0; i < LED_PINS_COUNT; i++) {
          for (uint8_t b = 0; b < 201; b++) {
            if (b <= 100) led_pwm = b;
            if (b > 100) --led_pwm ;
            LIMIT(led_pwm,0,100);
            if (i == 0) set_pwm_duty(pin_t(RGB_LED_R_PIN), led_pwm);
            if (i == 1) set_pwm_duty(pin_t(RGB_LED_G_PIN), led_pwm);
            if (i == 2) set_pwm_duty(pin_t(RGB_LED_B_PIN), led_pwm);
            #if ENABLED(RGBW_LED)
              if (i == 3) set_pwm_duty(pin_t(RGB_LED_W_PIN), led_pwm);
            #endif
            if (i == 3) {
              delay(5);
            }
            else delay(2);
          }
        }
        //Ending
        /*set_pwm_duty(pin_t(RGB_LED_R_PIN), 255); delay(300); set_pwm_duty(RGB_LED_R_PIN, LOW);
        set_pwm_duty(pin_t(RGB_LED_G_PIN), 255); delay(300); set_pwm_duty(RGB_LED_G_PIN, LOW);
        set_pwm_duty(pin_t(RGB_LED_B_PIN), 255); delay(300); set_pwm_duty(RGB_LED_B_PIN, LOW);
        #if ENABLED(RGBW_LED)
          set_pwm_duty(pin_t(RGB_LED_W_PIN), 255); delay(300); set_pwm_duty(RGB_LED_W_PIN, LOW);
        #endif*/
      #endif//RGB_LED_STARTUP_ANIM
    #endif//RGB_HW_PWM_FEATURE
  #endif
  TERN_(NEOPIXEL_LED, neo.init());
  TERN_(PCA9533, PCA9533_init());
  TERN_(LED_USER_PRESET_STARTUP, set_default());
}

void LEDLights::set_color(const LEDColor &incol
  #if ENABLED(NEOPIXEL_LED)
    , bool isSequence/*=false*/
  #endif
) {

  #if ENABLED(NEOPIXEL_LED)

    const uint32_t neocolor = LEDColorWhite() == incol
                            ? neo.Color(NEO_WHITE)
                            : neo.Color(incol.r, incol.g, incol.b, incol.w);
    static uint16_t nextLed = 0;

    #ifdef NEOPIXEL_BKGD_LED_INDEX
      if (NEOPIXEL_BKGD_LED_INDEX == nextLed) {
        neo.set_color_background();
        if (++nextLed >= neo.pixels()) {
          nextLed = 0;
          return;
        }
      }
    #endif

    neo.set_brightness(incol.i);

    if (isSequence) {
      neo.set_pixel_color(nextLed, neocolor);
      neo.show();
      if (++nextLed >= neo.pixels()) nextLed = 0;
      return;
    }

    neo.set_color(neocolor);

  #endif

  #if ENABLED(BLINKM)

    // This variant uses i2c to send the RGB components to the device.
    blinkm_set_led_color(incol);

  #endif

  #if EITHER(RGB_LED, RGBW_LED)

    // This variant uses 3-4 separate pins for the RGB(W) components.
    // If the pins can do PWM then their intensity will be set.
    #define _UPDATE_RGBW(C,c) do {                \
      if (PWM_PIN(RGB_LED_##C##_PIN))             \
        analogWrite(pin_t(RGB_LED_##C##_PIN), c); \
      else                                        \
        WRITE(RGB_LED_##C##_PIN, c ? HIGH : LOW); \
    }while(0)
    #ifdef RGB_HW_PWM_FEATURE
      #undef _UPDATE_RGBW
      #define _UPDATE_RGBW(C,c) do {                \
      if (PWM_PIN(RGB_LED_##C##_PIN))             \
        set_pwm_duty(pin_t(RGB_LED_##C##_PIN), c); \
      else                                        \
        WRITE(RGB_LED_##C##_PIN, c ? HIGH : LOW); \
      }while(0)
    #endif
    #define UPDATE_RGBW(C,c) _UPDATE_RGBW(C, TERN1(CASE_LIGHT_USE_RGB_LED, caselight.on) ? c : 0)
    uint8_t new_r, new_g, new_b
      #if ENABLED(RGBW_LED)
        , new_w
      #endif
      ;
    uint8_t old_r = leds.color.r,old_g = leds.color.g,old_b = leds.color.b
      #if ENABLED(RGBW_LED)
        ,old_w = leds.color.w;
      #endif
      ;
      if (!leds.lights_on) {
        old_r = old_g = old_b
        #if ENABLED(RGBW_LED)
          = old_w
        #endif
        = 0;
      }
      // To avoid fading loop when too little on LCD menu, and blocked screen
      if (abs(incol.r - old_r) > 10 || abs(incol.g - old_g) > 10 || abs(incol.b - old_b) > 10
       #if ENABLED(RGBW_LED)
         || abs(incol.w - old_w) > 10
       #endif
         ) {
        for (int16_t i = 0; i < 256; i++) {
          new_r= old_r + ( (i * (incol.r - old_r) ) / 255);
          new_g= old_g + ( (i * (incol.g - old_g) ) / 255);
          new_b= old_b + ( (i * (incol.b - old_b) ) / 255);
          UPDATE_RGBW(R,new_r); UPDATE_RGBW(G,new_g); UPDATE_RGBW(B,new_b);
          #if ENABLED(RGBW_LED)
            new_w= old_w + ( (i * (incol.w - old_w) ) / 255);
            UPDATE_RGBW(W,new_w);
          #endif
          delay(1);
        }
      }
      else
      {
        UPDATE_RGBW(R,incol.r); UPDATE_RGBW(G,incol.g); UPDATE_RGBW(B,incol.b);
        #if ENABLED(RGBW_LED)
          UPDATE_RGBW(W,incol.w);
        #endif
      }

  #endif

  // Update I2C LED driver
  TERN_(PCA9632, PCA9632_set_led_color(incol));
  TERN_(PCA9533, PCA9533_set_rgb(incol.r, incol.g, incol.b));

  #if EITHER(LED_CONTROL_MENU, PRINTER_EVENT_LEDS)
    // Don't update the color when OFF
    lights_on = !incol.is_off();
    if (lights_on) color = incol;
  #endif
}

#if ENABLED(LED_CONTROL_MENU)
  void LEDLights::toggle() { if (lights_on) set_off(); else update(); }
#endif

#ifdef LED_BACKLIGHT_TIMEOUT

  millis_t LEDLights::led_off_time; // = 0

  void LEDLights::update_timeout(const bool power_on) {
    if (lights_on) {
      const millis_t ms = millis();
      if (power_on)
        reset_timeout(ms);
      else if (ELAPSED(ms, led_off_time))
        set_off();
    }
  }

#endif

#if ENABLED(NEOPIXEL2_SEPARATE)

  #if ENABLED(NEO2_COLOR_PRESETS)
    const LEDColor LEDLights2::defaultLEDColor = MakeLEDColor(
      NEO2_USER_PRESET_RED, NEO2_USER_PRESET_GREEN, NEO2_USER_PRESET_BLUE,
      NEO2_USER_PRESET_WHITE, NEO2_USER_PRESET_BRIGHTNESS
    );
  #endif

  #if ENABLED(LED_CONTROL_MENU)
    LEDColor LEDLights2::color;
    bool LEDLights2::lights_on;
  #endif

  LEDLights2 leds2;

  void LEDLights2::setup() {
    neo2.init();
    TERN_(NEO2_USER_PRESET_STARTUP, set_default());
  }

  void LEDLights2::set_color(const LEDColor &incol) {
    const uint32_t neocolor = LEDColorWhite() == incol
                            ? neo2.Color(NEO2_WHITE)
                            : neo2.Color(incol.r, incol.g, incol.b, incol.w);
    neo2.set_brightness(incol.i);
    neo2.set_color(neocolor);

    #if ENABLED(LED_CONTROL_MENU)
      // Don't update the color when OFF
      lights_on = !incol.is_off();
      if (lights_on) color = incol;
    #endif
  }

  #if ENABLED(LED_CONTROL_MENU)
    void LEDLights2::toggle() { if (lights_on) set_off(); else update(); }
  #endif

#endif  // NEOPIXEL2_SEPARATE

#endif  // HAS_COLOR_LEDS
