#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <WiFi.h>
#include "LittleFS.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>



//network credentials
const char* ssid = "TELUS9086"; // need to change acording to diff wifi
const char* password = "647KhKmG8mtB";

//creates a webserver to handle multple client conenctions without blocking main program loop
AsyncWebServer server(80); //or change to 8080 if have multiple esp32

//host web pages (html, css, js) from esp32 to web browser
AsyncEventSource events("/events"); 

//timer vairables
unsigned long lastTimeAngle = 0; // for angle updates
unsigned long lastTimeAcc = 0; //for accelerometer updates
unsigned long gyroDelay = 10; // send angles evry 10ms
unsigned long accelerometerDelay = 200; // send accelerometer data evry 200ms



//ACCEROMETER CONSTANTS
const float ACCEL_SCALE = 16384.0;    // For ±2g (default)
const float GYRO_SCALE = 131.0;       // For ±250 deg/s

// dt global vairables;
unsigned long last_time = 0;
float dt = 0.0;

// Global variables for raw data
int16_t accel_x_raw, accel_y_raw, accel_z_raw;
int16_t gyro_x_raw, gyro_y_raw, gyro_z_raw;

// Global variables for real units
float accel_x, accel_y, accel_z;
float gyro_x, gyro_y, gyro_z;

// Angle estimates
float pitch_accel, roll_accel;

// Complimentary filter coefficient
float alpha = 0.98;
float pitch = 0.0;
float roll = 0.0;
float yaw = 0.0; 

// gyroscope calibration coefficients
float offsetX;
float offsetY;
float offsetZ;

//gyrosocope calibration coefficients:
float new_gyro_x;
float new_gyro_y;
float new_gyro_z;

void read_raw_accel();
void read_raw_gyro();
void compute_scaled_data();
void compute_accel_angle();
void complementary_filter();
void gyro_calibration(); 

//json declare to hold sensor readings & holds up to 256 bytes
DynamicJsonDocument angleDoc(256);
DynamicJsonDocument accelDoc(256);
DynamicJsonDocument gyroDoc(256);

//JSON funcs
String getAngleReadings(){
  angleDoc["pitch"] = String(pitch, 3); // Format to 2 decimal places
  angleDoc["roll"] = String(roll, 3);
  angleDoc["yaw"] = String(yaw, 3);
  String jsonString;
  serializeJson(angleDoc, jsonString);
  return jsonString;
}

String getAccReadingsJSON() { // Renamed for clarity, as you had one called getAccReadings earlier
  accelDoc["accX"] = String(accel_x, 3);
  accelDoc["accY"] = String(accel_y, 3);
  accelDoc["accZ"] = String(accel_z, 3);
  String accString;
  serializeJson(accelDoc, accString);
  return accString;
}

void setup() {
  Serial.begin(115200);         // Start serial monitor

  //initalizing accelerometer
  Wire.begin();                 // Start I2C
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);             // Power management register
  Wire.write(0);                // Wake up MPU6050
  Wire.endTransmission(true);
  delay(100);                   // Wait for sensor to stabilize
  
  // perform gyroscope calibration to find offsets
  Serial.println("Starting Gyroscope Calibration...");
  gyro_calibration();
  Serial.println("Gyroscope Calibration Complete.");

  //connecting to wifi
  WiFi.begin(ssid, password);
  Serial.println("connecting to wifi... ");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print('.');
    delay(1000);
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  //initalize littlefs
  //this is a file system used to store html,css & js directly on the esp32 flash memory
  if (!LittleFS.begin()){
    Serial.println("An error has occured while mounting LittleFS");
    while(true) { // Halt execution if LittleFS fails
      delay(1000);
    }
  }
  Serial.println("LittleFS mounted successfully");

  //Handling web server
  //serve index.html from littlefs when root path is requested
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  //serve all other static files (css, js) from littlefs
  server.serveStatic("/", LittleFS, "/");

  // reset calculated imu angles back to zero
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    pitch=0;
    roll=0;
    yaw=0;
    //having a rest button allows to zero out the angles, effectivel recalibrating their reference point, without needing to reset the esp32
    request->send(200, "text/plain", "OK"); // sends http response back to client
  });

  //reset each position individually
  server.on("/resetX", HTTP_GET, [](AsyncWebServerRequest *request){
    pitch=0;
    request->send(200, "text/plain", "OK");
  });

  server.on("/resetY", HTTP_GET, [](AsyncWebServerRequest *request){
    roll=0;
    request->send(200, "text/plain", "OK");
  });

  server.on("/resetZ", HTTP_GET, [](AsyncWebServerRequest *request){
    yaw=0;
    request->send(200, "text/plain", "OK");
  });


  //Server sent events handling
  // registers a callbakc function that is executed every time a new client successfully connects to the SSE endpoint (/events)
  events.onConnect([](AsyncEventSourceClient *client){ 
    //eventsource in hs remebers the id of the last event it recieved
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // Send an initial message when a client connects
    client->send("Hello from ESP32!", NULL, millis(), 10000);
  });
  server.addHandler(&events); // register the event source with the web server

  server.begin();
  Serial.println("HTTP server started");
  
}


