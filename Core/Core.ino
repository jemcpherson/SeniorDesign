#include <Bounce2.h>

#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 300
#define LED_OUT_HIGH_LEFT 6
#define LED_OUT_LOW_LEFT 11
#define CYCLE_THRESHOLD 1

//Start of Adafruit//
#define ARM_MATH_CM4
#include <arm_math.h>
#include <FastLED.h>

int SAMPLE_RATE_HZ = 9000;             // Sample rate of the audio in hertz.
float SPECTRUM_MIN_DB = 20.0;          // Audio intensity (in decibels) that maps to low LED brightness.
float SPECTRUM_MAX_DB = 70.0;          // Audio intensity (in decibels) that maps to high LED brightness.
const int FFT_SIZE = 256;              // Size of the FFT.  Realistically can only be at most 256 
                                       // without running out of memory for buffers and other state.
const int PATTERN_INPUT_PIN = 2;       // Input digital pin for changing pattern
const int COLOR_INPUT_PIN = 3;         // Input digital pin for changing color
const int BRIGHTNESS_INPUT_PIN = 4;    // Input digital pin for changing brightness

// temporary interrupt buttons - will be changed later to be simply incoming 0 or 1 bits
const int SCORE_INPUT_PIN = 21;
const int WIN_INPUT_PIN = 22;
const int NEWGAME_INPUT_PIN = 23;

const int AUDIO_INPUT_PIN = 14;        // Input ADC pin for audio data.
const int ANALOG_READ_RESOLUTION = 10; // Bits of resolution for the ADC.
const int ANALOG_READ_AVERAGING = 16;  // Number of samples to average with each ADC reading.

IntervalTimer samplingTimer;
float samples[FFT_SIZE*2];
float magnitudes[FFT_SIZE];
int sampleCounter = 0;
float frequencyWindow[NUM_LEDS+1];
//float hues[NUM_LEDS];
CRGB leds[NUM_LEDS];
//End of Adafruit Sample Vars//

Bounce patternButton = Bounce();
Bounce colorButton = Bounce();
Bounce brightnessButton = Bounce();
Bounce scoreButton = Bounce();
Bounce winButton = Bounce();
Bounce newGameButton = Bounce();

int cycleCount = 0;

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define LED_OUT.

// Define the array of leds
CRGB h_leds[NUM_LEDS]; //Upper strips of leds on trailer
CRGB l_leds[NUM_LEDS]; //Lower strips of leds on trailer 
int color_selection = 4;
int pattern_selection = 1;
int brightness = 0;    //Brightness selection variable, from 0-2
CRGB colors[5] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, 0x123456}; 
CRGB p3_colors[6] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Orange, CRGB::Violet}; 
CRGB rainbow[256];

//Audio Range Divisions
int bass_lowmid_division;
int lowmid_mid_division;
int mid_highmid_division;
int highmid_presence_division;

//Pattern2 Variables
int highstrip_midrange_midpoint = 50;
int highstrip_highmidrange_midpoint = 150;
int highstrip_presencerange_midpoint = 250;
int lowstrip_bassrange_midpoint = 75;
int lowstrip_lowmidrange_midpoint = 225;

int highstrip_rangesize = 100;
int lowstrip_rangesize = 150;

//Rainbow vars
int rainbow_index = 0;

void setup() {

  pinMode(PATTERN_INPUT_PIN, INPUT);
  pinMode(COLOR_INPUT_PIN, INPUT);
  pinMode(BRIGHTNESS_INPUT_PIN, INPUT);
  pinMode(SCORE_INPUT_PIN, INPUT);
  pinMode(WIN_INPUT_PIN, INPUT);
  pinMode(NEWGAME_INPUT_PIN, INPUT);

  patternButton.attach(PATTERN_INPUT_PIN); patternButton.interval(5);
  colorButton.attach(COLOR_INPUT_PIN); colorButton.interval(5);
  brightnessButton.attach(BRIGHTNESS_INPUT_PIN); brightnessButton.interval(5);
  scoreButton.attach(SCORE_INPUT_PIN); scoreButton.interval(5);
  winButton.attach(WIN_INPUT_PIN); winButton.interval(5);
  newGameButton.attach(NEWGAME_INPUT_PIN); newGameButton.interval(5);
    
  // Initialize fastled library and turn off the LEDs
  FastLED.addLeds<NEOPIXEL, LED_OUT_HIGH_LEFT>(h_leds, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, LED_OUT_LOW_LEFT>(l_leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
  
  //Generate range of colors
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
  
  // Initialize spectrum display
  spectrumSetup();
  
  // Begin sampling audio
  samplingBegin();
  Serial.begin(9600);
}

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
    Serial.println("Completed FFTs");
    //spectrumLoop();
    if(get_average_magnitude() > 40)
    { 
      light_step();
    }
    else
    {
      FastLED.clear();
      FastLED.show();  
    }
    Serial.println("Completed light_step()");
    // Restart audio sampling.
    samplingBegin();
    Serial.println("Completed sampling_begin()");
    cycleCount++;
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
          pattern1c();
          break;
        case 2:
          pattern2();
          break;
        case 3:
          pattern5();
          Serial.println("Completed pattern3()");
          break;
    }
    Serial.println("Exited Pattern Switch");
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
      FastLED.setBrightness(40);
      break;
    case 2:
      FastLED.setBrightness(90);
      break;
    case 3:
      FastLED.setBrightness(180);
      break;
  }
  if (brightness > 2) brightness = 0;
}

