#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

/*
need to define broadcast address (need to make multiples files for 
transmitter & reciever or check if can just do same code for both)
need to define variables for accelerometer

need to define data structure
*/

// define data sturcture
typedef struct struct_message{

} struct_message;

//registe peer
esp_now_peer_info_t peerInfo;

//sent data callback function
void OnDataSent(const uint8_t *macAddr, esp_now_send_status_t status){
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

}


void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength){
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2]);
}

// this is called when data is recieved 
void recieveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen){

}

// this is called when data is sent
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status){
  
}

// emulates broad cast
void broadcast (const String &message){

  //broadcast a message to every device in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress)){
    esp_now_add_peer(&peerInfo);
  }

  //send message
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());

  // print results to serial monitor
  if (result == ESP_OK){
    Serial.println("Broadcast message success");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_INIT){
    Serial.println("ESP-Now not Init");
  }

}

void setup() {
  Serial.begin(115200);  

  WiFi.mode(WIFI_MODE_STA);
  Serial.println("ESP-now broadcast on");

  //printing mac address to serial monitor
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  //disconnect from wifi
  WiFi.disconnect();

  //initalize ESP-now
  if (esp_now_init() == ESP_OK){
    Serial.println("Esp-now initalization success");
    esp_now_register_recv_cb(recieveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else{
    Serial.println("esp-now init failed");
  }

}
void loop() {

}

  
