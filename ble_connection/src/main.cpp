#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <Wire.h>


// Global variables
BLEServer* pServer = NULL; // represents our ble server itself
BLECharacteristic* pCharacteristic_1 = NULL; // this is a pointer that will notify/send sensor data to the client
BLE2902 *pBLE2902_1;

BLECharacteristic* pCharacteristic_2 = NULL;
BLE2902 *pBLE2902_2;

bool deviceConnected = false; // a flag to track whether a client is currently connected to the esp32 via ble
bool oldDeviceConnected = false; // stores the previous connection state
uint32_t value = 0; // an integer used to store and send sensor values


// Generating UUIDs:
#define SERVICE_UUID        "296728af-7101-479d-be0d-0831dbf93239"
#define CHARACTERISTIC_UUID_1 "a968211d-49cc-40ed-8125-4900d1d75b8b"

#define CHARACTERISTIC_UUID_2 "993b523c-1346-4009-878a-7c9828bf043d"

 

// call back classes used to tracks when a ble client connects/disconnects
class MyServerCallbacks: public BLEServerCallbacks {
  // this func is called when a client connects to the server 
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Client Connected");
  };

  // this func is called when a client disconnects from the server
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Client Disconnected");
  }
};

// this class handles events related to our specific characteristics (reads/writes)
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks{
  // this function is called when a client tries to write a value to our characteristic
  void onWrite(BLECharacteristic *pCharacteristic_1){
    std::string rxValue = pCharacteristic_1->getValue(); // this gets value the client wrote
    
    if (rxValue.length() > 0) { // Check if any data was actually sent
        Serial.print("Received Value from Client: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]); // Print each character of the received string
        Serial.println();
    }
  }

  // This function is called when a client tries to READ the value from our characteristic.
  void onRead(BLECharacteristic *pCharacteristic_1) {
    Serial.print("Client requested to READ. Current value: ");
    Serial.println(pCharacteristic_1->getValue().c_str());
  }
};


// initalize serial communication for debugging
void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Server");

  // Create the BLE Device
  BLEDevice::init("ESP32_SERVER");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic_1 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_1,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic_2 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_2,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );                   

  // Set callbacks
  // these characteristics are called to esp32 when a client reads or writes characteristics
  pCharacteristic_1->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic_2->setCallbacks(new MyCharacteristicCallbacks());

  // Set initial values
  pCharacteristic_1->setValue("whats up mate (from the server)");
  pCharacteristic_2->setValue("0");

  // Enable notifications (only one descriptor per characteristic)
  pBLE2902_1 = new BLE2902();
  pBLE2902_1->setNotifications(true);
  pCharacteristic_1->addDescriptor(pBLE2902_1);

  pBLE2902_2 = new BLE2902();
  pBLE2902_2->setNotifications(true);
  pCharacteristic_2->addDescriptor(pBLE2902_2);
  


  // starting the service
  pService->start();

  // Start advertising
  // this makes the esp32 visible to other BLE devies (clients) that are scanning
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); // advertise our custom services UUID
  pAdvertising->setScanResponse(true); // allow clients to get more info (like device name)
  pAdvertising->setMinPreferred(0x60);  // sets min advertising interval
  pAdvertising->setMaxPreferred(0x80); // set max advertising interval
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // checks whether a client is currently connected to the esp32
  if (deviceConnected) {
    
    std::string message = "HEY";
    pCharacteristic_1->setValue(message); // sending a message to the client
    pCharacteristic_1->notify();
    
    // value is a uint32_t variable and increments value by 1
    value++;
    pCharacteristic_2->setValue(value);  
    pCharacteristic_2->notify();


    Serial.print("New value notified: ");
    Serial.println(value);

    Serial.print("message sent: ");
    Serial.println(message.c_str());

    delay(1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    Serial.println("Device disconnected.");
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
    Serial.println("Device Connected");
  }
}
