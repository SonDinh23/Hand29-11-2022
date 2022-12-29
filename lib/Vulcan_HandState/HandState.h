#ifndef HAND_STATE_H
#define HAND_STATE_H

#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Vulcan_Utils.h>
#include <LinearRegression.h>
#include <Preferences.h>

#define LED_PIN   		33
#define LED_COUNT 		3
#define SERVO_PIN 		16
#define FEEDBACK_PIN  4

#define IDLE_CONSTANT				1500		//time to enter resting state
#define CURRENT_THRESHOLD 	250			//threshold current to exit relax mode

#define BATTERY_MAX   8.2						//battery parameter setting 
#define BATTERY_MIN   7.4

#define BUFF_CLOSE_ANGLE	3
#define BUFF_OPEN_ANGLE		5

#define DELTA_ANGLE_OBSTACLE_MAX		20
#define DELTA_ANGLE_OBSTACLE				10
#define DELTA_ANGLE_RELAX						0

#define CONST_FILTER			0.01

class HandState
{
	public:
		HandState(bool onLRHand, uint8_t pminAngle, uint8_t pmaxAngle); 	//input code for left or right hand: LEFT HAND: true;  RIGHT HAND: false
		void begin();
		void update();																										//update hand
		void updateSensor(char pchar);																		//update Sensor box

		void testAngle();																									//Check hand angle for minAngle and maxAngle
		void testPower();																									//Check power of the hand
		void findLinearFB(uint16_t _minAngle, uint16_t _maxAngle);				//Find Linear Regression Feedback servo

		void setStartAngle(uint8_t pstartAngle);
		void setMinAngle(uint8_t pminAngle);
		void setMaxAngle(uint8_t pmaxAngle);
		void setSpeed(uint8_t pspeed);
		void setRGB(uint8_t rgbArry[]);

		uint8_t getAngle();
		uint8_t getMinAngle();
		uint8_t getMaxAngle();
		uint8_t getBattery();

		void onConnect();
		void onDisconnect();

		uint8_t* getRGB(); 
		
	private:
		Preferences pref;
		Servo myServo;	
		Adafruit_INA219 ina219;
		ESP32PWM pwm;
		LinearRegression lr = LinearRegression();
		
		Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

		bool typeHand;												// true: left hand | false: right hand
		uint8_t* pCloseAngle;
		uint8_t* pOpenAngle;

		int8_t angleOpen = -1;								//gradually go to large angle
		int8_t angleClose = 1;								//gradually go to small angle

		uint8_t openSpeed = 1;             		// multiply this per step
		uint8_t closeSpeed = 1;             	// multiply this per step 
		uint8_t startAngle = 90;

		uint8_t angle = startAngle;         	// when turn on, hand goes to this angle
		uint8_t minAngle = 80;            		// servo lower limit
		uint8_t maxAngle = 100;            		// servo upper limit
		
		float valueFB;
		double linearA;												// Linear Regression Y = Ax +B
		double linearB;

		bool isFirstReadPWM = true;
		bool isFirstReadAngle = true;
		uint16_t pwmValue;
		uint16_t pwmValueMin;
		uint16_t pwmValueMax;

		float current = 0;
		float loadvoltage = 0;

		uint8_t batteryPercent = 69;

		bool go = false ;
		bool onDetach = false;
	
		bool onRelax = true;
		int angleFactor = 0 ;

		unsigned long idle_time = 0; 				//time between constantly similar value before the servo gets detached	

		uint8_t RGB[3] = {0,0,0};					

		bool isConnect = false;							//state BLE
		
		bool isObstacle(int8_t _deltaAngle);

		void readBattery();
		void setServo();
		void detachServo();

		uint8_t convertFBAngle(float _FBvalue);
};

#endif