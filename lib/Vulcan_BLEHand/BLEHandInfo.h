#ifndef BLE_HAND_H
#define BLE_HAND_H

#include "BLEServiceManager.h"
#include <Preferences.h>
#include "SPIFFS.h"
#include <FS.h>
#include <Update.h>
#include "HandState.h"

#define FIRMWARE_CHAR_UUID	"76095ead-54c4-4883-88a6-8297ba18211a"
#define NAME_CHAR_UUID			"f2c513b7-6b51-4363-b6aa-1ef8bd08c56a"
#define VERSION_UUID				"344aac91-da0e-4bd8-b4e6-913c7e8e63dc"
#define ANGLE_UUID					"e962a2e2-7645-4e6d-8fdc-b33b635c4450"
#define BATTERY_UUID				"fd932c13-8aab-484d-b8b3-2e56591a7f9d"
#define LED_UUID						"0ec9ec12-5d9f-4c9e-80d9-c3e29be09f1f"

#define UPDATE_START_MSG    "START_OTA"
#define UPDATE_END_MSG      "END_OTA"

#define VERSION             "9.7"
#define INIT_NAME           "Vulcan Hand"

#define MAX_VERSION_LENGTH  10
#define MAX_NAME_LENGTH     30

class BLEHandInfo:public BLEServiceManager<6>
{
  public:

		BLEHandInfo(BLEService* service, HandState& phandState);
		virtual void begin();

		const char* getName();

  private:
		Preferences pref;
		HandState& handState;
		char version[MAX_VERSION_LENGTH + 1];
		char name[MAX_NAME_LENGTH + 1];

		uint8_t RGB[3];
		bool onOTA = false;
		uint32_t count = 0;

		File file;

		void onWriteFirmware(BLECharacteristic* pChar);
		void onNotifyFirmware(uint32_t stateOTA);

		void onReadName(BLECharacteristic* pChar);
		void onWriteName(BLECharacteristic* pChar);

		void onReadVersion(BLECharacteristic* pChar);

		void onReadAngle(BLECharacteristic* pChar);
		void onWriteAngle(BLECharacteristic* pChar);

		void onReadBattery(BLECharacteristic* pChar);

		void onWriteLed(BLECharacteristic* pChar);

		void getStringPref(Preferences& p, const char* key, char* data, size_t maxLen, const char* defaultVal);
		
		void performUpdate(Stream &updateSource, size_t updateSize);
		void updateFromFS(fs::FS &fs);
		static void rebootEspWithReason(String reason);
};

#endif