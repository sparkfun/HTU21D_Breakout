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

#define TRIGGER_TEMP_MEASURE_HOLD  0xE3
#define TRIGGER_HUMD_MEASURE_HOLD  0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD  0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD  0xF5
#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE

int temperature; //14 bit max
int humidity; //12 bit max

void setup()
{
  Serial.begin(9600);
  Wire.begin();
}

void loop()
{
  temperature = htdu21d_readTemp();
  //humidity = htdu21d_readHumidity();
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
int htdu21d_readTemp()
{
  int temperature;
  
  // Request the temperature
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(TRIGGER_TEMP_MEASURE_NOHOLD);
  Wire.endTransmission();
  
  //Wait for sensor to complete measurement
  delay(50); //44-50 ms max

  // Comes back in three bytes, data(MSB) / data(LSB) / CRC
  Wire.requestFrom(HTDU21D_ADDRESS, 3);

  unsigned char msb, lsb;
  msb = Wire.read();
  lsb = Wire.read();
  
  temperature = msb << 8;
  temperature |= lsb;
  
  temperature >>= 2; //Shift right by two to remove the status info bits - don't care
  
  return temperature;
}

// Read the humidity
int htdu21d_readHumidity()
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

