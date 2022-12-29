#include "HandState.h"

HandState::HandState(bool onLRHand, uint8_t pminAngle, uint8_t pmaxAngle):
	minAngle(pminAngle),
	maxAngle(pmaxAngle),
	typeHand(onLRHand) {
	if (typeHand) {
		angleOpen = 1;
		angleClose = -1;
	}
}

void HandState::begin() {
	if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
  }
	pref.begin("hand-info", false);

	ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
	myServo.attach(SERVO_PIN);
  delay(500);

	minAngle = pref.getUShort("minAngle", minAngle);
	maxAngle = pref.getUShort("maxAngle", maxAngle);

	Serial.printf("MinAngle: %d\n", minAngle);
	Serial.printf("MaxAngle: %d\n", maxAngle);

	pCloseAngle = typeHand? &minAngle : &maxAngle;		
	*pCloseAngle += angleClose * BUFF_CLOSE_ANGLE;			//increase the closing angle BUFF_CLOSE_ANGLE unit
	// Serial.printf("Angle close: %d\n", *pCloseAngle);

	pOpenAngle = typeHand? &maxAngle : &minAngle;		
	*pOpenAngle += angleOpen * BUFF_OPEN_ANGLE;				//increase the opening angle BUFF_OPEN_ANGLE unit
	// Serial.printf("Angle open: %d\n", *pOpenAngle);
	startAngle = (minAngle + maxAngle) / 2;
	// Serial.printf("Angle start: %d\n", startAngle);
	
	linearA = pref.getDouble("linearA", 0);
	linearB = pref.getDouble("linearB", 0);

	UTILS_LOW_PASS_FILTER(valueFB, analogRead(FEEDBACK_PIN), 0.01);

	// Linear Regression Feedback Servo
	Serial.printf("Linear Regression: Y = %.4fx + %.4f\n",linearA, linearB);
	
	if (linearA == 0) {
		Serial.println("////////////////////////////");
		Serial.println("// WARNING! MUST RUN TEST //");
		Serial.println("////////////////////////////");
		myServo.write(90);
		delay(500);
	}
	else {
		valueFB = analogRead(FEEDBACK_PIN);
		for (uint16_t i = 0; i < 3000; i++) {
			UTILS_LOW_PASS_FILTER(valueFB, analogRead(FEEDBACK_PIN), CONST_FILTER);
		}
		angle = constrain(convertFBAngle(valueFB), minAngle, maxAngle);
		myServo.write(angle);
	}

	pwmValueMin = map(minAngle, 0, 180, DEFAULT_uS_LOW, DEFAULT_uS_HIGH);
	pwmValueMax = map(maxAngle, 0, 180, DEFAULT_uS_LOW, DEFAULT_uS_HIGH);
	pwmValue = myServo.readMicroseconds();

	pixels.begin();
	pixels.setPixelColor(1, pixels.Color(RGB[0], RGB[1], RGB[2]));
}

void HandState::updateSensor(char pchar) {
	switch (pchar) {
		case 'a':
			{
				go = true;
				angleFactor = angleOpen * openSpeed;
				pixels.setPixelColor(0, pixels.Color(0, 0, 250));
				pixels.show();
				break;
			}
		case 'b':
			{
				go = true;
				angleFactor = angleClose * closeSpeed;
				pixels.setPixelColor(0, pixels.Color(0, 250, 0));
				pixels.show();
				break;
			}
		case 'c':
			{
				go = false;
				angleFactor = 0;
				pixels.setPixelColor(0, pixels.Color(0, 0, 0));
				pixels.show();
				break;
			}
		default:{
			go = false;
			angleFactor = 0;
			break;
		}
	}
}

void HandState::update() {
	// readBattery();
	setServo();
}

