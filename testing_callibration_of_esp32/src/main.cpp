#include <Arduino.h>
#include <Wire.h>

//global variables for raw data
float accel_x, accel_y, accel_z;
float gyro_x, gyro_y, gyro_z;

void read_accel();
void read_gyro();
void gyro_callibration();

#define SAMPLE_INTERVAL_MS 500 //milliseconds

//this runs once to setup the mpu6050
void setup() {
  Serial.begin(115200);
  Wire.begin(); //intialize communication
  Wire.setClock(400000L); //increases i2c bus speed (read sensor data faster)

  //start communication with mpu6050
  Wire.beginTransmission(0x68); //0x68 is the 7-bit address of the mpu6050
  Wire.write(0x6B); //talks to the register 6B
  Wire.write(0x00);
  Wire.endTransmission(true);
  delay(100); //delay added to wait for sensor to stabilize before running

  gyro_callibration();
}

//this runs continously to read data
void loop() {
  read_accel();
  read_gyro();

  Serial.print(">");
  Serial.print("accel x:");
  Serial.print(accel_x);
  Serial.print(", ");

  Serial.print("accel y:");
  Serial.print(accel_y);
  Serial.print(", ");

  Serial.print("accel z:");
  Serial.print(accel_z);
  Serial.println();
  
  /*
  Serial.print("Gyro values: ");
  Serial.print(gyro_x); Serial.print("/");
  Serial.print(gyro_y); Serial.print("/");
  Serial.println(gyro_z);

  */


}

//Processing Accelerometer Code

//using a range of +- 2g
void read_accel(){
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6);

  int16_t raw_accel_x = (Wire.read() << 8) | Wire.read(); // combines two 8-bit to 16-bit
  int16_t raw_accel_y = (Wire.read() << 8) | Wire.read();
  int16_t raw_accel_z = (Wire.read() << 8) | Wire.read();

  accel_x = raw_accel_x / 16384.0;
  accel_y = raw_accel_x / 16384.0;
  accel_z = raw_accel_x / 16384.0;
}

void read_gyro(){
  Wire.beginTransmission(0x68);
  Wire.write(0x43); // the gyro's register
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6);

  int16_t raw_gyro_x = (Wire.read() << 8) | Wire.read();
  int16_t raw_gyro_y = (Wire.read() << 8) | Wire.read();
  int16_t raw_gyro_z = (Wire.read() << 8) | Wire.read();

  gyro_x = raw_gyro_x / 131.0;
  gyro_y = raw_gyro_y / 131.0;
  gyro_z = raw_gyro_z / 131.0;
}

void gyro_callibration(){
  float offsets[3] = {0.0, 0.0, 0.0};
  int num_of_points = 0;
  long startTime = millis();
  const unsigned long callibrationTime = 4000;

  while ((millis() - startTime) < callibrationTime){
    num_of_points += 1;
    read_gyro();

    offsets[0] += gyro_x;
    offsets[1] += gyro_y;
    offsets[2] += gyro_z;

    //show its still callibrating
    if (num_of_points % 100 == 0){
      Serial.print("Still callibrating Gyro... ");
      Serial.print(num_of_points);
      Serial.println(" points so far");
    }
  }

  float offsetsX = offsets[0]/num_of_points;
  float offsetsY = offsets[1]/num_of_points;
  float offsetsZ = offsets[2]/num_of_points;

  Serial.print("callibration is complete: ");
  Serial.println(num_of_points);


  
  Serial.print(offsetsX);
  Serial.print(", ");
  Serial.print(offsetsY);
  Serial.print(", ");
  Serial.print(offsetsZ);

}

/*

void linear_accel_callibration(){
  const unsigned long callibrationTime = 4000;  
  int num_of_points = 0;
  int x_sum = 0;
  int y_sum = 0;
  int x_squared_sum = 0;
  int y_squared_sum = 0;
  long startTime = millis();

  while ((millis() - startTime) < callibrationTime){
    num_of_points += 1;
     
  }


}

*/


