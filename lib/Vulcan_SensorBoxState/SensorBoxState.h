#ifndef SENSOR_BOX_STATE_H
#define SENSOR_BOX_STATE_H

#include <Adafruit_INA219.h>
#include <Adafruit_NeoPixel.h>

#define BATTERY_MAX   4.2
#define BATTERY_MIN   3.5

#define LED_RGB_PIN     5 

class SBState {
  public:
    SBState();
    void begin();
    void setup();
    void showPixels(uint8_t pR, uint8_t pG, uint8_t pB);
    void showBatterry();

    uint8_t getBattery();

  private:
    Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, LED_RGB_PIN, NEO_GRB + NEO_KHZ800);
    Adafruit_INA219 ina219;

};

#endif