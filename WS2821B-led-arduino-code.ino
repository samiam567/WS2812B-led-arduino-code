#include <IRremote.h>
#include <FastLED.h>
#include "a_IrRemoteController.h"


#define IR_PIN 13
#define LED_PIN 10
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 120

CRGB leds[NUM_LEDS];
decode_results results; // create a results object of the decode_results class


int BRIGHTNESS = 30;  //max 94

unsigned long key_value = 0; // variable to store the pressed key value

#include "a_rainbow.h"

// Now, let's use our new class of Thread
SensorThread analog1 = SensorThread();


// Instantiate a new ThreadController
ThreadController controller = ThreadController();

// This is the callback for the Timer
void timerCallback(){
  controller.run();
}

void setup() {

  #if !defined(__AVR_ATtiny85__)
    pinMode(DEBUG_BUTTON_PIN, INPUT_PULLUP);
   #endif

    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_USB) || defined(SERIAL_PORT_USBVIRTUAL)  || defined(ARDUINO_attiny3217)
delay(4000); // To be able to connect Serial monitor after reset or power up and before first printout
#endif
// Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

// In case the interrupt driver crashes on setup, give a clue
// to the user what's going on.
    Serial.println(F("Enabling IRin"));

    /*
     * Start the receiver, enable feedback LED and (if not 3. parameter specified) take LED feedback pin from the internal boards definition
     */
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

    Serial.print(F("Ready to receive IR signals at pin "));
    Serial.println(IR_RECEIVE_PIN);


  
    // Configures Thread analog1
    analog1.pin = IR_PIN;
    analog1.setInterval(100);
  
    // Add the Threads to our ThreadController
    controller.add(&analog1);
  
    /*
      If using TimerOne...
    */
     Timer1.initialize(20000);
     Timer1.attachInterrupt(timerCallback);
     Timer1.start();

  //led setup code:
  FastLED.addLeds<LED_TYPE,LED_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}

CRGB mostRecentColor = CRGB::Black;

void setAll(CRGB color) {
  mostRecentColor = color;
  for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = color;
  }  
}


//MODE:
#define MODE_REMOTE 0
#define MODE_RAINBOW 1

int mode = MODE_REMOTE;
void loop() {

  
  if (analog1.valueRecieved) {
    analog1.valueRecieved = false;

    Serial.println("RECIEVING SIGNAL: ");
    Serial.println(analog1.command, DEC);

   
    mode = MODE_REMOTE; // let the remote set color
    
    switch(analog1.command) {
      case(88): //Red
        setAll(CRGB::Red);
        break;
      case(69): // blue
        setAll(CRGB::Blue);
        break;
      case(89): //green
        setAll(CRGB::Green);
        break;
      case(68): //white
        setAll(CRGB::White);
        break;
      case(84): // orange
        setAll(CRGB::Orange);
        break;
      case(24): //Yellow
        setAll(CRGB::Yellow);
        break;
      case(80): // O2
        setAll(CRGB::Orange);
        break;
      case(28): // O3
        setAll(CRGB::Orange);
        break;
      case(85): // lime green
        setAll(CRGB::Green);
        break;
      case(25): // teal
        setAll(CRGB::Teal);
        break;
      case(73): // Berry blue
        setAll(CRGB::Blue);
        break;
      case(77): // purple
        setAll(CRGB::Purple);
        break;
      case(30): // hot pink
        setAll(CRGB::Magenta);
        break;
      case(26): // pink
        setAll(CRGB::Pink);
        break;
      case(72): // light pink
        setAll(CRGB::Pink);
        break;
      case(76): // lighter pink
        setAll(CRGB::Pink);
        break;
      case(31): // light blue
        setAll(CRGB::Blue);
        break;
      case(27): // lighter blue
        setAll(CRGB::Blue);
        break;
      case(81): //turquoise
        setAll(CRGB::Turquoise);
        break;
      case(29):
        setAll(CRGB::Cyan);
        break;
      case(64): // also OFF
        setAll(CRGB::Black);
        break;
      case(0):
        //repeated button
        break;
      default:
        break;

      case(4): // jump 3 (
        mode = MODE_RAINBOW;
        break;
      
    }
    
    FastLED.show();

    
  }



  if (mode == MODE_RAINBOW) {
    runRainbow(); 

    
  }

  //the remote mode will have already called this
  if (mode != MODE_REMOTE) {
    FastLED.show();
  }
 
     
  
}
  
