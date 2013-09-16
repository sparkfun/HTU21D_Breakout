/* 
  HTU21D Humidity Sensor Example Code
  By: Nathan Seidle
  SparkFun Electronics
  Date: September 15th, 2013
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Get pressure and temperature from the HTU21D sensor.
  
  Serial.print it out at 9600 baud to serial monitor.
*/

#include <Wire.h>

#define HTDU21D_ADDRESS 0x80  // 7-bit I2C address for the sensor

int temperature; //14 bit max
int humidity; //12 bit max

void setup()
{
  Serial.begin(9600);
  Wire.begin();
}

void loop()
{
//  temperature = bmp085GetTemperature(bmp085ReadUT());
//  pressure = bmp085GetPressure(bmp085ReadUP());
  Serial.print("Temperature: ");
  Serial.print(temperature, DEC);
  Serial.println(" *0.1 deg C");
  Serial.print("Humidity: ");
  Serial.print(humidity, DEC);
  Serial.println(" %");
  Serial.println();
  delay(1000);
}


// Read the uncompensated temperature value
unsigned int htdu21d_readTemp()
{
  unsigned int ut;
  
  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x2E);
  Wire.endTransmission();
  
  // Wait at least 4.5ms
  delay(5);
  
  // Read two bytes from registers 0xF6 and 0xF7
  //ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated humidity value
unsigned long htdu21d_readHumidity()
{
  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;
  
  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(0xF4);
  //Wire.write(0x34 + (OSS<<6));
  Wire.endTransmission();
  
  // Wait for conversion, delay time dependent on OSS
  //delay(2 + (3<<OSS));
  
  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(0xF6);
  Wire.endTransmission();
  Wire.requestFrom(HTDU21D_ADDRESS, 3);
  
  // Wait for data to become available
  while(Wire.available() < 3)
    ;
  msb = Wire.read();
  lsb = Wire.read();
  xlsb = Wire.read();
  
  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8);
  
  return up;
}