//Ours
void pattern1()
{
    //int num_buckets = 30;
    //int bucket_size = NUM_LEDS/num_buckets;
    //int* buckets = shrinkArray(magnitudes, num_buckets);
    for (int i = 0; i < NUM_LEDS; i++)
    {
      if(i < 256 && magnitudes[i] >= 200)
      {
        h_leds[i] = rainbow[((int)(((log(magnitudes[i])/log(3.4))/3.8) * 255.0))];
      }
      if(i >= 256 || magnitudes[i] < 200)
      {
          h_leds[i] = CRGB::Black; 
      }
    }
    FastLED.show();
}

//Ours
void pattern2()
{
    float bass_mag = get_average_portion_magnitude(0,bass_lowmid_division);
    float lowmid_mag = get_average_portion_magnitude(bass_lowmid_division,lowmid_mid_division);
    float mid_mag = get_average_portion_magnitude(lowmid_mid_division,mid_highmid_division);
    float highmid_mag = get_average_portion_magnitude(mid_highmid_division,highmid_presence_division);
    float presence_mag = get_average_portion_magnitude(highmid_presence_division, FFT_SIZE);
    float avg = (bass_mag + lowmid_mag + mid_mag + highmid_mag + presence_mag)/5;
    
    clear_leds();
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
    for(int i = highstrip_highmidrange_midpoint-m_leds; i < highstrip_highmidrange_midpoint+m_leds; i++)
    {
      h_leds[i] = curr_color;  
    }
    for(int i = highstrip_presencerange_midpoint-m_leds; i < highstrip_presencerange_midpoint+m_leds; i++)
    {
      h_leds[i] = curr_color;  
    }
    for(int i = lowstrip_bassrange_midpoint-m_leds; i < lowstrip_bassrange_midpoint+m_leds; i++)
    {
      l_leds[i] = curr_color;  
    }
    for(int i = lowstrip_lowmidrange_midpoint-m_leds; i < lowstrip_lowmidrange_midpoint+m_leds; i++)
    {
      l_leds[i] = curr_color;  
    }
    FastLED.show();
}

//Ours
void pattern1b()
{
    float avg = get_average_magnitude();
    for (int i = 0; i < 256; i++)
    {
          if (magnitudes[i] >= avg*2)
          {
              h_leds[i] = colors[color_selection];
              l_leds[i] = CRGB::Black;
          }
          else
          {
              h_leds[i] = CRGB::Black;
              l_leds[i] = colors[color_selection];
          }
    }
    FastLED.show();
}

//Ours
void pattern1c_rainbow_spread()
{
    float avg = get_average_magnitude();
    for (int i = 0; i < 256; i++)
    {
          if (magnitudes[i] >= 700)
          {
              if (color_selection == 4)
              {
                  rainbow_index++;
                  if(rainbow_index >= 256) rainbow_index = 0;
                  h_leds[i] = rainbow[rainbow_index];
              }
              else
              {
                h_leds[i] = colors[color_selection];
              }
              l_leds[i] = CRGB::Black;
          }
          else
          {
              h_leds[i] = CRGB::Black;
              if (color_selection == 4)
              {
                  rainbow_index++;
                  if(rainbow_index >= 256) rainbow_index = 0;
                  l_leds[i] = rainbow[rainbow_index];
              }
              else
              {
                l_leds[i] = colors[color_selection];
              }
          }
    }
    FastLED.show();
}

//Ours
void pattern1c()
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
          if (magnitudes[i] >= 700)
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

