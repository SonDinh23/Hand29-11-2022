#include "EMGSensor.h"

EMGSensor::EMGSensor(MCP3208 &_adc):
	adc(_adc){
}

void EMGSensor::begin() {
	pixels.begin();
	pixels.clear();
	pixels.setPixelColor(0, pixels.Color(250, 0, 0));  //Start Ring LED RED
	pixels.show();
	
	for (uint8_t i = 0; i < 8; i++) {
		ftHighSensor[i].inithighpass(0, 5, fs);
		ftLowSensor[i].initlowpass(0, 0.1, fs);
	}
	SPIFFS.begin(true);
	Serial.println("Setup Myo Band");
	pixels.setPixelColor(0, pixels.Color(0, 0, 250));		//Setup Ring LED BLUE
	pixels.show();
	waitRelax();
	Serial.println("Myo Band running");
	pixels.setPixelColor(0, pixels.Color(0, 255, 0));		//Ready Ring LED GREEN
	pixels.show();
}

void EMGSensor::firstReadSensor() {
	readSensor();
	filterSensor();
	setSensorDC();

	// for(uint8_t i = 0; i < CHANELS; i++) {
	// 	kalmanFt[i].setFirstValue(abs(ftHighSensor[i].highpass(data[i])));
	// }
}

void EMGSensor::readSensor() {
	static uint32_t lastTimeCycle = micros();
	sensor[0] = adc.read(MCP3208::Channel::SINGLE_0);
	sensor[1] = adc.read(MCP3208::Channel::SINGLE_1);
	sensor[2] = adc.read(MCP3208::Channel::SINGLE_2);
	// Serial.printf("%d\t%d\t%d\n", sensor[0], sensor[1], sensor[2]);
	data[3] = micros() - lastTimeCycle;							//timeCycle
	lastTimeCycle = micros();
}

void EMGSensor::filterSensor() {
	for (uint8_t i = 0; i < CHANELS; i++) {
		UTILS_LOW_PASS_FILTER(value[i], abs(ftHighSensor[i].highpass(sensor[i])), 0.05);
	}
	// Serial.printf("%.2f\t%.2f\t%.2f\n", value[0], value[1], value[2]);
}

void EMGSensor::setSensorDC() {
	for(uint8_t i = 0; i < CHANELS; i++) {
		valueDC[i] = ftLowSensor[i].lowpass(value[i]);
	}
	// Serial.printf("%.2f\t%.2f\t%.2f\n", value[0], value[1], value[2]);
}

void EMGSensor::smoothSensor() {
	for(uint8_t i = 0; i < CHANELS; i++) {
		valueOut[i] = value[i] - valueDC[i];
		// valueOut[i] = ftLowSensor1[i].lowpass(value[i]);
		UTILS_LOW_PASS_FILTER(data[i], valueOut[i], 0.05);
	}
	// Serial.printf("%.2f\t%.2f\t%.2f\n", data[0], data[1], data[2]);
}

bool EMGSensor::waitSensor(uint16_t threshold, uint16_t waitTime) {
	static uint32_t _timeReady = millis();
	readSensor();
	filterSensor();
	setSensorDC();
  // Serial.printf("%.2f\t%.2f\t%.2f\t", value[0], value[1], value[2]);
  // Serial.printf("%.2f\t%.2f\t%.2f\t", valueDC[0], valueDC[1], valueDC[2]);
  // Serial.println();
  if ((value[0] < threshold) && (value[1] < threshold) && (value[2] < threshold)) {
    if (!stateWait) {
      _timeReady = millis();
      stateWait = true;
    }
    if (millis() - _timeReady > waitTime) return true;
    else return false;
  } else {
    stateWait = false;
    return false;
  }
}

bool EMGSensor::waitRelax() {
	memset(sensor, 0, sizeof(sensor));
  memset(value, 0, sizeof(value));
  memset(valueDC, 0, sizeof(valueDC));
  memset(valueOut, 0, sizeof(valueOut));
  memset(filterKalman, 0, sizeof(filterKalman));
	for (int i = 0; i < 5000; i++) {
    firstReadSensor();
  }
	stateWait = false;
	while (!waitSensor(threshold[0], TIME_READY));
 	Serial.println("myoban ready");
 	Serial.println("myoban wait relax");
  delay(500);
	stateWait = false;
  while (!waitSensor(threshold[0], TIME_READY));
  delay(500);
	return true;
}

int8_t EMGSensor::sync(uint8_t _mode) {
	if (!_mode) return stateControl = 0;
	readSensor();
	filterSensor();
	smoothSensor();
	float output = data[0] + data[1] + data[2];
	// Serial.printf("%.2f\t%.2f\t%.2f\t", data[0], data[1], data[2]);
	// Serial.print(output);
	// Serial.printf("\n");
  if (output > THRESHOLD_MAX) {
    waitRelax();
    return stateControl = 0;
  } else {
    if (output > threshold[2]) {
      return stateControl = close;
    } else {
      if (output < threshold[1]) {
        return stateControl = open;
      } else return stateControl = 0;
    }
  }
}

uint8_t EMGSensor::readSensorStreamBLE() {
	// stateSensor:
	// 0: don't read
	// !0: onRead

	// _state return:
	// 0: don't read
	// 2: onRead

	if (stateSensor == 0) {
		return 0;
	}
	readSensor();
	filterSensor();
	smoothSensor();
	memcpy((void *)buffer, (void *)data, SIZE_ONE_PACKET_REV);
	delay(5);
	return 2;
}

