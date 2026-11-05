#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h> 

// variables for test
int int_value;
float float_value;
bool bool_value = true;

//broadcast address: mac address of responder
uint8_t broadcastAddress[] = {0x08, 0x3A, 0xF2, 0x30, 0xC6, 0x48};

// defining a data structure
typedef struct struct_message{
  char a[32];
  int b;
  float c;
  bool d;
} struct_message;

// create a structured object
struct_message myData;

//peer info
esp_now_peer_info_t peerInfo;

//callback function when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);

  //setting up wifi station
  WiFi.mode(WIFI_STA);

  //initalizing espnow
  if (esp_now_init() != ESP_OK){
    Serial.println("Error initalzing ESP-Now");
    return;
  }

  // register the send callback
  esp_now_register_send_cb(OnDataSent);

  // register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  //add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  
}

void loop() {

  // creating test data

  int_value = random(1,20);

  float_value = 1.3*int_value;

  bool_value = !bool_value;

  strcpy(myData.a, "welcome lol");
  myData.b = int_value;
  myData.c = float_value;
  myData.d = bool_value;

  //send message via esp-now
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK){
    Serial.println("Sending confirmed");
  }
  else{
    Serial.println("Sending error");
  }
  delay(2000);

}