//Ours
void pattern3()
{
    int num_buckets = 20;
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
    Serial.println("Populated Buckets");
    int num_maxes = 6;
    Serial.println("Indices of Maxes:");
    int* flagged = nMaxValIndices(num_maxes, buckets, num_buckets);
    Serial.println("");
    Serial.println("Calculated Indices of Maxes");
    clear_leds();
    Serial.println("Cleared LEDs");
    for (int i = 1; i < num_maxes; i++)
    {
      for (int j = 0; j < NUM_LEDS/buckets_per_strip; j++)
      {
        if(flagged[i] < buckets_per_strip)
        {
          int loc = flagged[i]*(NUM_LEDS/buckets_per_strip) + j - 1;
          h_leds[loc] = p3_colors[flagged[i]];
        }
      }
    }  
        
        //if(flagged[i] < num_buckets/2)
        //{
        //  int loc = (flagged[i])*(NUM_LEDS/(num_buckets*2)) + j;
        //  h_leds[loc] = p3_colors[flagged[i]];
        //}  
        //if(flagged[i] > num_buckets/2)
        //{
        //    l_leds[(flagged[i]-(num_buckets/2))*60 + j] = colors[color_selection];
        //}
      //}
    //}  
    Serial.println("Colored LEDs");
    free(buckets);
    free(flagged);
    FastLED.show();
    Serial.println("Displayed LEDs");
}

//Ours
void pattern4()
{
    
}

//Ours
void pattern5()
{
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
    int num_buckets = 20;
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
    clear_leds();
    Serial.println("Cleared LEDs");
    for (int i = 0; i < buckets_per_strip; i++)
    {
      for (int j = 0; j < NUM_LEDS/buckets_per_strip; j++)
      {
        int loc = i*(NUM_LEDS/buckets_per_strip) + j;
        if(buckets[i] > 600) h_leds[loc] = curr_color;
        else h_leds[loc] = CRGB::Black;
      }
    }  
        
        //if(flagged[i] < num_buckets/2)
        //{
        //  int loc = (flagged[i])*(NUM_LEDS/(num_buckets*2)) + j;
        //  h_leds[loc] = p3_colors[flagged[i]];
        //}  
        //if(flagged[i] > num_buckets/2)
        //{
        //    l_leds[(flagged[i]-(num_buckets/2))*60 + j] = colors[color_selection];
        //}
      //}
    //}  
    Serial.println("Colored LEDs");
    free(buckets);
    FastLED.show();
    Serial.println("Displayed LEDs");
}

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


void winInterrupt()
{
  // make really cool stuff happen here for teh win!!11!!!!1
}


void newGameInterrupt()
{
  // rainbow stuff happens I guess
}


//Ours
void clear_leds()
{
  for (int i = 0; i < NUM_LEDS; i++) 
  {
    h_leds[i] = CRGB::Black;
    l_leds[i] = CRGB::Black;
  }  
}

//Ours
int* shrinkArray(float* in, int num_buckets)
{
  //I need to think on the best way to shrink the array while minimizing data loss
  int* new_buckets = (int*)(malloc(sizeof(int)*(num_buckets + 1))); //Transfer the Average
  new_buckets[0] = in[0];
  float* raw_in = &(in[1]); //Ignores the first bin, which stores the average magnitude 
  int fft_size = sizeof(raw_in);
  int excess = fft_size % num_buckets;
  fft_size -= excess;
  raw_in = raw_in + (int)(excess/2); //Trim what won't divide evenly, balance remaining frequencies by pulling from both sides
  int old_buckets_per_new = (int)(fft_size/num_buckets);
  for(int i = 0; i < num_buckets; i++)
  {
      int avg = 0;
      for(int j = 0; j < old_buckets_per_new; j++)
      {
          avg += in[(num_buckets * old_buckets_per_new) + j];
      }
      avg = avg/old_buckets_per_new;
      new_buckets[i+1] = avg;
  }
  return new_buckets;
}

int* nMaxValIndices(int num_maxes, float arr[], int arr_len)
{
  int* indices = (int*)(malloc(sizeof(int)*num_maxes));
  for(int i = 0; i < num_maxes; i++)
  {
      float max_val = -1;
      int max_ind = -1;
      for(int j = 0; j < arr_len; j++)
      {
        if(contains(indices, j, i) == 0 && arr[j] > max_val)
        {
          max_val = arr[j];
          max_ind = j;
        }
      }
      indices[i] = max_ind;
      Serial.print(max_ind);
      Serial.print(" ");
    }
    return indices;
}

