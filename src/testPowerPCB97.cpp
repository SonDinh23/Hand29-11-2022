#include "BLEHandManager.h"
#include "freertos/FreeRTOS.h"

#include "Vulcan_RF24L01.h"
#include "HandState.h"

byte txAddr[6] = "ZZZZZ";   // not use
byte rxAddr[6] = "A0001";   //Sensor box address

#define NRF_CS       5
#define NRF_CE       12

#define ON_HAND      true     //LEFT HAND: true;  RIGHT HAND: false
#define MIN_ANGLE    61
#define MAX_ANGLE    120

SPIClass spiNRF(VSPI);
RF rf(NRF_CE, NRF_CS);

HandState handState(ON_HAND, MIN_ANGLE, MAX_ANGLE); 
BLEHandManager ble(handState);

uint8_t stateTest = 0;
char buff[2];

void checkSensor() {
  // static uint8_t i = 82;
  // static int8_t vec = 1;
  // static char pchar = 'a';
  // if (i < 73) {vec = 1; pchar = 'a';}
  // else if (i > 105) { vec = -1; pchar = 'b'; vTaskDelay(1000);}
  // i = i + vec;

  // handState.updateSensor(pchar);
  // vTaskDelay(50);
  
  handState.updateSensor(rf.readData());
}

void BleOp(void* pParamter) { //task for BLE
  while (1) {
    if (Serial.available() > 0) {
      Serial.readBytes(buff, 1);
      stateTest = atoi(buff);
      Serial.println(stateTest);
    }
    vTaskDelay(100);
  }
}

void HandOp(void* pParamter) { // task for Hand control
  while(1) {
    switch (stateTest)
    {
      case 1:
        Serial.println("Begin test Angle");
        handState.testAngle();
        stateTest = 0;
        Serial.println("End test Angle");
        break;
      case 2:
        Serial.println("Begin test Power");
        handState.testPower();
        stateTest = 0;
        Serial.println("End test Power");
      case 3:
        Serial.println("Begin find linear FB");
        handState.findLinearFB(handState.getMinAngle(), handState.getMaxAngle());
        stateTest = 0;
        Serial.println("End find linear FB");
      
      default:
        stateTest = 0;
        //Serial.println("x");
        break;
    }
    vTaskDelay(1);
  }
  
}

void setup() {
  Serial.begin(115200);

  rf.begin(txAddr, rxAddr, &spiNRF);

  ble.begin();
  handState.begin();
  
  xTaskCreatePinnedToCore(BleOp, "BleOp", 5000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(HandOp, "HandOp", 5000, NULL, 1, NULL, 1);

  Serial.println("Start");
  Serial.print("ID: ");
  Serial.write(rxAddr, sizeof(rxAddr));
  Serial.println();
}

void loop() {
  vTaskDelete(NULL);
}