/*
	LCD.h - library for controlling a standard LCD (HD44780 Hitachi 2x40 LCD display)
	        connected via MCP23017 serial port extender
	Created by Charles B. Malloch, PhD, May 7, 2009
	Released into the public domain
	
	Timing: even after deleting unnecessary delays, time required for writes is 
	        about 1ms per character. It behooves to write only those characters 
	        necessary, addressing randomly rather than repeating unchanging text.
	        It therefore also behooves to have fixed-width fields.
	        
*/

#ifndef LCD_h
#define LCD_h

#define LCD_VERSION "1.001.000"

#include <MCP23017.h>

#define LCDRowLen 16

class LCD {
	public:
		LCD             ();
		LCD             (MCP23017);
		void command    (byte);
		void clear      ();
		void clearToEOL ();                      // from current column to EOL
		void setLoc     (byte row, byte col);    // ! origin at (0,0) !
    void getLoc     (byte *row, byte *col);
    // note - these were dispString, dispNum, and dispBits respectively
    //        before I learned to overload the operator
		void display    (char *);
		void display    (int, char *);
		void display    (byte);
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> needed only for disambiguation
		void init  (MCP23017);
		void write (char);
		
	private:
		char          _LCDCONT;
		MCP23017      _interface;
		char          _buffer[LCDRowLen+1];
		byte          _row, _col;                // current row and column, zero based
		
};

#endif
