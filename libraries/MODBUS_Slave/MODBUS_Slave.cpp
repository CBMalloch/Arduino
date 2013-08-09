/*
	MODBUS_Slave.cpp is a library for the Arduino System.  
  It complies with MODBUS RTU protocol, customized for exchanging 
	information between Industrial controllers and the Arduino board.
	
	Copyright (c) 2012 Tim W. Shilling (www.ShillingSystems.com)
	Arduino MODBUS Slave is free software: you can redistribute it and/or modify it under the terms of the 
	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
	or (at your option) any later version.

	Arduino MODBUS Slave is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.

	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
  
  Heavily modified by Charles B. Malloch, PhD  2013-02-12
  Mainly to allow me to use either hardware serial or SoftwareSerial for the RS485 communications
  But then also for my peculiar anally-motivated readability standards
  
  MODBUS RTU Memory Map
    Modbus RTU
    Data Type 	      Common name 	               Starting address
    Modbus Coils 	    Bits, binary values, flags 	    00001
    Digital Inputs 	  Binary inputs                  	10001
    Analog Inputs 	  Binary inputs 	                30001
    Modbus Registers 	Analog values, variables 	      40001

  I (Chuck) am assuming that the addresses are *not* as above, but are zero-radix
  addresses into coilArray and regArray.
  
  counts in requests are in terms of the units requested ( coils are bits, registers are 2 bytes )
  
*/

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <MODBUS_Slave.h>

// #define TESTMODE 1
#undef TESTMODE

// a character time at 57600 baud = 5760 cps is 1/5.76 ms = 0.17 ms
// at 9600 baud = 960 cps it's 1/.96 = 1 ms
#define MODBUS_RECEIVE_TIMEOUT_ms 2

char hexBuf [ 8 ];

//################## MODBUS_Slave ###################
// Takes:   Slave Address, Pointer to Registers and Number of Available Registers
// Returns: Nothing
// Effect:  Initializes Library

MODBUS_Slave::MODBUS_Slave() {
}

MODBUS_Slave::MODBUS_Slave(  
                            char my_address,
                            short nCoils,
                            unsigned short * coilArray,
                            short nRegs,
                            short * regArray,
                            MODBUS MODBUS_port
                          ) {
                          
	Init (my_address, nCoils, coilArray, nRegs, regArray, MODBUS_port);
}

void MODBUS_Slave::Init(  
                          char my_address, 
                          short nCoils,
                          unsigned short * coilArray,
                          short nRegs,
                          short * regArray,
                          MODBUS MODBUS_port
                        ) {
                        
	_address = my_address;
	_nCoils = nCoils;
	_coilArray = coilArray;
  _nRegs = nRegs;
  _regArray = regArray;
  
  _MODBUS_port = MODBUS_port;
  
  #if ! defined(ATtiny85)
    _VERBOSITY = 0;
  #endif
  
	_error = 0;
  
}

//################## Check Data Frame ###################
// Takes:   Data stream buffer from serial port, number of characters
// Returns: -1 if frame incomplete, 
//           0 if frame complete but invalid, 
//           0 if frame complete but not for us
//           1 if frame complete and valid
// Effect:  Checks completeness, validity, and appropriateness of frame