int contains(int my_list[], int val_to_check, int my_list_len)
{
    //This is to catch the very rare (maybe impossible) occurrence of two average floats being equal to each other.
    for(int i = 0; i < my_list_len; i++)
    {
      if(my_list[i] == val_to_check) return 1;
    }
    return 0;
}

//Ours
float get_average_magnitude()
{
    return get_average_portion_magnitude(0, FFT_SIZE);
}

//Ours
float get_average_portion_magnitude(int low, int high)
{
    float sum = 0;
    for(int i = low; i < high; i++)
    {
        sum += magnitudes[i];     
    }
    float avg = sum/(high-low);
    return avg;
}

//Ours (Debug Function)
void cycleRainbow()
{
    for (int c = 0; c < 255; c++)
    {
      for (int i = 0; i < NUM_LEDS; i++)
      {
        leds[i] = rainbow[c];        
      }
      FastLED.show();
      FastLED.delay(100);
    }
}

// Adafruit Function
// Compute the average magnitude of a target frequency window vs. all other frequencies.
void windowMean(float* magnitudes, int lowBin, int highBin, float* windowMean, float* otherMean) {
    *windowMean = 0;
    *otherMean = 0;
    // Notice the first magnitude bin is skipped because it represents the
    // average power of the signal.
    for (int i = 1; i < FFT_SIZE/2; ++i) {
      if (i >= lowBin && i <= highBin) {
        *windowMean += magnitudes[i];
      }
      else {
        *otherMean += magnitudes[i];
      }
    }
    *windowMean /= (highBin - lowBin) + 1;
    *otherMean /= (FFT_SIZE / 2 - (highBin - lowBin));
}

// Adafruit Function
// Convert a frequency to the appropriate FFT bin it will fall within.
int frequencyToBin(float frequency) {
  float binFrequency = float(SAMPLE_RATE_HZ) / float(FFT_SIZE);
  return int(frequency / binFrequency);
}

//Adafruit Function
void spectrumSetup() {
  // Set the frequency window values by evenly dividing the possible frequency
  // spectrum across the number of neo pixels.
  float windowSize = (SAMPLE_RATE_HZ / 2.0) / float(NUM_LEDS);
  for (int i = 0; i < NUM_LEDS+1; ++i) {
    frequencyWindow[i] = i*windowSize;
  }
  // Evenly spread hues across all pixels.
  //for (int i = 0; i < NUM_LEDS; ++i) {
  //  hues[i] = 360.0*(float(i)/float(NUM_LEDS-1));
  //}
}

//Adafruit Function
void spectrumLoop() {
  // Update each LED based on the intensity of the audio 
  // in the associated frequency window.
  float intensity, otherMean;
  for (int i = 0; i < NUM_LEDS; ++i) {
    windowMean(magnitudes, 
               frequencyToBin(frequencyWindow[i]),
               frequencyToBin(frequencyWindow[i+1]),
               &intensity,
               &otherMean);
    // Convert intensity to decibels.
    intensity = 20.0*log10(intensity);
    // Scale the intensity and clamp between 0 and 1.0.
    intensity -= SPECTRUM_MIN_DB;
    intensity = intensity < 0.0 ? 0.0 : intensity;
    intensity /= (SPECTRUM_MAX_DB-SPECTRUM_MIN_DB);
    intensity = intensity > 1.0 ? 1.0 : intensity;  
  }
  
  intensity = 20.0*log10(magnitudes[0]);
  intensity -= SPECTRUM_MIN_DB;
    intensity = intensity < 0.0 ? 0.0 : intensity;
    intensity /= (SPECTRUM_MAX_DB-SPECTRUM_MIN_DB);
    intensity = intensity > 1.0 ? 1.0 : intensity;  
  {
    int j = 0;
  
    for (j = 0; j < (int) NUM_LEDS * intensity; j++) {
      leds[j] = CRGB::Blue;
    }

    while (j < NUM_LEDS){
      leds[j] = CRGB::Black;
      j++;
    }
  }
  FastLED.show();
}


////////////////////////////////////////////////////////////////////////////////
// SAMPLING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

//Adafruit Function
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

//Adafruit Function
void samplingBegin() {
  // Reset sample buffer position and start callback at necessary rate.
  sampleCounter = 0;
  samplingTimer.begin(samplingCallback, 1000000/SAMPLE_RATE_HZ);
}

//Adafruit Function
boolean samplingIsDone() {
  return sampleCounter >= FFT_SIZE*2;
}
