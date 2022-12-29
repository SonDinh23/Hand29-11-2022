#include "BLEHandInfo.h"

void  BLEHandInfo::getStringPref(Preferences& pref, 
  const char* key, char* data, size_t maxLen, const char* defaultVal) {
  
  size_t len = pref.getBytes(key, data, maxLen);
  if (len == 0) {
    len = strlen(defaultVal);
    memcpy(data, defaultVal, len);
  }
  data[len] = '\0';
}

static esp_ble_adv_data_t ADVERTISING_CONFIG = {
  .set_scan_rsp = false,
  .include_name = true,
  .include_txpower = true,
  .min_interval = 0x0006,
  .max_interval = 0x0010,
  .appearance = 0x00,
  .manufacturer_len = 0,
  .p_manufacturer_data = nullptr,
  .service_data_len = 0,
  .p_service_data = nullptr,
  .service_uuid_len = 0,
  .p_service_uuid = nullptr,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

BLEHandInfo::BLEHandInfo(BLEService* service, HandState& phandState): 
    BLEServiceManager(service), handState(phandState) {
  addReadWriteNotify(FIRMWARE_CHAR_UUID,
    doNothing,
    authed([this](BLECharacteristic* p) {onWriteFirmware(p);}));
  
  addReadWrite(NAME_CHAR_UUID, 
    authed([this](BLECharacteristic* p) {onReadName(p);}), 
    authed([this](BLECharacteristic* p) {onWriteName(p);}));

  addReadWrite(VERSION_UUID, 
    authed([this](BLECharacteristic* p) {onReadVersion(p);}), 
    doNothing);
    
  addReadWrite(ANGLE_UUID, 
    authed([this](BLECharacteristic* p) {onReadAngle(p);}), 
    authed([this](BLECharacteristic* p) {onWriteAngle(p);}));

  addReadWrite(BATTERY_UUID, 
    authed([this](BLECharacteristic* p) {onReadBattery(p);}),
    authed([this](BLECharacteristic* p) {}));
  
	addReadWrite(LED_UUID, 
    doNothing,
    authed([this](BLECharacteristic* p) {onWriteLed(p);}));
  // Increasing WDT timeout to 10s to fix esp_ota_begin issue
  // https://github.com/espressif/esp-idf/issues/1479
  // https://github.com/espressif/esp-idf/issues/578
  //esp_task_wdt_init(10, false);
}

void BLEHandInfo::begin() {
  BLEServiceManager::begin();
  SPIFFS.begin(true);
  pref.begin("ble-info", false);
  getStringPref(pref, "name", name, MAX_NAME_LENGTH, INIT_NAME);
  Serial.printf("Name BLE Hand: %s\n",name);
  if (pref.getBytes("RGB", RGB, 3)) {
    handState.setRGB(RGB);
  }
  handState.setSpeed(pref.getUShort("speedAngle", 1));
}

const char* BLEHandInfo::getName() {
  return name;
}

void BLEHandInfo::onWriteFirmware(BLECharacteristic* pChar) {
  std::string rxData = pChar->getValue();
  uint16_t len = rxData.length();
  uint8_t* data = pChar->getData();
  if (rxData == UPDATE_START_MSG) {
    onOTA = true;
    file = SPIFFS.open("/update.bin", FILE_APPEND);
    count = 0;
    Serial.println("Begin update OTA");
    Serial.println(count);
    onNotifyFirmware(count);
  }
  else if (rxData == UPDATE_END_MSG) {
    Serial.println(++count);
    onNotifyFirmware(count);
    onOTA = false;
    file.close();
    Serial.println("End update OTA");
    file = SPIFFS.open("/update.bin");
    
    while(file.available()){
      Serial.print(file.read());
    }
    updateFromFS(SPIFFS);
    //SPIFFS.remove("/update.bin");
  }
  else if (onOTA) {
    file.write(data, len);
    Serial.println(++count);
    onNotifyFirmware(count);
  }
}

void BLEHandInfo::onNotifyFirmware(uint32_t stateOTA) {
  BLECharacteristic* pCharacteristic = service->getCharacteristic(FIRMWARE_CHAR_UUID);
  pCharacteristic->setValue(stateOTA);
  pCharacteristic->notify();
}

void BLEHandInfo::onWriteName(BLECharacteristic* pChar) {
  const char* s = pChar->getValue().c_str();
  if (strlen(s) > 0) {
    memcpy(name, s, strlen(s) + 1);
    pref.putBytes("name", s, strlen(s));
    
    // Update device name, name is change after esp reset 
    esp_ble_gap_set_device_name(s);
    // Advertise the changes to clients
    esp_ble_gap_config_adv_data(&ADVERTISING_CONFIG);
    Serial.printf("name: %s\n", name);
  }
}

void BLEHandInfo::onReadName(BLECharacteristic* pChar) {
  pChar->setValue(name);
}

void BLEHandInfo::onReadVersion(BLECharacteristic* pChar) {
	pChar->setValue(VERSION);
}

void BLEHandInfo::onWriteAngle(BLECharacteristic* pChar) {
  const char* s = pChar->getValue().c_str();
  if (strlen(s) > 0) {
    uint8_t speedAngle = atoi(s);
    if (speedAngle != 0 ) {
      pref.putUShort("speedAngle", speedAngle);
      handState.setSpeed(speedAngle);
    }
  }
}

void BLEHandInfo::onReadAngle(BLECharacteristic* pChar) {
  uint8_t frame[3];
  frame[0] = handState.getAngle();
  frame[1] = handState.getMinAngle();
  frame[2] = handState.getMaxAngle();
	pChar->setValue(frame, 3);
}

void BLEHandInfo::onReadBattery(BLECharacteristic* pChar) {
  uint8_t battery = handState.getBattery();
  pChar->setValue(&battery, 1);
}

void BLEHandInfo::onWriteLed(BLECharacteristic* pChar) {
  const char* s = pChar->getValue().c_str();
  if (strlen(s) > 8) {
		for (uint8_t i = 0; i < 3; i++) {
			char p[3];
			strncpy(p, s + i*3, 3);
      RGB[i] = atoi(p);
		}
    handState.setRGB(RGB);
    pref.putBytes("RGB", RGB, 3);
  }
}

void BLEHandInfo::performUpdate(Stream &updateSource, size_t updateSize) {
  if (Update.begin(updateSize)) {      
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      Serial.println("Written : " + String(written) + " successfully");
    }
    else {
      Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    if (Update.end()) {
      Serial.println("OTA done!");
      if (Update.isFinished()) {
        Serial.println("Update successfully completed. Rebooting.");
      }
      else {
        Serial.println("Update not finished? Something went wrong!");
      }
    }
    else {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
    }

  }
  else
  {
    Serial.println("Not enough space to begin OTA");
  }
}
void BLEHandInfo::updateFromFS(fs::FS &fs) {
   File updateBin = fs.open("/update.bin");
   if (updateBin) {
      if(updateBin.isDirectory()){
         Serial.println("Error, update.bin is not a file");
         updateBin.close();
         return;
      }
      size_t updateSize = updateBin.size();

      if (updateSize > 0) {
         Serial.println("Start install firmware");
         performUpdate(updateBin, updateSize);
      }
      else {
         Serial.println("Error, file is empty");
      }

      updateBin.close();
    
      // whe finished remove the binary from sd card to indicate end of the process
      fs.remove("/update.bin");     
      rebootEspWithReason("Rebooting to complete OTA update"); 
   }
   else {
      Serial.println("Could not load update.bin from sd root");
   }
}
void BLEHandInfo::rebootEspWithReason(String reason) {
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}