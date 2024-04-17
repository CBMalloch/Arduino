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

void EWMA::load ( unsigned long n, double loadValue ) {
	this->_n             = n;
	this->_value  = loadValue;
}

void EWMA::reset() {
	load(0, 0.0);
}

void EWMA::init ( double alpha ) {
	this->_alpha = alpha;
	reset();
}

double EWMA::setAlpha ( double alpha ) {
  if ( alpha > 0.0 && alpha < 1.0 ) this->_alpha = alpha;
  return ( this->_alpha );
}

double EWMA::record(double x) {
  if (this->_n == 0) {
  	this->_value = x;
  } else {
		this->_value  = this->_value * (1.0 - this->_alpha) 
		    + x * this->_alpha;
	}
  this->_n++;
	return (this->_value);
}

double *EWMA::_internals() {
	static double ret[2];
	ret[0] = this->_n;
	ret[1] = this->_value;
	return ret;
}

unsigned long EWMA::count() {
	return (this->_n);
}

double EWMA::value() {
  return this->_value;
}

void EWMA::results(double *ret) {
	ret[0] = this->_n;
	ret[1] = this->_value;
}

/*
char *EWMA::resultString() {
	static char res[60];
	this->_calculate();
	sprintf(res, "[%2.4f, %2.4f]", this->_m, this->_stDev);
	return res;
}

*/

/*
  LPF - Low pass filter
*/

LPF::LPF () {
	this->reset();
}

LPF::LPF ( double cutoff, double F_sampling ) {
  this->init( cutoff, F_sampling );
}

void LPF::load ( int n, double value ) {
  this->_n = n;
  this->_value = value;
}
  
void LPF::reset () {
  this->_ewma.reset();
  this->load ( 0, 0.0 );
}
    
void LPF::init ( double cutoff, double F_sampling ) {
  // -3dB cutoff frequency
  double omega3db = 2.0 * M_PI * cutoff / F_sampling;
  // double k = 2.0 * ( 1 - cos ( 2.0 * M_PI * cutoff / F_sampling ) )
  // math cos expects radians
  double k = 2.0 * ( 1.0 - cos ( omega3db ) );
  // alpha = cos(omega3db) - 1 + sqrt(cos(omega3db)^2 - 4*cos(omega3db) + 3)
  this->_ewma.setAlpha ( ( -k + sqrt ( k*k + 4.0 * k ) ) / 2.0 );
  this->reset ();
}
  
void LPF::record ( double x ) {
  this->_n += 1;
  this->_ewma.record ( x );
  this->_value = this->_ewma.value();
}

unsigned long LPF::count() {
	return (this->_n);
}

double LPF::value() {
  return this->_value;
}


/*
  HPF - high-pass filter
  
  for each sample, calculate the requisite LPF value; subtract that from the
  full signal to create a high-pass filter
*/
  
HPF::HPF () {
	this->reset();
}

HPF::HPF ( double cutoff, double F_sampling ) {
  this->init( cutoff, F_sampling );
}

void HPF::load ( int n, double value ) {
  this->_n = n;
  this->_value = value;
}
  
void HPF::reset () {
  this->_lpf.reset();
  this->load ( 0, 0.0 );
}
        
void HPF::init ( double cutoff, double F_sampling ) {
  this->_lpf = LPF ( cutoff, F_sampling );
  this->reset ();
}
  
void HPF::record ( double x ) {
  this->_n += 1;
  this->_lpf.record ( x );
  this->_value = x - this->_lpf.value();
}
    
unsigned long HPF::count() {
	return (this->_n);
}

double HPF::value() {
  return this->_value;
}


/*
  BPF - band pass filter
*/
  
BPF::BPF () {
	this->reset();
}

BPF::BPF ( double low_cutoff, double high_cutoff, double F_sampling ) {
  this->init( low_cutoff, high_cutoff, F_sampling );
}

void BPF::load ( int n, double value ) {
  this->_n = n;
  this->_value = value;
}
  
void BPF::reset () {
  this->_hpf.reset();
  this->_lpf.reset();
  this->load ( 0, 0.0 );
}
        
void BPF::init ( double low_cutoff, double high_cutoff, double F_sampling ) {
  this->_hpf = HPF ( low_cutoff, F_sampling );
  this->_lpf = LPF ( high_cutoff, F_sampling );
  this->reset ();
}
  
void BPF::record ( double x ) {
  this->_n += 1;
  this->_lpf.record ( x );
  this->_hpf.record ( this->_lpf.value() );
  this->_value = this->_hpf.value();
}

unsigned long BPF::count() {
	return (this->_n);
}

double BPF::value() {
  return this->_value;
}

