#ifndef RING_STATE_H
#define RING_STATE_H

#include <Arduino.h>
#include "Vulcan_RF24L01.h"

class RingState
{
  public:
		RingState(SPIClass &_spiNRF, RF &_rf, byte _txAddr[], byte _rxAddr[]);
		
		void begin();
		void onConnect();
		void onDisconnect();
		void sync(int8_t _stateControl);

		void setMode(uint8_t _mode);
		uint8_t getMode();
		
	private:
		SPIClass &spiNRF;
		RF &rf;
		const char data[3] = { 'a', 'b', 'c' };  //data control OPEN, CLOSE, STOP
		byte txAddr[6];   // Sensor box address
		byte rxAddr[6];   // not use

		bool onConnectBle = false;
		uint8_t mode = 0;
		
		void sendControl(int8_t _stateControl);
};

#endif