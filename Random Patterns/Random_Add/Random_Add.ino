#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 60
#define DATA_PIN 6

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN

// Define the array of leds
CRGB leds[NUM_LEDS];
int base_color = 0;
int gHue;
int gHueDelta = 3;
CRGB color = 0xFF00FF;

void setup() { 
      // Uncomment/edit one of the following lines for your leds arrangement.
      // FastLED.addLeds<TM1803, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<TM1804, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<TM1809, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
      FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
      // FastLED.addLeds<APA104, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<UCS1903, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<UCS1903B, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<GW6205, DATA_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<GW6205_400, DATA_PIN, RGB>(leds, NUM_LEDS);
      
      // FastLED.addLeds<WS2801, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<SM16716, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<LPD8806, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<P9813, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<APA102, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<DOTSTAR, RGB>(leds, NUM_LEDS);

      // FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<SM16716, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<LPD8806, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<P9813, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      // FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
      for(int i = 0; i < 60; i++)
      {
        leds[i] = color;//CHSV(gHue, 126, 126);
      }
      FastLED.setBrightness(50);
}

void loop() { 
  // Turn the LED on, then pause
  int i = random8(60);
  //int j = random8(60);
 // int k = random8(60);
  leds[i] = color;
  //leds[j] = color;
  //leds[k] = color;
  FastLED.show();
  delay(40);
  nscale8(leds, 60, 220);
}

CRGB randcolor()
{
  int r = random8();
  int g = random8();
  int b = random8();
  int maxval = max3(r,g,b);
  if(r == maxval)
  {
    g /= 3;
    b /= 3;
  }
  else if(g == maxval)
  {
    r /= 3;
    b /= 3;
  }
  else if(b == maxval)
  {
    r /= 3;
    g /= 3;
  }
  return CRGB(r,g,b);  
}

int max3(int r, int g, int b)
{
  int maxval = r;
  if(g > maxval) maxval = g;
  if(b > maxval) maxval = b;
  return maxval;  
}
