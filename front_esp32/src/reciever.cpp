#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define ACCEL_SCALE 2048.0 // +-16g

// structure to match sender
struct WirelessPacket {
    uint8_t tireID;
    int16_t ax;
    int16_t ay;
    int16_t az;
    uint32_t timestamp;
};

WirelessPacket incomingData;

// callback function for receiving
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataBytes, int len) {

  
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));

    float ax = (float)incomingData.ax / ACCEL_SCALE;
    float ay = (float)incomingData.ay / ACCEL_SCALE;
    float az = (float)incomingData.az / ACCEL_SCALE;

    if(incomingData.tireID == 1)
        Serial.println("Front Tire");
    else if(incomingData.tireID == 2)
        Serial.println("Rear Tire");
    else
        Serial.println("Unknown Tire");

    Serial.print("Ax: ");
    Serial.print(ax);

    Serial.print(" Ay: ");
    Serial.print(ay);

    Serial.print(" Az: ");
    Serial.println(az);

    Serial.print("Timestamp: ");
    Serial.println(incomingData.timestamp);

    Serial.println("----------------");
 
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



