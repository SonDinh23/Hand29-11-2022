#ifndef BLE_RING_H
#define BLE_RING_H

#include "BLEServiceManager.h"
#include <Preferences.h>
#include "SPIFFS.h"
#include <FS.h>
#include <Update.h>
#include "esp_ota_ops.h"
#include "esp_task_wdt.h"
#include "RingState.h"

#define OTA_UUID						"149f93ef-7481-4536-8f75-50b5b55ab058"
#define NAME_CHAR_UUID			"514bd5a1-1ef9-49c8-b569-127a84896d25"
#define MODE_UUID						"4e1dd354-3a27-466f-bd2f-4a4b870a132a"

#define UPDATE_START_MSG    "START_OTA"
#define UPDATE_END_MSG      "END_OTA"

#define VERSION             "0.2"
#define INIT_NAME           "Vulcan MyoBand"

#define MAX_VERSION_LENGTH  10
#define MAX_NAME_LENGTH     30

#define FILE_DATA						"/data.txt"
#define	FILE_FW							"/update.bin"

class BLERing:public BLEServiceManager<3>
{
  public:
		BLERing(BLEService* service, RingState& pRingState);
		virtual void begin();

		const char* getName();

  private:
		Preferences pref;
		RingState& ringState;
		char version[MAX_VERSION_LENGTH + 1];
		char name[MAX_NAME_LENGTH + 1];

		bool updateInProgress = false;

		bool onOTA = false;
		uint32_t count = 0;
		File file;
		esp_ota_handle_t otaHandler = 0;

		void onReadOTA(BLECharacteristic* pChar);
		void onWriteOTA(BLECharacteristic* pChar);
		void onNotifyOTA(uint32_t stateOTA);

		void onReadName(BLECharacteristic* pChar);
		void onWriteName(BLECharacteristic* pChar);

		void onReadMode(BLECharacteristic* pChar);
		void onWriteMode(BLECharacteristic* pChar);

		void getStringPref(Preferences& p, const char* key, char* data, size_t maxLen, const char* defaultVal);

		void performUpdate(Stream &updateSource, size_t updateSize);
		void updateFromFS(fs::FS &fs);
		static void rebootEspWithReason(String reason);
};

#endif