/*
	I2C.cpp - library to facilitate I2C communications
	Created by Charles B. Malloch, PhD, May 7, 2009
	Released into the public domain
*/

#include <Wire.h>

#include "I2C.h"

//    NOTE: the use of this-> is optional

cbmI2C::cbmI2C()
{
}


//************************************************************************************************
// 						                                 I2C TX RX
//************************************************************************************************

byte cbmI2C::deviceAddress (byte deviceType, byte addressLines) { 
  // Wire will left-shift one more bit and or in the required last bit (R/~W)
  return ((deviceType << 3) + addressLines);  
}

void cbmI2C::init()
{
  Wire.begin();                 // join i2c bus (address optional for master)
}

void cbmI2C::TX (byte device, byte regadd, byte tx_data)                        // Transmit I2C Data
{
  Wire.beginTransmission (device);
  Wire.write (regadd); 
  Wire.write (tx_data); 
  Wire.endTransmission();
}

int cbmI2C::RX (byte device, byte regadd, int requestedbytecount, byte *buf) {  // Receive I2C Data
  Wire.beginTransmission (device);
  Wire.write (regadd); 
  Wire.endTransmission();
  
  Wire.requestFrom(int(device), requestedbytecount);   

	int received = 0;
  if (Wire.available() >= requestedbytecount) {
  	for (received = 0; received <= requestedbytecount; received++) {
    	*(buf+received) = Wire.read();             
    }
  }
  return (received);
}

//    preinstate the singleton object

cbmI2C I2C = cbmI2C();