void HandState::readBattery() {	
  float current_mA = ina219.getCurrent_mA();
	UTILS_LOW_PASS_FILTER(current, current_mA, 0.005);
	UTILS_LOW_PASS_FILTER(loadvoltage, ina219.getBusVoltage_V() + (ina219.getShuntVoltage_mV() / 1000), 0.001);

	if (current < 170 && onRelax) {
		batteryPercent = (loadvoltage + 0.7 - BATTERY_MIN) * 100 / (BATTERY_MAX - BATTERY_MIN);
  	batteryPercent = constrain(batteryPercent, 0, 100);
  }

 	// Serial.print(loadvoltage); Serial.print("\t");
	// Serial.print(batteryPercent); Serial.print("\t");
  // Serial.print(current_mA); Serial.print("\t");
  // Serial.print(current); Serial.print("\t");
	// Serial.print(servoFBRaw ); Serial.print("\t");
	// Serial.print(servoFB); Serial.print("\t");
}

void HandState::setServo() {
	static uint32_t lastTimePWM = millis();

	UTILS_LOW_PASS_FILTER(valueFB, analogRead(FEEDBACK_PIN), CONST_FILTER);
	uint8_t angleConvert = convertFBAngle(valueFB);
	angle = myServo.read();
	int8_t deltaAngle = angle - angleConvert;
	bool obstacleFlag = isObstacle(abs(deltaAngle));

	if (go) {
		if ((!obstacleFlag) || (constrain(deltaAngle, -1, 1) != angleFactor))
		{
			pwmValue = constrain(pwmValue + angleFactor, pwmValueMin, pwmValueMax);
		}
		idle_time = millis();
	}

	if (millis() - idle_time > IDLE_CONSTANT) {
		onDetach = true;
	}
	else {
		onDetach = false;
	}

	if (onDetach) {
		if ((abs(deltaAngle) > DELTA_ANGLE_RELAX) && (!onRelax)) {
			if (millis() - lastTimePWM > 50) {
				pwmValue = pwmValue - (1 * constrain(deltaAngle, -1, 1));
				pwmValue = constrain(pwmValue, pwmValueMin, pwmValueMax);
				myServo.writeMicroseconds(pwmValue);
				lastTimePWM = millis();
			}
		}
		else {
			onRelax = true;
		}
	}
	else {
		myServo.writeMicroseconds(pwmValue);
		onRelax = false;
	}

	// Serial.print(onRelax); Serial.print("\t");
	// Serial.print(onDetach); Serial.print("\t");
	// Serial.print(pwmValue); Serial.print("\t");
	// Serial.print(vectorCurrent); Serial.print("\t");
	// Serial.print(obstacleFlag); Serial.print("\t");
	// Serial.print(firstobstacleFlag); Serial.print("\t");
	// Serial.print(angle); Serial.print("\t");
	// Serial.print(angleConvert); Serial.print("\t");
	// Serial.print(deltaAngle); Serial.print("\t");

	// Serial.println();
}

bool HandState::isObstacle(int8_t _deltaAngle) {
	static uint32_t lastTimeObstacleFlag;
	static bool firstobstacleFlag = true;
	bool obstacleFlag = false;
	if (abs(_deltaAngle) > DELTA_ANGLE_OBSTACLE_MAX) {
		obstacleFlag = true;
		return obstacleFlag;
	}
	if (abs(_deltaAngle) > DELTA_ANGLE_OBSTACLE) {
		if (firstobstacleFlag) {
			lastTimeObstacleFlag = millis();
			firstobstacleFlag = false;
			
		}
		if (millis() - lastTimeObstacleFlag > 2000) {
			obstacleFlag = true;
		} 
	}
	else {
		firstobstacleFlag = true;
		obstacleFlag = false;
	}
	return obstacleFlag;
}

uint8_t HandState::convertFBAngle(float _FBvalue) {
	return (uint8_t)(linearA * _FBvalue + linearB);
}

void HandState::setRGB(uint8_t rgbArry[]) {
	for (uint8_t i = 0; i < 3; i++) {
		RGB[i] = rgbArry[i];
	}
	pixels.setPixelColor(1, pixels.Color(RGB[0], RGB[1], RGB[2]));
	pixels.show();
	Serial.printf("RGB: %d:%d:%d\n", RGB[0], RGB[1], RGB[2]);
}

void HandState::setStartAngle(uint8_t pstartAngle) {
	startAngle = pstartAngle;
	Serial.printf("startAngle: %d\n",pstartAngle);
}

