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
 
  MODBUS RTU Memory Map
    Modbus RTU
    Data Type 	      Common name 	               Starting address
    Modbus Coils 	    Bits, binary values, flags 	    00001
    Digital Inputs 	  Binary inputs                  	10001
    Analog Inputs 	  Binary inputs 	                30001
    Modbus Registers 	Analog values, variables 	      40001

  In this implementation, the addresses are *not* as above, but are zero-radix
  addresses into coilArray and regArray.
  
  counts in requests are in terms of the units requested ( coils are bits, registers are 2 bytes )
  
*/

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <MODBUS.h>

// #define TESTMODE 1
#undef TESTMODE

//################## MODBUS ###################

MODBUS::MODBUS() {
}

MODBUS::MODBUS( 
                            Stream * port,
                            short pdTalkEnable
                          ) {
                          
	Init ( port, pdTalkEnable );
  
}

void MODBUS::Init (  
                    Stream * port,
                    short pdTalkEnable
                  ) {
                  
  _RS485.Init ( port, pdTalkEnable );
  
}


// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

//################## Send ###################
// Takes:   In Data Buffer, Length
// Returns: Nothing
// Effect:  Sends Response over serial port

void MODBUS::Send ( unsigned char * buf, short nChars ) {
  CRC CheckSum;	// From Checksum Library, CRC16.h, CRC16.cpp
  if ( nChars > 0 )	{
    unsigned short CRC = CheckSum.CRC16 ( buf, nChars );
    buf[nChars++] = CRC & 0x00ff;				// lower byte
    buf[nChars++] = CRC >> 8;						// upper byte
    _RS485.Send ( buf, nChars );
  }
  
}

int MODBUS::Receive ( unsigned char * buf, unsigned int bufLen, long receive_timeout_ms ) {

  return ( _RS485.Receive ( buf, bufLen, receive_timeout_ms ) );
  
}


// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

// helper function

void formatHex ( unsigned short x, char * buf, unsigned char len ) {
  // expect buf to be of length at least 7
  char h[] = {'0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  buf[0] = '0'; buf[1] = 'x';
  for ( int i = 0; i < len; i++ ) {
    buf [ i + 2 ] = h [ ( x >> ( ( len - i - 1 ) * 4 ) ) & 0x000f ];
    buf [ i + 3 ] = '\0';
  }
}

