 /*
	LinearAlgebra is a library that I am developing to do linear algebra 
	operations. I have started with standard 2-D arrays and printing and 
	multiplying them.
    
	Copyright (c) 2017 Charles B. Malloch, PhD
	Arduino LinearAlgebra is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by 
	the Free Software Foundation, either version 3 of the License, 
	or (at your option) any later version.

	Arduino LinearAlgebra is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.
  
	To get a copy of the GNU General Public License 
	see <http://www.gnu.org/licenses/>.
  
  Written by Charles B. Malloch, PhD  2017-05-06
 
*/

#include <LinearAlgebra.h>

// #define TESTMODE 1
#undef TESTMODE

void MMult ( 
              double * a, int rows, int middle,
              double * b, int cols,
              double * result ) {
    
	int row, mid, col;

	// Initializing elements of matrix mult to 0.
	for ( row = 0; row < rows; row++ ) {
		for ( col = 0; col < cols; col++ ) {
			* ( result + offset ( row, col, cols ) ) = 0.0;
		}
	}

	for ( row = 0; row < rows; row++ ) {
		for ( col = 0; col < cols; col++ ) {
			for ( mid = 0; mid < middle; mid++ ) {
        * ( result + offset ( row, col, cols ) ) += 
          * ( a + offset ( row, mid, middle ) ) * 
          * ( b + offset ( mid, col, cols ) );
        #if TESTMODE >= 1
          Serial.print ( row ); Serial.print ( ", " );
          Serial.print ( mid ); Serial.print ( ", " );
          Serial.print ( col ); Serial.print ( ": " );
          Serial.print ( * ( a + offset ( row, mid, middle ) ) ); 
          Serial.print ( " | " );
          Serial.print ( * ( b + offset ( mid, col, cols ) ) );
          Serial.print ( " => " );
          Serial.println ( * ( result + offset ( row, col, cols ) ) );
        #endif
			}
		}
	}
  return;
}

void MPrint ( double * theArray, int rows, int cols, char * label ) {
	int row, col;

	Serial.println ( * label );
	for ( row = 0; row < rows; row++ ) {
		for ( col = 0; col < cols; col++ ) {
			Serial.print ( * ( theArray + offset ( row, col, cols ) ) ); 
			Serial.print ( " " );
			if ( col == cols - 1 ) Serial.print ( "\n" );
		}
	}
  Serial.println ();
  return;
}

int offset ( int row, int col, int cols ) {
  const int size = 1;
  #if TESTMODE >= 10
    Serial.print ( "offset: size " );
    Serial.print ( size );
    Serial.print ( " * ( row " );
    Serial.print ( row );
    Serial.print ( " * cols " );
    Serial.print ( cols );
    Serial.print ( " + col " );
    Serial.print ( col );
    Serial.print ( " ) = " );
    Serial.print ( size * ( row * cols + col ) );
    Serial.println ();
  #endif
  return ( size * ( row * cols + col ) );
}
