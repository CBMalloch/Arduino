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

#ifndef LinearAlgebra_h
#define LinearAlgebra_h

#define LinearAlgebra_h_version 0.1.0

#include <Arduino.h>

void MMult ( 
        double * a, 
        int rows, int middle,
        double * b, 
        int cols,
        double * result );
        
void MPrint ( double * theArray, int rows, int cols, 
              char * label = ( char * ) "Array" );

int offset ( int row, int col, int cols );

#endif
		