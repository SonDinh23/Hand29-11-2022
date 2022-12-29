#include "Vulcan_RF24L01.h"
#include "SensorBoxState.h"
#include <LowPower.h>
#include <

#define NRF_CE          9     //CE pin NRF
#define NRF_CS          10    //CS pin NRF

#define WAKEUP_PIN0     2     // PIN insole to OPEN
#define WAKEUP_PIN1     3     // PIN insole to CLOSE

#define LED_PIN         8

byte txAddr[6] = "A0000";     // Sensor box address
byte rxAddr[6] = "ZZZZZ";     // not use

const char data[3] = {'a','b','c'};   //data control OPEN, CLOSE, STOP

int8_t stateSB = 0;

bool onsetup = true;
bool lowBattery = false;

RF rf(NRF_CE, NRF_CS);
SBState sbState;

void isLowBattery() {
  // Serial.println(sbState.getBattery());
  lowBattery = (sbState.getBattery() < 20) ? true : false;
  // Serial.println(lowBattery);
}

void stateSensorBox(int pstate) {   //switch state send data
  switch (pstate)
  {
  case -1:
    sbState.showPixels(0, 0, 255);  //Send data CLOSE
    rf.sendData(&data[0], 1);

    break;
  case 1:
    sbState.showPixels(0, 255, 0);  //Send data OPEN
    rf.sendData(&data[1], 1);

    break;
  case 0:
    sbState.showPixels(0, 0, 0);    //Send data STOP
    rf.sendData(&data[2], 1);
    break;
  
  default:
    break;
  }
}

void isOnStop() {
  isLowBattery();
  for (int i = 0; i < 20 && digitalRead(WAKEUP_PIN0) == 1 && digitalRead(WAKEUP_PIN1) == 1; i++) {
    stateSB = 0;
    if (lowBattery) {
      sbState.showPixels(255, 0, 0);
      delay(200);
    }
    stateSensorBox(stateSB);
  }
}

void sendOpen() {
  stateSB = 1;
}

void sendClose() {
  stateSB = -1; 
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  pinMode(WAKEUP_PIN0, INPUT_PULLUP); 
  pinMode(WAKEUP_PIN1, INPUT_PULLUP); 
  attachInterrupt(0, sendOpen, LOW);
  attachInterrupt(1, sendClose, LOW);

  rf.begin(txAddr, rxAddr, &SPI, false);
  sbState.begin();
  delay(100);
  sbState.showBatterry();

  while (!stateSB) {        //Wait conect insole
    delay(200);
  }
  digitalWrite(LED_PIN, HIGH);
}


void loop() {
  attachInterrupt(0, sendOpen, LOW);
  attachInterrupt(1, sendClose, LOW);
  while (digitalRead(WAKEUP_PIN0) == 0 || digitalRead(WAKEUP_PIN1) == 0) {
    stateSensorBox(stateSB);
  }
  isOnStop();

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);    //On state sleep
  detachInterrupt(0); 
  detachInterrupt(1);
}