/*
	cbmDS75.cpp - library to communicate to I2C DS75 thermometers
	Created by Charles B. Malloch, PhD, January 15, 2010
	Released into the public domain
*/

#include <Wire.h>
#include <wiring.h>
#include <stdio.h>
#include <DateTime.h>

#include "DS75.h"

cbmDS75::cbmDS75() {
}

byte cbmDS75::generateDeviceAddress (byte a_addressLines) { 
  // Wire will shift L 1; last bit (R/~W) will be added
  return ((0x09 << 3) + a_addressLines);
}

void cbmDS75::init() {
}

void cbmDS75::init(byte a_address, byte a_resolution, bool a_enabled) {
	addressLines = a_address;
	deviceAddress = generateDeviceAddress(addressLines);
	/*
     value   bits res    degC res    degF res    conversion time ms
      0          9          1/2        0.9             150
      1         10          1/4        0.45            300
      2         11          1/8        0.225           600
      3         12          1/16       0.1125         1200
	*/
	resolution = a_resolution;
	conversionTime = 150 << resolution;
	enabled = a_enabled;
  // /*
	
	if (enabled) {
		// set register 0 (temperature) for reading
		configValue = (resolution       & 0x03) << 5 
								| (CFaultTolerance  & 0x03) << 3
								| (CPolarity        & 0x01) << 2
								| (CThermostatMode  & 0x01) << 1
								| (CShutdown        & 0x01) << 0;
		Wire.beginTransmission(deviceAddress); Wire.send(CRegPtr); Wire.send(configValue); Wire.endTransmission();
    Wire.beginTransmission(deviceAddress); Wire.send(TRegPtr); Wire.endTransmission();
	}
  // */
}

void cbmDS75::read(char *buffer) {
	
	if (!enabled) {
		sprintf (buffer, "Disabled");
		return;
	}
	
	Wire.requestFrom(int(deviceAddress), 2);         // ask for data from DS75 

	delay(conversionTime + 10);                      // wait (ms) for conversion
	
	byte c1, c2;                    // the two words received from the DS75
	int c;                          // word comprising c1 and c2
	int	t1, t2; 										// int part and 1e3 times fract part degF
	float tc, tf;                   // temp in C and F

	if (Wire.available() >= 2) {    // slave may send less than requested
		c1 = Wire.receive();
		c2 = Wire.receive();
		c = c1 << 8 | c2;
		if (c & 0x8000) {
			// Serial.println ("Negative number");
			// negative number...
			tc = - ( (-c >> 8) + 0.0625 * ((-c >> 4) & 0x000f) );
		} else {
			tc = (c >> 8) + 0.0625 * ((c >> 4) & 0x000f);
		}
		// Serial.print(c, HEX);
		
		tf = tc * 1.8 + 32.0;                                               // -5.5 would be OK
		t1 = (int) tf;                                                      // -5 (holds the sign)
		t2 = (int) ((abs(tf) - abs(t1) + 0.00005) * 10000);                 // (5.5 - 5 + .00005) * 10000			= 5000 OK

		// check fe40 -> 28.85
		sprintf (buffer, "[at %lu DS75.%d 0x%02x%02x   %2d.%04d degF]", 
						 DateTime.now(), 
						 addressLines,
						 c1, c2, 
						 t1, t2);
						 
		return;
	} else {
		sprintf (buffer, "Unavailable");
		return;
	}
}
