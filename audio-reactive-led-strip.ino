/* The main program of

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

*/

#include "driver/i2s.h"
#include <FastLED.h>
#include "includes/FFT.h"
#include "includes/VisualEffect.h"

/* Some viriables in programs

  BUFFER_SIZE: How many samples to be received each time. This number must be the power of 2.
  const uint16_t BUFFER_SIZE = 1024;

  N_ROLLING_HISTORY: How many buffers to be processed each time. This number must be the power of 2: 1,2,4,8,...
  const uint8_t N_ROLLING_HISTORY = 2;

  SAMPLE_RATE: The sample rate of the audio every second.
  const uint16_t SAMPLE_RATE = 44100;

  N_PIXELS: The number of the LEDS on the led strip, it must be even.
  const uint16_t N_PIXELS = 60;

  N_MEL_BIN: The number of the channels of the mel frequency.
  const uint16_t N_MEL_BIN = 18;

  MIN_FREQUENCY, MAX_FREQUENCY: The audio's min/max frequency to be processed. The max frequency always less than SAMPLE_RATE/2
  const float MIN_FREQUENCY = 200;
  const float MAX_FREQUENCY = 12000;

  MIN_VOLUME_THRESHOLD: If the audio's volume is less than this number, the signal will not be processed.
  const float MIN_VOLUME_THRESHOLD = 0.0003;

  PDM_WS_IO_PIN, PDM_DATA_IN_PIN: Microphone(type of PDM)'s WS Pin and DATA_IN Pin, connecting to GPIO
  const int PDM_WS_IO_PIN = 19;
  const int PDM_DATA_IN_PIN = 22;

  LED_STRIP_DATA_PIN, LED_STRIP_CLOCK_PIN: Led-strip's data pin and clock pin, connecting to GPIO
  If you use a led-strip with clock pin, you should modify the FastLED.addLeds calling in programs.
  const int LED_STRIP_DATA_PIN = 21;
  const int LED_STRIP_CLOCK_PIN = 17;

  TOUCH_PAD_PIN: TOUCH PAD's number. TOUCH_PAD_NUM9 is GPIO32. https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/driver/driver/touch_pad.h
  const touch_pad_t TOUCH_PAD_PIN = TOUCH_PAD_NUM9;

*/

const uint16_t BUFFER_SIZE = 1024;
const uint8_t N_ROLLING_HISTORY = 2;
const uint16_t SAMPLE_RATE = 44100;
const uint16_t N_PIXELS = 60;
const uint16_t N_MEL_BIN = 18;
const float MIN_FREQUENCY = 200;
const float MAX_FREQUENCY = 12000;
const float MIN_VOLUME_THRESHOLD = 0.0003;

const int PDM_WS_IO_PIN = 19;
const int PDM_DATA_IN_PIN = 22;
const int LED_STRIP_DATA_PIN = 21;
const int LED_STRIP_CLOCK_PIN = 17;
const touch_pad_t TOUCH_PAD_PIN = TOUCH_PAD_NUM9; /*TOUCH_PAD_NUM9 is GPIO32*/

float y_data[BUFFER_SIZE * N_ROLLING_HISTORY];
class FFT fft(BUFFER_SIZE*N_ROLLING_HISTORY, N_MEL_BIN, MIN_FREQUENCY, MAX_FREQUENCY, SAMPLE_RATE, MIN_VOLUME_THRESHOLD);
class VisualEffect effect(N_MEL_BIN, N_PIXELS);
CRGB physic_leds[N_PIXELS];

typedef enum PLAYMODE {
  MODE_OFF = 0,
  MODE_ON = 1,
  MODE_RAINBOW = 2,
  MODE_SCROLL = 3,
  MODE_ENERGY = 4,
  MODE_SPECTRUM = 5,
  MODE_MAX
};
PLAYMODE CurrentMode = MODE_SCROLL;
#define LED_PIN    5
#define NUM_LEDS    49
#define BRIGHTNESS  255
#define LED_TYPE    UCS1903
#define COLOR_ORDER BRG
void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(physic_leds, N_PIXELS);
  //FastLED.addLeds<APA102, LED_STRIP_DATA_PIN, LED_STRIP_CLOCK_PIN, GRB>(physic_leds, N_PIXELS);
  Serial.begin(115200);

  touch_pad_init();
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
  touch_pad_config(TOUCH_PAD_PIN, 0);
}

void loop() {
  static float mel_data[N_MEL_BIN];

  for (int i = 0; i < N_ROLLING_HISTORY - 1; i++)
    memcpy(y_data + i * BUFFER_SIZE, y_data + (i + 1)*BUFFER_SIZE, sizeof(float)*BUFFER_SIZE);

  int16_t l[BUFFER_SIZE];

  unsigned int read_num;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    y_data[i] = analogRead(15)/4095.0f;
  }

 

  fft.t2mel( y_data, mel_data );

  switch (CurrentMode) {
    case MODE_OFF:
      fill_solid(physic_leds, N_PIXELS, CRGB::Black);
      break;
    case MODE_ON:
      fill_solid(physic_leds, N_PIXELS, CRGB::White);
      break;
    case MODE_SCROLL:
      effect.visualize_scroll(mel_data, physic_leds);
      break;
    case MODE_ENERGY:
      effect.visualize_energy(mel_data, physic_leds);
      break;
    case MODE_SPECTRUM:
      effect.visualize_spectrum(mel_data, physic_leds);
      break;
    case MODE_RAINBOW:
      static uint8_t gHue = 0;
      fill_rainbow(physic_leds, N_PIXELS, gHue, 7);
      EVERY_N_MILLISECONDS( 20 ) {
        gHue++;
      }
      break;
  }
  FastLED.show();

  static uint32_t oldtime = 0;
  uint16_t touch_value;
  touch_pad_read(TOUCH_PAD_NUM9, &touch_value);
  if ((touch_value < 1000) && (millis() - oldtime > 1000)) {
    oldtime = millis();
    CurrentMode = PLAYMODE((CurrentMode + 1) % MODE_MAX);
    Serial.println("Mudou");

  switch (CurrentMode) {
    case MODE_OFF:
     Serial.println("OFF");
      break;
    case MODE_ON:
      Serial.println("ON");
      break;
    case MODE_SCROLL:
       Serial.println("Scroll");
      break;
    case MODE_ENERGY:
      Serial.println("Energy");
      break;
    case MODE_SPECTRUM:
       Serial.println("Spectrum");
      break;
    case MODE_RAINBOW:
       Serial.println("Rainbow");
      break;
  }
  }
  else if (touch_value > 1000)
    oldtime = 0;

  yield();
}
