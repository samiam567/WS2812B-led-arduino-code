#include <FastLED.h>

FASTLED_USING_NAMESPACE

// 0: OFF
// 1: Frequencyd-> bars coming from either side, color change with thresh vrms
// 2: Frequency -> bar starting from center going out, color change with thresh vrms
// 3: color change with Vrms
// 4: rainbow
// 5: solid color
int MODE = 2;
// i love you alec

#define MAX_MODE 5



#define DATA_PIN 10
#define AUDIO_PORT 5
#define SAMPLE_SIZE 100 //400 //the number of audio samples to take
#define SAMPLE_DELAY 0//0 // the delay between audio samples
#define MODE_SWITCH_BUTTON_PIN 8





struct NormalizationData {
  float lastNormalizedMeasurement = 0;
  float minMeasurement = 10000;
  float maxMeasurement = -1000;
  float averageMeasurement = 0;
  int totalMeasurements = 0;
  
};

struct NormalizationData normalizationData;

struct NormalizationData VrmsNormalizationData;

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


  pinMode(MODE_SWITCH_BUTTON_PIN, INPUT);

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

    //Vrms += sample > 0 ? sample*sample : Vrms/(sampleNum+1);
    
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
  
  for (int sampleNum = 0; sampleNum < sample_size; sampleNum++) {
    double sample = analogRead(input_port);
    //Vrms += sample > 0 ? sample*sample : Vrms/(sampleNum+1);

    if (sample > triggerLevel) {
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
  data.averageMeasurement = (  data.averageMeasurement*data.totalMeasurements + measurement)/(++data.totalMeasurements);

  // reset after a while
  data.maxMeasurement -= 0.0001;
  data.minMeasurement += 0.0001;
  
  

  
  data.lastNormalizedMeasurement = (measurement-data.minMeasurement) / (data.maxMeasurement-data.minMeasurement == 0 ? 1 : data.maxMeasurement-data.minMeasurement); //normalize measurement

  data.lastNormalizedMeasurement = data.lastNormalizedMeasurement < 0 ? 0 : data.lastNormalizedMeasurement > 1 ? 1 : data.lastNormalizedMeasurement;
  
  if (measurement > data.maxMeasurement) data.maxMeasurement = measurement;
  

  if (measurement < data.minMeasurement && measurement != 0) data.minMeasurement = measurement;
  
  return data;

}


void resetNormalizationData() {
  struct NormalizationData newVrmsData;
  struct NormalizationData newFreqData;
  VrmsNormalizationData = newVrmsData;
  normalizationData = newFreqData;
}

CRGB getColorShift(long position) {
    int colorStep = position % 510;
    if (colorStep > 255) colorStep = 510-colorStep;

    int greenStep = ((int) ( ((float) position) * 1.3f)) % 510;
    if (greenStep > 255) greenStep = 510-greenStep;
    
    int r = colorStep;  // Redness starts at zero and goes up to full
    int b = 255-colorStep;  // Blue starts at full and goes down to zero
    int g = greenStep;              // No green needed to go from blue to red

    return CRGB(r,g,b);
}


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
}



void runMusicLeds() {

  //get signal data
  
  float* freqAndRms = getSignalFrequencyAndRms(AUDIO_PORT,avgSample,SAMPLE_SIZE);

  float frequency = freqAndRms[0];
  float Vrms = freqAndRms[1];




  delete[] freqAndRms;

  


  // if there is no music playing switch to rainbow

    if ( Vrms < 15000 ){
      loopsWithoutAudioDetected+=10;
    }else{
      if (loopsWithoutAudioDetected > 100000/SAMPLE_SIZE) {
        Serial.println("Reset normalization for new song");
        resetNormalizationData();
      }

      loopsWithoutAudioDetected = 0;
    }
    
  



  // normalize Vrms
  VrmsNormalizationData = normalize(Vrms,VrmsNormalizationData);
  Vrms = VrmsNormalizationData.lastNormalizedMeasurement;



  //set brightness with Vrms
  
   
   if (loops % 2 == 1) {
      FastLED.setBrightness(brightness*(0.5+sqrt(Vrms))/2);
   }
   loops++;


  
  
    // change color with Vrms
    if ((Vrms > 0.5)) {
      if (largeVrms == 1 || largeVrms == 2) {
        colorCycleIndx += 200*pow(1/(1-Vrms),3)/SAMPLE_SIZE;
      }else{
        colorCycleIndx += 20*pow(1/(1-Vrms),3)/SAMPLE_SIZE;
      }
      largeVrms++;
      
    }else{
      largeVrms = 0;
      colorCycleIndx += 2*pow(1/(1-Vrms),3)/SAMPLE_SIZE;
    }
  

 
  highStateColor = getColorShift(colorCycleIndx);
  lowStateColor = CRGB(0,50,50);//getColorShift(loops + 400);



  //show bars with frequency
  
  normalizationData = normalize(frequency,normalizationData);
  float measurement = normalizationData.lastNormalizedMeasurement;

  int ledNum = ((int) (measurement * NUM_LEDS));


  
  bool mesVrms = false;
  if (mesVrms) ledNum = measurement == 0 ? prevLEDNum * 1 : ledNum*0.6 + prevLEDNum*0.4;
  

 
  
  if (MODE == 0) {
    setAll(CRGB::Black);
   
  }else if (MODE == 1) {
    
    
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
  }else if (MODE == 2) {
    
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
  }else if (MODE == 3) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = highStateColor;
    }
  }else if (MODE == 5) {
    setAll(CRGB(102,255,50));
  }else{ //mode not one of the options  
    setAll(CRGB::Red);
  }

  prevLEDNum = ledNum;



  checkModeSwitch();



  for (int i = 0; i < START_POS; i++) {
    leds[i] = CRGB::Black;
  }
  
}
