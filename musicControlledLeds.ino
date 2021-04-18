#include <FastLED.h>

FASTLED_USING_NAMESPACE





#define DATA_PIN 10
#define AUDIO_PORT 4
#define MICROPHONE_PORT 4
#define SAMPLE_SIZE 100 //400 //the number of audio samples to take
#define LOOPS_TO_MES_FREQ_OVER 3;
#define SAMPLE_DELAY 0//0 // the delay between audio samples






struct NormalizationData {
  float lastNormalizedMeasurement = 0;
  float minMeasurement = 10000;
  float maxMeasurement = 0;
  float averageMeasurement = 0;
  int totalMesurements = 0;
  
};

struct NormalizationData frequencyNormalizationData;

struct NormalizationData VrmsNormalizationData;

struct NormalizationData averageFrequencyNormalizationData;

int prevLEDNum = 0;

int colorCycleIndx = 0;

int loopsWithoutAudioDetected = 0;

long loops = SAMPLE_SIZE;

float avgSample = 100;

int largeVrms = 0;


bool modeSwitchButtonPressed = false;

CRGB highStateColor = CRGB::Blue;
CRGB lowStateColor = CRGB::Red;

void musicLEDSsetup() {
  // put your setup code here, to run once:

  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);


  pinMode(AUDIO_PORT,INPUT);
}


float getSignalVrms(int input_port, int sample_size) {
  // read RMS of audioStream
 
  
  double Vrms = 0;
  float numNonZero = sample_size;
  for (int sampleNum = 0; sampleNum < sample_size; sampleNum++) {
    double sample = analogRead(input_port);

    //Vrms += sample > 0 ? sample*sample : Vrms/(sampleNum+1);
    
    if (! sample == 0) {
      Vrms += sample*sample;
    }else {
      numNonZero-= 0.8;
    }

    delay(SAMPLE_DELAY);
  }

  Vrms /= numNonZero;

  return Vrms;
}


float getSignalFrequency(int input_port, float triggerLevel, int sample_size) {
  
  double frequency = 0;

  boolean belowTrigger = true;

  for (int sampleNum = 0; sampleNum < sample_size; sampleNum++) {
    double sample = analogRead(input_port);

    avgSample = (20*avgSample + sample)/21;
    
    if (sample > triggerLevel) {

      if (belowTrigger) frequency += 1;  //rising edge triggering

      belowTrigger = false;
    }else {
      belowTrigger = true;
    }

    
    delay(SAMPLE_DELAY);
  }
  return frequency;
}

float freqVrms[2];
float* getSignalFrequencyAndRms(int input_port, float triggerLevel, int sample_size) {
  
  float frequency = 0;
  boolean belowTrigger = true;

  double Vrms = 0;
  float numNonZero = sample_size;

  double sample = 0;
  for (int sampleNum = 0; sampleNum < sample_size; sampleNum++) {
    sample = analogRead(input_port);
  
    if (sample > 300) {
       if (belowTrigger) frequency++;  //rising edge triggering
       belowTrigger = false;
    }else {
       belowTrigger = true;
    }
    
    if (! sample == 0) {
      Vrms += sample*sample;
    }else {
      numNonZero-= 0.8;
    }

    delay(SAMPLE_DELAY);
  }
  
  Vrms /= numNonZero;
 
  freqVrms[0] = frequency;
  freqVrms[1] = Vrms;
  return freqVrms;
}



struct NormalizationData normalize(float measurement,struct NormalizationData data) {
  
  measurement = measurement > 1 ? sqrt(measurement) : measurement*measurement;
  data.averageMeasurement = (  data.averageMeasurement*data.totalMesurements + measurement)/(++data.totalMesurements);

  // reset after a while
  data.maxMeasurement -= abs(data.maxMeasurement)/1000;
  data.minMeasurement += abs(data.minMeasurement)/1000;
  
  

  
  data.lastNormalizedMeasurement = (measurement-data.minMeasurement) / (data.maxMeasurement-data.minMeasurement == 0 ? 1 : data.maxMeasurement-data.minMeasurement); //normalize measurement

