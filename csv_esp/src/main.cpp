#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFi.h>

//esp access point set up
const char* ssid = "esp_wifi_meow";
const char* password = "12345678";

//server's ip address and port
const char* server_ip = "192.168.1.72"; //computer ip
const int server_port = 12334; 

//global variables
WiFiClient client;

void setup() {
  Serial.begin(115200);

  //access point 
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, 1);
  Serial.print("ESP32 AP IP: ");
  Serial.println(WiFi.softAPIP());

  delay(1000);

}

void loop() {
  Serial.println("Trying to connect to server...");

  //connecting to server
  if (client.connect(server_ip, server_port)){
    Serial.println("Connected to server");

    // Send header once
    client.println("timestamp,acc_x,acc_y,acc_z");

    // Simulate 10 rows of sensor data
    for (int i = 0; i < 10; i++) {
      float ts = i * 0.01;
      float ax = 0.1 * i;
      float ay = 0.2 * i;
      float az = 9.81;

      String line = String(ts, 2) + "," + String(ax, 3) + "," + String(ay, 3) + "," + String(az, 3);
      client.println(line);
      delay(100);

    }

    client.stop();
    Serial.println("Disconnected from server");

  }

  else{
    Serial.println("Failed to connect to server");
  }
  delay(10000); //wait for 10 sec before trying again

  

}

