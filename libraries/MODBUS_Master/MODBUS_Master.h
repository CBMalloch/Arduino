/*
	MODBUS_Master.cpp is a library for the Arduino System.  It complies with MODBUS protocol, 
  customized for exchanging information between Industrial controllers and the Arduino board.
	
	Copyright (c) 2013 Charles B. Malloch
	Arduino MODBUS_Master is free software: you can redistribute it and/or modify it under the terms of the 
	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
	or (at your option) any later version.

	Arduino MODBUS_Master is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.
  
	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
  
  Written by Charles B. Malloch, PhD  2013-02-21
  
*/

#ifndef MODBUS_Master_h
#define MODBUS_Master_h

#define MODBUS_Master_version 0.0.1

#include <Stream.h>
#include <RS485.h>
#include <MODBUS.h>
#include <CRC16.h>

#define MBM_TIME_TO_WAIT_FOR_REPLY_us 20000LU
#define MBM_MESSAGE_TTL_ms 5

class MODBUS_Master {
	public:
    MODBUS_Master ();
		MODBUS_Master (
                    MODBUS MODBUS_port
                  );
                  
    void Init     ( 
                    MODBUS MODBUS_port
                  );
                  
    void Write_Single_Coil ( unsigned char slave_address, short coilNo, short value );
    void Write_Regs ( unsigned char slave_address, short startReg, short nRegs, short values[] );

    void GetReply ( unsigned long timeToWaitForReply_us = MBM_TIME_TO_WAIT_FOR_REPLY_us );
    
    
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

    void Set_Verbose ( Stream * diagnostic_port, int VERBOSITY );

    RS485 _RS485;
    
  private:
    MODBUS _MODBUS_port;
    
    #if ! defined(ATtiny85)
    Stream * _diagnostic_port;
    int _VERBOSITY;
    #endif
    
    int _receive_status;
    int _nIterationsRequired;
    unsigned long _itTook_us;

    int _error;
    
    // the following *could* be made public for diagnostic purposes
    #define BUF_LEN 80
    unsigned char _strBuf [ BUF_LEN + 1 ];
    int _bufPtr;

};

#endif
		