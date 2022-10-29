#include <IRremote.h>
#include <FastLED.h>
#include "a_IrRemoteController.h"
#include "musicLeds.h"

#define IR_PIN 13
#define LED_PIN 10
#define MODE_BUTTON_PIN 4
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define LED_NUM 262
#define END_OFFSET 0
#define START_POS 0 // 60

#define NUM_LEDS (LED_NUM - END_OFFSET)

CRGB leds[LED_NUM];
decode_results results; // create a results object of the decode_results class

int brightness = 94; // max 94

unsigned long key_value = 0; // variable to store the pressed key value
unsigned long faderLoops = 0;

#include "a_rainbow.h"

// Now, let's use our new class of Thread
SensorThread analog1 = SensorThread();

// Instantiate a new ThreadController
ThreadController controller = ThreadController();

// This is the callback for the Timer
void timerCallback()
{
  controller.run();
}

void setup()
{

#if !defined(__AVR_ATtiny85__)
  pinMode(DEBUG_BUTTON_PIN, INPUT_PULLUP);
#endif

  Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_USB) || defined(SERIAL_PORT_USBVIRTUAL) || defined(ARDUINO_attiny3217)
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

  // led setup code:
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, LED_NUM).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);

  pinMode(MODE_BUTTON_PIN, INPUT);

  musicLEDSsetup();
}

CRGB mostRecentColor = CRGB::White;

void turnOff()
{
  for (int i = 0; i < LED_NUM; i++)
  {
    leds[i] = CRGB::Black;
  }
}
void setAll(CRGB color)
{
  mostRecentColor = color;
  for (int i = START_POS; i < NUM_LEDS; i++)
  {
    leds[i] = color;
  }
}

//#define __REMOTE_ONE__
#define __REMOTE_TWO__

// MODE:
#define MODE_REMOTE 0
#define MODE_MUSIC 1
#define MODE_RAINBOW 2
#define MODE_FADE 3
#define MODE_WAVE 4
#define MODE_OFF 5
#define MAX_MODE 5

int mode = MODE_FADE;
int music_mode = 1;

bool useMicrophone = false;
bool mode_button_pressed = false;

