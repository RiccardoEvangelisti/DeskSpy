#include "Arduino.h"
#include "CameraOV7670.h"
#include "DS3231.h"
#include "DeskSpyDefinitions.h"

void(* resetFunc) (void) = 0;

uint32_t actualTime;
uint32_t startTime;
uint32_t startWarningTime;
bool detected;

uint32_t early_break = 0; //seconds
uint32_t early_work = 0;  //seconds
uint8_t snoozeAvailable = snoozeAvailableTimes;

void setup() {
  pinMode(led_warning, OUTPUT);
  pinMode(led_break, OUTPUT);
  pinMode(led_work, OUTPUT);
  pinMode(buzzer, OUTPUT);

  initialize();
  delay(3000); // for setup

  reset();
}

void loop() {
  // ---- PREPARE_FOR_WORK State ----
  turnOn(led_work);
  if (early_work == 0) { // if not EARLY WORK
    buzz(1);
    blink(led_work, prepareForWorkTime, HIGH);
  }
  // ---- WORK State ----
  early_break = workState(early_work);
  turnOff(led_work);

  // ---- PREPARE_FOR_BREAK State ----
  turnOn(led_break);
  snoozeAvailable = snoozeAvailableTimes;
  if (early_break == 0) { // if not EARLY BREAK
    buzz(1);
    blink(led_break, prepareForBreakTime, HIGH, snoozeAvailable);
  }
  // ---- BREAK State ----
  early_work = breakState(early_break);
  turnOff(led_break);
}

uint32_t workState(uint32_t early_work) {
  clearReciveBuffer();
  startTime = getActualTime();
  do {
    takeAndSendFrame();
    switch (serialReadBlocking()) {
    case '1': // Reset
      resetFunc();
    case '2': // Detected
      //
      break;
    case '3': // Not Detected
      turnOn(led_warning);
      startWarningTime = getActualTime();
      detected = false;
      do {
        warningBlinkAndBuzz();
        delay(secsToMillis(captureFrequency_workNotDetected));
        takeAndSendFrame();
        switch (serialReadBlocking()) {
          case '2':
            // Detected
            detected = true;
            break;
          case '1': // Reset
            resetFunc();
        }
        actualTime = getActualTime();
      } while ((!detected) && //
                ((actualTime - startWarningTime) <= ((uint32_t) minsToSecs(maxAbsenceTime))) && //
                ((actualTime - startTime) <= (((uint32_t) minsToSecs(workTime)) - early_work))); // if the break starts anyway
      turnOff(led_warning);
      if (!detected) {
        // EARLY BREAK
        return getActualTime() - startWarningTime;
      }
      break;
    default:
      break;
    }
    Serial.print(getActualTime());
    delay(secsToMillis(captureFrequency_workDetected));
  } while ((getActualTime() - startTime) <= (((uint32_t) minsToSecs(workTime)) - early_work));

  return 0; //early_break = 0
}

void snoozeState() {
  snoozeAvailable--;
  turnOn(led_break);
  turnOn(led_work);
  delay(minsToMillis(snoozeTime));
  turnOff(led_work);
}

uint32_t breakState(uint32_t early_break) {
  clearReciveBuffer();
  startTime = getActualTime();
  do {
    takeAndSendFrame();
    switch (serialReadBlocking()) {
    case '1': // Reset
    resetFunc();
    case '3': // Not Detected
      //
      break;
    case '2': // Detected
      turnOn(led_warning);
      startWarningTime = getActualTime();
      detected = true;
      do {
        warningBlinkAndBuzz();
        delay(secsToMillis(captureFrequency_breakDetected));
        takeAndSendFrame();
        switch (serialReadBlocking()) {
          case '3':
            // Not Detected
            detected = false;
            break;
          case '1': // Reset
            resetFunc();
        }
        actualTime = getActualTime();
      } while (detected && //
                ((actualTime - startWarningTime) <= ((uint32_t) minsToSecs(maxPresenceTime))) && //
                ((actualTime - startTime) <= (((uint32_t) minsToSecs(breakTime)) - early_break))); // if the work starts anyway
      turnOff(led_warning);
      if (detected) {
        // EARLY WORK
        return getActualTime() - startWarningTime;
      }
      break;
    default:
      //
      break;
    }
    delay(secsToMillis(captureFrequency_breakNotDetected));
  } while ((getActualTime() - startTime) <= (((uint32_t) minsToSecs(breakTime)) - early_break));

  return 0; //early_work = 0
}
