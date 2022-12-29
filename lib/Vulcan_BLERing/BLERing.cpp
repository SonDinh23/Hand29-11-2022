#include "BLERing.h"
#include "freertos/FreeRTOS.h"

void  BLERing::getStringPref(Preferences& pref, 
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

BLERing::BLERing(BLEService* service, RingState& pRingState): 
    BLEServiceManager(service), ringState(pRingState) {
  addReadWriteNotify(OTA_UUID,
    authed([this](BLECharacteristic* p) {onReadOTA(p);}),
    authed([this](BLECharacteristic* p) {onWriteOTA(p);}));

  addReadWrite(NAME_CHAR_UUID, 
    authed([this](BLECharacteristic* p) {onReadName(p);}), 
    authed([this](BLECharacteristic* p) {onWriteName(p);}));

  addReadWrite(MODE_UUID, 
    authed([this](BLECharacteristic* p) {onReadMode(p);}), 
    authed([this](BLECharacteristic* p) {onWriteMode(p);}));
  
  // Increasing WDT timeout to 10s to fix esp_ota_begin issue
  // https://github.com/espressif/esp-idf/issues/1479
  // https://github.com/espressif/esp-idf/issues/578
  //esp_task_wdt_init(10, false);
}

void BLERing::begin() {
  BLEServiceManager::begin();
  SPIFFS.begin(true);
  
  pref.begin("ble-info", false);
  getStringPref(pref, "name", name, MAX_NAME_LENGTH, INIT_NAME);
  Serial.printf("BLE MyoBand: %s\n",name);
}

void BLERing::onReadOTA(BLECharacteristic* pChar) {
  pChar->setValue(VERSION);
}

void BLERing::onWriteOTA(BLECharacteristic* pChar) {
  /* // OTA save on SD
  std::string rxData = pChar->getValue();
  uint16_t len = rxData.length();
  uint8_t* data = pChar->getData();
  if (rxData == UPDATE_START_MSG) {
    onOTA = true;
    SPIFFS.remove(FILE_DATA);
    SPIFFS.remove(FILE_FW);
    file = SPIFFS.open(FILE_FW, FILE_APPEND);
    count = 0;
    Serial.println("Begin update OTA");
    Serial.println(count);
    onNotifyOTA(count);
  }
  else if (rxData == UPDATE_END_MSG) {
    Serial.println(++count);
    onNotifyOTA(count);
    onOTA = false;
    file.close();
    Serial.println("End update OTA");
    file = SPIFFS.open(FILE_FW);
    
    // while(file.available()){
    //   Serial.print(file.read());
    // }
    updateFromFS(SPIFFS);
    //SPIFFS.remove("/update.bin");
  }
  else if (onOTA) {
    file.write(data, len);
    Serial.println(++count);
    onNotifyOTA(count);
  }
  */
  
  // OTA save on esp_ota
  std::string rxData = pChar->getValue();
  if (rxData == UPDATE_START_MSG) {
    Serial.println("Begin OTA");
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    Serial.println("Found partition");
    esp_err_t result = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &otaHandler);
    if (result == ESP_OK) {
      Serial.println("OTA operation commenced successfully");
    } else {
      Serial.print("Failed to commence OTA operation, error: ");
      Serial.println(result);
    }
    updateInProgress = true;
    count = 0;
    Serial.println("Begin OTA done");
  }
  else if (rxData == UPDATE_END_MSG) {
    Serial.println("OTA: Upload completed");
    esp_err_t result = esp_ota_end(otaHandler);
    if (result == ESP_OK) {
      Serial.println("Newly written OTA app image is valid.");
    } else {
      Serial.print("Failed to validate OTA app image, error: ");
      Serial.println(result);
    }
    if (esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL)) == ESP_OK) {
      delay(1000);
      esp_restart();
    } else {
      Serial.println("OTA Error: Invalid boot partition");
    }
  }
  else if (rxData.length() > 0 && updateInProgress) {
    Serial.print("Received OTA packet: ");
    Serial.println(count++);
    esp_ota_write(otaHandler, rxData.c_str(), rxData.length());
    onNotifyOTA(count);
    // Make the writes much more reliable
    onNotifyOTA(count);
    vTaskDelay(1);
  }
}

void BLERing::onNotifyOTA(uint32_t stateOTA) {
  BLECharacteristic* pCharacteristic = service->getCharacteristic(OTA_UUID);
  pCharacteristic->setValue(stateOTA);
  pCharacteristic->notify();
}

const char* BLERing::getName() {
  return name;
}

void BLERing::onWriteName(BLECharacteristic* pChar) {
  const char* s = pChar->getValue().c_str();
  if (strlen(s) > 0) {
    memcpy(name, s, strlen(s) + 1);
    pref.putBytes("name", s, strlen(s));
    
    // Update device name, name is change after esp reset 
    esp_ble_gap_set_device_name(s);
    // Advertise the changes to clients
    esp_ble_gap_config_adv_data(&ADVERTISING_CONFIG);
    Serial.printf("Mame: %s\n", name);
  }
}

void BLERing::onReadName(BLECharacteristic* pChar) {
  pChar->setValue(name);
}

void BLERing::onReadMode(BLECharacteristic* pChar) {
  uint8_t _mode = ringState.getMode();
	pChar->setValue(&_mode, 1);
}

void BLERing::onWriteMode(BLECharacteristic* pChar) {
  const char* s = pChar->getValue().c_str();
  if (strlen(s) > 0) {
    uint8_t _mode = atoi(s);
    Serial.printf("Mode: %d\n",_mode);
    ringState.setMode(_mode);
  }
}

void BLERing::performUpdate(Stream &updateSource, size_t updateSize) {
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
void BLERing::updateFromFS(fs::FS &fs) {
   File updateBin = fs.open(FILE_FW);
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
      fs.remove(FILE_FW);     
      rebootEspWithReason("Rebooting to complete OTA update"); 
   }
   else {
      Serial.println("Could not load update.bin from sd root");
   }
}
void BLERing::rebootEspWithReason(String reason) {
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}