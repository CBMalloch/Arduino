/* test printHex
*/

// #include <Arduino.h>
#include <PrintHex.h>

#define BAUDRATE 115200

void setup () {
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 );
  
  Serial.print ( "0xffffffff: " ); printHex ( ( unsigned long ) 0xffffffff, 8 ); Serial.println ();
  Serial.print ( "0x00000000: " ); printHex ( ( unsigned long ) 0x00000000, 8 ); Serial.println ();
  Serial.print ( "0x80000001: " ); printHex ( ( unsigned long ) 0x80000001, 8 ); Serial.println ();
  Serial.print ( "0xffff: " );     printHex ( ( unsigned long ) 0xffff,     4 ); Serial.println ();
  Serial.print ( "0xff: " );       printHex ( ( unsigned long ) 0xff,       4 ); Serial.println ();
  Serial.print ( "0xff: " );       printHex ( ( unsigned long ) 0xff,       2 ); Serial.println ();
  Serial.print ( "0x00000001: " ); printHex ( ( unsigned long ) 0x00000001, 8 ); Serial.println ();
  Serial.print ( "0x00010001: " ); printHex ( ( unsigned long ) 0x00010001, 8 ); Serial.println ();
  Serial.print ( "0x00010001: " ); printHex ( ( unsigned long ) 0x00010001, 4 ); Serial.println ();
  Serial.print ( "-1: " );         printHex ( ( unsigned long ) -1,         8 ); Serial.println ();
  Serial.print ( "-1: " );         printHex ( ( unsigned long ) -1,         6 ); Serial.println ();
  Serial.print ( "-1: " );         printHex ( ( unsigned long ) -1,         4 ); Serial.println ();
  Serial.print ( "-1: " );         printHex ( ( unsigned long ) -1,         3 ); Serial.println ();
  Serial.print ( "-1: " );         printHex ( ( unsigned long ) -1,         2 ); Serial.println ();
  Serial.print ( "-1: " );         printHex ( ( unsigned long ) -1,         1 ); Serial.println ();
  Serial.print ( "0xfffff800: " ); printHex ( ( unsigned long ) 0xffff8000, 4 ); Serial.println ();
  Serial.print ( "0xfffff800: " ); printHex ( ( unsigned long ) 0xffff8000, 2 ); Serial.println ();
  Serial.print ( "0x00010001: " ); printHex ( ( unsigned long ) 0x00010001, 9 ); Serial.println ();
}

void loop () {}