  data.lastNormalizedMeasurement = data.lastNormalizedMeasurement < 0 ? 0 : data.lastNormalizedMeasurement > 1 ? 1 : data.lastNormalizedMeasurement;
  
  if (measurement > data.maxMeasurement && measurement < 1000000) data.maxMeasurement = measurement;
  

  if (measurement < data.minMeasurement ) data.minMeasurement = measurement;
  
  return data;

}


void resetNormalizationData() {
  struct NormalizationData newVrmsData;
  struct NormalizationData newFreqData;
  struct NormalizationData newAverageFrequencyNormalizationData;
  VrmsNormalizationData = newVrmsData;
  frequencyNormalizationData = newFreqData;
  averageFrequencyNormalizationData = newAverageFrequencyNormalizationData;
}

CRGB getColorShift(double pos, int brightness) {
  if (brightness > 255) brightness = 255;
  return CHSV(pos,255,brightness);
}

CRGB getColorShift2(double position, int brightnessI) {
  float pos = 0;
  if (position > 10000) {
    (((long)position) % 10000);
  }else{
    pos = position;
  }
    
  float brightness = brightnessI > 255 ? 255 : ((float) brightnessI);

  int r = ( (int)  brightness/2 * ( sin(pos/200) + 1) );
  int g = ( (int)  brightness/2 * ( sin(pos/170 + 0.27) + 1.0) );
  int b = ( (int)  brightness/2 * ( sin(pos/300 + 3.2) + 1.0) );
  
  return CRGB(r,g,b);
}

CRGB getColorShift1(long position, int brightness) {
    if (brightness > 255) brightness = 255;
    
    int colorStep = position % (2*brightness);
    if (colorStep > brightness) colorStep = (2*brightness)-colorStep;

    int greenStep = ((int) ( ((float) position) * 1.3f)) % (2*brightness);
    if (greenStep > brightness) greenStep = (2*brightness)-greenStep;
    
    int r = colorStep;  // Redness starts at zero and goes up to full
    int b = brightness-colorStep;  // Blue starts at full and goes down to zero
    int g = greenStep;              // No green needed to go from blue to red

    return CRGB(r,g,b);
}

CRGB getColorShift(double pos) {
    return getColorShift(pos,255);
}

CRGB getColorShiftOld(int position) {
    int colorStep = position % 510;
    if (colorStep > 255) colorStep = 510-colorStep;

    int greenStep = ((int) ( ((float) position) * 1.3f)) % 510;
    if (greenStep > 255) greenStep = 510-greenStep;
    
    int r = colorStep;  // Redness starts at zero and goes up to full
    int b = 255-colorStep;  // Blue starts at full and goes down to zero
    int g = greenStep;              // No green needed to go from blue to red

    return CRGB(r,g,b);
}


/*
void checkModeSwitch() {
  //Mode switching

  if (digitalRead(MODE_SWITCH_BUTTON_PIN) == HIGH) {
    if (! modeSwitchButtonPressed) {
      MODE = MODE >= MAX_MODE ? 0 : MODE + 1;
      
      delay(50);
    }
    modeSwitchButtonPressed = true;
  }else{
    modeSwitchButtonPressed = false;
  }
}*/



