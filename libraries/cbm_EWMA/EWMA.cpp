/*
	EWMA.cpp - library for doing exponentially weighted moving averages
	Created by Charles B. Malloch, PhD, May 11, 2009
	Released into the public domain
	
	
*/

#include <Math.h>
#include "EWMA.h"

//    NOTE: the use of this-> is optional

double EWMA::periods (int nPeriodsOfHalfLife) {
  // deprecated
  return (alpha (nPeriodsOfHalfLife));
}

double EWMA::alpha ( unsigned long nPeriodsOfHalfLife ) {
  /*
    in which we specify the number of periods of the half-life of a new 
    reading and the function returns the requisite value for alpha
	  WRONG -> return ( -log ( 0.5 ) / nPeriodsOfHalfLife );  
	  note log is ln, log10 is other
	*/
  return ( 1 - exp ( log ( 0.5 ) / nPeriodsOfHalfLife ) );
}

double EWMA::alpha ( int nPeriodsOfHalfLife ) {
  // in which we specify the number of periods of the half-life of a new reading
  // and the function returns the requisite value for alpha
	// WRONG -> return (-log(0.5)/nPeriodsOfHalfLife);  // note log is ln, log10 is other
  return (1 - exp(log(0.5) / nPeriodsOfHalfLife));
}

EWMA::EWMA () {
	reset();
}

EWMA::EWMA ( double alpha ) {
	init ( alpha );
}

void EWMA::init ( double alpha ) {
	this->_alpha = alpha;
	reset();
}

double EWMA::setAlpha ( double alpha ) {
  if ( alpha > 0.0 && alpha < 1.0 ) this->_alpha = alpha;
  return ( this->_alpha );
}

void EWMA::reset() {
	load(0, 0.0);
}

void EWMA::load ( unsigned long n, double loadValue ) {
	this->_n             = n;
	this->_currentValue  = loadValue;
}

double EWMA::record(double x) {
  if (this->_n == 0) {
  	this->_currentValue = x;
  } else {
		this->_currentValue  = this->_currentValue * (1.0 - this->_alpha) 
		    + x * this->_alpha;
	}
  this->_n++;
	return (this->_currentValue);
}

double *EWMA::_internals() {
	static double ret[2];
	ret[0] = this->_n;
	ret[1] = this->_currentValue;
	return ret;
}

double EWMA::value() {
  return this->_currentValue;
}

void EWMA::results(double *ret) {
	ret[0] = this->_n;
	ret[1] = this->_currentValue;
}

/*
char *EWMA::resultString() {
	static char res[60];
	this->_calculate();
	sprintf(res, "[%2.4f, %2.4f]", this->_m, this->_stDev);
	return res;
}

*/

