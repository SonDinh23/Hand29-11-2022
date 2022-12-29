#include "BLEHandManager.h"
//#include "freertos/FreeRTOS.h"

#include "Vulcan_RF24L01.h"
#include "HandState.h"

byte txAddr[6] = "ZZZZZ";     // not use
byte rxAddr[6] = "A0001";     //Sensor box address

#define NRF_CS       5        //CE pin NRF
#define NRF_CE       12       //CS pin NRF

#define ON_HAND      true     //LEFT HAND: true;  RIGHT HAND: false
#define MIN_ANGLE    75
#define MAX_ANGLE    121

#define PIN_FEEDBACK  4

SPIClass spiNRF(VSPI);
RF rf(NRF_CE, NRF_CS);

HandState handState(ON_HAND, MIN_ANGLE, MAX_ANGLE); 
BLEHandManager ble(handState);

void checkSensor() {
  handState.updateSensor(rf.readData());  
}

//task for BLE
void BleOp(void* pParamter) { 
  while (1) {
    vTaskDelay(1);
  }
}

// task for Hand control
void HandOp(void* pParamter) { 
  while(1) {
    checkSensor();
    handState.update();
    vTaskDelay(1);
  }
}

void setup() {
  Serial.begin(115200);

  rf.begin(txAddr, rxAddr, &spiNRF);

  ble.begin();
  handState.begin();
  
  //xTaskCreatePinnedToCore(BleOp, "BleOp", 5000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(HandOp, "HandOp", 5000, NULL, 1, NULL, 1);

  Serial.print("ID: ");
  Serial.write(rxAddr, sizeof(rxAddr));
  Serial.println();
  Serial.println("Start");
}

void loop() {
  vTaskDelete(NULL);
}
