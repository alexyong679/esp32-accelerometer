#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>


// structure to match sender
typedef struct struct_message {
  uint32_t timestamp;
  char id[16];
  float accel_x, accel_y, accel_z;
  float gyro_x, gyro_y, gyro_z;
  float roll, pitch, yaw;
} struct_message;

struct_message incomingData;

// callback function for receiving
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataBytes, int len) {

  
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));

  uint32_t now = millis();

  if (now >= incomingData.timestamp) {
    uint32_t latency = now - incomingData.timestamp;
    Serial.print("Latency: ");
    Serial.print(latency);
    Serial.println(" ms");
  } else {
    Serial.println("Invalid timestamp: receiver time earlier than sender (possible reboot mismatch)");
  }

  Serial.println("Received data:");
  Serial.println(incomingData.id);
  /*
  Serial.print("Acceleration Data: ");
  Serial.println(incomingData.accel_x);
  Serial.print(" / "); Serial.print(incomingData.accel_x);
  Serial.print(" / "); Serial.println(incomingData.accel_x);
  Serial.println("---------------------------");

  Serial.print("Gyro Data: ");
  Serial.println(incomingData.gyro_x);
  Serial.print(" / "); Serial.print(incomingData.gyro_y);
  Serial.print(" / "); Serial.println(incomingData.gyro_z);
  Serial.println("---------------------------");
  */
  Serial.print("Angles Data: ");
  Serial.println(incomingData.pitch);
  Serial.print(" / "); Serial.print(incomingData.roll);
  Serial.print(" / "); Serial.println(incomingData.yaw);
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Receiver ready!");
 
}

void loop() {

}



