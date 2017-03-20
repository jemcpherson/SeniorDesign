// Audio Spectrum Display
// Copyright 2013 Tony DiCola (tony@tonydicola.com)

// This code is part of the guide at http://learn.adafruit.com/fft-fun-with-fourier-transforms/

#define ARM_MATH_CM4
#include <arm_math.h>
#include <FastLED.h>


////////////////////////////////////////////////////////////////////////////////
// CONIFIGURATION 
// These values can be changed to alter the behavior of the spectrum display.
////////////////////////////////////////////////////////////////////////////////

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
const int LED_PIN = 6;           // Output pin for leds
const int LED_COUNT = 60;         // Number of neo pixels.  You should be able to increase this without
                                       // any other changes to the program.
const int MAX_CHARS = 65;              // Max size of the input command buffer


////////////////////////////////////////////////////////////////////////////////
// INTERNAL STATE
// These shouldn't be modified unless you know what you're doing.
////////////////////////////////////////////////////////////////////////////////

IntervalTimer samplingTimer;
float samples[FFT_SIZE*2];
float magnitudes[FFT_SIZE];
int sampleCounter = 0;
char commandBuffer[MAX_CHARS];
float frequencyWindow[LED_COUNT+1];
float hues[LED_COUNT];
CRGB leds[LED_COUNT];

////////////////////////////////////////////////////////////////////////////////
// MAIN SKETCH FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  
  // Set up ADC and audio input.
  pinMode(AUDIO_INPUT_PIN, INPUT);
  analogReadResolution(ANALOG_READ_RESOLUTION);
  analogReadAveraging(ANALOG_READ_AVERAGING);
  
  // Initialize fastled library and turn off the LEDs
  for (int i = 1; i < LED_COUNT; i++) {
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


////////////////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

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

// Convert a frequency to the appropriate FFT bin it will fall within.
int frequencyToBin(float frequency) {
  float binFrequency = float(SAMPLE_RATE_HZ) / float(FFT_SIZE);
  return int(frequency / binFrequency);
}

// Convert from HSV values (in floating point 0 to 1.0) to RGB colors usable
// by neo pixel functions.
void pixelHSVtoRGBColor(float hue, float saturation, float value, int i) {
  // Implemented from algorithm at http://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV
  float chroma = value * saturation;
  float h1 = float(hue)/60.0;
  float x = chroma*(1.0-fabs(fmod(h1, 2.0)-1.0));
  float r = 0;
  float g = 0;
  float b = 0;
  if (h1 < 1.0) {
    r = chroma;
    g = x;
  }
  else if (h1 < 2.0) {
    r = x;
    g = chroma;
  }
  else if (h1 < 3.0) {
    g = chroma;
    b = x;
  }
  else if (h1 < 4.0) {
    g = x;
    b = chroma;
  }
  else if (h1 < 5.0) {
    r = x;
    b = chroma;
  }
  else // h1 <= 6.0
  {
    r = chroma;
    b = x;
  }
  float m = value - chroma;
  r += m;
  g += m;
  b += m;
  

  leds[i] = CRGB(int(255*r), int(255*g), int(255*b));
}


////////////////////////////////////////////////////////////////////////////////
// SPECTRUM DISPLAY FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

void spectrumSetup() {
  // Set the frequency window values by evenly dividing the possible frequency
  // spectrum across the number of neo pixels.
  float windowSize = (SAMPLE_RATE_HZ / 2.0) / float(LED_COUNT);
  for (int i = 0; i < LED_COUNT+1; ++i) {
    frequencyWindow[i] = i*windowSize;
  }
  // Evenly spread hues across all pixels.
  for (int i = 0; i < LED_COUNT; ++i) {
    hues[i] = 360.0*(float(i)/float(LED_COUNT-1));
  }
}

void spectrumLoop() {
  // Update each LED based on the intensity of the audio 
  // in the associated frequency window.
  float intensity, otherMean;
  for (int i = 0; i < LED_COUNT; ++i) {
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
  
    for (j = 0; j < (int) LED_COUNT * intensity; j++) {
      leds[j] = CRGB::Blue;
    }

    while (j < LED_COUNT){
      leds[j] = CRGB::Black;
      j++;
    }
  }
  FastLED.show();
}


////////////////////////////////////////////////////////////////////////////////
// SAMPLING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

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

void samplingBegin() {
  // Reset sample buffer position and start callback at necessary rate.
  sampleCounter = 0;
  samplingTimer.begin(samplingCallback, 1000000/SAMPLE_RATE_HZ);
}

boolean samplingIsDone() {
  return sampleCounter >= FFT_SIZE*2;
}


