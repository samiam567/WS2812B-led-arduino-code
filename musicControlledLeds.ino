#include <FastLED.h>

FASTLED_USING_NAMESPACE

#define DATA_PIN 10
#define AUDIO_PORT 4
#define MICROPHONE_PORT 4
#define SAMPLE_SIZE 200 // 400 //the number of audio samples to take
#define LOOPS_TO_MES_FREQ_OVER 3;
#define SAMPLE_DELAY 0 // 0 // the delay between audio samples
#define NUM_LEDS_TILL_REPEAT 70

struct NormalizationData
{
  float lastNormalizedMeasurement = 0;
  float minMeasurement = 10000;
  float maxMeasurement = 0;
  float virtualMinMeasurement = 10000; // compared against for finding max and min but not used for normalization
  float virtualMaxMeasurement = 0;     // compared against for finding max and min but not used for normalization
  float averageMeasurement = 0;
  int totalMesurements = 0;
  int normalizationResetLoops = 10000;
};

struct NormalizationData frequencyNormalizationData;

struct NormalizationData VrmsNormalizationData;

struct NormalizationData averageFrequencyNormalizationData;

int prevLEDNum = 0;

int colorCycleIndx = 0;
short colorCycleDirection = 1;

int loopsWithoutAudioDetected = 0;

long loops = SAMPLE_SIZE;

float avgSample = 100;

int largeVrms = 0;

bool modeSwitchButtonPressed = false;

CRGB highStateColor = CRGB::Blue;
CRGB lowStateColor = CRGB::Red;

void musicLEDSsetup()
{
  // put your setup code here, to run once:

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  pinMode(AUDIO_PORT, INPUT);
  pinMode(MICROPHONE_PORT, INPUT);
}

float freqVrms[2];

float *getSignalFrequencyAndRms(int input_port, float triggerLevel, int sample_size)
{

  float frequency = 0;
  boolean belowTrigger = true;

  double Vrms = 0;
  float numNonZero = sample_size;

  double sample = 0;
  for (int sampleNum = 0; sampleNum < sample_size; sampleNum++)
  {
    sample = analogRead(MICROPHONE_PORT);

    if (sample > 25)
    {
      if (belowTrigger)
        frequency++; // rising edge triggering
      belowTrigger = false;
    }
    else
    {
      belowTrigger = true;
    }

    if (!sample == 0)
    {
      Vrms += sample * sample;
    }
    else
    {
      numNonZero -= 0.8;
    }

    delay(SAMPLE_DELAY);
  }

  Vrms /= numNonZero;

  freqVrms[0] = frequency;
  freqVrms[1] = Vrms;
  return freqVrms;
}

struct NormalizationData normalize(float measurement, struct NormalizationData data)
{

  measurement = measurement > 1 ? sqrt(measurement) : measurement * measurement;
  data.averageMeasurement = (data.averageMeasurement * data.totalMesurements + measurement) / (++data.totalMesurements);

  // reset after a while
  data.virtualMaxMeasurement -= abs(data.maxMeasurement) / data.normalizationResetLoops;
  data.virtualMinMeasurement += abs(data.minMeasurement) / data.normalizationResetLoops;

  data.lastNormalizedMeasurement = (measurement - data.minMeasurement) / (data.maxMeasurement - data.minMeasurement == 0 ? 1 : data.maxMeasurement - data.minMeasurement); // normalize measurement

  data.lastNormalizedMeasurement = data.lastNormalizedMeasurement < 0 ? 0 : data.lastNormalizedMeasurement > 1 ? 1
                                                                                                               : data.lastNormalizedMeasurement;

  if (measurement > data.virtualMaxMeasurement && measurement < 1000000)
  {
    data.maxMeasurement = measurement;
    data.virtualMaxMeasurement = measurement;
  }

  if (measurement < data.virtualMinMeasurement)
  {
    data.minMeasurement = measurement;
    data.virtualMinMeasurement = measurement;
  }

  return data;
}

void resetNormalizationData()
{
  struct NormalizationData newVrmsData;
  struct NormalizationData newFreqData;
  struct NormalizationData newAverageFrequencyNormalizationData;
  VrmsNormalizationData = newVrmsData;
  frequencyNormalizationData = newFreqData;
  averageFrequencyNormalizationData = newAverageFrequencyNormalizationData;

  VrmsNormalizationData.normalizationResetLoops = 1000;
  frequencyNormalizationData.normalizationResetLoops = 5000;
  averageFrequencyNormalizationData.normalizationResetLoops = 1000;
}

CRGB getColorShift(double pos, int brightness)
{
  if (brightness > 255)
    brightness = 255;
  return CHSV(pos, 255, brightness);
}

CRGB getColorShift(double pos)
{
  return getColorShift(pos, 255);
}

