//// Default Arduino includes
#include <Arduino.h>
#include <WiFi.h>
#include <nvs.h>
#include <nvs_flash.h>

//#include "main11.h"

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
/** Selected network 
    true = use primary network
    false = use secondary network
*/
bool usePrimAP = true;
/** Flag if stored AP credentials are available */
bool hasCredentials = false;
/** Connection status */
volatile bool isConnected = false;
/** Connection change status */
bool connStatusChanged = false;

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

// List of Service and Characteristic UUIDs
#define SERVICE_UUID  "0000aaaa-ead2-11e7-80c1-9a214cf093ae"
#define WIFI_UUID     "00005555-ead2-11e7-80c1-9a214cf093ae"

/** SSIDs of local WiFi networks */
String ssidPrim;
String ssidSec;
/** Password for local WiFi network */
String pwPrim;
String pwSec;

/** Characteristic for digital output */
BLECharacteristic *pCharacteristicWiFi;
/** BLE Advertiser */
BLEAdvertising* pAdvertising;
/** BLE Service */
BLEService *pService;
/** BLE Server */
BLEServer *pServer;

/** Document for JSON string */
// MAx size is 51 bytes for frame: 
// {"ssidPrim":"","pwPrim":"","ssidSec":"","pwSec":""}
// + 4 x 32 bytes for 2 SSID's and 2 passwords
StaticJsonDocument<200> jsonDocument;

struct WIFIAPNetwork {
  String ssid;
  int32_t rssi;
  bool isEncrypted;
};

/**
 * MyServerCallbacks
 * Callbacks for client connection and disconnection
 */
class MyServerCallbacks: public BLEServerCallbacks {
  // TODO this doesn't take into account several clients being connected
  void onConnect(BLEServer* pServer) {
    Serial.println("BLE client connected");
  };

  void onDisconnect(BLEServer* pServer) {
    Serial.println("BLE client disconnected");
    pAdvertising->start();
  }
};

/**
 * MyCallbackHandler
 * Callbacks for BLE client read/write requests
 */
class MyCallbackHandler: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() == 0) {
      return;
    }
    Serial.println("Received over BLE: " + String((char *)&value[0]));

    /** Json object for incoming data */
    auto error = deserializeJson(jsonDocument, (char *)&value[0]);
    //JsonObject& jsonIn = jsonBuffer.parseObject((char *)&value[0]);

    if (error) {
      Serial.println("Received invalid JSON");
      Serial.println(error.c_str());
    } else {
      if (jsonDocument.containsKey("method")) {
        String method = jsonDocument["method"].as<String>();
        handleMethod(method);
      } else if (jsonDocument.containsKey("ssidPrim") &&
          jsonDocument.containsKey("pwPrim") && 
          jsonDocument.containsKey("ssidSec") &&
          jsonDocument.containsKey("pwSec")) {
        ssidPrim = jsonDocument["ssidPrim"].as<String>();
        pwPrim = jsonDocument["pwPrim"].as<String>();
        ssidSec = jsonDocument["ssidSec"].as<String>();
        pwSec = jsonDocument["pwSec"].as<String>();

        Preferences preferences;
        preferences.begin("WiFiCred", false);
        preferences.putString("ssidPrim", ssidPrim);
        preferences.putString("ssidSec", ssidSec);
        preferences.putString("pwPrim", pwPrim);
        preferences.putString("pwSec", pwSec);
        preferences.putBool("valid", true);
        preferences.end();

        Serial.println("Received over bluetooth:");
        Serial.println("primary SSID: "+ssidPrim+" password: "+pwPrim);
        Serial.println("secondary SSID: "+ssidSec+" password: "+pwSec);
        connStatusChanged = true;
        hasCredentials = true;
      } else if (jsonDocument.containsKey("erase")) {
        Serial.println("Received erase command");
        Preferences preferences;
        preferences.begin("WiFiCred", false);
        preferences.clear();
        preferences.end();
        connStatusChanged = true;
        hasCredentials = false;
        ssidPrim = "";
        pwPrim = "";
        ssidSec = "";
        pwSec = "";

        int err;
        err=nvs_flash_init();
        Serial.println("nvs_flash_init: " + err);
        err=nvs_flash_erase();
        Serial.println("nvs_flash_erase: " + err);
      } else if (jsonDocument.containsKey("reset")) {
        WiFi.disconnect();
        esp_restart();
      }
    }
    jsonDocument.clear();
  };

  void handleMethod(String method) {
    if (method == "scanWIFI") {
      handleScanWIFI();
    } else if (method == "") {
      
    }
  }
  
  void handleScanWIFI() {
      Vector<WIFIAPNetwork> *networks = scanWiFi();

      Serial.println("Returned " + String(networks->size()) + " networks");

      for (int i = 0; i < networks->size(); i++) {  
        WIFIAPNetwork val = networks->at(i);
        Serial << val.ssid << endl;
      }

      String output = "[";
      for (int i = 0; i < 11, output.length() < 450; i++) {
        WIFIAPNetwork item = networks->at(i);
        
        if (output != "[") output += ',';
        output += "{\"s\":\"";
        output += item.ssid;
        output += "\",\"r\":";
        output += String(item.rssi);
        output += ",\"e\":";
        output += String(item.isEncrypted);
        output += "}";
      }      
      output += "]";
      
      Serial.println("Res " + output);

      pCharacteristicWiFi->setValue((uint8_t*)&output[0], output.length());
      pCharacteristicWiFi->notify();

      // remove array
      delete[] &*networks->begin();
      delete networks;
  }

  Vector<WIFIAPNetwork>* scanWiFi() {
    Serial.println("Start scanning for networks");

    WiFi.disconnect(true);
    WiFi.enableSTA(true);
    WiFi.mode(WIFI_STA);

    // Scan for AP
    int apNum = WiFi.scanNetworks(false,true,false,1000);
    Serial.println("Found " + String(apNum) + " networks");
    if (apNum == 0) {
      Serial.println("Found no networks?????");
      return false;
    }

    WIFIAPNetwork *networks_array = new WIFIAPNetwork[20];
    Vector<WIFIAPNetwork> *networks = new Vector<WIFIAPNetwork>();
    networks->setStorage(networks_array, 20, 0);
    
    for (int i = 0; i < apNum; i++) {
      bool isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      networks->push_back({ WiFi.SSID(i), WiFi.RSSI(i), isEncrypted });
    }

    return networks;
  }

  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("BLE onRead request");
    String wifiCredentials;

    /** Json object for outgoing data */
    // JsonObject& jsonOut = jsonDocument.createObject();
    jsonDocument["ssidPrim"] = ssidPrim;
    jsonDocument["pwPrim"] = pwPrim;
    jsonDocument["ssidSec"] = ssidSec;
    jsonDocument["pwSec"] = pwSec;
    // Convert JSON object into a string
    // jsonOut.printTo(wifiCredentials);
    serializeJson(jsonDocument, wifiCredentials);

    // encode the data
    int keyIndex = 0;
    Serial.println("Stored settings: " + wifiCredentials);
    for (int index = 0; index < wifiCredentials.length(); index ++) {
      wifiCredentials[index] = (char) wifiCredentials[index] ^ (char) apName[keyIndex];
      keyIndex++;
      if (keyIndex >= strlen(apName)) keyIndex = 0;
    }
    pCharacteristicWiFi->setValue((uint8_t*)&wifiCredentials[0],wifiCredentials.length());
    jsonDocument.clear();
  }
};

