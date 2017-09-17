/*
  writes to Mindsensors.com LCD216s 2-line LCD display
  device OK for RS-232, inverted RS-232, SPI, I2C

  5-pin connector on device:
    SDA/Tx       a4 (SDA)
    GND             GND
    SCL/Rx        a5 (SCL)
    Vcc               +5VDC
    /CS               GND  -- not required for I2C
    
*/

#include <Wire.h>
#include <LCD216s.h>
#include <stdio.h>

#define BAUDRATE 115200

#define LCD_I2C_address 0

int LEDPin = 13;                // LED connected to digital pin 13
int LEDStatus = 0;

#define bufLen 80
static char strBuf [ bufLen ];
static short bufPtr = 0;

LCD216s LCD ( LCD_I2C_address );

void setup() {

  Serial.begin( BAUDRATE );
  Serial.println ( "Hello from LCD_Mindsensors_LCD216s!" );
  
  Wire.begin ();        // join i2c bus (address optional for master)
  LCD.init ();
  
  pinMode ( LEDPin, OUTPUT );
    
}

void loop() {

  static int l = 0;
  
  static char mrBuf[] = "This is a test!";
  
  LCD.home ( );
  LCD.send ( (unsigned char *) &mrBuf[0], 15 );  // why not just mrBuf (which doesn't work!)?
  
  LCD.command ( CursPos, (unsigned char *) "\x00\x01", 2 );
  snprintf ( strBuf, bufLen, "%5d", l );
  LCD.send ( (unsigned char *) &strBuf[0], 5 );
  
  strBuf[0] = 0x0f;
  strBuf[1] = ( ( l / 16 ) % 2 ) ? 16 - ( l % 16 ) : ( l % 16 );
  strBuf[2] = 0x00;
  Serial.println ( strBuf[1], HEX );
  LCD.command ( VBar, (unsigned char *) &strBuf[0], 2 );

  LEDStatus = 1 - LEDStatus;
  if (LEDStatus) {
    digitalWrite(LEDPin, HIGH);   // sets the LED on
  } 
  else {
    digitalWrite(LEDPin, LOW);
  }
  
  l++;
  delay ( 250 );
}