int factorial(int x)
{
  int result = x;

  // Mode switching
  if (x >= 0)
  {
    x--;
    while (x > 1)
    {
      result *= x;
      x--;
    }
  }
  else
  {
    x++;
    while (x != 0)
    {
      result *= x;
      x++;
    }
  }
}

void runMusicLeds(bool useMicrophone)
{

  // get signal data

  float *freqAndRms = getSignalFrequencyAndRms(useMicrophone ? MICROPHONE_PORT : AUDIO_PORT, avgSample, SAMPLE_SIZE);

  float frequency = freqAndRms[0];
  float Vrms = freqAndRms[1];

  // if there is no music playing switch to rainbow

  if (Vrms < 15000)
  {
    loopsWithoutAudioDetected += 10;
  }
  else
  {
    if (loopsWithoutAudioDetected > 100000 / SAMPLE_SIZE)
    {
      resetNormalizationData();
    }

    loopsWithoutAudioDetected = 0;
  }

  // normalize Vrms
  VrmsNormalizationData = normalize(Vrms, VrmsNormalizationData);
  Vrms = VrmsNormalizationData.lastNormalizedMeasurement;

  // change color with Vrms
  if ((Vrms > 0.75))
  {
    largeVrms++;
    colorCycleIndx += (5.0f * pow(1.0f / (1.0f - Vrms), 2) / SAMPLE_SIZE) * colorCycleDirection;
  }
  else
  {
    if (largeVrms != 0 && largeVrms < 700.0f / SAMPLE_SIZE)
    {
      colorCycleIndx += (20.0f * pow(1.0f / (1.0f - Vrms), 2) / SAMPLE_SIZE) * colorCycleDirection;
    }
    largeVrms = 0;
  }

  colorCycleIndx += (20.0f * pow(1.0f / (1.0f - Vrms), 2) / SAMPLE_SIZE) * colorCycleDirection;

  // make sure colorCycle doesn't go our of bounds
  if (colorCycleIndx > 359)
  {
    colorCycleIndx = 359;
    colorCycleDirection = -1;
  }
  else if (colorCycleIndx < 0)
  {
    colorCycleIndx = 0;
    colorCycleDirection = 1;
  }

  // set brightness with Vrms
  if (loops % 2 == 0)
  {
    FastLED.setBrightness(((Vrms) / 1.3333f + 0.25) * brightness);
  }
  else if (loops > 100000)
  {
    loops = 0;
  }

  highStateColor = getColorShift(colorCycleIndx);
  lowStateColor = getColorShift(colorCycleIndx + 700, 150);
  loops++;

  // show bars with frequency

  frequencyNormalizationData.totalMesurements = LOOPS_TO_MES_FREQ_OVER;

  // if freq is zero don't change normalization
  if (frequency != 0)
  {
    frequencyNormalizationData = normalize(frequency * frequency, frequencyNormalizationData);
  }

  averageFrequencyNormalizationData = normalize(frequencyNormalizationData.averageMeasurement, averageFrequencyNormalizationData);

  float measurement = averageFrequencyNormalizationData.lastNormalizedMeasurement;

  int ledNum = ((int)(measurement * NUM_LEDS));

  /*
  Serial.print(Vrms);
  Serial.print(",");
  Serial.print(frequencyNormalizationData.lastNormalizedMeasurement);
  Serial.print(",");
  Serial.println(measurement);
  */

  bool mesVrms = false;
  if (mesVrms)
    ledNum = measurement == 0 ? prevLEDNum * 1 : ledNum * 0.6 + prevLEDNum * 0.4;

  if (music_mode == 0)
  {
    setAll(CRGB::Black);
  }
  else if (music_mode == 1)
  {

    // int ledNum = (int) ( ((float) measurement) * ((float) NUM_LEDS_TILL_REPEAT));
    float triangleWavePos = 0;
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = (measurement > triangleWavePos) ? highStateColor : lowStateColor;
      if ((i % NUM_LEDS_TILL_REPEAT) < (NUM_LEDS_TILL_REPEAT / 2))
      {
        triangleWavePos += 1.0f / (NUM_LEDS_TILL_REPEAT / 2.0f);
      }
      else
      {
        triangleWavePos -= 1.0f / (NUM_LEDS_TILL_REPEAT / 2.0f);
      }
    }
  }
  else if (music_mode == 2)
  {

    int ledNum = (int)(((float)measurement) * ((float)NUM_LEDS_TILL_REPEAT));
    bool onState = false;
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = onState ? highStateColor : lowStateColor;
      if ((-ledNum / 2 + i) % ledNum == 0)
      {
        onState = !onState;
      }
    }
  }
  else if (music_mode == 3)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = highStateColor;
    }
  }
  else
  { // mode not one of the options
    setAll(CRGB::Red);
  }

  prevLEDNum = ledNum;

  for (int i = 0; i < START_POS; i++)
  {
    leds[i] = CRGB::Black;
  }
}