void loop(){
  // Time calculation for dt
  unsigned long current_time = millis(); // current time in ms
  dt = (current_time - last_time) / 1000.0; // convert to seconds
  last_time = current_time;

  //sensor data
  read_raw_accel();
  read_raw_gyro();
  compute_scaled_data();
  compute_accel_angle();
  complementary_filter();

  // send Angles (Pitch, Roll, Yaw)
  if ((millis() - lastTimeAngle) > gyroDelay) {
    events.send(getAngleReadings().c_str(),"angles",millis());
    lastTimeAngle = millis();
  }

  // send Accelerometer Readings
  if ((millis() - lastTimeAcc) > accelerometerDelay) {
    events.send(getAccReadingsJSON().c_str(),"accel",millis());
    lastTimeAcc = millis();
  }

  //delay(10); // Slow down prints
  // use milis function instead of delay 
}

void read_raw_accel() {
  Wire.beginTransmission(0x68);
  Wire.write(0x3B); // Correct accel register (0x3B, not 0x3F)
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6);

  accel_x_raw = (Wire.read() << 8) | Wire.read();
  accel_y_raw = (Wire.read() << 8) | Wire.read();
  accel_z_raw = (Wire.read() << 8) | Wire.read();
}

void read_raw_gyro() {
  Wire.beginTransmission(0x68);
  Wire.write(0x43); // Gyro register
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6);

  gyro_x_raw = (Wire.read() << 8) | Wire.read();
  gyro_y_raw = (Wire.read() << 8) | Wire.read();
  gyro_z_raw = (Wire.read() << 8) | Wire.read();
}

// converting raw data to physical units
void compute_scaled_data() {
  accel_x = (float)accel_x_raw / ACCEL_SCALE;
  accel_y = (float)accel_y_raw / ACCEL_SCALE;
  accel_z = (float)accel_z_raw / ACCEL_SCALE;

  gyro_x = (float)gyro_x_raw / GYRO_SCALE;
  gyro_y = (float)gyro_y_raw / GYRO_SCALE;
  gyro_z = (float)gyro_z_raw / GYRO_SCALE;

  // calibrated gyro values
  // would it be better to just put the values or use this code?
  // Offsets: 1.54 | -0.50 | 1.01 (x, y, z)
  new_gyro_x = gyro_x - offsetX;
  new_gyro_y = gyro_y - offsetY;
  new_gyro_z = gyro_z - offsetZ;

}

//calculating angle from  accelerometer (tilt)

// check whether atan or atan2
void compute_accel_angle(){
  pitch_accel = atan(-accel_x/pow(pow(accel_y, 2) + pow(accel_z, 2), 0.5)) * 180 / PI;
  roll_accel = atan(accel_y / pow(pow(accel_x, 2) + pow(accel_z, 2), 0.5)) * 180 / PI;
}

// complimentary filter for angle estimation
void complementary_filter() {
  pitch = alpha * (pitch + new_gyro_x * dt) + (1-alpha) * pitch_accel;
  roll  = alpha * (roll + new_gyro_y * dt) + (1-alpha) * roll_accel; 
  yaw = yaw + new_gyro_z * dt;
}

// Calculating IMU error
void calculateIMUError(){
  
  int numReadings = 200;
  float accErrorX = 0, accErrorY = 0;
  float gyroErrorX = 0, gyroErrorY = 0, gyroErrorZ = 0;

  // Take multiple readings & accumulate them
  for (int i = 0; i < numReadings; i++){
    read_raw_accel();
    read_raw_gyro();
    compute_scaled_data();

    accErrorX += atan(accel_y / sqrt(pow(accel_x, 2) + pow(accel_z, 2))) * 180 / PI;
    accErrorY += atan(-1 * accel_x / sqrt(pow(accel_y, 2) + pow(accel_z, 2))) * 180 / PI;

    gyroErrorX += new_gyro_x;
    gyroErrorY += new_gyro_y;
    gyroErrorZ += new_gyro_z;

    delay(5);

  }

  accErrorX /= 200.0;
  accErrorY /= 200.0;
  gyroErrorX /= 200.0;
  gyroErrorY /= 200.0;
  gyroErrorZ /= 200.0;

  Serial.println("IMU Error Calibration Done:");
  Serial.print("AccErrorX: "); Serial.println(accErrorX, 4);
  Serial.print("AccErrorY: "); Serial.println(accErrorY, 4);
  Serial.print("GyroErrorX: "); Serial.println(gyroErrorX, 4);
  Serial.print("GyroErrorY: "); Serial.println(gyroErrorY, 4);
  Serial.print("GyroErrorZ: "); Serial.println(gyroErrorZ, 4);

}

//gyro calibration calculation
void gyro_calibration(){
  long startTime = millis();
  const long calibrationTime = 10000; // 10 seconds
  long endTime = startTime + calibrationTime;

  long count = 0;
  float sumX = 0;
  float sumY = 0;
  float sumZ = 0;

  while (millis() < endTime){
    read_raw_gyro();
    compute_scaled_data();

    sumX += gyro_x;
    sumY += gyro_y;
    sumZ += gyro_z;
    count++;

    if (count % 100 == 0){
      Serial.println("Still calibrating... points so far: " + count);
    }
  }
  
  // Compute average offset (bias)
  offsetX = sumX / count;
  offsetY = sumY / count;
  offsetZ = sumZ / count;

  /*
  Serial.println("Gyro Calibration Complete!");
  Serial.print("Offsets: ");
  Serial.print(offsetX, 2); Serial.print(" | ");
  Serial.print(offsetY, 2); Serial.print(" | ");
  Serial.println(offsetZ, 2);

  // Offsets: 1.54 | -0.50 | 1.01
  */
}

