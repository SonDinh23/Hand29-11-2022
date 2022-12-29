#ifndef BLE_HAND_MANAGER_H
#define BLE_HAND_MANAGER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>

#include "HandState.h"

#include "BLEHandInfo.h"

#define HAND_SERVICE_UUID         "390950ed-51ff-445f-a6c6-f6a95a6a465f"

class BLEHandManager: public BLEServerCallbacks {
  public:
    BLEHandManager(HandState& state);
    ~BLEHandManager();
    void begin();

    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param);
    void onDisconnect(BLEServer* pServer);

  private: 
    double distance;
    HandState& handstate;

    BLEHandInfo* handService;
  
    void setPower();
    void startAdvertising();
    void startServer();
};

#endif