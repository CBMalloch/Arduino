// Writes some stuff to LCD, which happens to be marked LCM1602A

#include <DateTime.h>

#include <LiquidCrystal.h>

#define BAUDRATE  115200

/*
  LCD pin definitions
     1 Vss (gnd)
     2 Vdd (+5V)
     3 V0 - contrast - from a 10K pot
     4 RS - register select
     5 R/~W - read/write select - ground if not using and remove pdRW from the constructor list
     6 E - read/write enable
     7-10 DB0-DB3 - low-order bidirectional tristate data bus lines, not used during 4-bit operation
    11-14 DB4-DB7 - high-order " " "
    15 LED+ backlight up to 5V
    16 LED- backlight
*/

#define pdD7          2
#define pdD6          3
#define pdD5          4
#define pdD4          5
#define pdRW         10
#define pdEN         11
#define pdRS         12

#define pdLED        13

void bigDigit ( LiquidCrystal lcd, uint8_t d, uint8_t loc = -1 );

LiquidCrystal lcd(pdRS, pdRW, pdEN, pdD4, pdD5, pdD6, pdD7);

#define testPace 2000 // ms delay after each test piece

//                  1234567890123456
char *helloWorld = "Hello, world!";
char *prefix     = "Switches: B";

byte myRow, myCol;

// prepare for accurate looping
#define LOOPPERIOD_ms 200                // milliseconds
unsigned long loopTime;


//************************************************************************************************
// 						                    Standard setup and loop routines
//************************************************************************************************

void setup()
{
  pinMode(pdLED, OUTPUT);
  Serial.begin(BAUDRATE);

  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  
  lcd.print (helloWorld);
  Serial.println (helloWorld);
  delay(testPace);

  loopTime = millis();
    
}

void loop() {
	unsigned long theMillis;
  int theDelay;
  byte c;
	char buffer[40];
  
	
/*
  // Turn off the display:
  lcd.noDisplay();
  delay(500);
   // Turn on the display:
  lcd.display();
  delay(500);
*/
  
  // location 0-7 is all we get; char map is 8 bytes; uses 5 low-order bits of each byte (0x1f)
  static uint8_t pattern [] [ 8 ] = { { 0x1f, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00 },  // top
                                      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f },  // bottom
                                      { 0x1f, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x1f, 0x1f },  // both
                                      { 0x1c, 0x1e, 0x1f, 0x1f, 0x1f, 0x1f, 0x1e, 0x1c },  // rt curve
                                      { 0x07, 0x0f, 0x1f, 0x1f, 0x1f, 0x1f, 0x0f, 0x07 },  // lt curve
                                      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0f, 0x1f },  // comma
                                      { 0x00, 0x00, 0x00, 0x00, 0x0e, 0x0e, 0x0e, 0x00 },  // dot
                                    };
  for ( uint8_t j = 0; j < sizeof ( pattern ) / sizeof ( uint8_t ) / 8; j++ ) {
    lcd.createChar ( (uint8_t) j, pattern [ j ] );
    // lcd.setCursor ( 0x02 + j, 0x01 );
    // lcd.write ( (uint8_t) j );
  }
  
  // bigDigit ( lcd, ( millis() / 1000 ) % 10, 3 );
  displayTime ( lcd, ( millis() / 1000 ) % 10000 );

/*  
		// blank the first line
		myLCD.command (0x80 + 0);    // set DDRAM address to 0x00 (beginning of first row)
		myLCD.dispString ("                ");

		myLCD.command (0x80 + 0);    // set DDRAM address to 0x00 (beginning of first row)
		myLCD.dispNum (loopTime, "%5u");
		
		
		// put seconds into the first line
		myLCD.command (0x80 + 5);    // set DDRAM address
		myLCD.dispNum(DateTime.now(), " %10ul");
		
		// put stuff on the second line
		myLCD.command (0x80 | 0x40 + 0);    // set DDRAM address to 0x40 (second row L)
		myLCD.dispString ("Switches: B");
		myLCD.dispBits (c);

  //}

  LEDStatus = 1 - LEDStatus;
  if (LEDStatus) {
    digitalWrite(pdLED, HIGH);   // sets the LED on
  } 
  else {
    digitalWrite(pdLED, LOW);
  }
  */
  
  theMillis = millis();
  theDelay = LOOPPERIOD_ms - (theMillis - loopTime);
  if (theDelay < 0) { theDelay = 0; }
	loopTime = theMillis + theDelay;
  // sprintf (buffer, "Planned delay: %d\n", theDelay);
  // Serial.print (buffer);
  delay (theDelay);

}

void bigDigit ( LiquidCrystal lcd, uint8_t d, uint8_t loc ) {
  // bigDigits are 3 characters wide, with one space; there's room for 4 of them
  static uint8_t bigDigits [] [ 6 ] = {
                                        { 0xff, 0x00, 0xff, 0xff, 0x01, 0xff },  // 0
                                        { 0x00, 0xff, 0x20, 0x01, 0xff, 0x01 },  // 1
                                        { 0x00, 0x02, 0x03, 0x04, 0x01, 0x01 },  // 2
                                        { 0x02, 0x02, 0x03, 0x01, 0x01, 0x03 },  // 3
                                        { 0xff, 0x01, 0x01, 0x20, 0xff, 0x20 },  // 4
                                        { 0xff, 0x02, 0x00, 0x01, 0x01, 0x03 },  // 5
                                        { 0x04, 0x02, 0x00, 0x04, 0x01, 0x03 },  // 6
                                        { 0x00, 0x00, 0x03, 0x20, 0xff, 0x20 },  // 7
                                        { 0x04, 0x02, 0x03, 0x04, 0x01, 0x03 },  // 8
                                        { 0x04, 0x02, 0x03, 0x20, 0x20, 0xff }   // 9
                                      };
  for ( uint8_t i = 0; i < 2; i++ ) {
    for ( uint8_t j = 0; j < 3; j++ ) {
      lcd.setCursor ( loc * 4 + j, i );
      lcd.write ( bigDigits [ d ] [ i * 3 + j ] );
    }
  }
}

void displayTime ( LiquidCrystal lcd, int hhmm ) {
  lcd.clear ();
  for ( int i = 3; i >= 0; i-- ) {
    bigDigit ( lcd, hhmm % 10, i );
    hhmm /= 10;
  }
  lcd.setCursor ( 7, 0 ); lcd.write ( ( uint8_t ) 0x06 );
  lcd.setCursor ( 7, 1 ); lcd.write ( ( uint8_t ) 0x06 );
}
