#include "BLEHandManager.h"

BLEHandManager::BLEHandManager(HandState& state): handstate(state) {
}

void BLEHandManager::onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
  Serial.println("Connected");

  // Update connection configuration with peer
  // latency = 0; min_int = 0x10; max_int = 0x20; timeout = 10s;
  pServer->updateConnParams(param->connect.remote_bda, 0x10, 0x20, 0, 1000);

  handstate.onConnect();
}

void BLEHandManager::onDisconnect(BLEServer* pServer) {
  Serial.println("Disconnected");
  handstate.onDisconnect();

  BLEDevice::startAdvertising();
}

void BLEHandManager::setPower() {
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P9);
}

void BLEHandManager::startAdvertising() {
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(HAND_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);  // for iPhone connections issue
  advertising->setMaxPreferred(0x12);

  BLEAdvertisementData data;
  data.setName(handService->getName());
  //data.setManufacturerData(handService->getFrameNumber());
  advertising->setScanResponseData(data);

  BLEDevice::startAdvertising();
}

void BLEHandManager::startServer() {
  // Init without device name, which will be set later 
  // accordingly to handService.getName()
  BLEDevice::init("");

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(this);

  handService = new BLEHandInfo(server->createService(HAND_SERVICE_UUID), handstate);

  handService->begin();

  // Restore original name after firmware upgrade
  esp_ble_gap_set_device_name(handService->getName());

  Serial.println("BLE started");
}

void BLEHandManager::begin() {
  startServer();
  setPower();
  startAdvertising();
}


BLEHandManager::~BLEHandManager() {
  free(handService);
}