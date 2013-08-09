/*
	MODBUS.cpp is a library for the Arduino System.  It complies with MODBUS protocol, 
  customized for exchanging information between Industrial controllers and the Arduino board.
	
	Copyright (c) 2013 Charles B. Malloch
	Arduino MODBUS is free software: you can redistribute it and/or modify it under the terms of the 
	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
	or (at your option) any later version.

	Arduino MODBUS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.
  
	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
  
  Written by Charles B. Malloch, PhD  2013-02-21
  
  Note that in main program, one should define coilArray thus:
    unsigned short coils[ ( nCoils + 15 ) >> 4 ];

*/

#ifndef MODBUS_h
#define MODBUS_h

#define MODBUS_version 0.0.1

#include <Stream.h>
#include <RS485.h>
#include <CRC16.h>

// how much time we allow for an incoming message to be under construction before we
// decide it's time to discard it as an orphan or as a malformed frame
// #define MESSAGE_TIME_TO_LIVE_ms ( 2 * ( bufLen * 10 ) / ( BAUDRATE / 1000 ) )
// 80 * 10 / 11.5 is about 70 plus some fudge is 100
// #define MESSAGE_TIME_TO_LIVE_ms 100
// Instead of the *first* char starting the timer, let's use the most recent one. 
// At 9600 baud (slowest likely rate), one character of 10 bits takes 1/960 sec,
// so timeout could be 1 ms
#define MESSAGE_TIMEOUT_ms 5

// len is in units of bytes -- 2 hex characters per byte
// buf must be at least 2 * len + 3 to account for "0x" and "\0"
void formatHex ( unsigned char * x, char * buf, unsigned char len = 2 );

class MODBUS {

	public:
    MODBUS ();
		MODBUS (
                Stream *serialPort,
                short paTalkEnable = -1
           );
		void Init ( 
                Stream *serialPort,
                short pdTalkEnable = -1
              );
    
    void Send ( unsigned char * buf, short nChars );
    
    
    // Receive returns the number of characters received
    // receive_timeout_ms is how long after the most recent character receipt we should 
    // wait to decide that a message sending is over ( without regard to completeness ).
    // At 9600 baud (slowest likely rate), one character of 10 bits takes 1/960 sec,
    // so timeout could be 1 ms

    int Receive  ( 
                  unsigned char * buf, 
                  unsigned int bufLen, 
                  long receive_timeout_ms = 2
                 );
                 
    RS485 _RS485;
    
    // errorFlag: 0 -> OK; 1 -> buffer overrun on RS485 receive
    short errorFlag;
    
  private:
  
};

#endif
		