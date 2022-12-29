#include "Vulcan_RF24L01.h"
#include <LowPower.h>
//#include <freertos/FreeRTOS.h>
//#include "freertos/FreeRTOS.h"

#define NRF_CE       9      //CE pin NRF
#define NRF_CS       10     //CS pin NRF
  
#define WAKEUP_PIN0    2    // PIN insole to OPEN
#define WAKEUP_PIN1    3    // PIN insole to CLOSE

byte txAddr[6] = "A0000";   // Sensor box address
byte rxAddr[6] = "ZZZZZ";   // not use

const char data[3] = {'a','b','c'};   //data control OPEN, CLOSE, STOP

bool onSetup = true;

RF rf(NRF_CE, NRF_CS);

void sendStop() {
  for (int i = 0; i < 20 && digitalRead(WAKEUP_PIN0) == 1 && digitalRead(WAKEUP_PIN1) == 1; i++) {
    rf.sendData(&data[2], 1);
    digitalWrite(8, HIGH);
  }
}

void sendOpen() {
  digitalWrite(8, LOW); 
  onSetup = false;
  rf.sendData(&data[0], 1);
  digitalWrite(8, HIGH);
  sendStop();
}

void sendClose(){
  digitalWrite(8, LOW); 
  onSetup = false;
  rf.sendData(&data[1], 1);
  digitalWrite(8, HIGH); 
  sendStop();   
}

void setup(){
  pinMode(8, OUTPUT);
 
  pinMode(WAKEUP_PIN0, INPUT_PULLUP); 
  pinMode(WAKEUP_PIN1, INPUT_PULLUP); 
  attachInterrupt(0, sendOpen, LOW);
  attachInterrupt(1, sendClose, LOW);
  
  rf.begin(txAddr, rxAddr, &SPI, false);

  while (onSetup){                    //Wait conect insole
    digitalWrite(8, HIGH);
    delay(100);
    digitalWrite(8, LOW);
    delay(100);   
  }
  digitalWrite(8, HIGH);
}

void loop() {
  attachInterrupt(0, sendOpen, LOW);
  attachInterrupt(1, sendClose, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); //On state sleep
  detachInterrupt(0); 
  detachInterrupt(1);
}