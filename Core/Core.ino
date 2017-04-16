// The Adafruit code sample specified in the User Manual provided some of the global variables here, as well as a portion of the setup() and loop() functions here.
// The sample also provided us with some ideas on how to approach te development of our pattern methods.

#include <Bounce2.h>
#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 300                   // Number of LEDs per strip
#define LED_OUT_HIGH 6                 // Digital output pin for sending data to high led strips
#define LED_OUT_LOW 11                 // Digital output pin for sending data to low led strips

#define ARM_MATH_CM4                   // From Adafruit Sample.
#include <arm_math.h>                  // From Adafruit Sample.
#include <FastLED.h>                   // From Adafruit Sample.

int SAMPLE_RATE_HZ = 9000;             // Sample rate of the audio in hertz. From Adafruit Sample.
float SPECTRUM_MIN_DB = 20.0;          // Audio intensity (in decibels) that maps to low LED brightness. From Adafruit Sample.
float SPECTRUM_MAX_DB = 70.0;          // Audio intensity (in decibels) that maps to high LED brightness. From Adafruit Sample.
const int FFT_SIZE = 256;              // Size of the FFT.  Realistically can only be at most 256 
                                       // without running out of memory for buffers and other state. From Adafruit Sample.

const int AUDIO_INPUT_PIN = 14;        // Input ADC pin for audio data. From Adafruit Sample.
const int ANALOG_READ_RESOLUTION = 10; // Bits of resolution for the ADC. From Adafruit Sample.
const int ANALOG_READ_AVERAGING = 16;  // Number of samples to average with each ADC reading. From Adafruit Sample.

IntervalTimer samplingTimer;           // From Adafruit Sample.
float samples[FFT_SIZE*2];             // From Adafruit Sample.
float magnitudes[FFT_SIZE];            // From Adafruit Sample.
int sampleCounter = 0;                 // From Adafruit Sample.

const int PATTERN_INPUT_PIN = 2;       // Input digital pin for changing pattern
const int COLOR_INPUT_PIN = 3;         // Input digital pin for changing color
const int BRIGHTNESS_INPUT_PIN = 4;    // Input digital pin for changing brightness

const int SCORE_INPUT_PIN = 21;        // Pin corresponding to the digital interrupt signal for a point being scored
const int WIN_INPUT_PIN = 22;          // Pin corresponding to the digital interrupt signal for a game being won
const int NEWGAME_INPUT_PIN = 23;      // Pin corresponding to the digital interrupt signal for a new game being started

// Objects utilized to check for button presses
Bounce patternButton = Bounce();        
Bounce colorButton = Bounce();
Bounce brightnessButton = Bounce();
Bounce scoreButton = Bounce();
Bounce winButton = Bounce();
Bounce newGameButton = Bounce();

CRGB h_leds[NUM_LEDS];                // Array of colors corresponding to upper strips of leds on trailer
CRGB l_leds[NUM_LEDS];                // Array of colors corresponding to lower strips of leds on trailer 
int color_selection = 4;              // Index used to identify the current color scheme
int pattern_selection = 1;            // Index used to identify the current pattern
int brightness = 1;                   // Index used to identify the curret brightness setting. 

// Array holding the different color schemes. 0x123456 is a placeholder indicating the multicolor scheme.
CRGB colors[5] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, 0x123456}; 

// Array which will be populated with a full spectrum of colors. Used during the multicolored color scheme.
CRGB rainbow[256];
int rainbow_index = 0;  //Index identifying the current location in rainbow. Used during the multicolored color scheme.

// Indices of FFT bins which divide two frequency ranges
int bass_lowmid_division;
int lowmid_mid_division;
int mid_highmid_division;
int highmid_presence_division;

int highstrip_rangesize = 100;  // Width of pulses on upper led strip for pattern 2 
int lowstrip_rangesize = 150;   // Width of pulses on lower led strip for pattern 2

//Midpoints of pulses on led strips for pattern 2
int highstrip_midrange_midpoint = 50;
int highstrip_highmidrange_midpoint = 150;
int highstrip_presencerange_midpoint = 250;
int lowstrip_bassrange_midpoint = 75;
int lowstrip_lowmidrange_midpoint = 225;

