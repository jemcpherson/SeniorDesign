#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 300
#define DATA_PIN 6

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN

// Define the array of leds
CRGB leds[NUM_LEDS];
int base_color = 0;

void setup() { 
      FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
      for(int i = 0; i < NUM_LEDS; i++)
      {
        leds[i] = 0xFFFFFF;
      }
      //FastLED.setBrightness(127);
      FastLED.show();
}

void loop() { 
  // Turn the LED on, then pause
  //FastLED.setBrightness(10);
  //for(int i = 0; i < 60; i++)
  //{
  //  leds[i] = 0xFF0000;
  //  FastLED.show();
  //  delay(5);
  //}
  //for(int i = 59; i >= 0; i--)
  //{
  //  leds[i] = 0x0000FF;
  //  FastLED.show();
  //  delay(5);
  //}
}
