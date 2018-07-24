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

int getEEPROMstring ( byte baseAdd, byte maxLen, char result [] );
void writeEEPROMstring ( byte baseAdd, char str [] );


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
	
	EEPROM.begin ( 512 );                               // NOTE
	
	Serial.print ( "Reading: " );
	for ( int i = 0; i < labelLen; i++ ) {
	  Serial.print ( (char) EEPROM.read ( baseAdd + i ) );
	}
	Serial.print ( "\n\n" );
	
	Serial.print ( "Writing: " );
	for ( int i = 0; i < strlen ( label ); i++ ) {
	  Serial.print ( (char) label [ i ] );
	  EEPROM.write ( baseAdd + i, label [ i ] );
	}
	for ( int i = strlen ( label ); i < labelLen; i++ ) {
	  Serial.print ( "\0" );
	  EEPROM.write ( baseAdd + i, 0x00 );
	}
	
	EEPROM.commit ();                                   // NOTE
	
	Serial.print ( "\n\n" );
	
	Serial.print ( "Reading back: " );
	for ( int i = 0; i < labelLen; i++ ) {
	  Serial.print ( (char) EEPROM.read ( baseAdd + i ) );
	}
	Serial.print ( "\n\n" );
	
	Serial.print ( "Reading back (hex): " );
	for ( int i = 0; i < labelLen; i++ ) {
	  int c = EEPROM.read ( baseAdd + i );
	  if ( c < 16 ) Serial.print ( "0" );
	  Serial.print ( c, HEX);
	  Serial.print ( " " );
	}
	Serial.print ( "\n\n" );	
	
	
	
	
	
	char tmpw [ 4 ] = "xyz";
	char tmpw2 [ 5 ] = "xyzz";
  char tmpr [ 4 ];
  int n;
	
	Serial.print ( "Writing string: " ); Serial.print ( tmpw ); Serial.print ( "\n" );
  writeEEPROMstring ( 0, tmpw );
  
  n = getEEPROMstring ( 0, 4, tmpr );
	Serial.print ( "Reading back: got: n = " );
  Serial.print ( n ); 
  Serial.print ( "; string: " );
  Serial.print ( tmpr );
  Serial.print ( "\n\n" );

	Serial.print ( "Writing string: " ); Serial.print ( tmpw2 ); Serial.print ( "\n" );
  writeEEPROMstring ( 0, tmpw2 );
  
  n = getEEPROMstring ( 0, 4, tmpr );
	Serial.print ( "Reading back: got: n = " );
  Serial.print ( n ); 
  Serial.print ( "; string: " );
  Serial.print ( tmpr );
  Serial.print ( "\n\n" );

	Serial.print ( "Writing string: " ); Serial.print ( "abc" ); Serial.print ( "\n" );
  writeEEPROMstring ( 0, "abc" );
  
  n = getEEPROMstring ( 0, 4, tmpr );
	Serial.print ( "Reading back: got: n = " );
  Serial.print ( n ); 
  Serial.print ( "; string: " );
  Serial.print ( tmpr );
  Serial.print ( "\n\n" );

}

void loop() {
}

int getEEPROMstring ( byte baseAdd, byte maxLen, char result [] ) {
  int nChars = -1;
	Serial.print ( "Reading: " );
  for ( int i = 0; i < maxLen; i++ ) {
    result [ i ] = (char) EEPROM.read ( baseAdd + i );
    Serial.print ( result [ i ] );
    if ( result [ i ] == '\0' ) {
      nChars = i;
      break;
    }
  }
  Serial.print ( "\n" );
  return ( nChars );
}
    
void writeEEPROMstring ( byte baseAdd, char str [] ) {

	Serial.print ( "Writing: " );
	for ( int i = 0; i < strlen ( str ); i++ ) {
	  Serial.print ( (char) str [ i ] );
	  EEPROM.write ( baseAdd + i, str [ i ] );
	}
	Serial.print ( "\\0\n" );
	EEPROM.write ( baseAdd + strlen ( str ), 0x00 );

	EEPROM.commit ();                                   // NOTE
	
}

