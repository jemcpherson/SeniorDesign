#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 60
#define DATA_PIN 6

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.

// Define the array of leds
CRGB leds[NUM_LEDS];
int base_color = 0;
int pattern = 1;

//Joshua McPherson
void setup() { 
      FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
      FastLED.setBrightness(40);
      fill_rainbow(leds, 60, 0, 43);
}

//Joshua McPherson
void loop() { 
  check_inputs();
  process_audio();
  light_step();
}

//Joshua McPherson
void check_inputs()
{
  
}

void initialize_leds()
{
   switch(pattern)
    {
        case 1:
          break;
        case 2:
          break;
        case 3:
          break;
    }
}

//Joshua McPherson
void process_audio()
{
  
}

//Joshua McPherson
void light_step()
{
    switch(pattern)
    {
        case 1:
          pattern1();
          break;
        case 2:
          break;
        case 3:
          break;
    }
}

//Joshua McPherson
void pattern1()
{
    CRGB temp[NUM_LEDS];
    memcpy(temp, leds, sizeof(leds));
    for(int i = 0; i < 59; i++)
    {
      leds[i] = temp[i+1];
    }
    leds[59] = leds[0];
    FastLED.show();
    delay(40);
}
