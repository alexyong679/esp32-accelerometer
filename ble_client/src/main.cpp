#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>
#include <Arduino.h>

// UUID definitions
#define SERVICE_UUID        "296728af-7101-479d-be0d-0831dbf93239"
#define CHARACTERISTIC_UUID_1 "a968211d-49cc-40ed-8125-4900d1d75b8b"
#define CHARACTERISTIC_UUID_2 "993b523c-1346-4009-878a-7c9828bf043d"

//Global variables
bool doConnect = false;             // Flag to indicate we found a device and should try to connect
bool connected = false;             // Flag to track if we are currently connected to the server
BLEAdvertisedDevice* myDevice;      // Pointer to store information about the server device we find
BLEClient* pClient;                 // Pointer to our BLE client instance
BLERemoteCharacteristic* pRemoteCharacteristic1; // Pointer to the characteristic on the remote server
BLERemoteCharacteristic* pRemoteCharacteristic2; 
BLERemoteCharacteristic* char1;
BLERemoteCharacteristic* char2;

//Callback classes/functions

//this class handles devices discovered during the BLE scan
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str()); // print info about the discovered device

        //check if the discovered device has the specific service UUID were looking for
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))){
          Serial.println("Found our target server!");
          BLEDevice::getScan()->stop(); // Stop scanning once we find our server (to save power and connect faster)
          myDevice = new BLEAdvertisedDevice(advertisedDevice); // Store a copy of this device's info
          doConnect = true; // Set the flag to attempt connection in the loop()
        }
    }
};

// function is called every time server notifys clients
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

    if (pBLERemoteCharacteristic->getUUID() == BLEUUID(CHARACTERISTIC_UUID_1)) {
      Serial.print("UUID 1 of data length ");
      Serial.println(length);

      uint32_t counter = pData[0];
      for (int i = 1; i < length; i++) {
          counter |= (pData[i] << (i * 8));
      }

      Serial.print("data: ");
      Serial.println(counter);
    } 
    else if (pBLERemoteCharacteristic->getUUID() == BLEUUID(CHARACTERISTIC_UUID_2)) {
      Serial.print("Data: ");
      for (int i = 0; i < length; i++) {
          Serial.print((char)pData[i]);
      }
      Serial.println(".");
    }
}

// this class handles events related to the client's connection to the server
class MyClientCallback : public BLEClientCallbacks{
  void onConnect (BLEClient* pclient){
    Serial.println("connected to the server succefully");
  }

  void onDisconnect (BLEClient* pclient){
    connected = false; //this updates connection status
    Serial.println("Disconnected from server");
  }
};


// this func looks up a remote ble characteristic from a connected ble service
// it then saves the characteristic into a variable passed in by the caller (outchar)
// returns true if succsseful and false if it fails
bool connectCharacteristic(BLERemoteService* pRemoteService, BLEUUID charUUID, BLERemoteCharacteristic** outChar) {
  *outChar = pRemoteService->getCharacteristic(charUUID);
  if (*outChar == nullptr) {
    Serial.print("Failed to find characteristic: ");
    Serial.println(charUUID.toString().c_str());
    return false;
  }

  Serial.println(" - Found characteristic.");

  if ((*outChar)->canNotify()) {
    (*outChar)->registerForNotify(notifyCallback);
    Serial.println(" - Subscribed to notifications.");
  }

  return true;
}


// function called when client to server
bool connectToServer(BLEAdvertisedDevice* advertisedDevice) {
    Serial.print("Connecting to: ");
    Serial.println(advertisedDevice->getAddress().toString().c_str());

    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    if (!pClient->connect(advertisedDevice)) {
        Serial.println("Failed to connect.");
        return false;
    }

    Serial.println("Connected.");

    BLERemoteService* remoteService = pClient->getService(SERVICE_UUID);
    if (remoteService == nullptr) {
        Serial.println("Service not found.");
        pClient->disconnect();
        return false;
    }

    if (!connectCharacteristic(remoteService, BLEUUID(CHARACTERISTIC_UUID_1), &char1) ||
        !connectCharacteristic(remoteService, BLEUUID(CHARACTERISTIC_UUID_2), &char2)) {
        Serial.println("Characteristic(s) not found.");
        pClient->disconnect();
        return false;
    }

    connected = true;
    Serial.println("Fully connected and subscribed.");
    return true;
}






void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Client...");

    BLEDevice::init("ESP32_CLIENT");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(100); // how often to scan
    pBLEScan->setWindow(99); // how long to scan within the interval
    pBLEScan->setActiveScan(true);
    pBLEScan->start(30, false);
    Serial.println("Scanning for BLE server...");
}

void loop() {
  if (doConnect) {
    if (connectToServer(myDevice)) {
      Serial.println("Connection to BLE server successful!");
      doConnect = false; // Reset the flag so we don't try to connect again immediately
    } else {
      Serial.println("Failed to connect to server... trying again later.");
      // If connection fails, restart scan to find the server again.
      BLEDevice::getScan()->start(5, false);
      doConnect = false; // Ensure this is false after failed attempt
    }
  }


 // If we are connected to the server:
  if (connected) {
    // --- READ FROM CHARACTERISTIC ---
    /*
    std::string value = pRemoteCharacteristic->readValue(); // Read the current value
    Serial.print("Read Value from Server: ");
    Serial.println(value.c_str());

    // --- WRITE TO CHARACTERISTIC ---
    // Create a string to send to the server.
    String writeValue = "Client MSG: " + String(millis() / 1000); // Example: "Client MSG: 10"
    // Write the new value to the characteristic.
    // The 'true' argument means a "write with response" (server acknowledges receipt).
    pRemoteCharacteristic->writeValue(writeValue.c_str(), true);
    Serial.print("Wrote Value to Server: ");
    Serial.println(writeValue);

    // Notifications will be received automatically by notifyCallback().

    delay(3000); // Wait for 3 seconds before next read/write

    */
  } else {
    // If not connected, and not currently trying to connect (doConnect is false),
    // then restart scanning to find the server again.

    
    if (!doConnect) {
      /*
        Serial.println("Not connected, restarting scan to find server...");
        BLEDevice::getScan()->start(5, false); // Scan for 5 seconds
        delay(5000); // Wait for the scan to complete
        */
    }
      
  }
}
