#ifndef EMG_SENSOR_H
#define EMG_SENSOR_H

#include <SPI.h>
#include <Mcp320x.h>
#include "SPIFFS.h"

#include "KickFiltersRT.h"
#include "Vulcan_Utils.h"
#include <Adafruit_NeoPixel.h>

#define PIN_RGB        32

#define THRESHOLD_SETUP 40
#define THRESHOLD_DC    40
#define THRESHOLD_LOW   10
#define THRESHOLD_HIGH  20
#define THRESHOLD_MAX   4000

#define TIME_READY 3000

#define ADC_VREF    					3300    				// 3.3V Vref
#define CHANELS     					3      					// samples

#define SIZE_WRITE						CHANELS * 4

#define SIZE_ONE_PACKET_REV		sizeof(float) * (CHANELS + 1) // array CHANELS + 1 float (CHANELS sensor + 1 time read)
#define NUMBER_PACKET_REV			10							//10 packet/send
#define LENGTH_DATA_REV				NUMBER_PACKET_REV * SIZE_ONE_PACKET_REV // <600 byte

#define FILE_DATA							"/data.txt"

class EMGSensor
{
	public:
		EMGSensor(MCP3208 &_adc);
		void begin();
		int8_t sync(uint8_t _mode);
		uint8_t readSensorStreamBLE();
		uint8_t readSensorSDBLE();
		uint8_t readSensorBLE();
		
		void setStateSensor(uint8_t _stateSensor);
		void setTimeReadSensor(uint8_t _timeRead);

		uint16_t getVref();
		float getSampleRate();
		void setSampleRate(uint16_t sampleRate);

		uint16_t* getThreshold();
		void setThreshold(uint16_t _threshol[]);

		int8_t getStateControl();
		void setModeLogicControl(uint8_t _mode);
		uint8_t getModeLogicControl();

		// int readData();
		int readSD();

		uint8_t buffer[SIZE_ONE_PACKET_REV];
		uint8_t rev[LENGTH_DATA_REV]; 			
		float data[CHANELS+1] = {0};  //8 chanels + timeCycle
		float dataFilter[CHANELS] = {0};

	private:
		Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
		uint16_t SWSPL_FREQ;
		
		MCP3208 &adc;
		KickFiltersRT<int16_t> ftNotch;
		KickFiltersRT<float> ftHighSensor[8];
		KickFiltersRT<float> ftLowSensor[8];
		KickFiltersRT<float> ftLowSensor1[8];
		const float fs = 230;

		//Variable of logic Control
		uint8_t modeLogic = 1; 
		int8_t open = -1;
		int8_t close = 1;

		int8_t stateControl = 0;

		// Variable of time read
		uint8_t stateSensor = 0;

		uint32_t timeCycle;
		uint32_t timeSetRead;
		uint32_t timeRead = micros();
		uint8_t chanels = 3; 							//number of channels

		//Dynamic memory pointer
		float *dataPoint;

		//Variable to store sensor signal using flash memory
		File file;
		bool isFirtRead = true;
		bool isStartSend = true;

		uint32_t count = 0;
		uint32_t lengthData = 0;

		//Threshold for control processing
		uint16_t threshold[3]= {THRESHOLD_SETUP, THRESHOLD_LOW, THRESHOLD_HIGH};

		//Variable for sensor signal processing
		uint16_t sensor[CHANELS];
		float value[CHANELS];
		float valueDC[CHANELS];
		float valueOut[CHANELS];
		float filterKalman[CHANELS];
		float output;
		float outputDC[3];

		bool stateWait = false;

		//Function for sensor signal processing
		void firstReadSensor();
		void readSensor();
		void filterSensor();
		void setSensorDC();
		void smoothSensor();
		bool waitSensor(uint16_t threshold, uint16_t waitTime);
		bool waitRelax();

		//Function for reading and writing sensor signals using dynamic memory
		// void setupData();
		// void writeData();
		// void closeData();
		// void openData();
		// void clearData();
		
		//Function for reading and writing sensor signals using flash memory
		void setupSD();
		void writeSD();
		void openSD();
		void closeSD();

};

#endif