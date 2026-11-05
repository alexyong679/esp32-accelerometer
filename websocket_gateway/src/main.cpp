#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// -----------------------------------
// ESP-NOW Struture
// -----------------------------------
typedef struct struct_message {
  uint32_t timestamp;
  char id[16];
  float accel_x, accel_y, accel_z;
  float roll, pitch, yaw;
} struct_message;

struct_message incomingData;

// ---------------------------------
// ESP WIFI credentials
// ---------------------------------

//constants
const char *ssid = "esp_connect_pls";
const char *password = "12345678";

//globals
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// ---------------------------------
// FreeRtos Handling
// --------------------------------------
QueueHandle_t sensorQueue; //pointer in a queue

void createQueue(){
  //this creates a freertos queue named sensorqueue(number of items, item size in bytes)
  sensorQueue = xQueueCreate(10, sizeof(struct_message)); //10 == max num of items the queue can hold
  if(sensorQueue == NULL){
    Serial.println("Queue creation failed!");
    while(1);
  } 
  //this checks if queue was created successflly
}

// ----------------------------------
// ESP-NOW calback function for recieiving data
// -----------------------------------

// Declare JSON documents globally
DynamicJsonDocument angleDoc(512);
DynamicJsonDocument accelDoc(512);

// JSON function for angles
String getAngleReadings(const char* espID, const struct_message& data) {
  angleDoc.clear();
  angleDoc["esp"] = espID;
  angleDoc["type"] = "angles";
  angleDoc["pitch"] = data.pitch;
  angleDoc["roll"] = data.roll;
  angleDoc["yaw"] = data.yaw;
  
  String jsonString;
  serializeJson(angleDoc, jsonString);
  return jsonString;
}

// JSON function for accelerometer
String getAccReadingsJSON(const char* espID, const struct_message& data) {
  accelDoc.clear();
  accelDoc["esp"] = espID;
  accelDoc["type"] = "accel";
  accelDoc["accX"] = data.accel_x;
  accelDoc["accY"] = data.accel_y;
  accelDoc["accZ"] = data.accel_z;
  
  String accString;
  serializeJson(accelDoc, accString);
  return accString;
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataBytes, int len) {
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));

  Serial.println("Received data:");
  Serial.println(incomingData.id);


  xQueueSend(sensorQueue, &incomingData, 0); // this sends to freertos queue
  // queue handle, pointer to the data to send, wait time(0 means dont wait if queue is full)

  /*
  Serial.printf("ID: %s, Pitch: %.2f, Roll: %.2f, Yaw: %.2f\n",
                incomingData.id, incomingData.pitch, incomingData.roll, incomingData.yaw);
  */
}


// ------------------------------------
// websocket events
// ------------------------------------

//callback function for recieving messages
void onWebSocketEvent(uint8_t client_num,
                      WStype_t type,
                      uint8_t * payload,
                      size_t length){
  switch (type){
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", client_num);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;
    
    // this case lets messages exhcange back and fourth
    case WStype_TEXT:
      Serial.printf("[%u] Received Text: %s\n", client_num, payload);
      break;
  }
}

// -------------------------------------
// http request handlers
// -------------------------------------

//callback send homepage
void onIndexRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request-> url());
  request->send(LittleFS, "/index.html", "text/html");
}

//callback send css
void onCSSRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request-> url());
  request->send(LittleFS, "/style.css", "text/css");
}

/*
//callback js
void onJSRequest(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request-> url());
  request->send(LittleFS, "/main.js", "application/javascript");
}
*/

//callback: send 404 if requested file does not exist
void onPageNotFound(AsyncWebServerRequest *request){
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request-> url());
  request->send(404, "text/plain", "Not Found");
}

// -----------------------------------------
// freeRtos tasks
// -----------------------------------------

//task 1 esp now reciever

void taskEspReciever (void * pvParameters){
  //initalzing espnow
  //WiFi.mode(WIFI_AP_STA);

  if(esp_now_init() != ESP_OK){
    Serial.println("ESP-NOW init failed");
    vTaskDelete(NULL);
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW ready");

  while(1){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //makes task yeild cpu for 1 second (prevent the task hogging the cpu)
  }
}

//task 2 websocket sender

void taskWebSocketSender(void *pvParameters){
  struct_message msg;

  while(1){
    if (xQueueReceive(sensorQueue, &msg, portMAX_DELAY)){
      //this is differentatie between diff esps and data recieved
      if (strcmp(msg.id, "A") == 0) {
        String jsonAnglesA = getAngleReadings("A", msg);
        webSocket.broadcastTXT(jsonAnglesA);

        String jsonAccA = getAccReadingsJSON("A", msg);
        webSocket.broadcastTXT(jsonAccA);

      } else if (strcmp(msg.id, "B") == 0) {
        String jsonAnglesB = getAngleReadings("B", msg);
        webSocket.broadcastTXT(jsonAnglesB);

        String jsonAccB = getAccReadingsJSON("B", msg);
        webSocket.broadcastTXT(jsonAccB);

      }
    }
  }
}


void setup() {
   Serial.begin(115200);

  //Starting access point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password, 1);

  //print out ip address
  Serial.println();
  Serial.println("My IP address: ");
  Serial.println(WiFi.softAPIP()); //returns ip address assigned to esp32 acces point interface

  //to read the file system
  if (!LittleFS.begin()){
    Serial.println("Error mounting littlfs");
    while (1);
  }

  Serial.println("LittleFS mounted successfully");

  //webserver handles
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/style.css", HTTP_GET, onCSSRequest);
  //server.on("/main.js", HTTP_GET, onJSRequest);
  server.serveStatic("/", LittleFS, "/");  // Serve everything else automatically
  server.onNotFound(onPageNotFound);
  server.begin(); //Start web server

  //Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  createQueue();

  //this func creates and starts a task and binds it to a specific cpu core 1 or 0
  xTaskCreatePinnedToCore(
    taskEspReciever, //function name
    "ESPNOW Reciever", //task name
    4096, //stack size (in bytes) & increase for more complex func
    NULL, //task parameters
    2, //task priority
    NULL, //task handle
    0 //pin to core 0
  );

   xTaskCreatePinnedToCore(
    taskWebSocketSender, 
    "WebSocket Sender", 
    8192, // 8192 becuase working with json so needs more bytes
    NULL,
    1,
    NULL,
    1
  );

}

void loop() {
  /*
  Serial.print("ESP32 AP IP Address: ");
  Serial.println(WiFi.softAPIP());
  delay(3000);
  */

  webSocket.loop();
}





