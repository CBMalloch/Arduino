/*
	MODBUS_Slave.cpp is a library for the Arduino System.  It complies with MODBUS_ protocol, customized for exchanging 
	information between Industrial controllers and the Arduino board.
	
	Copyright (c) 2012 Tim W. Shilling (www.ShillingSystems.com)
	Arduino MODBUS_ Slave is free software: you can redistribute it and/or modify it under the terms of the 
	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
	or (at your option) any later version.

	Arduino MODBUS_ Slave is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.
  
	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
  
  Heavily modified by Charles B. Malloch, PhD  2013-02-12
  Mainly to allow me to use either hardware serial or SoftwareSerial for the RS485 communications
  But then also for my peculiar anally-motivated readability standards
  
  Note that in main program, one should define coilArray thus:
    unsigned short coils[ ( nCoils + 15 ) >> 4 ];

*/

#ifndef MODBUS_Slave_h
#define MODBUS_Slave_h

#define MODBUS_Slave_version 1.2.3

#include <Stream.h>
#include <CRC16.h>
#include <MODBUS.h>

#define pdLED 13

// how much time we allow for an incoming message to be under construction before we
// decide it's time to discard it as an orphan or as a malformed frame
// #define MESSAGE_TIME_TO_LIVE_ms ( 2 * ( bufLen * 10 ) / ( BAUDRATE / 1000 ) )
// 80 * 10 / 11.5 is about 70 plus some fudge is 100
// #define MESSAGE_TIME_TO_LIVE_ms 100
// Instead of the *first* char starting the timer, let's use the most recent one. 
// At 9600 baud (slowest likely rate), one character of 10 bits takes 1/960 sec,
// so timeout could be 1 ms
#define MESSAGE_TTL_ms 5

/*
  Trying to strip it down to run on an ATtiny85 or ATtiny84. In order to do that, I want to 
  remove all extra code from the class. In particular, the VERBOSITY code should be disabled 
  when compiling for an ATtiny. To this end, I wanted to use conditional compilation 
  and so needed to find the designation for the ATtiny85 that I am using.
  
#if defined(__AVR_ATmega328P__)
#error GOT 328
#elif ! defined(ATtiny85)
#error NOT GOT ATtiny85
#elif defined(__AVR_ATtiny85__)
#error GOT __AVR_ATtiny85__
#else
#error ERROR BAD ASS!!!
#endif

*/



class MODBUS_Slave {
	public:
    MODBUS_Slave ();

		MODBUS_Slave (  
                    char address,
                    short nCoils,       // number of individual *bits*
                    unsigned short * coilArray,
                    short nRegs,
                    short * regArray,
                    MODBUS MODBUS_port
                 );

		void Init ( 
                char address,
                short nCoils,       // number of individual *bits*
                unsigned short * coilArray,
                short nRegs,
                short * regArray,
                MODBUS MODBUS_port
             );

    int Execute ();
    int Check_Data_Frame ( unsigned char * msg_buffer, char msg_len );
		void Process_Data    ( unsigned char * msg_buffer, char msg_len );
    
    void Set_Verbose ( Stream * diagnostic_port, int VERBOSITY );
    
	private:
    void Send_Response ( unsigned char * Data_In, short Length );
    
    void Read_Exception ( unsigned char * Data_In );    // Function code 7
    
    void Read_Coils ( unsigned char * Data_In );        // Function code 1 (coils), 2 (discrete inputs)
    void Write_Single_Coil ( unsigned char * Data_In ); // Function code 5
    void Write_Coils ( unsigned char * Data_In );       // Function code 15
    
    void Read_Reg ( unsigned char * Data_In );          // Function code 3 (holding reg), 4 (input reg)
    void Write_Single_Reg ( unsigned char * Data_In );  // Function code 6
    void Write_Regs ( unsigned char * Data_In );        // Function code 16
    
		char _address;
		short _nCoils;
		unsigned short * _coilArray;
    short _nRegs;
    short * _regArray;
    
    MODBUS _MODBUS_port;
    #if ! defined(ATtiny85)
    Stream * _diagnostic_port;
    int _VERBOSITY;
    #endif

		unsigned char _error;
};

#endif
		