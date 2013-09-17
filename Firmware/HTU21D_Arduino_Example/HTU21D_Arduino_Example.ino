/* 
 HTU21D Humidity Sensor Example Code
 By: Nathan Seidle
 SparkFun Electronics
 Date: September 15th, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 Get humidity and temperature from the HTU21D sensor.
 
 Serial.print it out at 9600 baud to serial monitor.
 */

#include <Wire.h>

#define HTDU21D_ADDRESS 0x40  // Unshifted 7-bit I2C address for the sensor

#define TRIGGER_TEMP_MEASURE_HOLD  0xE3
#define TRIGGER_HUMD_MEASURE_HOLD  0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD  0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD  0xF5
#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE

float temperature;
float relativeHumidity;

void setup()
{
  Serial.begin(9600);
  Serial.println("HTU21D Example!");

  Wire.begin();
}

void loop()
{
  /*  int SigTemp; //Raw 14 bit max reading
   int SigHumidity; //Raw 12 bit max reading
   
   SigTemp = 0x683A;
   SigHumidity = 0x4E85;
   //temperature = htdu21d_readTemp();
   //humidity = htdu21d_readHumidity();
   
   temperature = calc_temp(SigTemp);
   relativeHumidity = calc_humidity(SigHumidity); //Turn the humidity signal into actual humidity
   
   Serial.print("Temperature: ");
   Serial.print(temperature, 1);
   Serial.print(" C");
   Serial.print(" Relative Humidity: ");
   Serial.print(relativeHumidity, 1);
   Serial.print(" %");*/

  //message = 0xDC, result is 0x79
  //message = 0x683A, result is 0x7C
  //message = 0x4E85, result is 0x6B

  unsigned int crc = check_crc(0x4E85, 0x6B);
  Serial.print(" CRC: ");
  Serial.print(crc);  

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
  delay(60); //44-50 ms max - we could also poll the sensor

  // Comes back in three bytes, data(MSB) / data(LSB) / CRC
  Wire.requestFrom(HTDU21D_ADDRESS, 3);

  // Wait for data to become available
  while(Wire.available() < 3) ;

  unsigned char msb, lsb, crc;
  msb = Wire.read();
  lsb = Wire.read();
  crc = Wire.read(); //We don't do anything with CRC for now

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

//Given the raw temperature data, calculate the actual temperature
float calc_temp(int SigTemp)
{
  float tempSigTemp = SigTemp / (float)65536; //2^16 = 65536
  float rh = -46.85 + (175.72 * tempSigTemp); //From page 14

  return(rh);  
}

//Given the raw humidity data, calculate the actual relative humidity
float calc_humidity(int SigRH)
{
  float tempSigRH = SigRH / (float)65536; //2^16 = 65536
  float rh = -6 + (125 * tempSigRH); //From page 14

  return(rh);  
}

//Give this function the 2 byte message (measurement) and the CRC byte from the HTU21D
//If it returns 0, then the transmission was good
//If it returns something other than 0, then the communication was corrupted

#define DIVISOR 0x0131 //x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
//This is the same as the Maxim 1-Wire CRC

unsigned int check_crc(uint16_t message_from_sensor, uint8_t check_value_from_sensor)
{
  uint8_t result;
  
  result = quickTest(0x79, 0xDC);  
  Serial.print("result: ");
  Serial.println(result, BIN);


//result = quickTest(result, 0x68);  
  ////Serial.print("result: ");
  //Serial.println(result, BIN);
  
  
  while(1);

  //Test cases from datasheet:
  //message = 0xDC, result is 0x79
  //message = 0x683A, result is 0x7C
  //message = 0x4E85, result is 0x6B

  //From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
  //More from: http://en.wikipedia.org/wiki/Cyclic_redundancy_check

  uint32_t message = (uint32_t)message_from_sensor << 8; //Pad with 8 bits because we have to add in the result/check value
  message |= check_value_from_sensor; //Add on the check value

  uint32_t divser = (uint32_t)DIVISOR;

  //Determine the width of the thing under test
  int width;
  for(width = 0 ; (1<<width) < message ; width++) ;
  width--;
  Serial.print("width: ");
  Serial.println(width, DEC);

  //Left align the divser to the three byte test word
  divser <<= width;

  for ( ; width > 0 ; width--)
  {
    Serial.print("message: ");
    Serial.println(message, BIN);
    Serial.print("divser:  ");
    Serial.println(divser, BIN);
    Serial.println();

    if( message & ((uint32_t)1<<(width + 8)) )
    {
      //Serial.print("!");
      message ^= divser;
    }

    divser >>= 1;
  }

  while(1);

  return message;
}

uint8_t quickTest(uint8_t crc, uint8_t data)
{
  uint8_t i;

  crc = crc ^ data;
  for (i = 0; i < 8; i++)
  {
    if (crc & 0x01)
      crc = (crc >> 1) ^ 0x8C;
    else
      crc >>= 1;
  }

  return crc;
}