/**
 * initBLE
 * Initialize BLE service and characteristic
 * Start BLE server and service advertising
 */
void initBLE() {
  // Initialize BLE and set output power
  BLEDevice::init(apName);
  BLEDevice::setPower(ESP_PWR_LVL_P7);

  // Create BLE Server
  pServer = BLEDevice::createServer();

  // Set server callbacks
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  pService = pServer->createService(BLEUUID(SERVICE_UUID),20);

  // Create BLE Characteristic for WiFi settings
  pCharacteristicWiFi = pService->createCharacteristic(
    BLEUUID(WIFI_UUID),
    // WIFI_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristicWiFi->setCallbacks(new MyCallbackHandler());

  // Start the service
  pService->start();

  // Start advertising
  pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

/** Callback for receiving IP address from AP */
void gotIP(system_event_id_t event) {
  isConnected = true;
  connStatusChanged = true;
}

/** Callback for connection loss */
void lostCon(system_event_id_t event) {
  isConnected = false;
  connStatusChanged = true;
}

/**
 * Start connection to AP
 */
void connectWiFi() {
  // Setup callback function for successful connection
  WiFi.onEvent(gotIP, SYSTEM_EVENT_STA_GOT_IP);
  // Setup callback function for lost connection
  WiFi.onEvent(lostCon, SYSTEM_EVENT_STA_DISCONNECTED);

  WiFi.disconnect(true);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.print("Start connection to ");

  Serial.println(ssidPrim);
  WiFi.begin(ssidPrim.c_str(), pwPrim.c_str());
}

void setup1() {
  // Create unique device name
  createName();

  // Initialize Serial port
  Serial.begin(115200);
  // Send some device info
  Serial.print("Build: ");
  Serial.println(compileDate);

  Preferences preferences;
  preferences.begin("WiFiCred", false);
  bool hasPref = preferences.getBool("valid", false);
  if (hasPref) {
    ssidPrim = preferences.getString("ssidPrim","");
    ssidSec = preferences.getString("ssidSec","");
    pwPrim = preferences.getString("pwPrim","");
    pwSec = preferences.getString("pwSec","");

    if (ssidPrim.equals("") 
        || pwPrim.equals("")
        || ssidSec.equals("")
        || pwPrim.equals("")) {
      Serial.println("Found preferences but credentials are invalid");
    } else {
      Serial.println("Read from preferences:");
      Serial.println("primary SSID: "+ssidPrim+" password: "+pwPrim);
      Serial.println("secondary SSID: "+ssidSec+" password: "+pwSec);
      hasCredentials = true;
    }
  } else {
    Serial.println("Could not find preferences, need send data over BLE");
  }
  preferences.end();

  // Start BLE server
  initBLE();

  if (hasCredentials) {
      // connectWiFi();
  }
}

void loop1() {
  // if (connStatusChanged) {
  //   if (isConnected) {
  //     Serial.print("Connected to AP: ");
  //     Serial.print(WiFi.SSID());
  //     Serial.print(" with IP: ");
  //     Serial.print(WiFi.localIP());
  //     Serial.print(" RSSI: ");
  //     Serial.println(WiFi.RSSI());
  //   } else {
  //     if (hasCredentials) {
  //       Serial.println("Lost WiFi connection");
  //       // Received WiFi credentials
  //       if (!scanWiFi) { // Check for available AP's
  //         Serial.println("Could not find any AP");
  //       } else { // If AP was found, start connection
  //         connectWiFi();
  //       }
  //     } 
  //   }
  //   connStatusChanged = false;
  // }
}

void setup() {
  setup1();
}

void loop() {
  loop1();
}
