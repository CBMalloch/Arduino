/*
	LCD216s - library for controlling a Mindsensors LCD216s 2x16 LCD screen
	Created by Charles B. Malloch, PhD, February 16, 2014
	Released into the public domain
	
	Timing: even after deleting unnecessary delays, time required for writes is 
	        about 1ms per character. It behooves to write only those characters 
	        necessary, addressing randomly rather than repeating unchanging text.
	        It therefore also behooves to have fixed-width fields.
	        
*/

#include "LCD216s.h"

extern "C" {
  #include <stdio.h>
  #include <string.h>
}

#include <Arduino.h>
// #include <wiring.h>
#include <Wire.h>

//************************************************************************************************
// 						                             LCD216s definitions
//************************************************************************************************

#define LCD_216S_pause_ms 10


// LCD216s controller; device address on its setup screen is 50
// note: dt stands for device type! 
#define dtLCD216s  0x05
// the device address will be ttttaaa0

// command indicator
#define   CmdChr    0xfe    // command indicator
// auto LCD wrap on
#define		WrapOn		0x43
// auto LCD wrap off
#define		WrapOff		0x44
// auto line wrap on (stays on current line)
#define		LnWrOn		0x45
// auto LCD scroll on
#define		ScrlOn		0x51
// auto LCD scroll off
#define		ScrlOff		0x52
// auto line scroll on (stays on current line)
#define		LnScOn		0x4f
// set cursor pos <col> <row> radix 0
#define		CursPos		0x47
// cursor home
#define		CursHom		0x48
// cursor on, underline
#define		CrUndrL		0x4a
// underline cursor off
#define   CrUnLOf   0x4b
// cursor on, block
#define		CrBlock		0x53
// cursor ff
#define		CrOff 		0x54
// cursor left
#define		CursLf		0x4c
// cursor right
#define		CursRt		0x4d
// print as hex <character>
#define		PrHex			0x20
// print as decimal <character>
#define		PrDec 		0x21
// print as unsigned integer <low byte> <high byte>
#define   PrUInt    0x22
// clear display
#define   Clear     0x58
// set contrast <contrast> (not in SPI mode)
#define   SetCtr    0x50
// backlight on
#define   BLOn      0x42
// backlight off
#define   BLOff     0x46
// vertical bar graph <column> <height>
#define   VBar      0x3d
// define custom char <position> <8 bytes>
#define   Custom    0x4e

  


int deviceAddress ( int deviceType, int addressLines ) {
  // return 0ttttaaa
  // Wire will shift L 1; last bit (R/~W) will be added, so will become ttttaaaR
  return ( ( ( deviceType & 0x0f ) << 3 ) + ( addressLines & 0x07 ) );
}


//************************************************************************************************
// 						                             LCD216s initialization
//************************************************************************************************


LCD216s::LCD216s () {
}

LCD216s::LCD216s ( int address ) {
	_address = deviceAddress ( dtLCD216s, address );
}

//************************************************************************************************
// 						                             LCD216s subroutines
//************************************************************************************************

void LCD216s::init( ) {
  init ( ( _address  & 0x07 ) << 4 );
}

void LCD216s::init( int address ) {

  // OK. It appears that these units don't start up well. Maybe they require better power smoothing?
  
	_address = deviceAddress ( dtLCD216s, address );
  
  // digitalWrite ( 13, HIGH );
  // Serial.begin ( 115200 );
  // Serial.print ( "LCD216s init address: 0x" ); Serial.println ( _address, HEX );

  for ( int i = 0; i < 1; i++ ) {
    // Force the Command protocol to be P1 ( 0xFE 0xFE 0x55 0xAA < protocol>)
    // page 7 of PDF
    // delay ( 1000 );
    // digitalWrite ( 12, 1 - digitalRead ( 12 ) );
    send ( (unsigned char *) "\xfe\xfe\x55\xaa\x01", 5 );
    // send ( (unsigned char *) "\xfe\xfe\x55\xaa\x02", 5 );
    
    // delay ( 1000 );
    // digitalWrite ( 11, digitalRead ( 12 ) );
    clear ( );
    // command ( 0x01, (unsigned char *) "", 0 );
  }
  command ( BLOn, (unsigned char *) "", 0 );
  command ( CrOff, (unsigned char *) "", 0 );
  command ( WrapOff, (unsigned char *) "", 0 );
  command ( ScrlOff, (unsigned char *) "", 0 );
  
  // clear ( );
}

void LCD216s::home ( ) {
  command ( CursHom, (unsigned char *) "", 0 );
}

void LCD216s::clear ( ) {
  command ( Clear, (unsigned char *) "", 0 );
}

void LCD216s::setLoc (byte row, byte col) {
  // from ( 0, 0 )
  byte pos[] = { col, row };
  command ( CursPos, pos, 2 );
}

void LCD216s::command ( int character, unsigned char * args, int nChars ) {
  Wire.beginTransmission ( _address ); 
  Wire.write ( CmdChr );
  Wire.write ( ( unsigned char ) character );
  if ( nChars > 0 ) Wire.write ( ( const uint8_t * ) args, nChars );
  Wire.endTransmission();
  delay ( LCD_216S_pause_ms );
}

void LCD216s::command ( unsigned char character, unsigned char * args, int nChars ) {
  Wire.beginTransmission ( _address ); 
  Wire.write ( CmdChr );
  Wire.write ( character );
  if ( nChars > 0 ) Wire.write ( ( const uint8_t * ) args, nChars );
  Wire.endTransmission();
  delay ( LCD_216S_pause_ms );
}

void LCD216s::send ( unsigned char * strBuf, int nChars ) {
  Wire.beginTransmission ( _address ); 
  Wire.write ( ( const uint8_t * ) strBuf, nChars );
  Wire.endTransmission();
  delay ( LCD_216S_pause_ms );
}