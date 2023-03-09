#ifndef _DESK_SPY_DEFINITIONS_h_
#define _DESK_SPY_DEFINITIONS_h_

#include "Arduino.h"
#include "DS3231.h"
#include "CameraOV7670.h"

// --------------------------------------------------------------------------------
// Pins
#define buzzer  11
#define led_warning  13
#define led_break  10
#define led_work  9
#define button_snooze  8

// --------------------------------------------------------------------------------
// Time constants
//#define DS_DEBUG 1
#ifdef DS_DEBUG

  #define prepareForWorkTime 5 //seconds
  #define workTime 5 //minutes

  #define prepareForBreakTime 5 //seconds
  #define breakTime 5 //minutes
  
  #define snoozeTime 1 //minutes
  #define snoozeAvailableTimes 2 //Number of times the user can snooze before breakState
  
  #define maxAbsenceTime 1 //minutes. Max absence from work before an early break
  #define maxPresenceTime 1 //minutes. Max presence during break before an early work
  
  #define captureFrequency_workDetected 10 //seconds
  #define captureFrequency_workNotDetected 3 //seconds
  
  #define captureFrequency_breakNotDetected 5 //seconds
  #define captureFrequency_breakDetected 3 //seconds

#else

  #define prepareForWorkTime 60 //seconds
  #define workTime 60 //minutes

  #define prepareForBreakTime 60 //seconds
  #define breakTime 10 //minutes
  
  #define snoozeTime 10 //minutes
  #define snoozeAvailableTimes 2 //Number of times the user can snooze before breakState
  
  #define maxAbsenceTime 5  //minutes. Max absence from work before an early break
  #define maxPresenceTime 5 //minutes. Max presence during break before an early work
  
  #define captureFrequency_workDetected 60 //seconds
  #define captureFrequency_workNotDetected 3 //seconds
  
  #define captureFrequency_breakNotDetected 5 //seconds
  #define captureFrequency_breakDetected 3 //seconds

#endif

// --------------------------------------------------------------------------------
// Camera functions

void takeAndSendFrame();
inline void waitUntilPreviousUartByteIsSent() __attribute__((always_inline));
inline bool isUartReady() __attribute__((always_inline));

// --------------------------------------------------------------------------------

void initialize();

uint32_t workState(int early_work);
void snoozeState();
int breakState(int early_break);

void reset();

// --------------------------------------------------------------------------------
// Utility funcions

uint32_t getActualTime();

unsigned long minsToMillis(int mins);
unsigned long minsToSecs(int mins);
unsigned long secsToMillis(int secs);

char serialReadBlocking();

void clearReciveBuffer();

// --------------------------------------------------------------------------------
// Digital pins interactions

void turnOn(uint8_t pin);
void turnOff(uint8_t pin);

void buzz(int secs);

void blink(int pin, int secs, int state);
void blink(int pin, int secs, int state, uint8_t snoozeAvailable);

void warningBlink();
void warningBlinkAndBuzz();


#endif // _DESK_SPY_DEFINITIONS_H_
