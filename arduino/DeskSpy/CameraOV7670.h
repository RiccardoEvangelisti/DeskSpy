#ifndef _CAMERA_OV7670_h_
#define _CAMERA_OV7670_h_

#include "Arduino.h"
#include "CameraOV7670Registers.h"


/*
  B (digital pin 8 to 13)
  C (analog input pins)
  D (digital pins 0 to 7)
*/

#ifndef OV7670_VSYNC
#define OV7670_VSYNC (PIND & 0b00000100) // PIN 2
#endif

#ifndef OV7670_PIXEL_CLOCK
#define OV7670_PIXEL_CLOCK_PIN 12
#define OV7670_PIXEL_CLOCK (PINB & 0b00010000) // PIN 12
#endif

#ifndef OV7670_READ_PIXEL_BYTE
// (PIN 4..7) | (PIN A0..A3)
#define OV7670_READ_PIXEL_BYTE(b) \
                    uint8_t pinc=PINC;\
                    b=((PIND & 0b11110000) | (pinc & 0b00001111))
#endif

// pin 3 to 8Mhz (LiveOV7670Library clock)
#ifndef OV7670_INIT_CLOCK_OUT
#define OV7670_INIT_CLOCK_OUT \
                    pinMode(3, OUTPUT); \
                    TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20); \
                    TCCR2B = _BV(WGM22) | _BV(CS20); \
                    OCR2A = 1; \
                    OCR2B = 0
#endif



class CameraOV7670 {

public:
    enum PixelFormat {
        PIXEL_YUV422
    };

    enum Resolution {
        RESOLUTION_QQVGA_160x120 = 160
    };

    enum PLLMultiplier {
        PLL_MULTIPLIER_BYPASS = 0
    };


protected:
    static const uint8_t i2cAddress = 0x21;

    const Resolution resolution;
    PixelFormat pixelFormat;
    uint8_t internalClockPreScaler;
    PLLMultiplier pllMultiplier;
    CameraOV7670Registers registers;
    uint8_t verticalPadding = 0;

public:

    CameraOV7670(
        Resolution resolution = RESOLUTION_QQVGA_160x120,
        PixelFormat format = PIXEL_YUV422,
        uint8_t internalClockPreScaler = 2,
        PLLMultiplier pllMultiplier = PLL_MULTIPLIER_BYPASS
    ) :
        resolution(resolution),
        pixelFormat(format),
        internalClockPreScaler(internalClockPreScaler),
        pllMultiplier(pllMultiplier),
        registers(i2cAddress) {};

    bool init();

    inline void waitForVsync(void) __attribute__((always_inline));
    inline void waitForPixelClockRisingEdge(void) __attribute__((always_inline));
    inline void waitForPixelClockLow(void) __attribute__((always_inline));
    inline void waitForPixelClockHigh(void) __attribute__((always_inline));
    inline void ignoreHorizontalPaddingLeft(void) __attribute__((always_inline));
    inline void ignoreHorizontalPaddingRight(void) __attribute__((always_inline));
    inline void readPixelByte(uint8_t & byte) __attribute__((always_inline));

    virtual void ignoreVerticalPadding();

protected:
    virtual bool setUpCamera();

private:
    void initIO();
};



void CameraOV7670::waitForVsync() {
  while(!OV7670_VSYNC);
}

void CameraOV7670::waitForPixelClockRisingEdge() {
  waitForPixelClockLow();
  waitForPixelClockHigh();
}

void CameraOV7670::waitForPixelClockLow() {
  while(OV7670_PIXEL_CLOCK);
}

void CameraOV7670::waitForPixelClockHigh() {
  while(!OV7670_PIXEL_CLOCK);
}

// One byte at the beginning
void CameraOV7670::ignoreHorizontalPaddingLeft() {
  waitForPixelClockRisingEdge();
}

// Three bytes at the end
void CameraOV7670::ignoreHorizontalPaddingRight() {
  volatile uint16_t pixelTime = 0;

  waitForPixelClockRisingEdge();
  waitForPixelClockRisingEdge();

  // After the last pixel byte of an image line there is a very small pixel clock pulse.
  // To avoid accidentally counting this small pulse we measure the length of the
  // last pulse and then wait the same time to add a byte wide delay at the
  // end of the line.
  while(OV7670_PIXEL_CLOCK) pixelTime++;
  while(!OV7670_PIXEL_CLOCK) pixelTime++;
  while(pixelTime) pixelTime--;
}

void CameraOV7670::readPixelByte(uint8_t & byte) {
  OV7670_READ_PIXEL_BYTE(byte);
}


#endif // _CAMERA_OV7670_h_
