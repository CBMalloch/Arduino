
// Reads data from an I2C/TWI slave device

// expects analog 4 to SDA, analog 5 SCL

/*
Notes:
	I thought I couldn't work the DS75 via a long (5m) wire, but have now discovered that I can.
	Here's what I did:
		1) changed the I2C clock speed to 10MHz from 100MHz
		2) added pullups to the two lines - recommendation was 4.7K to 10K, but I had 3.9K handy...
	Now both the white nasty plenum wire and the easier-to-work-with Cat3 cable work just fine.
	
	Next step - change the clock speed back to 100MHz to see if we can still read.

*/

#include <Wire.h>
#undef int()
#include <stdio.h>
#include <DateTime.h>

#include "DS75.h"

#define enabledDevices 0xff      // (I2C addresses) active devices have one bits
#define VERBOSE 2
#define BAUDRATE 115200

#define LEDPin           13      // LED connected to digital pin 13
int LEDStatus = 0;

int addressLines;

cbmDS75 DS75s[8];

char buffer[80];

void setup()
{
  pinMode(LEDPin, OUTPUT);      // sets the digital pin as output
  Serial.begin(BAUDRATE);       // start serial for output
	
	delay(1000);
	Serial.println ("Checkpoint 1");
  
  Wire.begin();        // join i2c bus (address optional for master)

	
	//                    0  1  2  3  4  5  6  7
	byte resolutions[] = {0, 0, 0, 0, 0, 0, 0, 3};
	
	for (int i = 0; i < 8; i++) {
		// cbmDS75(byte address, byte resolution, boolean enabled)
		DS75s[i].init(byte(i), resolutions[i], (enabledDevices >> i) & 0x01);
	}
	
  if (VERBOSE >= 2) {
    Serial.print ("Arduino ready to read DS75 devices 0x");
    Serial.println (enabledDevices + 0, HEX);
  }
	
}

void loop()
{
  
  for (addressLines = 0; addressLines < 8; addressLines++) {
    if (! ((enabledDevices >> addressLines) & 0x01)) {
      // skip inactive devices
      if (VERBOSE >= 2) {
        sprintf (buffer, "%7lu: Skipping disabled device %1d\n", millis(), addressLines);
        Serial.print (buffer);
      }
      continue;
    }
		
		DS75s[addressLines].read(buffer);
		if (strncmp(buffer, "[at ", 4) == 0) {
      Serial.println (buffer);
    } else {
      if (VERBOSE >= 1) {
				// Serial.println (buffer);
        sprintf (buffer, "%7lu: Device %1d: no response\n", millis(), addressLines);
        Serial.print (buffer);
      }
    }
  }

  LEDStatus = 1 - LEDStatus;
  if (LEDStatus) {
    digitalWrite(LEDPin, HIGH);   // sets the LED on
  } 
  else {
    digitalWrite(LEDPin, LOW);
  }
  // delay(500);
}
