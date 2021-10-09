
#include "main11.h"
#include "ConnectionManager.h"

#include <Streaming.h>
#include <Vector.h>

/** Build time */
const char compileDate[] = __DATE__ " " __TIME__;

// List of Service and Characteristic UUIDs
#define SERVICE_UUID "0000aaaa-ead2-11e7-80c1-9a214cf093ae"
#define WIFI_UUID "00005555-ead2-11e7-80c1-9a214cf093ae"
#define SCAN_WIFI_UUID "00006666-ead2-11e7-80c1-9a214cf093ae"

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

  private:
  BLEAdvertising *_pAdvertising;

  public:
  MyServerCallbacks(BLEAdvertising *pAdvertising) : BLEServerCallbacks() {
    _pAdvertising = pAdvertising;
  }

	// TODO this doesn't take into account several clients being connected
	void onConnect(BLEServer* pServer) {
		Serial.println("BLE client connected");
	};

	void onDisconnect(BLEServer* pServer) {
		Serial.println("BLE client disconnected");
		_pAdvertising->start();
	}
};

/**
* MyCallbackHandler
* Callbacks for BLE client read/write requests
*/
class MyCallbackHandler: public BLECharacteristicCallbacks {
  private:
  char *apName;
  BLECharacteristic *pCharacteristicWiFi;

  public:
  
  MyCallbackHandler(char *_apName, BLECharacteristic *_pCharacteristicWiFi) : BLECharacteristicCallbacks() {
    apName = _apName;
    pCharacteristicWiFi = _pCharacteristicWiFi;
  }

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
        JsonArray params = jsonDocument["params"].as<JsonArray>();
        handleMethod(method, params);
      }
    }
    jsonDocument.clear();
  };

    void handleMethod(String method, JsonArray params) {
      if (method == "setup") {
        String ssid = params[0].as<String>();
        String password = params[1].as<String>();
        handleSetup(ssid, password);
      } else if (method == "erase") {
        Serial.println("Received erase command");
        
        Preferences preferences;
        preferences.begin("WiFiCred", false);
        preferences.clear();
        preferences.end();
      } else if (method == "reset") {
        Serial.println("Received reset command");
        WiFi.disconnect();
        esp_restart();      
      }
    }
  
    void handleSetup(String ssid, String password) {
        bool connected = connectToWIFI(ssid.c_str(), password.c_str());
        if (connected) {
            storeDeviceConfiguration(ssid, password);
        }

        String output = connected ? "{\"success\":1}" : "{\"success\":0}";
        pCharacteristicWiFi->setValue((uint8_t*)&output[0], output.length());
        pCharacteristicWiFi->notify();
    }

    void storeDeviceConfiguration(String ssid, String password) {
        Serial.println("Storing Device Configuration");
    
        Preferences preferences;
        preferences.begin("WiFiCred", false);
        preferences.putString("ssid", ssid);
        preferences.putString("pw", password);
        preferences.end();

        Serial.println("Device configuration stored successfully");
    }

  	void onRead(BLECharacteristic *pCharacteristic) {
  		  Serial.println("BLE onRead request");
        String output = "";
  		  pCharacteristic->setValue((uint8_t*)&output[0], output.length());
  	}

};

/**
* ScanWIFICallbackHandler
* Callbacks for BLE client read requests
*/
class ScanWIFICallbackHandler: public BLECharacteristicCallbacks {
  	void onRead(BLECharacteristic *pCharacteristic) {
  		  Serial.println("BLE ScanWIFI onRead request");
  
        Vector<WIFIAPNetwork> *networks = scanWiFi();
  
        Serial.println("Returned " + String(networks->size()) + " networks");
  
        for (int i = 0; i < networks->size(); i++) {  
          WIFIAPNetwork val = networks->at(i);
          Serial << val.ssid << endl;
        }
  
        String output = "[";
        for (int i = 0; i < networks->size() && output.length() < 450; i++) {
          WIFIAPNetwork item = networks->at(i);
          
          if (output != "[") output += ',';
          output += "{\"s\":\"";
          output += item.ssid;
          output += "\",\"r\":";
          output += String(item.rssi);
          output += ",\"e\":";
          output += String(item.isEncrypted);
          output += "}";
  
          Serial << i << ": " << item.ssid << endl;
        }      
        output += "]";
        
        Serial.println("Res " + output);
     
  		pCharacteristic->setValue((uint8_t*)&output[0], output.length());
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
};

WiFiSetup::WiFiSetup(char *apName) {
  WiFiSetup::_apName = apName;
}

/**
* initBLE
* Initialize BLE service and characteristic
* Start BLE server and service advertising
*/
void WiFiSetup::_initBLE() {
	// Initialize BLE and set output power
	BLEDevice::init(WiFiSetup::_apName);
	BLEDevice::setPower(ESP_PWR_LVL_P7);

	// Create BLE Server
	WiFiSetup::_pServer = BLEDevice::createServer();

	// Create BLE Service
	WiFiSetup::_pService = WiFiSetup::_pServer->createService(BLEUUID(SERVICE_UUID),20);

	// Create BLE Characteristic for WiFi settings
	WiFiSetup::_pCharacteristicWiFi = WiFiSetup::_pService->createCharacteristic(
		BLEUUID(WIFI_UUID),
		// WIFI_UUID,
		BLECharacteristic::PROPERTY_READ |
		BLECharacteristic::PROPERTY_WRITE
	);
	WiFiSetup::_pCharacteristicWiFi->setCallbacks(new MyCallbackHandler(WiFiSetup::_apName, WiFiSetup::_pCharacteristicWiFi));

  _pCharacteristicScanWiFI = _pService->createCharacteristic(
		BLEUUID(SCAN_WIFI_UUID),
		// SCAN_WIFI_UUID,
		BLECharacteristic::PROPERTY_READ
	);
	WiFiSetup::_pCharacteristicScanWiFI->setCallbacks(new ScanWIFICallbackHandler());

	// Start the service
	WiFiSetup::_pService->start();

	// Start advertising
	WiFiSetup::_pAdvertising = WiFiSetup::_pServer->getAdvertising();
	WiFiSetup::_pAdvertising->start();

	// Set server callbacks
	WiFiSetup::_pServer->setCallbacks(new MyServerCallbacks(WiFiSetup::_pAdvertising));
}

void WiFiSetup::setup() {
  // Start BLE server
  WiFiSetup::_initBLE();
}

void WiFiSetup::loop() {

}