void HandState::setMinAngle(uint8_t pminAngle) {
	minAngle = pminAngle;
	pwmValueMin = map(minAngle, 0, 180, DEFAULT_uS_LOW, DEFAULT_uS_HIGH);
	Serial.printf("minAngle: %d\n",pminAngle);
}

void HandState::setMaxAngle(uint8_t pmaxAngle) {
	maxAngle = pmaxAngle;
	pwmValueMax = map(maxAngle, 0, 180, DEFAULT_uS_LOW, DEFAULT_uS_HIGH);
	Serial.printf("maxAngle: %d\n",pmaxAngle);
}

void HandState::setSpeed(uint8_t pspeed) {
	openSpeed = pspeed; 
	closeSpeed = pspeed;
	Serial.printf("SpeedAngle: %d\n",pspeed);
}

uint8_t* HandState::getRGB() {
	return RGB;
}

uint8_t HandState::getAngle() {
	return angle;
}

uint8_t HandState::getMinAngle() {
	return minAngle;
}

uint8_t HandState::getMaxAngle() {
	return maxAngle;
}

uint8_t HandState::getBattery() { 
	return batteryPercent;
}

void HandState::onConnect() {
	isConnect = true;
	pixels.setPixelColor(2, pixels.Color(0, 0, 255));
	pixels.show();
}

void HandState::onDisconnect() {
	isConnect = false;
	pixels.setPixelColor(2, pixels.Color(255, 0, 0));
	pixels.show();
}

void HandState::testAngle() {
	uint8_t angleLimitTest[2] = {0,0};
	uint16_t currentLimitTest[2] = {500, 200};
	for (uint8_t i = 0; i < 2; i++) {
		myServo.write(startAngle);
		delay(1000);
		pwmValue = myServo.readMicroseconds();
		bool stateLimmit = false;
		current = 0;
		readBattery();
		Serial.println(pwmValue);
		uint32_t lastTimeTestAngle = millis();
		int8_t vectorTestAngle = 0;
		if (i == 0) vectorTestAngle = angleClose;
		else vectorTestAngle = angleOpen;
		while (!stateLimmit) {
			if (millis() - lastTimeTestAngle > 200) {
				pwmValue += vectorTestAngle;
				myServo.writeMicroseconds(pwmValue);
				lastTimeTestAngle = millis();
			}
			readBattery();
			Serial.println(pwmValue);
			if (current > currentLimitTest[i]) stateLimmit = true;
		}
		delay(1000);
		angleLimitTest[i] = myServo.read();
	}

	if (angleLimitTest[0] > angleLimitTest[1]) {
		uint8_t t = angleLimitTest[0];
		angleLimitTest[0] =  angleLimitTest[1];
		angleLimitTest[1] = t;
	}

	minAngle = angleLimitTest[0];
	maxAngle = angleLimitTest[1];

	myServo.write((minAngle + maxAngle) / 2); 
	Serial.printf("Limmed Angle:  %d  %d\nStart Angle: %d\n", minAngle, maxAngle, (maxAngle + minAngle) / 2);
	delay(2000);
	Serial.println("Begin find linear FB");
	delay(2000);
	findLinearFB(minAngle, maxAngle);
	Serial.println("End find linear FB");

	pref.putUShort("minAngle", minAngle);
	pref.putUShort("maxAngle", maxAngle);

	*pCloseAngle += angleClose * BUFF_CLOSE_ANGLE;
	*pOpenAngle += angleClose * BUFF_OPEN_ANGLE;	

	delay(1000);
}

