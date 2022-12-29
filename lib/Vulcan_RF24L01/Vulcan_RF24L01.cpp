#include "Vulcan_RF24L01.h"

RF::RF(uint8_t nrfPinCE, uint8_t nrfPinCS){
  radio = RF24(nrfPinCE, nrfPinCS);
}

void RF::begin(byte txAddr[], byte rxAddr[], SPIClass* pspi,  boolean isListening) {
  pspi->begin();
  radio.begin(pspi);
  radio.setPALevel(RF24_PA_HIGH);       // RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate(RF24_250KBPS );     // RF24_250KBPS, RF24_1MBPS, RF24_2MBPS
  radio.setPayloadSize(1);
  radio.setRetries(15, 15);
  radio.openWritingPipe(txAddr);
  radio.openReadingPipe(0, rxAddr);

  if (isListening) radio.startListening();
  else radio.stopListening();
}

char RF::readData() {
  // if (millis() - lastTime > timeReadRF) {
    radio.read(&text, 1);
    // lastTime = millis();
    return text[0];
  // }
  // return 'z';
}

bool RF::sendData(const void* buf, uint8_t len) {
  return radio.write(buf, len);
}

void RF::setRxAddr(byte rxAddr[]) {
  radio.openReadingPipe(0, rxAddr);
}

void RF::setTxAddr(byte txAddr[]) {
  radio.openWritingPipe(txAddr);
}

// void RF::setTimeReadSB(uint16_t pTime) {
  // timeReadRF = pTime;
// }