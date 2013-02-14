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
  
  Heavily modified by Charles B. Malloch, PhD  2013-02-12
  Mainly to allow me to use either hardware serial or SoftwareSerial for the RS485 communications
  But then also for my peculiar anally-motivated readability standards
  
  Note that in main program, one should define coilArray thus:
    unsigned short coils[ ( nCoils + 15 ) >> 4 ];

	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
*/

#ifndef MODBUS_Slave_h
#define MODBUS_Slave_h

#include <Stream.h>

class MODBUS_Slave {
	public:
    MODBUS_Slave ();
		MODBUS_Slave (  unsigned char address, 
                    unsigned short nCoils,       // number of individual *bits*
                    unsigned short * coilArray,
                    unsigned short nRegs,
                    short * regArray,
                    Stream *serialPort
                 );
		void Init ( unsigned char address, 
                unsigned short nCoils,       // number of individual *bits*
                unsigned short * coilArray,
                unsigned short nRegs,
                short * regArray,
                Stream *serialPort
             );
		void Process_Data    ( unsigned char* msg_buffer, unsigned char msg_len );
    int Check_Data_Frame ( unsigned char* msg_buffer, unsigned char msg_len );
    
	private:
    void Send_Response( unsigned char *Data_In,unsigned short Length );
    
    void Read_Exception( unsigned char *Data_In );    // Function code 7
    
    void Read_Coils( unsigned char *Data_In );        // Function code 1 (coils), 2 (discrete inputs)
    void Write_Single_Coil( unsigned char *Data_In ); // Function code 5
    void Write_Coils( unsigned char *Data_In );       // Function code 15
    
    void Read_Reg( unsigned char *Data_In );  // Function code 3 (holding reg), 4 (input reg)
    void Write_Single_Reg( unsigned char *Data_In );  // Function code 6
    void Write_Reg( unsigned char *Data_In );  // Function code 16
    
		unsigned char _address;
		unsigned short _nCoils;
		unsigned short * _coilArray;
    unsigned short _nRegs;
    short * _regArray;
    Stream * _port;
		unsigned char _error;
};

#endif
		