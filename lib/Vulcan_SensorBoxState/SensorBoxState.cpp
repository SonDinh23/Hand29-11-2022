#include "SensorBoxState.h"

SBState::SBState() {

}

void SBState::begin() {
  ina219.begin();
  pixels.begin();
  pixels.setBrightness(90);
  //showPixels(255, 255, 255);
  //delay(500);
  //showPixels(0, 0, 0);
}

void SBState::setup() {
  uint32_t lastTimeSetup = millis();
  while (millis() - lastTimeSetup < 2000) {
    showBatterry();
  }
  pixels.clear();
}

void SBState::showPixels(uint8_t pR, uint8_t pG, uint8_t pB) {
  pixels.setPixelColor(0, pixels.Color(pR, pG, pB));
  pixels.show();
}

uint8_t SBState::getBattery() {
  float loadvoltage = ina219.getBusVoltage_V() + (ina219.getShuntVoltage_mV() / 1000);
  uint8_t persen = (loadvoltage - BATTERY_MIN) * 100 / (BATTERY_MAX - BATTERY_MIN);
  persen = constrain(persen, 0, 100);
  return persen;
}

void SBState::showBatterry() {
  uint8_t percent = getBattery();
  uint8_t RGB[3] = {0, 0, 0};
  if (percent > 50)  RGB[1] = 255;
  else if (percent > 25) { RGB[0] = 255; RGB[1] = 255; RGB[2] = 0; } 
  else  RGB[0] = 255;
  showPixels(RGB[0], RGB[1], RGB[2]);
  Serial.println(percent);
}