int MODBUS_Slave::Check_Data_Frame ( unsigned char * msg_buffer, char msg_len ) {

  if ( msg_len < 4 ) {
    // valid frame has address (1), function code (1), and CRC (2)
    return (-1);
  }
  
  unsigned char msg_address = msg_buffer[0];
	if ( msg_address > 247 ) {
    // invalid
		_error = 1;
    #if ! defined(ATtiny85)
      if ( _VERBOSITY >= 5 ) {
        _diagnostic_port->print ( "  invalid addressee: 0x" ); _diagnostic_port->println ( msg_address, HEX );
      }
    #endif
		return (0);
	}
  if ( msg_address != 0 && msg_address != _address) {
    // ignore unless address matches or it's a broadcast (0)
    #if ! defined(ATtiny85)
      if ( _VERBOSITY >= 10 ) {
        _diagnostic_port->print ( "  msg not for me (msg: 0x" ); 
        _diagnostic_port->print ( msg_address, HEX ); _diagnostic_port->print ( " != me: 0x" ); 
        _diagnostic_port->print ( _address, HEX ); _diagnostic_port->print ( ")\n" ); 
      }
    #endif
    return (0);
  }
  
  char function_code = msg_buffer[1];
  char required_len;
  // check completeness by function code
  switch (function_code) {
    case 0x01:  // read MODBUS coils
    case 0x02:  // read discrete inputs
    case 0x03:  // read holding reg
    case 0x04:  // read input reg
    case 0x05:  // write one coil
    case 0x06:  // write one register
      required_len = 6;
      break;
    case 0x07:   // read exception
      required_len =  2;
      break;
    case 0x0f: // write multiple coils
    case 0x10: // write multiple registers
      if ( msg_len < 7 ) {
        // no length character available yet
        return ( -1 );  // frame is incomplete
      }
      required_len = 7 + msg_buffer[6];  // length in bytes of data segment...
      break;
    default: // unimplemented function code
      #if ! defined(ATtiny85)
        if ( _VERBOSITY >= 5 ) {
          _diagnostic_port->print ( "  unimplemented function code (0x" );
          _diagnostic_port->print ( function_code, HEX ); 
          _diagnostic_port->print ( ")\n" );
        }
      #endif
      
      _error = 0x01;
      return (0);
  }
  required_len += 2;  // two more bytes needed for CRC
  if ( msg_len < required_len ) {
    return (-1);
  }
  
  // check CRC
  unsigned short msg_CRC = ( ( msg_buffer [ msg_len - 1 ] << 8 ) | msg_buffer [ msg_len - 2 ] );
  // my calculated CRC
  CRC CheckSum;
  unsigned short recalculated_CRC = CheckSum.CRC16( msg_buffer, msg_len - 2 );
  if ( recalculated_CRC != msg_CRC ) {
    _error = 2;
    #if ! defined(ATtiny85)
      if ( _VERBOSITY >= 5 ) {
        char buf[7];
        _diagnostic_port->print ( " -- failed CRC -- got " );
        formatHex ( ( unsigned char * ) &msg_CRC, buf, 2 );
        _diagnostic_port->print ( buf );
        _diagnostic_port->print ( "; expected " );
        formatHex ( ( unsigned char * ) &recalculated_CRC, buf, 2 );
        _diagnostic_port->print ( buf );
        _diagnostic_port->print ( "\n" );
      }
    #endif
    #ifndef TESTMODE
      // don't return - simply ignore invalidity of CRC
      return (0);
    #endif
  }
  
  return (1);
}


//################## Process Data ###################
// Takes:   Data stream buffer from serial port, number of characters to read
// Returns: Nothing
// Effect:  Accepts and parses data

void MODBUS_Slave::Process_Data ( unsigned char * msg_buffer, char msg_len ) {

  int frame_valid = Check_Data_Frame ( msg_buffer, msg_len );
  switch ( frame_valid ) {
    case -1:  // short frame
      return;
    case 0:   // invalid frame or not for us
      return;
    case 1:   // frame valid and complete
      break;
  }
  
  char function_code = msg_buffer[1];
  switch ( function_code ) {
    case 0x01:  // read MODBUS coils
    case 0x02:  // read discrete inputs
      Read_Coils ( msg_buffer );
      break;
    case 0x03:  // read register
    case 0x04:  // read input reg
      Read_Reg ( msg_buffer );
      break;
    case 0x05:
      Write_Single_Coil ( msg_buffer );
      break;
    case 0x06: 
      Write_Single_Reg ( msg_buffer );
      break;
    case 0x07:
      Read_Exception ( msg_buffer );
      break;
    case 0x0f:
      Write_Coils ( msg_buffer );
      break;
    case 0x10:
      Write_Regs ( msg_buffer );
      break;
    default:
      #if ! defined(ATtiny85)
        if ( _VERBOSITY >= 5 ) {
          _diagnostic_port->print ("bad cmd code: 0x"); _diagnostic_port->println ( function_code, HEX );
        }
      #endif
      _error = 0x01;
      break;
  }
  
}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

//################## Send Response ###################
// Takes:   In Data Buffer, Length of data (exclusive of CRC which MODBUS.Send supplies) in bytes
// Returns: Nothing
// Effect:  Sends Response over serial port

