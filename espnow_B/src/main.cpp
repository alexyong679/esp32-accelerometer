#include <Arduino.h>
#include <esp_now.h>
#include <Wire.h>
#include <WiFi.h>


// macaddress of receiver
uint8_t nextHopAddress[] = {0x08, 0x3A, 0xF2, 0x30, 0xC6, 0x48}; // need to change to correct macaddress (next esp)
esp_now_peer_info_t peerInfo = {};  //declare globally or locally inside setup

//define the data structure
typedef struct struct_message{
  uint32_t timestamp;
  char id[16]; // esp B
  // float accel_x, accel_y, accel_z;
  //float gyro_x, gyro_y, gyro_z;
  float roll, pitch, yaw;
  float gyro_z, new_gyro_z;
  //float gyro_z_diff;
  float accel_mag;
} struct_message;

struct_message data;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 100;

//defining mpu6050 variables:

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

float accel_mag;

void read_raw_accel();
void read_raw_gyro();
void compute_scaled_data();
void compute_accel_angle();
void complementary_filter();
void gyro_calibration();

//callback function for sending
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  //handle send status
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Send OK" : "Send Failed");

}

void fillData(){
  strcpy(data.id, "B");
  data.timestamp = millis();
  
  /*
  data.accel_x = accel_x;
  data.accel_y = accel_y;
  data.accel_z = accel_z;
  data.gyro_x = gyro_x;
  data.gyro_y = gyro_y;
  
  */

  data.pitch = pitch;
  data.roll = roll;
  data.yaw = yaw;
  data.new_gyro_z = new_gyro_z;
  data.gyro_z = gyro_z;
  data.accel_mag = accel_mag;
  
  
}

void setup() {
  //setting esp as a station
  WiFi.mode(WIFI_STA);

  //initalizing accelerometer
  Wire.begin();                 // Start I2C
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);             // Power management register
  Wire.write(0);                // Wake up MPU6050
  Wire.endTransmission(true);
  delay(100);                   // Wait for sensor to stabilize
  
  gyro_calibration();


  //initalizing esp-now
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initilization failed");
    return;
  }
  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // register the gateway as a peer
  memcpy(peerInfo.peer_addr, nextHopAddress, 6);  // Set MAC
  peerInfo.channel = 0;                      // Use 0 to match current Wi-Fi channel
  peerInfo.encrypt = false;                  // Encryption off for simplicity

  // add the peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Peer added. Ready to send!");

}

void loop() {
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

  //fill data struct with current readings
  unsigned long now = millis();
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;
    
    //this copies the device id string into the sender field of the data structure
    fillData();

    // sending the data struct via esp-now
    
    esp_err_t result = esp_now_send(nextHopAddress, (uint8_t *) &data, sizeof(data));

    if (result == ESP_OK){
      Serial.println("Data sent!");
    } else{
      Serial.println("Send failed");
    }  

  }
  
  

  
}

// ACCElEROMETER CODE NOW
// ------------------------------------------------------


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

  accel_mag = sqrt(pow(accel_x, 2) + pow(accel_y, 2) + pow(accel_z, 2));

}

//calculating angle from  accelerometer (tilt)

// check whether atan or atan2
void compute_accel_angle(){
  pitch_accel = atan2(-accel_x, pow(pow(accel_y, 2) + pow(accel_z, 2), 0.5)) * 180 / PI;
  roll_accel = atan2(accel_y, pow(pow(accel_x, 2) + pow(accel_z, 2), 0.5)) * 180 / PI;
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

  }
  
  // Compute average offset (bias)
  offsetX = sumX / count;
  offsetY = sumY / count;
  offsetZ = sumZ / count;

}


/*
  // Forward to C
  esp_now_send(nextHopAddress, (uint8_t*)&incomingData, sizeof(incomingData));
  Serial.println("Forwarded to C");

  */