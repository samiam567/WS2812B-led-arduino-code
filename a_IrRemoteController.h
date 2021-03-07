#ifndef REMOTE_CONTROLLER_DEFINED
#define REMOTE_CONTROLLER_DEFINED

//#define DECODE_LG           1
//#define DECODE_NEC          1
// etc. see IRremote.h
#if defined(__AVR_ATtiny85__)
#define EXCLUDE_EXOTIC_PROTOCOLS
#endif
//#define EXCLUDE_EXOTIC_PROTOCOLS // saves around 670 bytes program space if all protocols are active

// MARK_EXCESS_MICROS is subtracted from all marks and added to all spaces before decoding,
// to compensate for the signal forming of different IR receiver modules.
#define MARK_EXCESS_MICROS    20 // 20 is recommended for the cheap VS1838 modules

#include <IRremote.h>

#include "Thread.h"
#include "ThreadController.h"

#include <TimerOne.h>


/*
 * Set sensible receive pin for different CPU's
 */
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
// Serial output for Digispark boards is at pin 2
#  if defined(ARDUINO_AVR_DIGISPARKPRO)
#define IR_RECEIVE_PIN    9 // PA3 - on Digispark board labeled as pin 9
#  else
#define IR_RECEIVE_PIN    0
#  endif
#  if defined(ARDUINO_AVR_DIGISPARK)
#define LED_BUILTIN PB1
#  endif
#elif defined(ESP32)
int IR_RECEIVE_PIN = 15;
#elif defined(ARDUINO_AVR_PROMICRO)
int IR_RECEIVE_PIN = 10;
#else
int IR_RECEIVE_PIN = 11;
#endif

#define BUZZER_PIN          5
#define DEBUG_BUTTON_PIN    6 // if held low, print timing for each received data

// On the Zero and others we switch explicitly to SerialUSB


#if defined(ARDUINO_ARCH_SAMD)
#define Serial SerialUSB
#endif


class SensorThread: public Thread
{
public:
  int command;
  int raw_data;
  int pin;
  bool valueRecieved = false;
  // No, "run" cannot be anything...
  // Because Thread uses the method "run" to run threads,
  // we MUST overload this method here. using anything other
  // than "run" will not work properly...
  
  
  
  void run() {
      /*
       * Check if received data is available and if yes, try to decode it.
       * Decoded result is in the IrReceiver.decodedIRData structure.
       *
       * E.g. command is in IrReceiver.decodedIRData.command
       * address is in command is in IrReceiver.decodedIRData.address
       * and up to 32 bit raw data in IrReceiver.decodedIRData.decodedRawData
       */
      if (IrReceiver.decode()) {
  #if defined(__AVR_ATtiny85__)
          // Print a minimal summary of received data
          IrReceiver.printIRResultMinimal(&Serial);
  #else
          if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
              IrReceiver.decodedIRData.flags = false; // yes we have recognized the flag :-)
              Serial.println(F("Overflow detected"));
  #  if !defined(ESP32)
              /*
               * do double beep
               */
              IrReceiver.stop();
              tone(BUZZER_PIN, 1100, 10);
              delay(50);
  #  endif
  
          } else {
              // Print a short summary of received data
              IrReceiver.printIRResultShort(&Serial);
  
              if (IrReceiver.decodedIRData.protocol == UNKNOWN || digitalRead(DEBUG_BUTTON_PIN) == LOW) {
                  // We have an unknown protocol, print more info
                  IrReceiver.printIRResultRawFormatted(&Serial, true);
              }
          }
  
  #  if !defined(ESP32)
          /*
           * Play tone, wait and restore IR timer
           */
          IrReceiver.stop();
          tone(BUZZER_PIN, 2200, 10);
          delay(8);
          IrReceiver.start(8000); // to compensate for 11 ms stop of receiver. This enables a correct gap measurement.
  #  endif
  #endif // defined(__AVR_ATtiny85__)
  
          Serial.println();
          /*
           * !!!Important!!! Enable receiving of the next value,
           * since receiving has stopped after the end of the current received data packet.
           */
          IrReceiver.resume();
  
          /*
           * Finally check the received data and perform actions according to the received address and commands
           */

          valueRecieved = true;
          command = IrReceiver.decodedIRData.command;
          raw_data = IrReceiver.decodedIRData.decodedRawData;
          if (IrReceiver.decodedIRData.address == 0) {
              
              if (IrReceiver.decodedIRData.command == 0x10) {
                  
                  // do something
              } else if (IrReceiver.decodedIRData.command == 0x11) {
                  // do something else
              }
          }
      }

      runned();
  }
};

#endif
