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

#define HTDU21D_ADDRESS 0x40  //Unshifted 7-bit I2C address for the sensor

#define TRIGGER_TEMP_MEASURE_HOLD  0xE3
#define TRIGGER_HUMD_MEASURE_HOLD  0xE5
#define TRIGGER_TEMP_MEASURE_NOHOLD  0xF3
#define TRIGGER_HUMD_MEASURE_NOHOLD  0xF5
#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE

float temperature;
float relativeHumidity;

unsigned int rawHumidity;
unsigned int rawTemperature;
byte sensorStatus;

void setup()
{
  Serial.begin(9600);
  Serial.println("HTU21D Example!");

  Wire.begin();
}

void loop()
{
   //htdu21d_readTemp();
   //htdu21d_readHumidity();
   
   //Test cases:
   //p14 Humidity = 0x7C80 = 54.8% RH
   //p14 Temp = 0x683A = 24.7 C
   //p14 Humidity = 0x4E85 = 32.3% RH
   rawTemperature = 0x683A;
   rawHumidity = 0x4E85;
   
   temperature = calc_temp(rawTemperature);
   relativeHumidity = calc_humidity(rawHumidity); //Turn the humidity signal into actual humidity
   
   Serial.print("Temperature: ");
   Serial.print(temperature, 1);
   Serial.print(" C");
   Serial.print(" Relative Humidity: ");
   Serial.print(relativeHumidity, 1);
   Serial.print(" %");

  int crc;
  
  crc = check_crc(0xDC, 0x79);
  Serial.print(" CRC: ");
  Serial.print(crc);

  while(1);

  Serial.println();
  delay(1000);
}

//Read the uncompensated temperature value
int htdu21d_readTemp()
{
  int temperature;

  //Request the temperature
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(TRIGGER_TEMP_MEASURE_NOHOLD);
  Wire.endTransmission();

  //Wait for sensor to complete measurement
  delay(60); //44-50 ms max - we could also poll the sensor

  //Comes back in three bytes, data(MSB) / data(LSB) / CRC
  Wire.requestFrom(HTDU21D_ADDRESS, 3);

  //Wait for data to become available
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

//Read the humidity
unsigned int htdu21d_readHumidity()
{
  byte msb, lsb, checksum;

  //Request a humidity reading
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(TRIGGER_HUMD_MEASURE_NOHOLD); //Measure humidity with no bus holding
  Wire.endTransmission();
  
  //Hang out while measurement is taken. 50mS max, page 4 of datasheet.
  delay(55);

  //Read result
  Wire.requestFrom(HTDU21D_ADDRESS, 3);

  //Wait for data to become available
  int counter = 0;
  while(Wire.available() < 3)
  {
    counter++;
    delay(1);
    if(counter > 100) return 0; //Error out
  }
  
  msb = Wire.read();
  lsb = Wire.read();
  checksum = Wire.read();

  rawHumidity = ((unsigned int) msb << 8) | (unsigned int) lsb;
  //sensorStatus = rawHumidity & 0x0003; //Grab only the right two bits
  rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place
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

//Read the user register
byte read_user_register(void)
{
  byte userRegister;
  
  //Request the user register
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(READ_USER_REG); //Read the user register
  Wire.endTransmission();
  
  //Read result
  Wire.requestFrom(HTDU21D_ADDRESS, 1);
  
  userRegister = Wire.read();

  return(userRegister);  
}

//Write to the user register
//NOTE: We disable all bits except for measurement resolution
//Bit 7 & 0 = Measurement resolution
//Bit 6 = Status of battery
//Bit 5/4/3 = Reserved
//Bit 2 = Enable on-board heater
//Bit 1 = Disable OTP reload
void write_user_register(byte thing_to_write)
{
  byte userRegister = read_user_register(); //Go get the current register state
  userRegister &= 0b01111110; //Turn off the resolution bits
  thing_to_write &= 0b10000001; //Turn off all other bits but resolution bits
  userRegister |= thing_to_write; //Mask in the requested resolution bits
  
  //Request a write to user register
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(WRITE_USER_REG); //Write to the user register
  Wire.write(userRegister); //Write to the data
  Wire.endTransmission();
}

//Give this function the 2 byte message (measurement) and the check_value byte from the HTU21D
//If it returns 0, then the transmission was good
//If it returns something other than 0, then the communication was corrupted
//From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
//POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
#define SHIFTED_DIVISOR 0x988000 //This is the 0x0131 polynomial shifted to farthest left of three bytes

unsigned int check_crc(uint16_t message_from_sensor, uint8_t check_value_from_sensor)
{
  //Test cases from datasheet:
  //message = 0xDC, result is 0x79
  //message = 0x683A, result is 0x7C
  //message = 0x4E85, result is 0x6B

  uint32_t remainder = (uint32_t)message_from_sensor << 8; //Pad with 8 bits because we have to add in the result/check value
  remainder |= check_value_from_sensor; //Add on the check value

  uint32_t divsor = (uint32_t)SHIFTED_DIVISOR;

  for (int i = 0 ; i < 16 ; i++) //Operate on only 16 positions of max 24. The remaining 8 are our remainder and should be zero when we're done.
  {
    //Serial.print("remainder:  ");
    //Serial.println(remainder, BIN);
    //Serial.print("divsor:     ");
    //Serial.println(divsor, BIN);
    //Serial.println();

    if( remainder & (uint32_t)1<<(23 - i) ) //Check if there is a one in the left position
      remainder ^= divsor;

    divsor >>= 1; //Rotate the divsor max 16 times so that we have 8 bits left of a remainder
  }

  return remainder;
}




