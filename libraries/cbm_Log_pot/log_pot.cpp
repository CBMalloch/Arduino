/*
	Log_pot.cpp - library for doing simple log- or exponentially-scaled transformations
	Created by Charles B. Malloch, PhD, September 13, 2014
	Released into the public domain
  
  Scale a curve so that it passes through ( 0, 0 ), ( 1, 1 ), and ( 1/2, p ).
  If p < 1/2, and more particularly if p = 0.2, this would simulate an audio-taper 
  potentiometer's response curve.
  
  Ref 
	
*/

#include <Math.h>
#include "log_pot.h"

//    NOTE: the use of this-> is optional

Log_pot::Log_pot() {
	init ( 0.20 );
}

Log_pot::Log_pot(double fifty_pct_value)
{
	init ( fifty_pct_value );
}

void Log_pot::init ( double fifty_pct_value ) {
	this->p = fifty_pct_value;
  if ( this->p < 0.5 ) {
    this->r = ( 1.0 + sqrt ( 1.0 - 4.0 * this->p * ( 1.0 - this->p ) ) ) / ( 2.0 * this->p );
    this->a = this->p / ( this->r - 1.0 );
    this->b = 2.0 * log ( this->r );
    this->upper = 0;
  } else {
    double pinv = 1.0 - this->p;
    this->r = ( 1.0 + sqrt ( 1.0 - 4.0 * pinv * ( 1.0 - pinv ) ) ) / ( 2.0 * pinv );
    this->a = pinv / ( this->r - 1.0 );
    this->b = 2.0 * log ( this->r );
    this->upper = 1;
  }
}

void Log_pot::_internals ( double * internals ) {
	internals[0] = this->upper;
	internals[1] = this->p;
	internals[2] = this->r;
	internals[3] = this->a;
	internals[4] = this->b;
	return;
}

double Log_pot::value ( double x ) {
  if ( this->upper ) {
    return 1.0 + this->a * ( 1.0 - exp ( this->b * ( 1.0 - x ) ) );
  } else {
    return this->a * ( exp ( this->b * x ) - 1.0 );
  }
}

