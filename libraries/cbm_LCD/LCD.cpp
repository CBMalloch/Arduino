/*
	LCD.cpp - library to control a standard LCD (HD44780 Hitachi 2x40 LCD display)
	Created by Charles B. Malloch, PhD, May 7, 2009
	Released into the public domain
	
	All delays are LCD_write_delay_us microseconds in duration (currently defined as 50)
	
	00000001 reset
	00000011 return home                      1.52ms
	00000111 entry mode, incr, display shift    37us
	
*/

extern "C" {
  #include <stdio.h>
  #include <string.h>
}

#include <Arduino.h>
#include <wiring.h>
#include "LCD.h"

// specific to LCD

#define		ClrLCD		0x01		           // clear the LCD
#define		CrsrHm		0x02		           // move cursor to home position
#define		CrsrLf		0x10		           // move cursor left
#define		CrsrRt		0x14		           // move cursor right
#define		DispLf		0x18		           // shift displayed chars left
#define		DispRt		0x1C		           // shift displayed chars right
#define		DDRam			0x80		           // Display Data RAM control
#define		ddram2		0xC0		           // 9th position of display (not in next 

#define		RS_pin		0x04               // B00000100 reg B pin 2
#define		RW_pin		0x02               // B00000010 reg B pin 1
#define		E_pin			0x01               // B00000001 reg B pin 0

#define LCD_write_delay_us 5           // 50 works; 5 works; 2 works

LCD::LCD ()
{
}

LCD::LCD (MCP23017 interface)
{
	init(interface);
}

//************************************************************************************************
// 						                             LCD subroutines
//************************************************************************************************

void LCD::init(MCP23017 interface)
{
	_interface = interface;
  // Only used with port expander
  _LCDCONT = RS_pin | E_pin;
  _interface.TX (GPIOB, _LCDCONT);  // turn on RS and E

  // Standard Hitachi initialization for 8-bit mode from spec sheets
  delayMicroseconds (LCD_write_delay_us);
  command (0x30);  // B00110000 set data len 8 bits, n lines = 1, font 5x8
  delayMicroseconds (4100);
  command (0x30);  // B00110000 set data len 8 bits, n lines = 1, font 5x8
  delayMicroseconds (100);
  command (0x30);  // B00110000 set data len 8 bits, n lines = 1, font 5x8
  delayMicroseconds (LCD_write_delay_us);
  command (0x38);  // B00111000set data len 8 bits, n lines = 2, font 5x8
  delayMicroseconds (LCD_write_delay_us);
  command (0x08);  // B00001000 display off, cursor off, cursor char blink off
  delayMicroseconds (LCD_write_delay_us);
  command (ClrLCD);     // clear display, set DDRAM address 0
  delayMicroseconds (LCD_write_delay_us);
  command (0x06);  // B00000110 set cursor move direction increment, display shift off
  delayMicroseconds (LCD_write_delay_us);
  command (0x0c);  // B0001100 display on, cursor off, cursor char blink off
  setLoc(0, 0);               // enables clear
  // _row = 0; _col = 0; 
  // unsigned char loc = 0x80 | (0x40 * _row) | _col;
  // myLCD.command(loc); delay(100);    //  doesn't enable clear!  
  clear();  
}

void LCD::write (char lcdChar) {
	if (_col <= LCDRowLen) {
		_interface.TX (GPIOA, lcdChar);              // write and latch the char to A
		_LCDCONT = _LCDCONT | E_pin;                  // set E_pin
		_interface.TX (GPIOB, _LCDCONT);
		_LCDCONT = RS_pin;                           // toggle E_pin off
		_interface.TX (GPIOB, _LCDCONT);
  	_col++;
  }  // write beyond end of row fails with no error
}

void LCD::command (byte cmdlcd) {
  _LCDCONT = 0;                                  // was bcf RS_pin
  _interface.TX (GPIOB, _LCDCONT);
  write (cmdlcd);
}

void LCD::clear () {
  // command (0x80);       // similar to cursor home but it *works*
  // delayMicroseconds (1000);              // 500us too few; 1000 enough
  setLoc(0, 0);
  command (ClrLCD);     // clears display and sends cursor home
  delayMicroseconds (1000);              // 500us too few; 1000 enough
  // delayMicroseconds (1520);           // CrsrHm needs 1.52 ms
}

void LCD::clearToEOL () {
	for (byte i = _col; i <= 16; i++) {
		write (' ');
	}
}

void LCD::setLoc (byte row, byte col) {
	_row = row;
	_col = col;
	byte loc = 0x80 | (0x40 * _row) | _col;
	command (loc);
}

void LCD::getLoc (byte *row, byte *col) {
	*row = _row;
	*col = _col;
}

void LCD::display (char *theString) {
  for (unsigned int i = 0; i < strlen(theString); i++) {
  	write (theString[i]);
  }
}

void LCD::display (int theNumber, char *theFormat) {
	sprintf (_buffer, theFormat, theNumber);
  for(byte i=0; i < strlen(_buffer); i++) {
    write(_buffer[i]);
  }
}

void LCD::display (byte theByte) {
	char theChar;
	for (byte i = 0; i < 5; i++) {
		if (theByte & 0x80) {
			theChar = '1';
		} else {
			theChar = '0';
		}
		theByte = theByte << 1;
		write(theChar);
	}
}

