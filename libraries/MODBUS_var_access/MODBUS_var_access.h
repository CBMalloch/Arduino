/*
	MODBUS_var_access is a set of routines to facilitate communications between
      program variables and MODBUS RTU coils and registers.
	
	Copyright (c) 2013 Charles B. Malloch
	Arduino MODBUS_var_access is free software: you can redistribute it and/or modify it under the terms of the 
	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
	or (at your option) any later version.

	Arduino MODBUS_var_access is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.
  
	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
  
  Written by Charles B. Malloch, PhD  2014-02-17
  
  By standard, a MODBUS coil is one bit, and a MODBUS register is 16 bits, big-endian.
  
  By my own standard, the coil array is packed into 8-bit shorts.

*/

#ifndef MODBUS_var_access_h
#define MODBUS_var_access_h

#define MODBUS_var_access_version 0.1.0

int coilValue ( unsigned short * coils, int num_coils, int coilNo );
int setCoilValue ( unsigned short * coils, int num_coils, int coilNo, int newValue );

int intRegValue ( short * regs, int num_regs, int startingRegNo );
unsigned long ULRegValue ( short * regs, int num_regs, int startingRegNo );
float floatRegValue ( short * regs, int num_regs, int startingRegNo );

int setRegValue ( short * regs, int num_regs, int startingRegNo, int i );  // short, or 16 bits
int setRegValue ( short * regs, int num_regs, int startingRegNo, unsigned long i );  // 32 bits
int setRegValue ( short * regs, int num_regs, int startingRegNo, float f );


#ifdef BAUDRATE
void reportCoils ( unsigned short * coils, int num_coils );
void reportRegs ( short * regs, int num_regs );
#endif

#endif