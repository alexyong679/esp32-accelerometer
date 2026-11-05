#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>


// STRUCT & GLOBAL VARIABLES -------------------
typedef struct struct_message {
  char id[16];
  float accel_x, accel_y, accel_z;
  float gyro_x, gyro_y, gyro_z;
  float roll, pitch, yaw;
} struct_message;

QueueHandle_t sensorQueue; 
// it is a pointer in a queue

BLECharacteristic* pCharacteristic = NULL; // this is a pointer that will notify/send sensor data to the client
bool deviceConnected = false; // a flag to track whether a client is currently connected to the esp32 via ble

// ---- ESPNOW CALLBACK -----------------------------
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  struct_message msg;
  memcpy(&msg, incomingData, sizeof(msg));
  xQueueSend(sensorQueue, &msg, 0); // this sends to freertos queue
  // queue handle, pointer to the data to send, wait time(0 means dont wait if queue is full)
}

// ---- BLE Setup -------------------------------------
class MyServerCallbacks: public BLEServerCallbacks{
  void onConnect(BLEServer* pServer){
    deviceConnected = true;
    Serial.println("BLE device connected");
  }

  void Disconnect(BLEServer* pServer){
    deviceConnected = false;
    Serial.println("BLE device disconnected");
  }
};

void setupBLE(){
  BLEDevice::init("BLE_Server");
  BLEServer *pServer = BLEDevice::createServer();
  pServer-> setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService("296728af-7101-479d-be0d-0831dbf93239");

  pCharacteristic = pService->createCharacteristic(
    "a968211d-49cc-40ed-8125-4900d1d75b8b",
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

}

// ----- task 1 esp now reciever --------

void taskEspReciever (void * pvParameters){
  //intiallizing espnow
  WiFi.mode(WIFI_STA);
  if(esp_now_init() != ESP_OK){
    Serial.println("ESP-NOW init failed");
    vTaskDelete(NULL);
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW ready");

  while(1){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //makes task yeild cpu for 1 second (prevent the task hogging the cpu)
  }
}

// ------ task 2 ble sender -------------

void taskBLESender(void * pvParameters){
  setupBLE();

  struct_message msg; // temporary variable to hold data recieved from the queue
  
  //this waits for new data in the queue
  while(1){
    if (xQueueReceive(sensorQueue, &msg, portMAX_DELAY)){
      if(deviceConnected){
        char payload[64];
        snprintf(payload, 
          sizeof(payload), 
          "Yaw: %.2f Pitch: %.2f Roll: %.2f", 
          msg.id,
          msg.yaw, msg.pitch, msg.roll);

        pCharacteristic->setValue(payload);
        pCharacteristic->notify();
        Serial.println("Sent BLE notification");
      }
    }
  }

}

void setup(){
  Serial.begin(115200);

  //this creates a freertos queue named sensorqueue(number of items, item size in bytes)
  sensorQueue = xQueueCreate(20, sizeof(struct_message)); //20 == max num of items the queu can hold
  if(sensorQueue == NULL){
    Serial.println("Queue creation failed!");
    while(1);
  } 
  //this checks if queue was created successflly


  //this func creates and starts a task and binds it to a specific cpu core 1 or 0
  xTaskCreatePinnedToCore(
    taskEspReciever, //function name
    "ESPNOW_RX", //task name
    4096, //stack size (in bytes) & increase for more complex func
    NULL, //task parameters
    2, //task priority
    NULL, //task handle
    1 //pin to core 1
  );

   xTaskCreatePinnedToCore(
    taskBLESender, 
    "BLE_TX", 
    4096,
    NULL,
    1,
    NULL,
    1
  );
}

void loop(){
  
}