uint8_t EMGSensor::readSensorSDBLE() {
	// stateSensor:
	// 0: don't read
	// 1: firstRead
	// 2: onRead
	// 3: endRead

	// _state return:
	// 0: don't read
	// 1: read raw signal
	// 2: read filter signal

	if (stateSensor == 0) {
		return 0;
	}
	if (isFirtRead) {
		// setupData();
		setupSD();
		isFirtRead = false;
		count = 0;
		Serial.println("Begin read sensor");
		timeRead = micros();
		return 1;
	}
	if (micros() - timeRead >= timeSetRead) {
		// closeData();
		closeSD();
		stateSensor = 0;
		Serial.println("End read sensor");
		Serial.printf("Total number of samples: %d\n", count);
		Serial.printf("Sampling frequency: %d\n",count/(timeSetRead/1000000));
		return 3;
	}
	// static uint32_t lasttime = micros();
	// if (micros() - lasttime >= 4000) {
		// lasttime = micros();
		readSensor();
		filterSensor();
		smoothSensor();
		// writeData();
		writeSD();
		count++;
		// Serial.println(count++);
		
	// }
	return 2;
}

uint8_t EMGSensor::readSensorBLE() {
	if (stateSensor == 0) {
		return 0;
	}
	switch (stateSensor) {
		case 1:
			return(readSensorSDBLE());
			break;
		case 2:
			return(readSensorStreamBLE());
			break;
		
		default:
			break;
	}
	return 0;
}

void EMGSensor::setTimeReadSensor(uint8_t _timeRead) {
	if (_timeRead == 0) {
		setStateSensor(0);
		Serial.println("End Read Sensor");
		return;
	}
	if (_timeRead == UINT8_MAX) {
		setStateSensor(2);
		Serial.println("Read sensor stream BLE");
	} else {
		timeSetRead = _timeRead * 1000 * 1000;
		Serial.printf("Set time read: %d s\n", _timeRead);
		setStateSensor(1);
	}
}

void EMGSensor::setStateSensor(uint8_t _stateSensor) {
	isStartSend = true;
	stateSensor = _stateSensor;
	isFirtRead = true;
}

void EMGSensor::setSampleRate(uint16_t sampleRate) {
	SWSPL_FREQ = sampleRate;
}

uint16_t EMGSensor::getVref() {
	return ADC_VREF;
}

float EMGSensor::getSampleRate() {
	return 1000000.0/timeCycle;
}

uint16_t* EMGSensor::getThreshold() {
  return threshold;
}

void EMGSensor::setThreshold(uint16_t _threshol[]) {
  memcpy((void*)threshold, (void*)_threshol, sizeof(threshold));
}

int8_t EMGSensor::getStateControl() {
	return stateControl;
}

void EMGSensor::setModeLogicControl(uint8_t _mode) {
	Serial.printf("Mode control: %d\n",_mode);
	if (!_mode) return;
	modeLogic = _mode;
	switch (_mode)
	{
		case 1:
			open = -1;
			close = 1;
			break;
		case 2: 
			open = 1;
			close = -1;
			break;
		default:
			break;
	}
}

uint8_t EMGSensor::getModeLogicControl() {
	return modeLogic;
}

// Save data sensor by pointer
// void EMGSensor::setupData() {
// 	dataPoint = (float *)malloc(sizeof(float) * 1000*(CHANELS + 1));
// }

// void EMGSensor::writeData() {
// 	memcpy(&dataPoint[count * (CHANELS + 1)], (void*)data, sizeof(data));
// }

// void EMGSensor::closeData() {
// 	lengthData = --count;
// 	for (uint16_t i = 0; i < lengthData; i++) {
// 		for (uint8_t j = 0; j < 9; j++) {
// 			Serial.printf("%.2f \t", dataPoint[i * 9 + j]);
// 		}
// 		Serial.println();
// 	}
// }

// void EMGSensor::openData() {
// 	count = 0;
// }

// int EMGSensor::readData() {
// 	if (isStartSend) {
// 		openData();
// 		isStartSend = false;
// 		return -2;
// 	}
// 	if (count < lengthData) {
// 		uint16_t buff = lengthData - count;
// 		buff = (buff < NUMBER_PACKET_REV) ? buff:NUMBER_PACKET_REV;
// 		memcpy((void*)rev, &dataPoint[count], buff * SIZE_ONE_PACKET_REV);
// 		count += buff;
// 		return buff;
// 	}
// 	else {
// 		clearData();
// 		isStartSend = true;
// 		return -1;
// 	}
// }

// void EMGSensor::clearData() {
// 	free(dataPoint);
// }

// Save data sensor by SD
void EMGSensor::setupSD() {
	file.close();
	SPIFFS.remove(FILE_DATA);
	file = SPIFFS.open(FILE_DATA, FILE_APPEND);
}

void EMGSensor::writeSD() {
	memcpy((void *)buffer, (void *)data, SIZE_ONE_PACKET_REV);
	file.write(buffer, SIZE_ONE_PACKET_REV);
}

void EMGSensor::closeSD() {
	Serial.printf("Size data: %d byte\n", file.size());
	file.close();
	
}

void EMGSensor::openSD() {
	file.close();
	file = SPIFFS.open(FILE_DATA);
}

int EMGSensor::readSD() {
	if (isStartSend) {
		openSD();
		isStartSend = false;
		count = 0;
		return -2;
	}
	if (file.available()) {
    file.read(rev, LENGTH_DATA_REV);
		for (uint8_t i = 0; i < NUMBER_PACKET_REV; i++) {
			float buff[CHANELS + 1];
			memcpy((void *)buff, (void *)(rev + SIZE_ONE_PACKET_REV * i), SIZE_ONE_PACKET_REV);
			for (uint8_t j = 0; j < CHANELS + 1; j++) {
				Serial.printf("%.2f\t", buff[j]);
			}
			Serial.print("\n");
		}
		count++;
		return count;
	}
	else {
		closeSD();
		isStartSend = true;
		return -1;
	}
}
