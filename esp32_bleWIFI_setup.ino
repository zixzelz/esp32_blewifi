//// Default Arduino includes
#include <Arduino.h>
#include <WiFi.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "main11.h"

#include <Streaming.h>
#include <Vector.h>

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

/** Build time */
const char compileDate[] = __DATE__ " " __TIME__;

/** Unique device name */
char apName[] = "ESP32-xxxxxxxxxxxx";

/**
 * Create unique device name from MAC address
 **/
void createName() {
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  // Write unique name into apName
  sprintf(apName, "ESP32-%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
}

///** Callback for receiving IP address from AP */
//void gotIP(system_event_id_t event) {
//  isConnected = true;
//  connStatusChanged = true;
//}
//
///** Callback for connection loss */
//void lostCon(system_event_id_t event) {
//  isConnected = false;
//  connStatusChanged = true;
//}

WiFiSetup *wifiSetup = new WiFiSetup(apName);

void setup() {
  // Initialize Serial port
  Serial.begin(115200);
  // Send some device info
  Serial.print("Build: ");
  Serial.println(compileDate);

  createName();
  
  wifiSetup->setup();
}

void loop() {

}

//Sketch uses 1337658 bytes (68%) of program storage space. Maximum is 1966080 bytes.
//Global variables use 50764 bytes (15%) of dynamic memory, leaving 276916 bytes for local variables. Maximum is 327680 bytes.
