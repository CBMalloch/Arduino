/*
	LCD216s - library for controlling a Mindsensors LCD216s 2x16 LCD screen
	Created by Charles B. Malloch, PhD, February 16, 2014
	Released into the public domain
	
	Timing: even after deleting unnecessary delays, time required for writes is 
	        about 1ms per character. It behooves to write only those characters 
	        necessary, addressing randomly rather than repeating unchanging text.
	        It therefore also behooves to have fixed-width fields.
	        
*/

#ifndef LCD216s_h
#define LCD216s_h

#define LCD216s_VERSION "1.000.000"

#include <Arduino.h>

class LCD216s {
	public:
		LCD216s         ( );
		LCD216s         ( int address );
    
    void init ();
		void init ( int address );
		void command    ( int character, unsigned char * args, int nChars );
		void command    ( unsigned char character, unsigned char * args, int nChars );
    void send ( unsigned char * strBuf, int nChars );
		void clear      ( );
    void home       ( );
		// void clearToEOL ();                      // from current column to EOL
		void setLoc     (byte row, byte col);    // ! origin at (0,0) !
    // void getLoc     (byte *row, byte *col);
    // note - these were dispString, dispNum, and dispBits respectively
    //        before I learned to overload the operator
		// void display    (char *);
		// void display    (int, char *);
		// void display    (byte);
    
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> needed only for disambiguation

		// void write ( char );
		
	private:
		int           _address;
		// char          _buffer[LCDRowLen+1];
		// byte          _row, _col;                // current row and column, zero based
		
};


#endif