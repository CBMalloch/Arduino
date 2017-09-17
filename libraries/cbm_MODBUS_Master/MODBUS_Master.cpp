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


int MODBUS_Master::Read_Coils ( unsigned char slave_address, short startCoil, short nCoils, short * values, short lenValues ) {

  //   1 0x01 read_coils        < ss 01 aaaa nnnn cccc >             -> < ss 01 bc xx ... cccc >

  //   memcpy ( strBuf485, "\x01\x05\x00\x00\x00\x01\x00\x00", 8 );
  
  // length of command will be 1 (slave address) + 1 (command) + 2 (startCoil) + 2 (nCoils) 
  //                          + (added later) 2 (CRC16)
  //                   = 8
  
  int msgLen = 8;
  int nBytes = ( nCoils + 7 ) >> 3;
  int expectedReplyLen = 5 + nBytes;
  
  // lenValues is in units of shorts, which are 2 bytes each
  if ( ( msgLen > MODBUS_MASTER_BUF_LEN ) || ( nBytes > ( lenValues << 1 ) ) ) {
    _error = -1;
    return ( _error );
  }
  
  _bufPtr = 0;
  
  // construct message
  _strBuf [ _bufPtr++ ] = slave_address;
  _strBuf [ _bufPtr++ ] = 0x01;            // function code: write single coil
  _bufPtr = appendShort ( _strBuf, _bufPtr, startCoil );
  _bufPtr = appendShort ( _strBuf, _bufPtr, nCoils );
  
  _MODBUS_port.Send ( _strBuf, msgLen - 2 );  // Send expects the length to exclude the CRC, which it places
  _bufPtr = 0;  

  if ( _VERBOSITY >= 6 ) {
    _diagnostic_port->print ( F ( "        Read " ) ); 
    _diagnostic_port->print ( nCoils );
    _diagnostic_port->print ( F ( " coils, starting with " ) );
    _diagnostic_port->println ( startCoil );

    // _diagnostic_port->print (F ( "        Sent: " ));
    _diagnostic_port->print ( '\n' );
  }

  // actual port writing is asynchronous; at 57600 baud, it will take 1/5760 sec per char, or 1/6 ms per char
  delay ( 1 + msgLen / 6 );
  
  for ( int i = 0; i < msgLen; i++ ) _strBuf [ i ] = 0x77;
  delay ( 20 );
      
  GetReply();
  
  if ( ( _receive_status == 0 ) && ( _bufPtr >= expectedReplyLen ) ) {
  
    if ( _VERBOSITY >= 7 ) {
      for ( int i = 0; i < _strBuf [ 2 ]; i++ ) {
        _diagnostic_port->print ( F ( " 0x" ) ); _diagnostic_port->print ( _strBuf [ i + 3 ], HEX );
      }
    }
    
    // the reply is in 8-bit bytes, with the first byte representing 
    //   coils Address+7 - Address in order, the second Address+15 - Address+8, etc.
    
    // copy the bytes in order without transposition
    memcpy ( ( byte * ) values, &_strBuf [ 3 ], nBytes );
    
    if ( _VERBOSITY >= 7 ) {
      _diagnostic_port->print ( F ( " -> " ) );
      for ( int i = 0; i < 4; i++ ) {
        _diagnostic_port->print ( F ( " 0x" ) ); 
        _diagnostic_port->print ( * ( ( byte * ) values + i ), HEX );
      }
      _diagnostic_port->println ();
    }
    
  } else {
    // _receive_status != 0 or reply too short
    
  
    if ( _receive_status != 0 ) {
      if ( _VERBOSITY >= 2 ) {
        _diagnostic_port->print ( F ( "Bad reply: discarding and resetting receive status of 0x" ) );
        _diagnostic_port->println ( _receive_status, HEX);
      }
      _error |= 0x01;
    }
    if ( _bufPtr < expectedReplyLen ) {
        // reply too short
      if ( _VERBOSITY >= 2 ) {
        _diagnostic_port->print ( F ( "Bad reply: reply len ( " ) );
        _diagnostic_port->print ( _bufPtr );
        _diagnostic_port->print ( F ( " ) < required ( " ) );
        _diagnostic_port->print ( expectedReplyLen );
        _diagnostic_port->println ( F ( " )" ) );
      }
      _error |= 0x02;
    }
    
    _receive_status = 0;
    _bufPtr = 0;
  }
    
  return ( _error );  

}


