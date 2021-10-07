#ifndef WiFiSetup_h
#define WiFiSetup_h

#include <Arduino.h>
#include <WiFi.h>
#include <nvs.h>
#include <nvs_flash.h>

// Includes for JSON object handling
// Requires ArduinoJson library
// https://arduinojson.org
// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

// Includes for BLE
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <Preferences.h>

class WiFiSetup {
    public:
        WiFiSetup(char *apName);

        void setup();
        void loop();
    private:
        char *_apName;

        /** Characteristic for digital output */
        BLECharacteristic *_pCharacteristicWiFi;
        /** BLE Advertiser */
        BLEAdvertising *_pAdvertising;
        /** BLE Service */
        BLEService *_pService;
        /** BLE Server */
        BLEServer *_pServer;

        void _createName();
        void _initBLE();
};

#endif
