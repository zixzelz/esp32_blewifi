//// Default Arduino includes
//
//#include "main11.h"
//
///** Build time */
//const char compileDate[] = __DATE__ " " __TIME__;
//
///** Unique device name */
//char apName[] = "ESP32-xxxxxxxxxxxx";
///** Selected network 
//    true = use primary network
//		false = use secondary network
//*/
//bool usePrimAP = true;
///** Flag if stored AP credentials are available */
//bool hasCredentials = false;
///** Connection status */
//volatile bool isConnected = false;
///** Connection change status */
//bool connStatusChanged = false;
//
///**
// * Create unique device name from MAC address
// **/
//void createName() {
//	uint8_t baseMac[6];
//	// Get MAC address for WiFi station
//	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
//	// Write unique name into apName
//	sprintf(apName, "ESP32-%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
//}
//
//// List of Service and Characteristic UUIDs
//#define SERVICE_UUID  "0000aaaa-ead2-11e7-80c1-9a214cf093ae"
//#define WIFI_UUID     "00005555-ead2-11e7-80c1-9a214cf093ae"
//
///** SSIDs of local WiFi networks */
//String ssidPrim;
//String ssidSec;
///** Password for local WiFi network */
//String pwPrim;
//String pwSec;
//
///** Characteristic for digital output */
//BLECharacteristic *pCharacteristicWiFi;
///** BLE Advertiser */
//BLEAdvertising* pAdvertising;
///** BLE Service */
//BLEService *pService;
///** BLE Server */
//BLEServer *pServer;
//
///** Document for JSON string */
//// MAx size is 51 bytes for frame: 
//// {"ssidPrim":"","pwPrim":"","ssidSec":"","pwSec":""}
//// + 4 x 32 bytes for 2 SSID's and 2 passwords
//StaticJsonDocument<200> jsonDocument;
//
///**
// * MyServerCallbacks
// * Callbacks for client connection and disconnection
// */
//class MyServerCallbacks: public BLEServerCallbacks {
//	// TODO this doesn't take into account several clients being connected
//	void onConnect(BLEServer* pServer) {
//		Serial.println("BLE client connected");
//	};
//
//	void onDisconnect(BLEServer* pServer) {
//		Serial.println("BLE client disconnected");
//		pAdvertising->start();
//	}
//};
//
///**
// * MyCallbackHandler
// * Callbacks for BLE client read/write requests
// */
//class MyCallbackHandler: public BLECharacteristicCallbacks {
//	void onWrite(BLECharacteristic *pCharacteristic) {
//		std::string value = pCharacteristic->getValue();
//		if (value.length() == 0) {
//			return;
//		}
//		Serial.println("Received over BLE: " + String((char *)&value[0]));
//
//		// Decode data
//		int keyIndex = 0;
//		for (int index = 0; index < value.length(); index ++) {
//			value[index] = (char) value[index] ^ (char) apName[keyIndex];
//			keyIndex++;
//			if (keyIndex >= strlen(apName)) keyIndex = 0;
//		}
//
//		/** Json object for incoming data */
//		auto error = deserializeJson(jsonDocument, (char *)&value[0]);
//		//JsonObject& jsonIn = jsonBuffer.parseObject((char *)&value[0]);
//
//		if (error) {
//			Serial.println("Received invalid JSON");
//			Serial.println(error.c_str());
//		} else {
//			if (jsonDocument.containsKey("ssidPrim") &&
//					jsonDocument.containsKey("pwPrim") && 
//					jsonDocument.containsKey("ssidSec") &&
//					jsonDocument.containsKey("pwSec")) {
//				ssidPrim = jsonDocument["ssidPrim"].as<String>();
//				pwPrim = jsonDocument["pwPrim"].as<String>();
//				ssidSec = jsonDocument["ssidSec"].as<String>();
//				pwSec = jsonDocument["pwSec"].as<String>();
//
//				Preferences preferences;
//				preferences.begin("WiFiCred", false);
//				preferences.putString("ssidPrim", ssidPrim);
//				preferences.putString("ssidSec", ssidSec);
//				preferences.putString("pwPrim", pwPrim);
//				preferences.putString("pwSec", pwSec);
//				preferences.putBool("valid", true);
//				preferences.end();
//
//				Serial.println("Received over bluetooth:");
//				Serial.println("primary SSID: "+ssidPrim+" password: "+pwPrim);
//				Serial.println("secondary SSID: "+ssidSec+" password: "+pwSec);
//				connStatusChanged = true;
//				hasCredentials = true;
//			} else if (jsonDocument.containsKey("erase")) {
//				Serial.println("Received erase command");
//				Preferences preferences;
//				preferences.begin("WiFiCred", false);
//				preferences.clear();
//				preferences.end();
//				connStatusChanged = true;
//				hasCredentials = false;
//				ssidPrim = "";
//				pwPrim = "";
//				ssidSec = "";
//				pwSec = "";
//
//				int err;
//				err=nvs_flash_init();
//				Serial.println("nvs_flash_init: " + err);
//				err=nvs_flash_erase();
//				Serial.println("nvs_flash_erase: " + err);
//			} else if (jsonDocument.containsKey("reset")) {
//				WiFi.disconnect();
//				esp_restart();
//			}
//		}
//		jsonDocument.clear();
//	};
//
//	void onRead(BLECharacteristic *pCharacteristic) {
//		Serial.println("BLE onRead request");
//		String wifiCredentials;
//
//		/** Json object for outgoing data */
//		// JsonObject& jsonOut = jsonDocument.createObject();
//		jsonDocument["ssidPrim"] = ssidPrim;
//		jsonDocument["pwPrim"] = pwPrim;
//		jsonDocument["ssidSec"] = ssidSec;
//		jsonDocument["pwSec"] = pwSec;
//		// Convert JSON object into a string
//		// jsonOut.printTo(wifiCredentials);
//		serializeJson(jsonDocument, wifiCredentials);
//
//		// encode the data
//		int keyIndex = 0;
//		Serial.println("Stored settings: " + wifiCredentials);
//		for (int index = 0; index < wifiCredentials.length(); index ++) {
//			wifiCredentials[index] = (char) wifiCredentials[index] ^ (char) apName[keyIndex];
//			keyIndex++;
//			if (keyIndex >= strlen(apName)) keyIndex = 0;
//		}
//		pCharacteristicWiFi->setValue((uint8_t*)&wifiCredentials[0],wifiCredentials.length());
//		jsonDocument.clear();
//	}
//};
//
///**
// * initBLE
// * Initialize BLE service and characteristic
// * Start BLE server and service advertising
// */
//void initBLE() {
//	// Initialize BLE and set output power
//	BLEDevice::init(apName);    
//	BLEDevice::setPower(ESP_PWR_LVL_P7);
//
//	// Create BLE Server
//	pServer = BLEDevice::createServer();
//
//	// Set server callbacks
//	pServer->setCallbacks(new MyServerCallbacks());
//
//	// Create BLE Service
//	pService = pServer->createService(BLEUUID(SERVICE_UUID),20);
//
//	// Create BLE Characteristic for WiFi settings
//	pCharacteristicWiFi = pService->createCharacteristic(
//		BLEUUID(WIFI_UUID),
//		// WIFI_UUID,
//		BLECharacteristic::PROPERTY_READ |
//		BLECharacteristic::PROPERTY_WRITE
//	);
//	pCharacteristicWiFi->setCallbacks(new MyCallbackHandler());
//
//	// Start the service
//	pService->start();
//
//	// Start advertising
//	pAdvertising = pServer->getAdvertising();
//	pAdvertising->start();
//}
//
///** Callback for receiving IP address from AP */
//void gotIP(system_event_id_t event) {
//	isConnected = true;
//	connStatusChanged = true;
//}
//
///** Callback for connection loss */
//void lostCon(system_event_id_t event) {
//	isConnected = false;
//	connStatusChanged = true;
//}
//
///**
//	 scanWiFi
//	 Scans for available networks 
//	 and decides if a switch between
//	 allowed networks makes sense
//
//	 @return <code>bool</code>
//	        True if at least one allowed network was found
//*/
//bool scanWiFi() {
//	/** RSSI for primary network */
//	int8_t rssiPrim;
//	/** RSSI for secondary network */
//	int8_t rssiSec;
//	/** Result of this function */
//	bool result = false;
//
//	Serial.println("Start scanning for networks");
//
//	WiFi.disconnect(true);
//	WiFi.enableSTA(true);
//	WiFi.mode(WIFI_STA);
//
//	// Scan for AP
//	int apNum = WiFi.scanNetworks(false,true,false,1000);
//	if (apNum == 0) {
//		Serial.println("Found no networks?????");
//		return false;
//	}
//	
//	byte foundAP = 0;
//	bool foundPrim = false;
//
//	for (int index=0; index<apNum; index++) {
//		String ssid = WiFi.SSID(index);
//		Serial.println("Found AP: " + ssid + " RSSI: " + WiFi.RSSI(index));
//		if (!strcmp((const char*) &ssid[0], (const char*) &ssidPrim[0])) {
//			Serial.println("Found primary AP");
//			foundAP++;
//			foundPrim = true;
//			rssiPrim = WiFi.RSSI(index);
//		}
//		if (!strcmp((const char*) &ssid[0], (const char*) &ssidSec[0])) {
//			Serial.println("Found secondary AP");
//			foundAP++;
//			rssiSec = WiFi.RSSI(index);
//		}
//	}
//
//	switch (foundAP) {
//		case 0:
//			result = false;
//			break;
//		case 1:
//			if (foundPrim) {
//				usePrimAP = true;
//			} else {
//				usePrimAP = false;
//			}
//			result = true;
//			break;
//		default:
//			Serial.printf("RSSI Prim: %d Sec: %d\n", rssiPrim, rssiSec);
//			if (rssiPrim > rssiSec) {
//				usePrimAP = true; // RSSI of primary network is better
//			} else {
//				usePrimAP = false; // RSSI of secondary network is better
//			}
//			result = true;
//			break;
//	}
//	return result;
//}
//
///**
// * Start connection to AP
// */
//void connectWiFi() {
//	// Setup callback function for successful connection
//	WiFi.onEvent(gotIP, SYSTEM_EVENT_STA_GOT_IP);
//	// Setup callback function for lost connection
//	WiFi.onEvent(lostCon, SYSTEM_EVENT_STA_DISCONNECTED);
//
//	WiFi.disconnect(true);
//	WiFi.enableSTA(true);
//	WiFi.mode(WIFI_STA);
//
//	Serial.println();
//	Serial.print("Start connection to ");
//	if (usePrimAP) {
//		Serial.println(ssidPrim);
//		WiFi.begin(ssidPrim.c_str(), pwPrim.c_str());
//	} else {
//		Serial.println(ssidSec);
//		WiFi.begin(ssidSec.c_str(), pwSec.c_str());
//	}
//}
//
//void setupBLEWIFI() {
//	// Create unique device name
//	createName();
//
//	// Initialize Serial port
//	Serial.begin(115200);
//	// Send some device info
//	Serial.print("Build: ");
//	Serial.println(compileDate);
//
//	Preferences preferences;
//	preferences.begin("WiFiCred", false);
//	bool hasPref = preferences.getBool("valid", false);
//	if (hasPref) {
//		ssidPrim = preferences.getString("ssidPrim","");
//		ssidSec = preferences.getString("ssidSec","");
//		pwPrim = preferences.getString("pwPrim","");
//		pwSec = preferences.getString("pwSec","");
//
//		if (ssidPrim.equals("") 
//				|| pwPrim.equals("")
//				|| ssidSec.equals("")
//				|| pwPrim.equals("")) {
//			Serial.println("Found preferences but credentials are invalid");
//		} else {
//			Serial.println("Read from preferences:");
//			Serial.println("primary SSID: "+ssidPrim+" password: "+pwPrim);
//			Serial.println("secondary SSID: "+ssidSec+" password: "+pwSec);
//			hasCredentials = true;
//		}
//	} else {
//		Serial.println("Could not find preferences, need send data over BLE");
//	}
//	preferences.end();
//
//	// Start BLE server
//	initBLE();
//
//	if (hasCredentials) {
//		// Check for available AP's
//		if (!scanWiFi) {
//			Serial.println("Could not find any AP");
//		} else {
//			// If AP was found, start connection
//			connectWiFi();
//		}
//	}
//}
//
//void loopBLEWIFI() {
//	if (connStatusChanged) {
//		if (isConnected) {
//			Serial.print("Connected to AP: ");
//			Serial.print(WiFi.SSID());
//			Serial.print(" with IP: ");
//			Serial.print(WiFi.localIP());
//			Serial.print(" RSSI: ");
//			Serial.println(WiFi.RSSI());
//		} else {
//			if (hasCredentials) {
//				Serial.println("Lost WiFi connection");
//				// Received WiFi credentials
//				if (!scanWiFi) { // Check for available AP's
//					Serial.println("Could not find any AP");
//				} else { // If AP was found, start connection
//					connectWiFi();
//				}
//			} 
//		}
//		connStatusChanged = false;
//	}
//}
