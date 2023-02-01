
#include "CameraOV7670.h"


bool CameraOV7670::init() {
  registers.init();
  initIO();
  delay(10); // give camera some time to run before starting setup
  return setUpCamera();
}


void CameraOV7670::initIO() {
#ifdef OV7670_INIT_INPUTS
  OV7670_INIT_INPUTS;
#endif
  OV7670_INIT_CLOCK_OUT;
}


bool CameraOV7670::setUpCamera() {
  if (registers.resetSettings()) {
    registers.setRegisters(CameraOV7670Registers::regsDefault);

    registers.setRegisters(CameraOV7670Registers::regsYUV422);

    registers.setRegisters(CameraOV7670Registers::regsQQVGA);
    verticalPadding = CameraOV7670Registers::QQVGA_VERTICAL_PADDING;
    
    registers.setDisablePixelClockDuringBlankLines();
    registers.setDisableHREFDuringBlankLines();
    registers.setInternalClockPreScaler(internalClockPreScaler);
    registers.setPLLMultiplier(pllMultiplier);

    return true;
  } else {
    return false;
  }
}

void CameraOV7670::ignoreVerticalPadding() {
  for (uint8_t i = 0; i < verticalPadding; i++) {
    ignoreHorizontalPaddingLeft();
    for (uint16_t x = 0; x < resolution * 2; x++) {
      waitForPixelClockRisingEdge();
    }
    ignoreHorizontalPaddingRight();
  }
}
