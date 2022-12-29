#include "RingState.h"

RingState::RingState(SPIClass &_spiNRF, RF &_rf, byte _txAddr[], byte _rxAddr[]):
  spiNRF(_spiNRF),
  rf(_rf) {
  memcpy((void*)txAddr, (void*)_txAddr, sizeof(txAddr));
  memcpy((void*)rxAddr, (void*)_rxAddr, sizeof(rxAddr));
}

void RingState::begin() {
  rf.begin(txAddr, rxAddr, &spiNRF, false);
}

void RingState::sync(int8_t _stateControl) {
  static int8_t lastState = 0;
  static bool isFirstSaveStateControl = true;
  static uint32_t lastTime = millis();
  isFirstSaveStateControl = _stateControl == lastState ? false:true;
  lastState = _stateControl;
  if (isFirstSaveStateControl) {
    lastTime = millis();
  }
  if (millis() - lastTime > 3000) {
    if (millis() - lastTime > 6000)
    {
      return;
    }
    _stateControl = 0;
  }
  sendControl(_stateControl);
}

void RingState::sendControl(int8_t _stateControl) {
  switch (_stateControl) {
    case 1:
      {
        rf.sendData(&data[1], 1);
        break;
      }
    case -1:
      {
        rf.sendData(&data[0], 1);
        break;
      }
    default:
      {
        rf.sendData(&data[2], 1);
        break;
      }
  }
}

void RingState::onConnect() {
}

void RingState::onDisconnect() {

}

uint8_t RingState::getMode() {
  return mode;
}

void RingState::setMode(uint8_t _mode) {
  mode = _mode;
}
