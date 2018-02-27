#include <stdio.h>
// #include "c_test_EEPROM.h"
#include <EEPROM.h>

/*
byte x;
byte y;
int yi;
long yl;
*/

const int labelLen = 20;
char label [ labelLen ] = "LoRa32u4 1";
const int baseAdd = 0x00;

void setup() {

	Serial.begin ( 115200 );
	while ( !Serial && millis() < 5000 );
	
	Serial.println ( "Testing EEPROM" );
	
	/*
	// x = EEPROM.read(add);
	// Serial.println( (int) x );
	EEP.read ( add, &y );
	Serial.println( (int) y );
	Serial.println("---");
	
	EEP.write(add  , (byte) 116);
	EEP.write(add+2, 2056);
	EEP.write(add+8, (long) 2000);
	
	EEP.read(add, &y);
	Serial.println( (int) y );
	
	EEP.read(add+2, &yi);
	Serial.println( yi );
	
	EEP.read(add+8, &yl);
	Serial.println( (unsigned long) yl );
	
	Serial.println("---");
	*/
	
	for ( int i = 0; i < labelLen; i++ ) {
	  Serial.print ( (char) EEPROM.read ( baseAdd + i ) );
	}
	Serial.println ();
	
	for ( int i = 0; i < strlen ( label ); i++ ) {
	  EEPROM.write ( baseAdd + i, label [ i ] );
	}
	for ( int i = strlen ( label ); i < labelLen; i++ ) {
	  EEPROM.write ( baseAdd + i, 0x00 );
	}
	
	for ( int i = 0; i < labelLen; i++ ) {
	  Serial.print ( (char) EEPROM.read ( baseAdd + i ) );
	}
	Serial.println ();
	
	for ( int i = 0; i < labelLen; i++ ) {
	  int c = EEPROM.read ( baseAdd + i );
	  if ( c < 16 ) Serial.print ( "0" );
	  Serial.print ( c, HEX);
	  Serial.print ( " " );
	}
	Serial.println ();
	

}

void loop() {
}