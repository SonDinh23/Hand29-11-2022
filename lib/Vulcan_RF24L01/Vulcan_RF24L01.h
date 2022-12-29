#ifndef VULCAN_RF24L01_H
#define VULCAN_RF24L01_H

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

class RF {
  public:
    RF(uint8_t nrfPinCE, uint8_t nrfPinCS);
    void begin(byte txAddr[], byte rxAddr[], SPIClass *_spi, boolean isListening = true);
    void setRxAddr(byte rxAddr[]);
    void setTxAddr(byte txAddr[]);
    char readData();
    bool sendData(const void* buf, uint8_t len);
    // void setTimeReadSB(uint16_t pTime);
    
  private:
    char text[1] = {0};
    // uint32_t lastTime = millis();
    // uint16_t timeReadRF = 20;
    RF24 radio;// = RF24(NRF_CE, NRF_CS);
};

#endif