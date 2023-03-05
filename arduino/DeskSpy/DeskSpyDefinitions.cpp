#include "DeskSpyDefinitions.h"

// --------------------------------------------------------------------------------
// Time variables
DS3231 clock;

// --------------------------------------------------------------------------------
// Camera constants and functions
const uint8_t lineLength = 160;
const uint8_t lineCount = 120;
const uint32_t baud = 1000000; // 1Mbps: communication speed
uint8_t lineBuffer[lineLength];
CameraOV7670 camera;

uint32_t previous_time = 0;

char byteRead[1];
int ret;

// --------------------------------------------------------------------------------

void initialize() {
  Serial.begin(baud);
  while(!Serial);
  Serial.setTimeout(10000); // 10 seconds
  camera.init();
  clock.begin(); // Initialize DS3231
}

void takeAndSendFrame() {
  waitUntilPreviousUartByteIsSent();
  UDR0 = 0x00; // START FRAME COMMAND

  noInterrupts();
  camera.waitForVsync();
  camera.ignoreVerticalPadding();

  for (uint16_t y = 0; y < lineCount; y++) {
    camera.ignoreHorizontalPaddingLeft();

    uint16_t x = 0;
    while (x < lineLength) {
      camera.waitForPixelClockRisingEdge(); // YUV422 grayscale byte: Y1
      camera.readPixelByte(lineBuffer[x]);

      camera.waitForPixelClockRisingEdge(); // YUV422 color byte. Ignore: U
      x++;

      camera.waitForPixelClockRisingEdge(); // YUV422 grayscale byte: Y2
      camera.readPixelByte(lineBuffer[x]);

      camera.waitForPixelClockRisingEdge(); // YUV422 color byte. Ignore: V
      x++;
    }
    camera.ignoreHorizontalPaddingRight();

    // Send the line
    for (uint16_t i = 0; i < x; i++) {
      waitUntilPreviousUartByteIsSent();
      if (lineBuffer[i] == 0b00000000) {
        lineBuffer[i] = 0b00000001; // Because 0x00 is the START FRAME COMMAND
      }
      UDR0 = lineBuffer[i];
    }
  }
  interrupts();
  waitUntilPreviousUartByteIsSent();
  Serial.flush();
}

void waitUntilPreviousUartByteIsSent() {
  while (!isUartReady()); // wait for byte to transmit
}

bool isUartReady() { return UCSR0A & (1 << UDRE0); }

// --------------------------------------------------------------------------------

// Reset leds, buzz, buffer.
// Waits for next connection
void reset() {
  turnOff(led_warning);
  turnOff(led_break);
  turnOff(led_work);
  turnOff(buzzer);

  clearReciveBuffer();
  // Sync with server
  Serial.write("s");
  // Wait the Ack
  turnOn(led_warning);
  while(Serial.read() != 'a');
  turnOff(led_warning);
}

// --------------------------------------------------------------------------------
// Utility funcions

uint32_t getActualTime() {
  uint32_t actual_time = clock.getDateTime().unixtime;
  if(((actual_time - previous_time) >= 100) && (previous_time != 0)) { // this prevents RTC errors
    return previous_time;
  }
  previous_time = actual_time;
  return previous_time;
}

unsigned long minsToMillis(int mins) { return ((unsigned long) mins) * 60L * 1000L; }

unsigned long minsToSecs(int mins) { return ((unsigned long) mins) * 60L; }

unsigned long secsToMillis(int secs) { return ((unsigned long) secs) * 1000L; }

// Serial read blocking, to sync with the server
char serialReadBlocking() {
  /*
  // WARNING: it may cause deadlock!
  Serial.setTimeout(10000); // 10 seconds
  delay(100); // wait 1 second to set up the timer
  ret = 0;
  byteRead[0] = '0';
  ret = Serial.readBytes(byteRead, 1);
  if (ret == 0) {
    return '0';
  } else {
    return byteRead[0];
  }*/
  delay(500);
  if (Serial.available() > 0) {
    return Serial.read();
  } else {
    return '0';
  }
}

// Clear the receive buffer
void clearReciveBuffer() {
  while(Serial.available()){
    Serial.read();
  }
}


// --------------------------------------------------------------------------------
// Digital pins interactions

// Turn On pin
void turnOn(uint8_t pin) {
  digitalWrite(pin, HIGH);
}
// Turn Off pin
void turnOff(uint8_t pin) {
  digitalWrite(pin, LOW);
}

// Buzz the buzzer for "secs" seconds
void buzz(int secs) {
  digitalWrite(buzzer, HIGH);
  delay(secsToMillis(secs));
  digitalWrite(buzzer, LOW);
}

void blink(int pin, int secs, int state) {
  blink(pin, secs, state, 0);
}

// Blink the led "pin" for "secs" seconds with initial "state"
void blink(int pin, int secs, int state, uint8_t snoozeAvailable) {
  for (int i = 0; i < secs; i++) {
    digitalWrite(pin, !state);
    state = !state;
    delay(1000);
    if ((snoozeAvailable > 0) && (pin == led_break)) {
      if (digitalRead(button_snooze) == HIGH) {
        snoozeState();
      }
    }
  }
  turnOn(pin); // always end HIGH
}

void warningBlink() {
  turnOff(led_warning);
  delay(200);
  turnOn(led_warning);
}

void warningBlinkAndBuzz() {
  turnOn(buzzer);
  turnOff(led_warning);
  delay(200);
  turnOff(buzzer);
  turnOn(led_warning);
}