void MODBUS_Master::Write_Single_Coil ( unsigned char slave_address, short coilNo, short value ) {


  //   memcpy ( strBuf485, "\x01\x05\x00\x00\x00\x01\x00\x00", 8 );
  
  // length of command will be 1 (slave address) + 1 (command) + 2 (coilNo) + 2 (nRegs) 
  //                          + 2 (data) + (added later) 2 (CRC16)
  //                   = 10
  
  int msgLen = 10;  // 8 -> 10 2014-02-21
  if ( msgLen > MODBUS_MASTER_BUF_LEN ) {
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
  
  // actual port writing is asynchronous; at 57600 baud, it will take 1/5760 sec per char, or 1/6 ms per char
  delay ( 1 + msgLen / 6 );
      
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
  if ( msgLen > MODBUS_MASTER_BUF_LEN ) {
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
  
  if ( _VERBOSITY >= 7 ) {
    _diagnostic_port->println (F ( "        Values: " ));
    for ( int i = 0; i < nRegs; i++ ) {
      _diagnostic_port->print ( F ( "          " ) ); _diagnostic_port->print ( values [ i ] );
    }
    _diagnostic_port->print ( '\n' );

    _diagnostic_port->print (F ( "        Sent: " ));
    for ( int i = 0; i < msgLen; i++ ) {
      _diagnostic_port->print ( F ( " 0x" ) ); _diagnostic_port->print ( _strBuf [ i ], HEX );
    }
    _diagnostic_port->print ( '\n' );
  }

  // actual port writing is asynchronous; at 57600 baud, it will take 1/5760 sec per char, or 1/6 ms per char
  delay ( 1 + msgLen / 6 );
      
  GetReply();

}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

int MODBUS_Master::Read_Reg ( unsigned char slave_address, short startReg, short nRegs, short * values, short lenValues ) {

  // standard Arduino shorts are 1 byte; ints are 2 bytes; standard floats are 4 bytes.
  // lenValues is in units of register_size, which is 2 bytes
  
  // memcpy ( strBuf485, "\x01\x10\x00\x00\x00\x02\x04\xff\xff\xff\xff\x00\x00", 13 );
  
  // length of command will be 1 (slave address) + 1 (command) + 2 (reg) + 2 (nRegs)
  //                          + (added later) 2 (CRC16)
  //                   = 8
  
  // length of response will be 1 (slave address) + 1 (command) + 1 (n data bytes) 
  //                          + < the data bytes > + 2 (CRC16)
  //                   = 5 + nBytes
  
  // see http://www.simplymodbus.ca/FC04.htm

  // return value of _error:
  //   -1 -> insufficient buffer length, either send buffer or destination vector (values)
  //   bit 0 -> failed receipt from slave with status in _receive_status
  //   bit 1 -> reply too short 
  
  int msgLen, expectedReplyLen;
  msgLen = 8;
  expectedReplyLen = 5 + 2 * nRegs;
  
  _error = 0;
  
  // lenValues is in units of shorts, which are 2 bytes each
  if ( ( msgLen > MODBUS_MASTER_BUF_LEN ) || ( nRegs > ( lenValues << 1 ) ) ) {
    _error = -1;
    return ( _error );
  }
  
  _bufPtr = 0;
  
  // construct message
  _strBuf [ _bufPtr++ ] = slave_address;
  _strBuf [ _bufPtr++ ] = 0x04;            // function code: read register
  _bufPtr = appendShort ( _strBuf, _bufPtr, startReg );
  _bufPtr = appendShort ( _strBuf, _bufPtr, nRegs  );  // register count
  
  _MODBUS_port.Send ( _strBuf, msgLen - 2 );  // Send expects the length to exclude the CRC, which it places
  _bufPtr = 0;  

  if ( _VERBOSITY >= 6 ) {
    _diagnostic_port->print ( F ( "        Read " ) ); 
    _diagnostic_port->print ( nRegs );
    _diagnostic_port->print ( F ( " regs, starting with " ));
    _diagnostic_port->println ( startReg );

    // _diagnostic_port->print (F ( "        Sent: " ));
    _diagnostic_port->print ( '\n' );
  }

  // actual port writing is asynchronous; at 57600 baud, it will take 1/5760 sec per char, or 1/6 ms per char
  delay ( 1 + msgLen / 6 );
  
  for ( int i = 0; i < msgLen; i++ ) _strBuf [ i ] = 0x77;
  delay ( 20 );
      
  GetReply();
  
  if ( ( _receive_status == 0 ) && ( _bufPtr >= expectedReplyLen ) ) {
  
    if ( _VERBOSITY >= 7 ) {
      for ( int i = 0; i < _strBuf [ 2 ]; i++ ) {
        _diagnostic_port->print ( F ( " 0x" ) ); _diagnostic_port->print ( _strBuf [ i + 3 ], HEX );
      }
    }
    
    // memcpy ( values, &_strBuf [ 3 ], _strBuf [ 2 ] );  NO NO NO
    
    // note that the byte endian-ness has been switched for MODBUS, so we have to gyrate it back
    for ( int k = 0; k < nRegs; k++ ) {
      memcpy ( ( byte * ) values + 0 + 2 * k, &_strBuf [ 3 + 1 ] + 2 * k, 1 );
      memcpy ( ( byte * ) values + 1 + 2 * k, &_strBuf [ 3 + 0 ] + 2 * k, 1 );
    }
    
    if ( _VERBOSITY >= 7 ) {
      _diagnostic_port->print ( F ( " -> " ) );
      for ( int i = 0; i < 4; i++ ) {
        _diagnostic_port->print ( F ( " 0x" ) ); 
        _diagnostic_port->print ( * ( ( byte * ) values + i ), HEX );
      }
      _diagnostic_port->println ();
    }
    
  } else {
    // _receive_status != 0 or reply too short
    
  
    if ( _receive_status != 0 ) {
      if ( _VERBOSITY >= 2 ) {
        _diagnostic_port->print ( F ( "Bad reply: discarding and resetting receive status of 0x" ) );
        _diagnostic_port->println ( _receive_status, HEX);
      }
      _error |= 0x01;
    }
    if ( _bufPtr < expectedReplyLen ) {
        // reply too short
      if ( _VERBOSITY >= 2 ) {
        _diagnostic_port->print ( F ( "Bad reply: reply len ( " ) );
        _diagnostic_port->print ( _bufPtr );
        _diagnostic_port->print ( F ( " ) < required ( " ) );
        _diagnostic_port->print ( expectedReplyLen );
        _diagnostic_port->println ( F ( " )" ) );
      }
      _error |= 0x02;
    }
    
    _receive_status = 0;
    _bufPtr = 0;
  }
    
  return ( _error );
  
}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

void MODBUS_Master::GetReply  (
                                unsigned long timeToWaitForReply_us 
                              ) {

  // will leave _bufPtr pointing at first byte of CRC (_bufPtr will equal number of bytes received)
  // will leave _receive_status alone unless there's an error
  //       -81 -> orphan message timeout
  //        99 -> error status from stream
  //          2 -> error in returned message CRC
  
  unsigned long beganWaitingForReply_us;
  unsigned long lastMsgReceivedAt_ms;

  // **************** receive reply ****************
    
  beganWaitingForReply_us = micros();
  _nIterationsRequired = 0;
  _bufPtr = 0;
  
  if ( _VERBOSITY >= 1 ) {
    for ( int k = 0; k < MODBUS_MASTER_BUF_LEN; k++ ) {
      _strBuf [ k ] = '\0';
    }
  }
  
  while ( ( ! _bufPtr ) && ( ( micros() - beganWaitingForReply_us ) < timeToWaitForReply_us ) ) {
    // RS485.Receive tests for buffer overrun and sets its errorFlag if it happens
    // MODBUS.Receive sets its own errorFlag if RS485 reports an error
    _bufPtr = _MODBUS_port.Receive ( _strBuf, MODBUS_MASTER_BUF_LEN, 2 );
    if ( _MODBUS_port.errorFlag == 1 ) { 
      _diagnostic_port->print ( F ( "MODBUS_Master receive buffer overflow!\n" ) );
    }
    if ( _VERBOSITY >= 3 ) _diagnostic_port->print ( '.' );
    _nIterationsRequired++;
  }
  _itTook_us = micros() - beganWaitingForReply_us;
  
  // _bufPtr is equal to the number of characters received, including any CRC
  
  if ( _bufPtr ) {
    lastMsgReceivedAt_ms = millis();
    if ( _VERBOSITY >= 3 ) {
      _diagnostic_port->print ( F ( "Reply receipt turnaround took " ) );
      _diagnostic_port->print ( _itTook_us );
      _diagnostic_port->print ( F ( "us in " ) );
      _diagnostic_port->print ( _nIterationsRequired );
      _diagnostic_port->println ( F ( " iterations" ) );
    }
  } else {
    if ( _VERBOSITY >= 1 ) {
      _diagnostic_port->print ( F ( "Message receive timeout (" ) );
      _diagnostic_port->print ( timeToWaitForReply_us );
      _diagnostic_port->println ( F ( "us) exceeded" ) );
    }
  }
  
  if ( ( ( millis() - lastMsgReceivedAt_ms ) > MBM_MESSAGE_TTL_ms ) && ( _bufPtr != 0 ) ) {
    // kill old messages to keep from blocking up with a bad message piece
    _diagnostic_port->println ( F ( "orphan message timeout" ) );
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
    
    // check CRC
    unsigned short msg_CRC = ( ( _strBuf [ _bufPtr - 1 ] << 8 ) | _strBuf [ _bufPtr - 2 ] );
    // my calculated CRC
    CRC CheckSum;
    unsigned short recalculated_CRC = CheckSum.CRC16 ( _strBuf, _bufPtr - 2 );
    if ( recalculated_CRC != msg_CRC ) {
      #if ! defined(ATtiny85)
        if ( _VERBOSITY >= 1 ) {
          char buf[7];
          _diagnostic_port->print ( F ( " -- failed CRC -- got " ) );
          formatHex ( ( unsigned char * ) &msg_CRC, buf );
          _diagnostic_port->print ( buf );
          _diagnostic_port->print ( F ( "; expected " ) );
          formatHex ( ( unsigned char * ) &recalculated_CRC, buf );
          _diagnostic_port->print ( buf );
          _diagnostic_port->print ( F ( "\n" ) );
        }
      #endif
      _receive_status = 2;
    }
    
    if ( _VERBOSITY >= 8 ) {
      _diagnostic_port->print (F ( "        Received: " ));
      for ( int i = 0; i < _bufPtr; i++ ) {
        _diagnostic_port->print ( F ( " 0x" ) ); _diagnostic_port->print ( _strBuf [ i ], HEX );
      }
      _diagnostic_port->print ( F ( "; status " ) );
      _diagnostic_port->println ( _receive_status );
    }
    
  } else {
    // _bufptr <= 0
    if ( _VERBOSITY >= 6 ) {
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


