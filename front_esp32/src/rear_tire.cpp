#include <Arduino.h>
#include <Wire.h>
#include <CAN.h>


#define ACCEL_SCALE 4096.0 // +-8g

// CAN Communication 
#define TX_GPIO_NUM   5
#define RX_GPIO_NUM   4

// Global variables for real units
float accel_x_raw, accel_y_raw, accel_z_raw;

void configure_sensitivity();
void read_accel();

void setup() {
  Serial.begin (115200);
  while (!Serial);

  Serial.println ("CAN Sender");

  // Set the pins
  CAN.setPins (RX_GPIO_NUM, TX_GPIO_NUM);

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }

  //initalizing accelerometer
  Wire.begin();                 // Start I2C
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);             // Power management register
  Wire.write(0);                // Wake up MPU6050
  Wire.endTransmission(true);
  delay(100);                   // Wait for sensor to stabilize

  configure_sensitivity();
}

struct SensorData {
    int16_t ax;
    int16_t ay;
    int16_t az;
    uint32_t timestamp;
};

SensorData data;

void loop() {
  read_accel();
  
  data.ax = accel_x_raw;
  data.ay = accel_y_raw;
  data.az = accel_z_raw;
  data.timestamp = millis();

  CAN.beginPacket(0x102); // ID indicating rear tire
  CAN.write((uint8_t*)&data, sizeof(data));
  CAN.endPacket();

  delay(100);
}

void configure_sensitivity(){
  Wire.beginTransmission(0x68);
  Wire.write(0x1C); //talk to the accelerometer configuration register
  Wire.write(0x10); //set the accelerometer sensitivity to +/- 8g
  Wire.endTransmission();
}

void read_accel(){
  Wire.beginTransmission(0x68); 
  Wire.write(0x3B); //start with the accel register
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 3, true); //request 3 bytes of data for x, y, z axes

  accel_x_raw = ((Wire.read() << 8) | Wire.read());
  accel_y_raw = ((Wire.read() << 8) | Wire.read());
  accel_z_raw = ((Wire.read() << 8) | Wire.read());
}