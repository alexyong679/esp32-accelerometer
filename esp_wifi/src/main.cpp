#include <Arduino.h>
#include <WiFiMulti.h>

#define WIFI_SSID "mywifi"
#define WIFI_PASSWORD "1234"

WiFiMulti wifiMulti; 



void setup() {
  Serial.begin(921600);

  //this allows multiple access points so if one doesnt work it will try another
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  // blocking part no code executed until microcontroller connected to wifi
  while (wifiMulti.run() != WL_CONNECTED){
    delay(100);
  }

  Serial.println("Connected");

}

void loop() {
  
}

