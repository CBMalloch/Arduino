
// Reads data from an I2C/TWI slave device

// expects analog 4 to SDA, analog 5 SCL
// SDA is O/W, SCL is O

/*
Notes:
	I thought I couldn't work the DS75 via a long (5m) wire, but have now discovered that I can.
	Here's what I did:
		1) changed the I2C clock speed to 10MHz from 100MHz
		2) added pullups to the two lines - recommendation was 4.7K to 10K, but I had 3.9K handy...
	Now both the white nasty plenum wire and the easier-to-work-with Cat3 cable work just fine.
	
	Next step - change the clock speed back to 100MHz to see if we can still read.

*/

// #include <DateTime.h>
#include <Wire.h>
#undef int()
#include <stdio.h>

#define enabledDevices 0xff      // (I2C addresses) active devices have one bits
#define VERBOSE 2
#define BAUDRATE 115200

#define LEDPin           13      // LED connected to digital pin 13
int LEDStatus = 0;

int addressLines;
int deviceAddress (int addressLines)
{ 
  // Wire will shift L 1; last bit (R/~W) will be added
  return ((0x09 << 3) + addressLines);
}

int CResolution = 1; 
/*
     value   bits res    degC res    degF res    conversion time ms
      0          9          1/2        0.9             150
      1         10          1/4        0.45            300
      2         11          1/8        0.225           600
      3         12          1/16       0.1125         1200
*/

int ConversionTime = 150 << CResolution;

void setup()
{
  pinMode(LEDPin, OUTPUT);      // sets the digital pin as output
  Serial.begin(BAUDRATE);       // start serial for output
	
	delay(1000);
	Serial.println ("Checkpoint 1");
  
  Wire.begin();        // join i2c bus (address optional for master)
  // /*
  // set register 0 (temperature) for reading
  int TRegPtr = 0x00;
  int CRegPtr = 0x01;
  int CFaultTolerance = 0;
  int CPolarity = 0;
  int CThermostatMode = 0;
  int CShutdown = 0;
  int ConfigValue = (CResolution      & 0x03) << 5 
                  | (CFaultTolerance  & 0x03) << 3
                  | (CPolarity        & 0x01) << 2
                  | (CThermostatMode  & 0x01) << 1
                  | (CShutdown        & 0x01) << 0;
  for (addressLines = 0; addressLines < 8; addressLines++) {
    if (enabledDevices & (0x01 << addressLines)) {
      int dA = deviceAddress(addressLines);
      Wire.beginTransmission(dA); Wire.write(CRegPtr); Wire.write(ConfigValue); Wire.endTransmission();
      Wire.beginTransmission(dA); Wire.write(TRegPtr); Wire.endTransmission();
		}
  }
	
  if (VERBOSE >= 2) {
    Serial.print ("Arduino ready to read DS75 devices 0x");
    Serial.println (enabledDevices + 0, HEX);
  }
	
  /*
  
  // test writing of hex

  Serial.print ("s.b. 61 hex: ");
  Serial.print (0x61, HEX);
  Serial.print ("               device address: ");
  Serial.print (deviceAddress, HEX);
  Serial.println();
  // */
}

void loop()
{
  
  for (addressLines = 0; addressLines < 8; addressLines++) {
    char buffer[40];
    if (! (enabledDevices & (0x01 << addressLines))) {
      // skip inactive devices
      if (VERBOSE >= 2) {
        sprintf (buffer, "%7lu: Skipping disabled device %1d\n", millis(), addressLines);
        Serial.print (buffer);
      }
      continue;
    }
  
    Wire.requestFrom(deviceAddress(addressLines), 2);    // ask for data from DS75 

    delay(ConversionTime + 10);                 // wait (ms) for conversion
    int c, c1, c2, t1, t2;
    float t;
    if (Wire.available() >= 2) {    // slave may send less than requested
      c1 = Wire.read();
      c2 = Wire.read();
      c = c1 << 8 | c2;
      // Serial.print(c, HEX);
      t = (c1 + 0.0625 * (c2 >> 4)) * 1.8 + 32.0;
      t1 = (int) t;
      t2 = (int) ((t - t1 + 0.00005) * 10000);
      sprintf (buffer, "[at %lu DS75.%d 0x%02x%02x   %2d.%04d degF]\n", 
			         millis(), 
							 addressLines,
							 c1, c2, t1, t2);
      Serial.print (buffer);
    } else {
      if (VERBOSE >= 1) {
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