void MODBUS_Slave::Send_Response ( unsigned char *buf, short nChars ) {

	if ( buf[0] == 0 ) {
    // don't reply	to broadcast
		return;
  }
  
  _MODBUS_port.Send ( buf, nChars );
  
  #if ! defined(ATtiny85)
    if ( _VERBOSITY >= 10 ) {
      
      _diagnostic_port->print ( "Reply ( " );
      _diagnostic_port->print ( nChars );
      _diagnostic_port->println ( " payload bytes ): " );
      for ( int i = 0; i < nChars + 2; i++ ) {
        formatHex ( ( unsigned char * ) &buf [ i ], hexBuf, 1);
        _diagnostic_port->print ( hexBuf );
        _diagnostic_port->print ( " " );
      }
      _diagnostic_port->println ();
    }
  #endif
  
}

//################## Read Exception ###################
// Takes:   In Data Buffer
// Returns: Nothing
// Effect:  Sets Reply Data and sends Response

/*

  MODBUS exception codes:
    1 illegal function
    2 illegal data address
    3 illegal data value
    4 slave device failure
    ... others at http://www.simplymodbus.ca/exceptions.htm
    
*/

void MODBUS_Slave::Read_Exception( unsigned char *buf ) {

	buf[2] = _error;
	Send_Response( buf, 3 );
	_error = 0;
}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

//################## Read Coils (bits) ###################
// Takes:   In Data Buffer: address (zero-radix) and count of bits requested
// Returns: Nothing
// Effect:  Reads coils a bit at a time, composing 8 bit replies.  Sets Reply Data and sends Response

void MODBUS_Slave::Read_Coils( unsigned char *buf ) {

  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Cnt_Hi = buf[4];
  unsigned short Cnt_Lo = buf[5];

  unsigned short Bit_Count = Cnt_Lo + ( Cnt_Hi << 8 );
  unsigned char Byte_Count = 0;
  unsigned char Sub_Bit_Count;
  unsigned char Working_Byte;
  
  unsigned short Address = Addr_Lo + ( Addr_Hi << 8 );
  
  if ( ( Address + Bit_Count ) > _nCoils ) {
    // Invalid Address;
		_error = 2;
		return;
	}
  
  short Item = 3;
  Working_Byte = 0;
  Sub_Bit_Count = 0;
  for ( unsigned int Bit = 0; Bit < Bit_Count; Bit++ ) {
    Working_Byte = Working_Byte 
                   | ( ( ( _coilArray [ Address >> 4 ] >> ( Address & 0x000f ) ) & 0x01 )
                       << Sub_Bit_Count );
    Address++;
    Sub_Bit_Count++;
    
    if ( Sub_Bit_Count >= 8 ) {
      #if ! defined(ATtiny85)
        if ( _VERBOSITY >= 15 ) {
          _diagnostic_port->print ("  BBBB 0x"); _diagnostic_port->println ( Working_Byte, HEX );
        }
      #endif
      buf[Item++] = Working_Byte;
      Working_Byte = 0;
      Sub_Bit_Count=0;
      Byte_Count++;
    }
  }
  // if Bit_Count didn't end at a word boundary, we have leftovers
  if ( Sub_Bit_Count != 0 ) {
      #if ! defined(ATtiny85)
        if ( _VERBOSITY >= 15 ) {
          _diagnostic_port->print ("  BBBC 0x"); _diagnostic_port->println ( Working_Byte, HEX );
        }
      #endif
    buf[Item++] = Working_Byte;
    Working_Byte = 0;
    Sub_Bit_Count = 0;
    Byte_Count++;
  }
  buf[2] = Byte_Count; 
  Send_Response ( buf, Item );
}

//################## Write_Single_Coil ###################
// Takes:   In Data Buffer
// Returns: Nothing
// Effect:  Writes single bit in Registers.  Sets Reply Data and sends Response

