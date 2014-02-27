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

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

// defined automatically for Arduino
// _BV creates a mask for bit n
// #define _BV(n) (1 << n) 

#include <FormatFloat.h>
#include "MODBUS_var_access.h"

extern const int fbufLen;
extern char strFBuf [];

#define coilArrayElementSize_log2bits 4
#define coilArrayElementSizeMask ( ~ ( 0xff << coilArrayElementSize_log2bits ) )
// coilNo >> 4 is a quicker way to divide coilNo by 16, giving us the coils array element number
#define coilArrayElement(coilNo) ( coilNo >> coilArrayElementSize_log2bits )

int coilValue ( unsigned short * coils, int num_coils, int coilNo ) {
  /*
    Given the coils array, its size, and a coil number, this subroutine returns the value of that coil
    Returns -1 if the specified coil is invalid
    in which coil coilNo is located
    coilNo & 0x000f is a way to identify the bit within the appropriate word for that particular coil
  */
  if (coilNo < num_coils) {
    short coilArrayEltNo = coilArrayElement ( coilNo );
    short coilBitMask = _BV ( coilNo & coilArrayElementSizeMask );
    return ( ( coils [ coilArrayEltNo ] & coilBitMask ) != 0 );
  } else {
    return ( -1 );
  }
}

int setCoilValue ( unsigned short * coils, int num_coils, int coilNo, int newValue ) {
  /*
    Given the coils array, its size, and a coil number, this subroutine sets the value of that coil
    Returns the new value if the specified coil is valid
    Returns -1 if the specified coil is invalid
  */
  if (coilNo < num_coils) {
    short coilArrayEltNo = coilArrayElement ( coilNo );
    short coilBitMask = _BV ( coilNo & coilArrayElementSizeMask );
    if ( newValue ) {
      // set the bit
      coils [ coilArrayEltNo ] |= coilBitMask;
    } else {
      // clear the bit
      coils [ coilArrayEltNo ] &= ~ coilBitMask;
    }
    return ( ( coils [ coilArrayEltNo ] & coilBitMask ) != 0 );
  } else {
    return ( -1 );
  }
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

float floatRegValue ( short * regs, int num_regs, int startingRegNo ) {
  /*
    Given the reg array, its size, and a starting reg number, this subroutine returns the value of the floating point number
      contained in the 2 regs ( 2 x 16 bits = 32 bits )
    Returns -1 if the specified reg span is invalid
  */
  float f;
  if ( ( startingRegNo + 1 ) < num_regs) {
    memcpy ( &f, &regs [ startingRegNo ], 4 );
    // Serial.print ( " float: " ); Serial.println ( strFBuf );
    return ( f );
  } else {
    return ( -1 );
  }
}

int intRegValue ( short * regs, int num_regs, int startingRegNo ) {
  /*
    Given the reg array, its size, and a starting reg number, this subroutine returns the value of the integer
      contained in the 1 regs ( 1 x 16 bits = 16 bits )
    Returns -1 if the specified reg span is invalid
  */
  int i;
  if ( ( startingRegNo + 1 ) < num_regs) {
    memcpy ( &i, &regs [ startingRegNo ], 2 );
    // Serial.print ( " int: " ); Serial.println ( i );
    return ( i );
  } else {
    return ( -1 );
  }
}

unsigned long ULRegValue ( short * regs, int num_regs, int startingRegNo ) {
  /*
    Given the reg array, its size, and a starting reg number, this subroutine returns the value of the integer
      contained in the 2 regs ( 2 x 16 bits = 32 bits )
    Returns -1 if the specified reg span is invalid
  */
  unsigned long i;
  if ( ( startingRegNo + 1 ) < num_regs) {
    memcpy ( &i, &regs [ startingRegNo ], 4 );
    // Serial.print ( " int: " ); Serial.println ( i );
    return ( i );
  } else {
    return ( -1 );
  }
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

int setRegValue ( short * regs, int num_regs, int startingRegNo, int i ) {
  /*
    Given the reg array, its size, and a starting reg number, this subroutine returns the value of the floating point number
      contained in the 1 regs ( 1 x 16 bits = 16 bits )
    Returns -1 if the specified reg span is invalid
  */
  if ( ( startingRegNo + 1 ) < num_regs) {
    memcpy ( &regs [ startingRegNo ], &i, 2 );
    // Serial.print ( " set float: " ); Serial.println ( f );
    return ( 0 );
  } else {
    return ( -1 );
  }
}

int setRegValue ( short * regs, int num_regs, int startingRegNo, float f ) {
  /*
    Given the reg array, its size, and a starting reg number, this subroutine returns the value of the floating point number
      contained in the 2 regs ( 2 x 16 bits = 32 bits )
    Returns -1 if the specified reg span is invalid
  */
  if ( ( startingRegNo + 1 ) < num_regs) {
    memcpy ( &regs [ startingRegNo ], &f, 4 );
    // Serial.print ( " set float: " ); Serial.println ( f );
    return ( 1 );
  } else {
    return ( -1 );
  }
}

int setRegValue ( short * regs, int num_regs, int startingRegNo, unsigned long i ) {
  /*
    Given the reg array, its size, and a starting reg number, this subroutine stores the value of i
      contained in the 2 regs ( 2 x 16 bits = 32 bits )
    Returns -1 if the specified reg span is invalid
  */
  if ( ( startingRegNo + 1 ) < num_regs) {
    memcpy ( &regs [ startingRegNo ], &i, 4 );
    // Serial.print ( " set int: " ); Serial.println ( i );
    return ( 1 );
  } else {
    return ( -1 );
  }
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void reportCoils ( unsigned short * coils, int num_coils ) {
  int i;
  unsigned short bitval;
  
  Serial.println ("Coils:");
  for (i = 0; i < num_coils; i++) {
    bitval = ( coils [ i >> 4 ] >> (  i & 0x000f ) ) & 0x0001;
    Serial.print ("  "); Serial.print (i); Serial.print (": "); Serial.print (bitval); 
    Serial.println ();
  }
}

void reportRegs ( short * regs, int num_regs ) {
  int i;
  Serial.println ("Registers:");
  for (i = 0; i < num_regs; i++) {
    Serial.print ("  "); Serial.print (i); Serial.print (": "); Serial.print (regs[i]); 
    Serial.println ();
  }
}


