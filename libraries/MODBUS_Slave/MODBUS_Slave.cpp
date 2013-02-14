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

	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
*/

#include "MODBUS_Slave.h"
#include <CRC16.h>

#define TESTMODE 1
// #undef TESTMODE

CRC CheckSum;	// From Checksum Library, CRC15.h, CRC16.cpp

//################## MODBUS_Slave ###################
// Takes:   Slave Address, Pointer to Registers and Number of Available Registers
// Returns: Nothing
// Effect:  Initializes Library

MODBUS_Slave::MODBUS_Slave() {
}

MODBUS_Slave::MODBUS_Slave( unsigned char my_address, 
                            unsigned short nCoils,
                            unsigned short * coilArray,
                            unsigned short nRegs,
                            short * regArray,
                            Stream *port
                          ) {
	Init (my_address, nCoils, coilArray, nRegs, regArray, port);
}

void MODBUS_Slave::Init(  unsigned char my_address, 
                          unsigned short nCoils,
                          unsigned short * coilArray,
                          unsigned short nRegs,
                          short * regArray,
                          Stream *port
                        ) {
	_address = my_address;
	_nCoils = nCoils;
	_coilArray = coilArray;
  _nRegs = nRegs;
  _regArray = regArray;
  
  _port = port;
  
	_error = 0;
  
}


// helper function