void MODBUS_Slave::Write_Single_Coil ( unsigned char *buf ) {

  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Value_Hi = buf[4];
  unsigned short Value_Lo = buf[5];
  
  unsigned short Address = Addr_Lo + ( Addr_Hi << 8 );  
  unsigned short Write_Address = Address / 16;
  unsigned char Write_Bit = Address & 0x000F;
  
  if ( Address + 1 > _nCoils ) {
  // Invalid Address;
		_error = 2;
		return;
	}
  if ( Value_Hi > 0 || Value_Lo > 0 ) {
    // Real Protocol requires 0xFF00 = On and 0x0000 = Off, Custom, using anything other than 0 => ON
    _coilArray[Write_Address] |=   ( 1 << Write_Bit );
  } else {
    _coilArray[Write_Address] &= ~ ( 1 << Write_Bit );
  }
  Send_Response ( buf, 6);
}

//################## Write_Coils ###################
// Takes:   In Data Buffer: address (coil number), count of coils to write, number of following data bytes
// Returns: Nothing
// Effect:  Writes multiple bits in Registers.  Sets Reply Data and sends Response

void MODBUS_Slave::Write_Coils ( unsigned char *buf ) {

  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Cnt_Hi = buf[4];
  unsigned short Cnt_Lo = buf[5];
  unsigned char Byte_Count = buf[6];  // number of data bytes following
  
  unsigned short Address = Addr_Lo + ( Addr_Hi << 8 );
  short Write_Bit_Count = Cnt_Lo + ( Cnt_Hi << 8 );
  
  #if ! defined(ATtiny85)
    if ( _VERBOSITY >= 10 ) {
      _diagnostic_port->print ("write bit count: "); 
      _diagnostic_port->print (Write_Bit_Count); 
      _diagnostic_port->print ('\n');
    }
  #endif

  	
  if ( ( Address + Write_Bit_Count ) > _nCoils ) {
		_error = 2;
		return;
	}
  
  if ( Byte_Count < ( 16 * Write_Bit_Count ) ) {
    _error = 2;
    return;
  }
  
  unsigned short Write_Address = Address >> 4;
  unsigned char Write_Bit = Address & 0x000f;
  
  unsigned short Read_Byte = 7; // First entry in input buf
  unsigned char Read_Bit = 0;
  
  for ( int i = 0; i < Write_Bit_Count; i++ ) {
    if ( ( buf [ Read_Byte ] & ( 1 << Read_Bit ) ) > 0 ) {
      _coilArray[Write_Address] |=   ( 0x01 << Write_Bit ); // set
    } else {
      _coilArray[Write_Address] &= ~ ( 0x01 << Write_Bit ); // clear
    }
    Read_Bit++;
    Write_Bit++;
    if ( Read_Bit >= 8 ) {
      // reads are from 8-bit bytes
      Read_Bit = 0;
      Read_Byte++;
    }
    if ( Write_Bit >= 16 ) {
      // writes are to 16-bit registers, so increment every 16th bit.	
      Write_Bit = 0;
      Write_Address++;
    }
  }
  Send_Response ( buf, 6 );
}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

//################## Write_Single_Reg ###################
// Takes:   In Data Buffer
// Returns: Nothing
// Effect:  Sets Reply Data and Register values, sends Response

void MODBUS_Slave::Write_Single_Reg ( unsigned char *buf ) {
  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Value_Hi = buf[4];
  unsigned short Value_Lo = buf[5];

  short Address = ( Addr_Lo + ( Addr_Hi << 8 ) );
  
  if ( Address + 1 > _nRegs )	{
  // Invalid Address;
		_error = 2;
		return;
	}
  _regArray[Address] = ( Value_Hi << 8 ) + Value_Lo;
  Send_Response ( buf, 6 );
}

//################## Write_Regs ###################
// Takes:   In Data Buffer: address (zero-radix), count ( 2-byte registers ), number of following data bytes
// Returns: Nothing
// Effect:  Writes Register in Registers.  Sets Reply Data and sends Response

void MODBUS_Slave::Write_Regs ( unsigned char *buf ) {
  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Cnt_Hi = buf[4];
  unsigned short Cnt_Lo = buf[5];
  char Byte_Count = buf[6];
  
  unsigned short Address = ( Addr_Lo + ( Addr_Hi << 8 ) );
  unsigned short Count = Cnt_Lo + ( Cnt_Hi << 8 );
  short Read_Byte = 7;  // First entry in input buf

  if ( ( Address + Count ) > _nRegs )	{
  // Invalid Address
		_error = 3;
		return;
	}

  for (int i = 0; i < Byte_Count; i += 2 ) {

    _regArray[ Address++ ] = ( buf [ Read_Byte ] << 8 ) + buf [ Read_Byte + 1 ];
    Read_Byte += 2;
  }
  Send_Response ( buf, Read_Byte ) ;
}



// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

//################## Read Reg ###################
// Takes:   In Data Buffer: address into regArray, count of registers to read
// Returns: Status: 
//     negative value means there was a MODBUS error
//       - 1 - MODBUS error: illegal function
//       - 2 - MODBUS error: illegal data address
//       - 3 - MODBUS error: illegal data value
//       -81 - timeout
// Effect:  Reads bytes at a time and composes 16 bit replies.  Sets Reply Data and sends Response

void MODBUS_Slave::Read_Reg ( unsigned char *buf ) {

  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Cnt_Hi = buf[4];
  unsigned short Cnt_Lo = buf[5];
  
  short Count = Cnt_Lo + ( Cnt_Hi << 8 );  // number of registers ( 2 bytes each )
  buf [ 2 ] = Count * 2;  // number of following bytes
  
  unsigned short Address = (Addr_Lo + ( Addr_Hi << 8 ) );
  if ( ( Address + Count) > _nRegs ) {
		_error = 2;
		return;
	}
  short Item = 3;
  for ( int i = 0; i < Count; i++ ) {
    buf [ Item     ] = ( _regArray [ Address ] & 0xff00 ) >> 8;
    buf [ Item + 1 ] =   _regArray [ Address ] & 0x00ff;
    Address++;
    Item += 2;
  }
  Send_Response ( buf, Item );
}



// ******************************************************************************
// ******************************************************************************
// ******************************************************************************
// ******************************************************************************
// ******************************************************************************
// ******************************************************************************


int MODBUS_Slave::Execute ( ) {

  unsigned long lastMsgReceivedAt_ms;
  unsigned char somethingChanged = 0;
  int status = 0;
  
  #define bufLen 80
  static unsigned char strBuf [ bufLen + 1 ];
  static int bufPtr;
  
  bufPtr = _MODBUS_port.Receive ( strBuf, bufLen, MODBUS_RECEIVE_TIMEOUT_ms );
  
  if ( bufPtr ) {
    lastMsgReceivedAt_ms = millis();
    #if ! defined(ATtiny85)
      if ( _VERBOSITY >= 1 ) {
        
        _diagnostic_port->println ( "Received: " );
        for ( int i = 0; i < bufPtr; i++ ) {
          formatHex ( ( unsigned char * ) &strBuf [ i ], hexBuf, 1);
          _diagnostic_port->print ( hexBuf );
          _diagnostic_port->print ( " " );
        }
        _diagnostic_port->print ( '\n' );
      }
    #endif
  }

  if ( ( ( millis() - lastMsgReceivedAt_ms ) > MESSAGE_TTL_ms ) && bufPtr != 0 ) {
    // kill old messages to keep from blocking up with a bad message piece
    #if ! defined(ATtiny85)
      if ( _VERBOSITY >= 1 ) {
        _diagnostic_port->println ( "timeout" );
      }
    #endif
    status = -81;  // timeout
    bufPtr = 0;
  }
    
  // bufPtr points to (vacant) char *after* the character string in strBuf,
  // and so it is the number of chars currently in the buffer
  switch ( Check_Data_Frame ( strBuf, bufPtr ) ) {
    case -1 : // incomplete message; keep looking
      break;
    case 0 : // not for me; discard it
      bufPtr = 0;
      break;
    case 1 : // process it
      digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
      // Process_Data sends the response
      Process_Data ( strBuf, bufPtr );
      digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
      somethingChanged = 1;
      bufPtr = 0;
      break;
  }
  
  // _error is normally 0, so status normally returns 0 ( nothing changed ) or 1 ( something did change )
  if ( _error ) {
    status = - _error;
    bufPtr = 0;
    _error = 0;
  }
  
  return ( status ? status : somethingChanged );
  
}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

#if ! defined(ATtiny85)

void MODBUS_Slave::Set_Verbose ( Stream * diagnostic_port, int VERBOSITY ) {
  _diagnostic_port = diagnostic_port;
  _VERBOSITY = VERBOSITY;
  return;
}

#endif

