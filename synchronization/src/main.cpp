#include <Arduino.h>
#include <esp_wifi.h>
#include <WiFi.h>

void setup() {
  Serial.begin(921600);


  /*
  
  WiFi.begin();

  Serial.print("Esp32 board address: ");

  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("Failed to read MAC address");
  }

  */

}

void loop() {
  Serial.println("1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    
}