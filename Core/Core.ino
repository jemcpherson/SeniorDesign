#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 300
#define DATA_PIN 6
#define DATA_PIN2 11

//Start of Adafruit//
#define ARM_MATH_CM4
#include <arm_math.h>
#include <FastLED.h>

int SAMPLE_RATE_HZ = 9000;             // Sample rate of the audio in hertz.
float SPECTRUM_MIN_DB = 20.0;          // Audio intensity (in decibels) that maps to low LED brightness.
float SPECTRUM_MAX_DB = 70.0;          // Audio intensity (in decibels) that maps to high LED brightness.
int LEDS_ENABLED = 1;                  // Control if the LED's should display the spectrum or not.  1 is true, 0 is false.
                                       // Useful for turning the LED display on and off with commands from the serial port.
const int FFT_SIZE = 256;              // Size of the FFT.  Realistically can only be at most 256 
                                       // without running out of memory for buffers and other state.
const int AUDIO_INPUT_PIN = 14;        // Input ADC pin for audio data.
const int ANALOG_READ_RESOLUTION = 10; // Bits of resolution for the ADC.
const int ANALOG_READ_AVERAGING = 16;  // Number of samples to average with each ADC reading.

const int MAX_CHARS = 65;              // Max size of the input command buffer

IntervalTimer samplingTimer;
float samples[FFT_SIZE*2];
float magnitudes[FFT_SIZE];
int sampleCounter = 0;
char commandBuffer[MAX_CHARS];
float frequencyWindow[NUM_LEDS+1];
float hues[NUM_LEDS];
CRGB leds[NUM_LEDS];
//End of Adafruit Sample Vars//

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.

// Define the array of leds
CRGB h_leds[NUM_LEDS]; //Upper strips of leds on trailer
CRGB l_leds[NUM_LEDS]; //Lower strips of leds on trailer 
int color_selection = 0;
int pattern = 1;
CRGB colors[2] = {CRGB::Red, CRGB::Green}; 
int* fftout = (int*)(malloc(sizeof(int)*256));

void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  
  // Set up ADC and audio input.
  pinMode(AUDIO_INPUT_PIN, INPUT);
  analogReadResolution(ANALOG_READ_RESOLUTION);
  analogReadAveraging(ANALOG_READ_AVERAGING);
  
  // Initialize fastled library and turn off the LEDs
  for (int i = 1; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show(); delay(30); 
  
  // Clear the input command buffer
  memset(commandBuffer, 0, sizeof(commandBuffer));
  
  // Initialize spectrum display
  spectrumSetup();
  
  // Begin sampling audio
  samplingBegin();
}

void loop() {
  // Calculate FFT if a full sample is available.
  if (samplingIsDone()) {
    // Run FFT on sample data.
    arm_cfft_radix4_instance_f32 fft_inst;
    arm_cfft_radix4_init_f32(&fft_inst, FFT_SIZE, 0, 1);
    arm_cfft_radix4_f32(&fft_inst, samples);
    // Calculate magnitude of complex numbers output by the FFT.
    arm_cmplx_mag_f32(samples, magnitudes, FFT_SIZE);
  
    if (LEDS_ENABLED == 1)
    {
      spectrumLoop();
    }
  
    // Restart audio sampling.
    samplingBegin();
  }
    
  // Parse any pending commands.
}

//Joshua McPherson
void setup2() { 
      FastLED.addLeds<NEOPIXEL, DATA_PIN>(h_leds, NUM_LEDS);
      FastLED.addLeds<NEOPIXEL, DATA_PIN2>(l_leds, NUM_LEDS);

      // Initialize fastled library and turn off the LEDs
      for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB::Black;
      }
      FastLED.show(); delay(30); 
      
      //Adafruit
      //Set up ADC and audio input. 
      pinMode(AUDIO_INPUT_PIN, INPUT);
      analogReadResolution(ANALOG_READ_RESOLUTION);
      analogReadAveraging(ANALOG_READ_AVERAGING);

       // Initialize spectrum display
       spectrumSetup();

       // Begin sampling audio
       samplingBegin();
       //End Adafruit
         
      FastLED.setBrightness(100);
}

//Joshua McPherson
void loop2() { 
  check_inputs();
  //Adafruit
  if (samplingIsDone()) {
    // Run FFT on sample data.
    arm_cfft_radix4_instance_f32 fft_inst;
    arm_cfft_radix4_init_f32(&fft_inst, FFT_SIZE, 0, 1);
    arm_cfft_radix4_f32(&fft_inst, samples);
    // Calculate magnitude of complex numbers output by the FFT.
    arm_cmplx_mag_f32(samples, magnitudes, FFT_SIZE);
  
    if (LEDS_ENABLED == 1)
    {
      spectrumLoop();
    }
  
    // Restart audio sampling.
    samplingBegin();
  }
  //End Adafruit
}

//Joshua McPherson
void check_inputs()
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
    int num_buckets = 30;
    int bucket_size = NUM_LEDS/num_buckets;
    int* buckets = shrinkArray(fftout, num_buckets);
    int avg = buckets[0];
    for (int i = 0; i < num_buckets; i++)
    {
      for (int j = 0; j < bucket_size; j++)
      {
        if(buckets[i+1] > avg)
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
    } 
    FastLED.show();
}

//Joshua McPherson
int* shrinkArray(int* in, int num_buckets)
{
  //I need to think on the best way to shrink the array while minimizing data loss
  int* new_buckets = (int*)(malloc(sizeof(int)*(num_buckets + 1))); //Transfer the Average
  new_buckets[0] = in[0];
  int* raw_in = &(in[1]); //Ignores the first bin, which stores the average magnitude 
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
  for (int i = 0; i < NUM_LEDS; ++i) {
    hues[i] = 360.0*(float(i)/float(NUM_LEDS-1));
  }
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



