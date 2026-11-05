#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

//MIMICING front ESP32, COM6, rear_node

uint32_t startTime = 0;
volatile bool recording = false;
int counter = 0;

uint8_t gatewayAddress[] = {0x08, 0x3A, 0xF2, 0x39, 0x0D, 0x08};

typedef struct struct_message {
  uint8_t id;
  uint32_t elapsed;
  float value;
} struct_message;

struct_message outgoingMessage;

esp_now_peer_info_t peerInfo;

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if(len == 1 && incomingData[0] == 1 && !recording) {
    Serial.println("Received start command"); 
    startTime = millis();
    recording = true;
    counter = 0;
  } 
  else if(incomingData[0] == 2 && recording) {
    recording = false;
    Serial.println("Stopped Recording");
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void peerSetup();
void fillData();
void sendTask(void *pvParameters);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  peerSetup();

  xTaskCreatePinnedToCore(
    sendTask,          
    "Send Task",      
    4096,             // stack size
    NULL,             
    1,                // priority of the task
    NULL,            
    1                 // core
  );
}

void loop() {

}

void peerSetup() {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, gatewayAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void fillData(){
  outgoingMessage.id = 2;
  outgoingMessage.elapsed = millis() - startTime;
  outgoingMessage.value = counter++; 
}

void sendTask(void *pvParameters) {
  while (true) {
    if (recording) {
      fillData();
      
      esp_now_send(gatewayAddress, (uint8_t *)&outgoingMessage, sizeof(outgoingMessage));
    }
    vTaskDelay(pdMS_TO_TICKS(100)); //controls the send interval every 100ms
  }
}