void HandState::findLinearFB(uint16_t _minAngle, uint16_t _maxAngle) {
	float valueTrain[_maxAngle - _minAngle + 1][2];
	uint16_t rawFB;
	uint8_t count = 0;
	double values[3];
	double valuesTest[2][2];

	// first learn
	myServo.write(_minAngle + 3);
	delay(500);
	myServo.write(_minAngle + 2);
	delay(500);
	myServo.write(_minAngle + 1);
	delay(500);
	valueFB = analogRead(FEEDBACK_PIN);
	for (angle = _minAngle; angle <= _maxAngle; angle++) {
		myServo.write(angle);
		for (uint16_t i = 0; i < 5000; i++) {
			UTILS_LOW_PASS_FILTER(valueFB, analogRead(FEEDBACK_PIN), CONST_FILTER);
			// Serial.print(angle);
			// Serial.print("\t");
			// Serial.print(rawFB);
			// Serial.print("\t");
			// Serial.println(valueFB);
		}
		lr.learn(valueFB, angle);
		valueTrain[count][0] = valueFB;
		valueTrain[count][1] = angle;
		count++;
		Serial.printf("%.2f - %.2f\n",valueFB, angle);
		delay(500);
	}

	for (uint16_t j = 0; j < _maxAngle - _minAngle + 1; j++) {
      Serial.print(valueTrain[j][0]);
      Serial.print("\t");
      Serial.println(valueTrain[j][1]);
    }
    Serial.print("Values: ");
    lr.getValues(values);
		valuesTest[0][0] = values[0];
		valuesTest[0][1] = values[1];

	// second learn
	count = 0;
	lr.reset();
	myServo.write(_maxAngle + 2);
	delay(100);
	valueFB = analogRead(FEEDBACK_PIN);
	for (angle = _maxAngle; angle >= _minAngle; angle--) {
		myServo.write(angle);
		for (uint16_t i = 0; i < 5000; i++) {
			UTILS_LOW_PASS_FILTER(valueFB, analogRead(FEEDBACK_PIN), 0.01);
			// Serial.print(angle);
			// Serial.print("\t");
			// Serial.print(rawFB);
			// Serial.print("\t");
			// Serial.println(valueFB);
		}
		lr.learn(valueFB, angle);
		valueTrain[count][0] = valueFB;
		valueTrain[count][1] = angle;
		count++;
		Serial.printf("%.2f - %.2f\n",valueFB, angle);
		delay(500);
	}

	for (uint16_t j = 0; j < _maxAngle - _minAngle + 1; j++) {
      Serial.print(valueTrain[j][0]);
      Serial.print("\t");
      Serial.println(valueTrain[j][1]);
    }
    Serial.print("Values: ");
    lr.getValues(values);
		valuesTest[1][0] = values[0];
		valuesTest[1][1] = values[1];

		// Save value
		linearA = (valuesTest[0][0] + valuesTest[1][0]) / 2;
		linearB = (valuesTest[0][1] + valuesTest[1][1]) / 2;
		
    Serial.printf("Y = %.4f x + %.4f\n", linearA, linearB);
		pref.putDouble("linearA", linearA);
		pref.putDouble("linearB", linearB);

		startAngle = (_minAngle + _maxAngle) / 2;
		myServo.write(startAngle);
    delay(1000);   
}

void HandState::testPower() {
	angle = startAngle;
	myServo.write(angle);
	delay(500);
	myServo.write(angle + 5);
	delay(500);
	myServo.write(angle + 3);
	delay(500);
	pwmValue = myServo.readMicroseconds();
	uint32_t lastTimeTestPower = millis();
	while(millis() - lastTimeTestPower < 5000) {
		readBattery(); 
		Serial.println();
	}
	float staticCurrent = current;
	float staticVolt = loadvoltage;
	delay(2000);

	uint16_t maxPWMClose = angleClose == 1 ? pwmValueMax:pwmValueMin;
	pwmValue = myServo.readMicroseconds();
	uint16_t differenceAngle = (maxPWMClose - pwmValue) * angleClose;  //
	lastTimeTestPower = millis();
	float maxCurrent = 0;
	float minVolt = loadvoltage;
	while(differenceAngle > 0) {
		if (millis() - lastTimeTestPower > 100) {
			pwmValue += angleClose;
			myServo.writeMicroseconds(pwmValue);
			differenceAngle = (maxPWMClose - pwmValue) * angleClose;
			lastTimeTestPower = millis();
		}
		readBattery();
		Serial.println(pwmValue);
		if (maxCurrent < current) maxCurrent = current;
		if (minVolt > loadvoltage) minVolt = loadvoltage;
	}
	delay(1000);
	myServo.write(startAngle);

	Serial.printf("Static Volt: %.1f\n", staticVolt);
	Serial.printf("Static Current: %.0f\n", staticCurrent);
	Serial.printf("\nMinVolt: %.1f\n", minVolt);
	Serial.printf("MaxCurrent: %.0f\n", maxCurrent);
}	