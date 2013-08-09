/*
	RS485.cpp is a library for Arduino which facilitates communications using the RS485 protocol
  implemented on top of either the built-in Arduino Serial object or the SoftwareSerial library 
  included with the Arduino software IDE distribution.
    
	Copyright (c) 2013 Charles B. Malloch, PhD
	Arduino RS485 is free software: you can redistribute it and/or modify it under the terms of the 
	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
	or (at your option) any later version.

	Arduino RS485 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.
  
	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
  
  Written by Charles B. Malloch, PhD  2013-02-20
  
*/

#ifndef RS485_h
#define RS485_h

#define RS485_h_version 0.1.0

#include <Stream.h>

class RS485 {
	public:
    RS485 ();
		RS485 ( 
                Stream *serialPort,
                short pdTalkEnable = -1
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
                 
    // errorFlag: 0 -> OK; 1 -> buffer overrun on receive
    short errorFlag;

	private:
    Stream * _port;
    short _pdTalkEnable;
};

#endif
		