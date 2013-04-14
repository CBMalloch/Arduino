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
#include <MODBUS_Master.h>

#define TESTMODE 1
// #undef TESTMODE

  
//################## MODBUS ###################

MODBUS_Master::MODBUS_Master () {
}

MODBUS_Master::MODBUS_Master( 
                              MODBUS MODBUS_port
                            ) {
                          
	Init ( MODBUS_port );
  
}

void MODBUS_Master::Init  (  
                            MODBUS MODBUS_port
                          ) {
  
  _MODBUS_port = MODBUS_port;
  
  #if ! defined(ATtiny85)
    _VERBOSITY = 0;
  #endif
  
  _bufPtr = 0;
	_error = 0;
  
}

// ******************************************************************************

int appendShort ( unsigned char * buffer, int bufPtr, short value );

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************




void MODBUS_Master::Write_Single_Coil ( unsigned char slave_address, short coilNo, short value ) {


  //   memcpy ( strBuf485, "\x01\x05\x00\x00\x00\x01\x00\x00", 8 );
  
  // length of command will be 1 (slave address) + 1 (command) + 2 (coilNo) + 2 (nRegs) 
  //                          + 2 (data) + (added later) 2 (CRC16)
  //                   = 8
  
  int msgLen = 8;
  if ( msgLen > BUF_LEN ) {
    _error = -1;
    return;
  }
  
  _bufPtr = 0;
  
  // construct message
  _strBuf [ _bufPtr++ ] = slave_address;
  _strBuf [ _bufPtr++ ] = 0x05;            // function code: write single coil
  _bufPtr = appendShort ( _strBuf, _bufPtr, coilNo );
  _bufPtr = appendShort ( _strBuf, _bufPtr, value );
  
  _MODBUS_port.Send ( _strBuf, msgLen - 2 );  // Send expects the length to exclude the CRC, which it places
      
  GetReply();

}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

void MODBUS_Master::Write_Regs ( unsigned char slave_address, short startReg, short nRegs, short values[] ) {


  // memcpy ( strBuf485, "\x01\x10\x00\x00\x00\x02\x04\xff\xff\xff\xff\x00\x00", 13 );
  
  // length of command will be 1 (slave address) + 1 (command) + 2 (starting reg) + 2 (nRegs) + 1 (byte count) 
  //                          + 2 * nRegs (data) + (added later) 2 (CRC16)
  //                   = 9 + 2 * nRegs
  
  int msgLen = 9 + 2 * nRegs;
  if ( msgLen > BUF_LEN ) {
    _error = -1;
    return;
  }
  
  _bufPtr = 0;
  
  // construct message
  _strBuf [ _bufPtr++ ] = slave_address;
  _strBuf [ _bufPtr++ ] = 0x10;            // function code: write multiple registers
  _bufPtr = appendShort ( _strBuf, _bufPtr, startReg );
  _bufPtr = appendShort ( _strBuf, _bufPtr, nRegs );
  _strBuf [ _bufPtr++ ] = 2 * nRegs;       // data byte count
  for ( short i = 0; i < nRegs; i++ ) {
    _bufPtr = appendShort ( _strBuf, _bufPtr, values [ i ] );
  }
  
  _MODBUS_port.Send ( _strBuf, msgLen - 2 );  // Send expects the length to exclude the CRC, which it places
  
  if ( _VERBOSITY > 5 ) {
    _diagnostic_port->println ("        Values: ");
    for ( int i = 0; i < nRegs; i++ ) {
      _diagnostic_port->print ( "          " ); _diagnostic_port->print ( values [ i ] );
    }
    _diagnostic_port->print ( '\n' );

    _diagnostic_port->print ("        Sent: ");
    for ( int i = 0; i < msgLen; i++ ) {
      _diagnostic_port->print ( " 0x" ); _diagnostic_port->print ( _strBuf [ i ], HEX );
    }
    _diagnostic_port->print ( '\n' );
  }

      
  GetReply();

}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

void MODBUS_Master::GetReply  (
                                unsigned long timeToWaitForReply_us 
                              ) {

  unsigned long beganWaitingForReply_us;
  unsigned long lastMsgReceivedAt_ms;

  // **************** receive reply ****************
    
  beganWaitingForReply_us = micros();
  _nIterationsRequired = 0;
  while ( ( ! _bufPtr ) && ( ( micros() - beganWaitingForReply_us ) < timeToWaitForReply_us ) ) {
    _bufPtr = _MODBUS_port.Receive ( _strBuf, BUF_LEN, 2 );
    if ( _VERBOSITY > 2 ) _diagnostic_port->print ( "." );
    _nIterationsRequired++;
  }
  _itTook_us = micros() - beganWaitingForReply_us;
  
  if ( _bufPtr ) {
    lastMsgReceivedAt_ms = millis();
    if ( _VERBOSITY > 2 ) {
      _diagnostic_port->print ( "Reply receipt turnaround took " );
      _diagnostic_port->print ( _itTook_us );
      _diagnostic_port->print ( "us in " );
      _diagnostic_port->print ( _nIterationsRequired );
      _diagnostic_port->println ( " iterations" );
    }
  } else {
    if ( _VERBOSITY > 0 ) {
      _diagnostic_port->print ( "Message receive timed out of testing (" );
      _diagnostic_port->print ( timeToWaitForReply_us );
      _diagnostic_port->println ( "us)" );
    }
  }
  
  if ( ( ( millis() - lastMsgReceivedAt_ms ) > MBM_MESSAGE_TTL_ms ) && ( _bufPtr != 0 ) ) {
    // kill old messages to keep from blocking up with a bad message piece
    _diagnostic_port->println ( "timeout" );
    _receive_status = -81;  // timeout
    _bufPtr = 0;
  }
  
  if ( _bufPtr > 0 ) {
  
    if ( ( _receive_status == 0 ) 
      && ( _bufPtr > 1 )
      && ( _strBuf[1] & 0x80 ) ) {
      // high order bit of command code in reply indicates error condition exists
      _receive_status = 99;
    }
    
    if ( _VERBOSITY > 5 ) {
      _diagnostic_port->print ("        Received: ");
      for ( int i = 0; i < _bufPtr; i++ ) {
        _diagnostic_port->print ( " 0x" ); _diagnostic_port->print ( _strBuf [ i ], HEX );
      }
      
      _diagnostic_port->print ( "; status " );
      _diagnostic_port->println ( _receive_status );
    }
    
    _bufPtr = 0;

  } else {
    if ( _VERBOSITY > 5 ) {
      _diagnostic_port->println ();
    }
  }
    
}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

// helper functions

int appendShort ( unsigned char * buffer, int bufPtr, short value ) {
  buffer[ bufPtr++ ] = value >> 8;
  buffer[ bufPtr++ ] = value & 0xff;
  return ( bufPtr );
}

// ******************************************************************************

#if ! defined(ATtiny85)

void MODBUS_Master::Set_Verbose ( Stream * diagnostic_port, int VERBOSITY ) {
  _diagnostic_port = diagnostic_port;
  _VERBOSITY = VERBOSITY;
  return;
}

#endif


