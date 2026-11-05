#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// data format recieving from esp32s
typedef struct struct_message {
  uint32_t timestamp;
  char id[16];
  //float accel_x, accel_y, accel_z;
  //float gyro_x, gyro_y, gyro_z;
  float roll, pitch, yaw;
  float gyro_z, new_gyro_z;
  //float gyro_z_diff;
  float accel_mag;
} struct_message;

struct_message incomingData;

unsigned long startTime;

// callback function for receiving data from other esps using espnow
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataBytes, int len) {

  
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));


  //Serial.println(incomingData.id);

  /*
  Serial.print(incomingData.pitch);
  Serial.print(" / "); Serial.print(incomingData.roll);
  Serial.print(" / "); Serial.print(incomingData.yaw);

  Serial.print(" / ");
  Serial.print(incomingData.gyro_x);
  Serial.print(" / "); Serial.print(incomingData.gyro_y);
  Serial.print(" / "); Serial.print(incomingData.gyro_z);

  Serial.print(" / ");
  Serial.print(incomingData.accel_x);
  Serial.print(" / "); Serial.print(incomingData.accel_y);
  Serial.print(" / "); Serial.println(incomingData.accel_z);

  */

  
  unsigned long t = millis() - startTime;
  Serial.println(incomingData.id); 
  //Serial.print("/"); 
  Serial.print(t); 
  //Serial.print("/"); Serial.print(incomingData.new_gyro_z);
  Serial.print("/"); Serial.print(incomingData.gyro_z);
  Serial.print("/"); Serial.println (incomingData.accel_mag);
  
}

void setup() {
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  //function called automatically every time a packet arrives over espnow
  esp_now_register_recv_cb(OnDataRecv);
  //Serial.println("Receiver ready!");

  startTime = millis();
  
}



void loop() {
  

}