void runMusicLeds(bool useMicrophone) {

  //get signal data
  
  float* freqAndRms = getSignalFrequencyAndRms(useMicrophone ? MICROPHONE_PORT : AUDIO_PORT,avgSample,SAMPLE_SIZE);

  float frequency = freqAndRms[0];  
  float Vrms = freqAndRms[1];


 
  // if there is no music playing switch to rainbow

    if ( Vrms < 15000 ){
      loopsWithoutAudioDetected+=10;
    }else{
      if (loopsWithoutAudioDetected > 100000/SAMPLE_SIZE) {
        resetNormalizationData();
      }

      loopsWithoutAudioDetected = 0;
    }
    
  



  // normalize Vrms
  VrmsNormalizationData = normalize(Vrms,VrmsNormalizationData);
  Vrms = VrmsNormalizationData.lastNormalizedMeasurement;

   

  //set brightness with Vrms
  
   
   
   
    Serial.println(Vrms);

   
    if (colorCycleIndx > 255 || colorCycleIndx < 0) colorCycleIndx = 0;
    // change color with Vrms
    if ((Vrms > 0.5)) {
      colorCycleIndx += 20*pow(1/(1-Vrms),2)/SAMPLE_SIZE;
      if (Vrms > 0.75){
        colorCycleIndx += 30*pow(1/(1-Vrms),2)/SAMPLE_SIZE;
      }else{      
        largeVrms++;
      }
      
    }else{
      if (largeVrms != 0 && largeVrms < 500/SAMPLE_SIZE) {
        colorCycleIndx += 50*pow(1/(1-Vrms),2)/SAMPLE_SIZE;
      }else{
        colorCycleIndx += 5*pow(1/(1-Vrms),2)/SAMPLE_SIZE;
      }
  
      largeVrms = 0;
      
    }



  if (loops % 3 == 0) {
    FastLED.setBrightness((1.0+Vrms)/2.0*brightness);
  }else if (loops > 100000){
      loops = 0;
  }
  
  highStateColor = getColorShift(colorCycleIndx);
  lowStateColor = getColorShift(colorCycleIndx +700);
  loops++;

  

   
  //show bars with frequency


 
  
  frequencyNormalizationData.totalMesurements = LOOPS_TO_MES_FREQ_OVER;

  
  
  //if freq is zero don't change normalization
  if (frequency != 0) {
    frequencyNormalizationData = normalize(frequency*frequency,frequencyNormalizationData);
  }

 
  averageFrequencyNormalizationData = normalize(frequencyNormalizationData.averageMeasurement,averageFrequencyNormalizationData);

  float measurement = averageFrequencyNormalizationData.lastNormalizedMeasurement;

  int ledNum = ((int) (measurement * NUM_LEDS));


  
  bool mesVrms = false;
  if (mesVrms) ledNum = measurement == 0 ? prevLEDNum * 1 : ledNum*0.6 + prevLEDNum*0.4;
  

 
  
  if (music_mode == 0) {
    setAll(CRGB::Black);
   
  }else if (music_mode == 1) {
    
    
    if (ledNum > 1) {
      int i = 1;
      for (; i < ledNum/2 && i < NUM_LEDS/2 ; i++) {
        leds[i] = highStateColor;
        leds[NUM_LEDS-i] = highStateColor;
      }
   
      for (; i < NUM_LEDS/2; i++) {
        leds[i] = lowStateColor;
        leds[NUM_LEDS-i] = lowStateColor;
      }
    }else{
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = lowStateColor;
      }
    }
  }else if (music_mode == 2) {
    
    ledNum = ((int) (sqrt(measurement/2) * NUM_LEDS));
    
    if (ledNum > 1) {
      int i = 0;
      for (; i < ledNum/2 && i < NUM_LEDS/2 ; i++) {
        leds[NUM_LEDS/2+i] = highStateColor;
        leds[NUM_LEDS/2-i] = highStateColor;
      }
  
      for (; i < NUM_LEDS/2; i++) {
        leds[NUM_LEDS/2+i] = lowStateColor;
        leds[NUM_LEDS/2-i] = lowStateColor;
      }
    }else{
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = lowStateColor;
      }
    }
  }else if (music_mode == 3) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = highStateColor;
    }
  }else{ //mode not one of the options  
    setAll(CRGB::Red);
  }

  prevLEDNum = ledNum;



  for (int i = 0; i < START_POS; i++) {
    leds[i] = CRGB::Black;
  }

  
 
}