void setup() {

  // Set the pin mode for inputs
  pinMode(PATTERN_INPUT_PIN, INPUT);
  pinMode(COLOR_INPUT_PIN, INPUT);
  pinMode(BRIGHTNESS_INPUT_PIN, INPUT);
  pinMode(SCORE_INPUT_PIN, INPUT);
  pinMode(WIN_INPUT_PIN, INPUT);
  pinMode(NEWGAME_INPUT_PIN, INPUT);

  //Attach button objects to appropriate pins and set appropriate delays
  patternButton.attach(PATTERN_INPUT_PIN); patternButton.interval(5);
  colorButton.attach(COLOR_INPUT_PIN); colorButton.interval(5);
  brightnessButton.attach(BRIGHTNESS_INPUT_PIN); brightnessButton.interval(5);
  scoreButton.attach(SCORE_INPUT_PIN); scoreButton.interval(5);
  winButton.attach(WIN_INPUT_PIN); winButton.interval(5);
  newGameButton.attach(NEWGAME_INPUT_PIN); newGameButton.interval(5);
    
  // Initialize fastled library and turn off the LEDs
  FastLED.addLeds<NEOPIXEL, LED_OUT_HIGH>(h_leds, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, LED_OUT_LOW>(l_leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
  
  //Generate full spectrum of colors and store to "rainbow"
  fill_rainbow(rainbow, 256, 0, 1);

  //Set up range divisions, based on frequencies listed at http://www.teachmeaudio.com/mixing/techniques/audio-spectrum 
  bass_lowmid_division = frequencyToBin(250.0);
  lowmid_mid_division = frequencyToBin(500.0);
  mid_highmid_division = frequencyToBin(2000.0);
  highmid_presence_division = frequencyToBin(4000.0);
  
  // Set up ADC and audio input.
  pinMode(AUDIO_INPUT_PIN, INPUT);
  analogReadResolution(ANALOG_READ_RESOLUTION);
  analogReadAveraging(ANALOG_READ_AVERAGING);
  
  // Begin sampling audio
  samplingBegin();
  Serial.begin(9600);
}

// Iterates through 
void loop() {
  check_inputs();
  // Calculate FFT if a full sample is available.
  if (samplingIsDone()) {
    // Run FFT on sample data.
    arm_cfft_radix4_instance_f32 fft_inst;
    arm_cfft_radix4_init_f32(&fft_inst, FFT_SIZE, 0, 1);
    arm_cfft_radix4_f32(&fft_inst, samples);
    // Calculate magnitude of complex numbers output by the FFT.
    arm_cmplx_mag_f32(samples, magnitudes, FFT_SIZE);
    
    if(get_average_magnitude() > 40)
    { 
      light_step();
    }
    else
    {
      FastLED.clear();
      FastLED.show();  
    }
    
    // Restart audio sampling.
    samplingBegin();
  }
}

//Joshua McPherson
void check_inputs()
{
  patternButton.update();
  colorButton.update();
  brightnessButton.update();
  scoreButton.update();
  winButton.update();
  newGameButton.update();
  if (patternButton.rose()) change_pattern_selection();
  if (colorButton.rose()) change_color_selection();
  if (brightnessButton.rose()) change_brightness();
  if (scoreButton.rose()) scoreInterrupt();
  if (winButton.rose()) winInterrupt();
  if (newGameButton.rose()) newGameInterrupt();
}

//Joshua McPherson
void light_step()
{
    switch(pattern_selection)
    {
        case 1:
          pattern1();
          break;
        case 2:
          pattern2();
          break;
        case 3:
          pattern3();
          break;
    }
}

// John Chaney
void change_pattern_selection()
{
  pattern_selection++;
  if (pattern_selection > 3) pattern_selection = 1;
}

// John Chaney
void change_color_selection()
{
  color_selection++;
  if (color_selection > 4) color_selection = 0;
}

// John Chaney
void change_brightness()
{
  brightness++;
  switch (brightness){
    case 1:
      FastLED.setBrightness(255);
      break;
    case 2:
      FastLED.setBrightness(90);
      break;
    case 3:
      FastLED.setBrightness(40);
      break;
    case 4:
      FastLED.setBrightness(0);
      break;
  }
  if (brightness > 3) brightness = 0;
}

// Controls the first pattern. Each FFT magnitude bin corresponds to one or two leds on each strip. 
// The top strip lights up if the FFT magnitude bin exceeds a threshhold, and the bottom strip is the inverse of the top strip.
void pattern1()
{
    float avg = get_average_magnitude();
    CRGB curr_color;
    if(color_selection == 4)
    {
      rainbow_index++;
      if(rainbow_index >= 256) rainbow_index = 0;
      curr_color = rainbow[rainbow_index];
    }
    else
    {
      curr_color = colors[color_selection];
    }
    for (int i = 0; i < NUM_LEDS; i++)
    {
          if (magnitudes[i] >= 400)
          {
              h_leds[i] = curr_color;
              l_leds[i] = CRGB::Black;
          }
          else
          {
              h_leds[i] = CRGB::Black;
              l_leds[i] = curr_color;
          }
    }
    FastLED.show();
}

// Controls the second pattern. Pulsars from multiple points flash with lengths with correspond with the intensity of the music in the mid range of frequencies
void pattern2()
{
    float bass_mag = get_average_portion_magnitude(0,bass_lowmid_division);
    float lowmid_mag = get_average_portion_magnitude(bass_lowmid_division,lowmid_mid_division);
    float mid_mag = get_average_portion_magnitude(lowmid_mid_division,mid_highmid_division);
    float highmid_mag = get_average_portion_magnitude(mid_highmid_division,highmid_presence_division);
    float presence_mag = get_average_portion_magnitude(highmid_presence_division, FFT_SIZE);
    float avg = (bass_mag + lowmid_mag + mid_mag + highmid_mag + presence_mag)/5;
    
    FastLED.clear();
    float ratio_base = avg;
    
    int m_leds = int((mid_mag/ratio_base)*highstrip_rangesize/2);
    if (m_leds >= 50) m_leds = 49;
    int hm_leds = int((highmid_mag/ratio_base)*highstrip_rangesize/2);
    if (hm_leds >= 50) hm_leds = 49;
    int p_leds = int((presence_mag/ratio_base)*highstrip_rangesize/2);
    if (p_leds >= 50) p_leds = 49;
    int b_leds = int((bass_mag/ratio_base)*lowstrip_rangesize/2);
    if (hm_leds >= 75) hm_leds = 74;
    int lm_leds = int((lowmid_mag/ratio_base)*lowstrip_rangesize/2);
    if (lm_leds >= 75) lm_leds = 74;

    CRGB curr_color;
    if(color_selection == 4)
    {
      rainbow_index++;
      if(rainbow_index >= 256) rainbow_index = 0;
      curr_color = rainbow[rainbow_index];
    }
    else
    {
      curr_color = colors[color_selection];
    }

    for(int i = highstrip_midrange_midpoint-m_leds; i < highstrip_midrange_midpoint+m_leds; i++)
    {
      h_leds[i] = curr_color;
    }
    for(int i = highstrip_highmidrange_midpoint-hm_leds; i < highstrip_highmidrange_midpoint+hm_leds; i++)
    {
      h_leds[i] = curr_color;  
    }
    for(int i = highstrip_presencerange_midpoint-p_leds; i < highstrip_presencerange_midpoint+p_leds; i++)
    {
      h_leds[i] = curr_color;  
    }
    for(int i = lowstrip_bassrange_midpoint-b_leds; i < lowstrip_bassrange_midpoint+b_leds; i++)
    {
      l_leds[i] = curr_color;  
    }
    for(int i = lowstrip_lowmidrange_midpoint-lm_leds; i < lowstrip_lowmidrange_midpoint+lm_leds; i++)
    {
      l_leds[i] = curr_color;  
    }
    FastLED.show();
}

// Controls the third pattern. A range of leds correspond to a range of FFT bins, and if the average magnitude for a range is above a given threshhold the entire range of leds lights up.
void pattern3()
{
    Serial.println("Getting Low Average:");
    float low_avg = get_average_portion_magnitude(1, FFT_SIZE/2);
    Serial.print("Low avg: ");
    Serial.println(low_avg);
    Serial.println("Getting High Average:");
    float high_avg = get_average_portion_magnitude(FFT_SIZE/2, FFT_SIZE);
    Serial.print("High avg: ");
    Serial.println(high_avg);
    CRGB curr_color;
    if(color_selection == 4)
    {
      rainbow_index++;
      if(rainbow_index >= 256) rainbow_index = 0;
      curr_color = rainbow[rainbow_index];
    }
    else
    {
      curr_color = colors[color_selection];
    }
    int num_buckets = 10;
    int buckets_per_strip = num_buckets/2;
    float* buckets = (float*)(malloc(sizeof(float)*num_buckets));
    Serial.println("Allocated Bucket Space");
    Serial.println("Contents of Buckets:");
    for(int i = 0; i < num_buckets; i++)
    {
      buckets[i] = get_average_portion_magnitude(i*int(float(FFT_SIZE)/num_buckets),(i+1)*int(float(FFT_SIZE)/num_buckets));
      Serial.print(buckets[i]);
      Serial.print(" ");
    }
    Serial.println("");
    FastLED.clear();
    Serial.println("Cleared LEDs");
    for (int i = 0; i < buckets_per_strip; i++)
    {
      for (int j = 0; j < NUM_LEDS/buckets_per_strip; j++)
      {
        int loc = i*(NUM_LEDS/buckets_per_strip) + j;
        if(buckets[i] > 220 || buckets[i] > high_avg) h_leds[loc] = curr_color;
        else h_leds[loc] = CRGB::Black;
      }
    }
    for (int i = buckets_per_strip; i < num_buckets; i++)
    {
      for (int j = 0; j < NUM_LEDS/buckets_per_strip; j++)
      {
        int loc = (i-buckets_per_strip)*(NUM_LEDS/buckets_per_strip) + j;
        if(buckets[i] > 240 || buckets[i] > low_avg) l_leds[loc] = curr_color;
        else l_leds[loc] = CRGB::Black;
      }
    } 
    Serial.println("Colored LEDs");
    free(buckets);
    FastLED.show();
    Serial.println("Displayed LEDs");
}

// Flashes red along the entire length of the strip for several seconds.
void scoreInterrupt()
{
  for (int i = 0; i < 2; i++)
  {
    for (int i = 0; i < NUM_LEDS; i++) 
    {
      h_leds[i] = CRGB::Red;
      l_leds[i] = CRGB::Red;
    }
    FastLED.show();
    FastLED.delay(200);
  
    FastLED.clear();
    FastLED.show();
    FastLED.delay(200);
  }
}

// Flashes a cycle of colors on the entire length of all strips for several seconds.
void winInterrupt()
{
  int col_val;
  for (int i = 0; i < 12; i++)
  {
    for (int i = 0; i < NUM_LEDS; i++) 
    {
      h_leds[i] = rainbow[col_val];
      l_leds[i] = rainbow[col_val];
    }
    FastLED.show();
    FastLED.delay(100);
    FastLED.clear();
    FastLED.show();
    FastLED.delay(100);
    col_val = (col_val + 70)%256;
  }
}

// Flashes random colors at random locations (the same color and relative location on each strip) quickly for several seconds.
void newGameInterrupt()
{
  FastLED.clear();
  FastLED.show();
  float currVal = 1;
  for(int t = 0; t < 70; t++)
  {
    for(int c = 0; c < 300; c++)
    {
      int c_index = random8();
      int led_index = random16(300);
      h_leds[led_index] = rainbow[c_index];
      l_leds[led_index] = rainbow[c_index];
    }
    if (t < 35)
    {
      currVal /= 2;  
    }
    else
    {
      currVal *= 2;  
    }
    FastLED.show();
    FastLED.delay(50);
    FastLED.clear();
  }
}

// Determine the average magnitude over the full range of FFT magnitude bins.
float get_average_magnitude()
{
    return get_average_portion_magnitude(0, FFT_SIZE);
}

// Determine the average magnitude over the range of FFT magnitude bins specified.
// int low: The inclusive minimum index of the range of magnitude bins to determine the average for.
// int high: The exclusive maximum index of the range of magnitude bins to determine the average for.
float get_average_portion_magnitude(int low, int high)
{
    float sum = 0;
    for(int i = low; i < high; i++)
    {
        sum += magnitudes[i];    
        Serial.print(magnitudes[i]);
        Serial.print(" "); 
    }
    Serial.println("");
    float avg = sum/(high-low);
    return avg;
}

// From Adafruit sample.
// Convert a frequency to the appropriate FFT bin it will fall within.
int frequencyToBin(float frequency) {
  float binFrequency = float(SAMPLE_RATE_HZ) / float(FFT_SIZE);
  return int(frequency / binFrequency);
}


////////////////////////////////////////////////////////////////////////////////
// SAMPLING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// From Adafruit sample.
void samplingCallback() {
  // Read from the ADC and store the sample data
  samples[sampleCounter] = (float32_t)analogRead(AUDIO_INPUT_PIN);
  // Complex FFT functions require a coefficient for the imaginary part of the input.
  // Since we only have real data, set this coefficient to zero.
  samples[sampleCounter+1] = 0.0;
  // Update sample buffer position and stop after the buffer is filled
  sampleCounter += 2;
  if (sampleCounter >= FFT_SIZE*2) {
    samplingTimer.end();
  }
}

// From Adafruit sample.
void samplingBegin() {
  // Reset sample buffer position and start callback at necessary rate.
  sampleCounter = 0;
  samplingTimer.begin(samplingCallback, 1000000/SAMPLE_RATE_HZ);
}

// From Adafruit sample.
boolean samplingIsDone() {
  return sampleCounter >= FFT_SIZE*2;
}
