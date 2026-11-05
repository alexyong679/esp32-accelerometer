#include <WiFi.h>
#include <WiFiClient.h>

//wifi netowrk detals
const char* ssid = "TELUS9086";
const char* password = "647KhKmG8mtB";

//server's ip address and port
const char* server_ip = "192.168.1.72";
const int server_port = 12334;  //need to change port num

WiFiClient client;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi from router 
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA); 



  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(WiFi.status());

  //printing out ip
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  //connecting to server
  if (client.connect(server_ip, server_port)){
    Serial.println("Connected to server");

    //sending data
    client.println("Hello, server!");
    Serial.println("Data sent");
    delay(100);

    //Recieve response
    while(client.available()){
      String response = client.readStringUntil('\n'); //read until newline
      Serial.print("Server: ");
      Serial.println(response);
    }

    //Disconnect
    client.stop();
    Serial.println("Disconnected from server");
  }
    
  else{
    Serial.println("Failed to connect to server");
    Serial.println(WiFi.status());
    delay(1000); // Shorter delay for retry
  }
  delay(1000); //wait for 10 sec before trying again
  

}