void loop()
{

  // check for mode button inputs
    if (digitalRead(MODE_BUTTON_PIN))
    {
      if (!mode_button_pressed)
      {
        mode_button_pressed = true;
        mode++;
        turnOff(); // reset before switching modes (this is for mode_off)
        if (mode > MAX_MODE)
        {
          mode = MODE_RAINBOW;
        }
      }
    }
    else
    {
      mode_button_pressed = false;
    }
  


  // check for ir remote input
  if (analog1.valueRecieved)
  {
    analog1.valueRecieved = false;

    Serial.println("RECIEVING SIGNAL: ");
    Serial.println(analog1.command, DEC);

    int prevMode = mode;
    mode = MODE_REMOTE; // let the remote set color

    // make sure START_POS is honored (*COUGH* *COUGH* Rainbow)
    for (int i = 0; i < START_POS; i++)
    {
      leds[i] = CRGB::Black;
    }

    switch (analog1.command)
    {

#ifdef __REMOTE_ONE__ // This code is for Emily's remote
    case (88):        // Red
      setAll(CRGB::Red);
      break;
    case (69): // blue
      setAll(CRGB::Blue);
      break;
    case (89): // green
      setAll(CRGB::Green);
      break;
    case (68): // white
      setAll(CRGB::White);
      break;
    case (84): // orange
      setAll(CRGB::Orange);
      break;
    case (24): // Yellow
      setAll(CRGB::Yellow);
      break;
    case (80): // O2
      setAll(CRGB::Orange);
      break;
    case (28): // O3
      setAll(CRGB::Orange);
      break;
    case (85): // lime green
      setAll(CRGB::Green);
      break;
    case (25): // teal
      setAll(CRGB::Teal);
      break;
    case (73): // Berry blue
      setAll(CRGB::Blue);
      break;
    case (77): // purple
      setAll(CRGB::Purple);
      break;
    case (30): // hot pink
      setAll(CRGB::Magenta);
      break;
    case (26): // pink
      setAll(CRGB::Pink);
      break;
    case (72): // light pink
      setAll(CRGB::Pink);
      break;
    case (76): // lighter pink
      setAll(CRGB::Pink);
      break;
    case (31): // light blue
      setAll(CRGB::Blue);
      break;
    case (27): // lighter blue
      setAll(CRGB::Blue);
      break;
    case (81): // turquoise
      setAll(CRGB::Turquoise);
      break;
    case (29):
      setAll(CRGB::Cyan);
      break;
    case (64): // also OFF
      setAll(CRGB::Black);
      break;
    case (0):
      // repeated button
      break;
    case (92):
      brightness += brightness + 5 > 93 ? 0 : 5;
      FastLED.setBrightness(brightness);
      Serial.print("Setting brightness: ");
      Serial.println(brightness);
      break;
    case (93):
      brightness -= brightness - 5 < 0 ? 0 : 5;
      FastLED.setBrightness(brightness);
      Serial.print("Setting brightness: ");
      Serial.println(brightness);
      break;

    case (4): // jump 3 (
      mode = MODE_RAINBOW;
      break;

#endif // end Emily's remote config

#ifdef __REMOTE_TWO__ // Alec's Remote

    case (3): // off button
      turnOff();
      mode = MODE_OFF;
      break;

    case (2): // on button
      setAll(mostRecentColor);
      break;

    case (4): // red
      setAll(CRGB::Red);
      break;

    case (5):
      setAll(CRGB::Green);
      break;

    case (14):
      mode = MODE_RAINBOW;
      break;

    case (15):
      mode = MODE_FADE;
      break;

    case (16):
      mode = MODE_MUSIC;
      break;

    case (17):
      resetNormalizationData();
      break;

    case (18):
      music_mode++;
      if (music_mode > MAX_MUSIC_MODE)
      {
        music_mode = 0;
      }
      mode = MODE_MUSIC;
      break;

    case (12): // jump 3
      mode = MODE_WAVE;
      break;
    case (6): // blue
      setAll(CRGB::Blue);
      break;

    case (7): // white
      setAll(CRGB::White);
      break;

    case (8): // orange
      setAll(CRGB::Orange);
      break;

    case (9): // yellow
      setAll(CRGB::Yellow);
      break;

    case (10): // cyan
      setAll(CRGB::Cyan);
      break;

    case (11): // magenta
      setAll(CRGB::Magenta);
      break;

    case (0):
      brightness += brightness + 5 > 93 ? 0 : 5;
      FastLED.setBrightness(brightness);
      Serial.print("Setting brightness: ");
      Serial.println(brightness);
      break;
    case (1):
      brightness -= brightness - 5 < 0 ? 0 : 5;
      FastLED.setBrightness(brightness);
      Serial.print("Setting brightness: ");
      Serial.println(brightness);
      break;

#endif // end Alec's remote config
    }

    FastLED.setBrightness(brightness);

    FastLED.show();
  }
  else if (mode == MODE_OFF)
  {
    // do nothing
    turnOff();
    return;
  }
 

 
    

  // for time-dependent color shifts
  faderLoops++;
  if (faderLoops > 25000)
    faderLoops = 0;

  if (mode == MODE_RAINBOW)
  {
    runRainbow();
  }
  else if (mode == MODE_MUSIC)
  {
    runMusicLeds(useMicrophone);
  }
  else if (mode == MODE_FADE)
  {
    setAll(getColorShift(((float)faderLoops) / 10.0f));
  }
  else if (mode == MODE_WAVE)
  {
    const float k = 0.01;
    const float w = 0.02f;
    const float A = 2000;
    double fadeLoops = faderLoops;
    for (float i = START_POS; i < NUM_LEDS; i++)
    {
      leds[(int)i] = getColorShift((sin(k * i - w * fadeLoops) + 1.0) * A / 2.0);
      if (i == 100)
        Serial.println((sin(k * i - w * fadeLoops) + 1.0) * A / 2.0);
    }
  }

  // the remote mode will have already called this
  if (mode != MODE_REMOTE)
  {
    FastLED.show();
  }
}
