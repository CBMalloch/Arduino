/*
	EWMA.cpp - library for doing exponentially weighted moving averages
	Created by Charles B. Malloch, PhD, May 11, 2009
	Released into the public domain
	
	
*/

#include <Arduino.h>
#include <Math.h>
#include "EWMA.h"

//    NOTE: the use of this-> is optional

double EWMA::periods (int nPeriodsOfHalfLife) {
  // deprecated
  return (alpha (nPeriodsOfHalfLife));
}

double EWMA::alpha (unsigned long nPeriodsOfHalfLife) {
  // in which we specify the number of periods of the half-life of a new reading
  // and the function returns the requisite value for alpha
	// WRONG -> return (-log(0.5)/nPeriodsOfHalfLife);  // note log is ln, log10 is other
  return (1 - exp(log(0.5) / nPeriodsOfHalfLife));
}

double EWMA::alpha (int nPeriodsOfHalfLife) {
  // in which we specify the number of periods of the half-life of a new reading
  // and the function returns the requisite value for alpha
	// WRONG -> return (-log(0.5)/nPeriodsOfHalfLife);  // note log is ln, log10 is other
  return (1 - exp(log(0.5) / nPeriodsOfHalfLife));
}

double EWMA::dynamic_alpha (double desired_response, double desired_time, double loop_time) {
  // if loop_time is specified, its units must match those of desired_time
  // if loop_time is not specified, then desired_time must be in units of seconds
  // The Excel formula is =1-EXP(LN(desired_response) / (desired_time / step_interval))
  
  static unsigned long lastCalledAt_us = 0;
  unsigned long step_interval_us;
  double alpha;

  if (loop_time > 0.0) {
    // called with actual loop time
    alpha = 1 - exp (   log ( desired_response ) 
                      / ( desired_time / loop_time ) );
  } else if ( lastCalledAt_us == 0 ) {
    // doing our own timing, but never called before -- force initialization of y(t+1) to new value
    alpha = 1.0;
  } else {
    step_interval_us = micros() - lastCalledAt_us;
    lastCalledAt_us = micros();
    // note log is natural log
    alpha = 1 - exp (   log ( desired_response ) 
                      / ( ( desired_time * 1e6) / double ( step_interval_us ) ) );
  }
  
  lastCalledAt_us = micros();
  
  return ( alpha );
}

EWMA::EWMA()
{
	reset();
}

EWMA::EWMA(double factor)
{
	init(factor);
}

void EWMA::init(double factor)
{
	this->factor = factor;
	reset();
}

void EWMA::reset()
{
	load(0, 0.0);
}

void EWMA::load(int n, double loadValue)
{
	this->n             = n;
	this->currentValue  = loadValue;
}

double EWMA::record(double x)
{
  if (this->n == 0) {
  	this->n++;
  	this->currentValue = x;
  } else {
		this->currentValue  = this->currentValue * (1.0 - this->factor) + x * this->factor;
	}
	return (this->currentValue);
}

double *EWMA::_internals()
{
	static double ret[2];
	ret[0] = this->n;
	ret[1] = this->currentValue;
	return ret;
}

double EWMA::value()
{
  return this->currentValue;
}

void EWMA::results(double *ret)
{
	ret[0] = this->n;
	ret[1] = this->currentValue;
}

/*
char *EWMA::resultString()
{
	static char res[60];
	this->_calculate();
	sprintf(res, "[%2.4f, %2.4f]", this->m, this->stDev);
	return res;
}

*/