#ifdef TESTMODE
void formatHex (unsigned short x, char * buf) {
  // expect buf to be of length at least 7
  char h[] = {'0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  buf[0] = '0'; buf[1] = 'x';
  buf[2] = h [ ( x >> 12 ) & 0x000f ];
  buf[3] = h [ ( x >>  8 ) & 0x000f ];
  buf[4] = h [ ( x >>  4 ) & 0x000f ];
  buf[5] = h [ ( x       ) & 0x000f ];
  buf[6] = '\0';
}
#endif

//################## Check Data Frame ###################
// Takes:   Data stream buffer from serial port, number of characters
// Returns: -1 if frame incomplete, 
//           0 if frame complete but invalid, 
//           0 if frame complete but not for us
//           1 if frame complete and valid
// Effect:  Checks completeness, validity, and appropriateness of frame

int MODBUS_Slave::Check_Data_Frame ( unsigned char *msg_buffer, unsigned char msg_len ) {

  if ( msg_len < 4 ) {
    // valid frame has address (1), function code (1), and CRC (2)
    return (-1);
  }
  
  unsigned char msg_address = msg_buffer[0];
	if ( msg_address > 247 ) {					
    // invalid
		_error = 1;
    #ifdef TESTMODE
      _port->write ( "  invalid message\n" );
    #endif
		return (0);
	}
  if ( msg_address != 0 && msg_address != _address) {
    // ignore unless address matches or it's a broadcast (0)
    #ifdef TESTMODE
      _port->write ( "  message not for me\n" );
    #endif
    return (0);
  }
  
  unsigned char function_code = msg_buffer[1];
  unsigned char len;
  // check completeness by function code
  switch (function_code) {
    case 1:  // read MODBUS coils
    case 2:  // read discrete inputs
    case 3:  // read holding reg
    case 4:  // read input reg
    case 5:  // write one coil
    case 6:  // write one register
      len = 6;
      break;
    case 7:   // read exception
      len =  2;
      break;
    case 15: // write multiple coils
    case 16: // write multiple registers
      if ( len < 7 ) {
        // no length character available yet
        return ( -1 );  // frame is incomplete
      }
      len = 7 + msg_buffer[6];  // length in bytes of data segment...
      break;
    default: // unimplemented function code
      return (0);
  }
  len += 2;  // two more bytes needed for CRC
  if ( msg_len < len ) {
    return (-1);
  }
  
  // check CRC
  unsigned short msg_CRC = ( ( msg_buffer [ msg_len - 1 ] << 8 ) | msg_buffer [ msg_len - 2 ] );
  // my calculated CRC
  unsigned short recalculated_CRC = CheckSum.CRC16( msg_buffer, msg_len - 2 );
  if ( recalculated_CRC != msg_CRC ) {
    _error = 2;
    #ifdef TESTMODE
      char buf[7];
      _port->write ( " -- failed CRC -- got " );
      formatHex ( msg_CRC, buf );
      _port->write ( buf );
      _port->write ( "; expected " );
      formatHex ( recalculated_CRC, buf );
      _port->write ( buf );
      _port->write ( "\n" );
      // don't return - simply ignore invalidity of CRC
    #else
      return (0);
    #endif
  }
  
  return (1);
}


//################## Process Data ###################
// Takes:   Data stream buffer from serial port, number of characters to read
// Returns: Nothing
// Effect:  Accepts and parses data

void MODBUS_Slave::Process_Data(unsigned char* msg_buffer, unsigned char msg_len) {

  int frame_valid = Check_Data_Frame ( msg_buffer, msg_len );
  switch ( frame_valid ) {
    case -1:  // short frame
      return;
    case 0:   // invalid frame or not for us
      return;
    case 1:   // frame valid and complete
      break;
  }
  
  unsigned char function_code = msg_buffer[1];
  switch ( function_code ) {
    case 1:  // read MODBUS coils
    case 2:  // read discrete inputs
      Read_Coils ( msg_buffer );
      break;
    case 3:  // read register
    case 4:  // read input reg
      Read_Reg ( msg_buffer );
      break;
    case 5:
      Write_Single_Coil ( msg_buffer );
      break;
    case 6: 
      Write_Single_Reg ( msg_buffer );
      break;
    case 7:
      Read_Exception ( msg_buffer );
      break;
    case 15:
      Write_Coils ( msg_buffer );
      break;
    case 16:
      Write_Reg ( msg_buffer );
      break;
    default:
      #ifdef TESTMODE
        _port->write ("bad cmd code");
      #endif
      break;
  }
  // _error = 0;
  
}

// ******************************************************************************
// ******************************************************************************
// ******************************************************************************

//################## Send Response ###################
// Takes:   In Data Buffer, Length
// Returns: Nothing
// Effect:  Sends Response over serial port

void MODBUS_Slave::Send_Response ( unsigned char *buf, unsigned short nChars ) {
	if ( buf[0] == 0 )
    // don't reply	to broadcast
		return;
  if ( nChars > 0 )	{
    unsigned short CRC = CheckSum.CRC16 ( buf, nChars );
    buf[nChars++] = CRC & 0x00ff;				// lower byte
    buf[nChars++] = CRC >> 8;						// upper byte
    for(int i = 0; i < nChars; i++) {
      _port->write ( buf[i] );
    }
  }
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
  
  unsigned short Item = 3;
  Working_Byte = 0;
  Sub_Bit_Count = 0;
  for ( int Bit = 0; Bit < Bit_Count; Bit++ ) {
    Working_Byte = Working_Byte 
                   | ( ( ( _coilArray [ Address >> 4 ] >> ( Address & 0x000f ) ) & 0x01 )
                       << Sub_Bit_Count );
    Address++;
    Sub_Bit_Count++;
    
    if ( Sub_Bit_Count >= 8 ) {
      #ifdef TESTMODE
        _port->write ("BBBB"); _port->write (Working_Byte);
      #endif
      buf[Item++] = Working_Byte;
      Working_Byte = 0;
      Sub_Bit_Count=0;
      Byte_Count++;
    }
  }
  // if Bit_Count didn't end at a word boundary, we have leftovers
  if ( Sub_Bit_Count != 0 ) {
      #ifdef TESTMODE
        _port->write ("BBBB"); _port->write (Working_Byte);
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
  if ( Value_Hi > 0 | Value_Lo > 0 ) {
    // Real Protocol requires 0xFF00 = On and 0x0000 = Off, Custom, using anything other than 0 => ON
    _coilArray[Write_Address] |= ( 1 << Write_Bit );
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
  unsigned short Write_Bit_Count = Cnt_Lo + ( Cnt_Hi << 8 );
  
  // #ifdef TESTMODE
    // _port->write ("write bit count: "); _port->write (Write_Bit_Count); _port->write ('\n');
  // #endif

  	
  if ( ( Address + Write_Bit_Count ) > _nCoils ) {
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

//################## Read Reg ###################
// Takes:   In Data Buffer: address into regArray, count of registers to read
// Returns: Nothing
// Effect:  Reads bytes at a time and composes 16 bit replies.  Sets Reply Data and sends Response

void MODBUS_Slave::Read_Reg ( unsigned char *buf ) {

  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Cnt_Hi = buf[4];
  unsigned short Cnt_Lo = buf[5];
  
  unsigned short Count = Cnt_Lo + ( Cnt_Hi << 8 );  // number of registers ( 2 bytes each )
  buf[2] = Count * 2;  // turn into bytes
  
  unsigned short Address = (Addr_Lo + ( Addr_Hi << 8 ) );
  if ( ( Address + Count) > _nRegs ) {
		_error = 2;
		return;
	}
  unsigned short Item = 3;
  for ( int i = 0; i < Count; i++ ) {
    buf [ Item     ] = ( _regArray [ Address ] & 0xff00 ) >> 8;
    buf [ Item + 1 ] =   _regArray [ Address ] & 0x00ff;
    Address++;
    Item += 2;
  }
  Send_Response ( buf, Item );
}

//################## Write_Single_Reg ###################
// Takes:   In Data Buffer
// Returns: Nothing
// Effect:  Sets Reply Data and Register values, sends Response

void MODBUS_Slave::Write_Single_Reg ( unsigned char *buf ) {
  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Value_Hi = buf[4];
  unsigned short Value_Lo = buf[5];

  unsigned short Address = ( Addr_Lo + ( Addr_Hi << 8 ) );
  
  if ( Address + 1 > _nRegs )	{
  // Invalid Address;
		_error = 2;
		return;
	}
  _regArray[Address] = ( Value_Hi << 8 ) + Value_Lo;
  Send_Response ( buf, 6 );
}

//################## Write_Reg ###################
// Takes:   In Data Buffer: address (zero-radix), count ( 2-byte registers ), number of following data bytes
// Returns: Nothing
// Effect:  Writes Register in Registers.  Sets Reply Data and sends Response

void MODBUS_Slave::Write_Reg ( unsigned char *buf ) {
  unsigned short Addr_Hi = buf[2];
  unsigned short Addr_Lo = buf[3];
  unsigned short Cnt_Hi = buf[4];
  unsigned short Cnt_Lo = buf[5];
  unsigned char Byte_Count = buf[6];
  
  unsigned short Address = ( Addr_Lo + ( Addr_Hi << 8 ) );
  unsigned short Count = Cnt_Lo + ( Cnt_Hi << 8 );
  unsigned short Read_Byte = 7;  // First entry in input buf

  if ( ( Address + Count ) > _nRegs )	{
  // Invalid Address
		_error = 3;
		return;
	}

  for (int i = 0; i < Byte_Count; i += 2 ) {

    _regArray[ Address++ ] = ( buf [ Read_Byte ] << 8 ) + buf [ Read_Byte + 1 ];
    Read_Byte += 2;
  }
  Send_Response ( buf, 6) ;
}

