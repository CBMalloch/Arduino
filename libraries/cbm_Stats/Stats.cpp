/*
	Stats.cpp - library for doing simple statistics
	Created by Charles B. Malloch, PhD, April 3, 2009
	Released into the public domain
*/

#include <Math.h>
#include "Stats.h"

/*
#undef int            // fixes stdio.h in v....11
#include <stdio.h>
*/

//    NOTE: the use of this-> is optional

Stats::Stats() {
  // for *sample* stats, use basis of n-1; for *population* stats, use n
  // the bias, which will be added to n, must be thus either 0 or -1
  this->bias = -1;
	reset();
}

void Stats::setBasis ( int bias ) {
  if ( bias == BASIS_SAMPLE ) this->bias = BASIS_SAMPLE;
  if ( bias == BASIS_POPULATION ) this->bias = BASIS_POPULATION;
}

const char *Stats::version() {
	return "1.002.000";
}

void Stats::reset() {
	load(0, 0.0, 0.0);
}

void Stats::load ( unsigned long n, double xSum, double x2Sum ) {
	this->n     = n;
	this->xSum  = xSum;
	this->x2Sum = x2Sum;
	this->calculated = false;    // force calculation of results
	this->m = -655.30;
	this->v = -655.30;
	this->s = -655.30;
}

void Stats::record ( double x ) {
  this->n++;
	this->xSum  += x;
	this->x2Sum += x * x;
	this->calculated = false;    // force calculation of results
}

double *Stats::_internals() {
	static double ret[3];
	ret[0] = this->n;
	ret[1] = this->xSum;
	ret[2] = this->x2Sum;
	return ret;
}

void Stats::_calculate() {
	if ( ! this->calculated ) {
		// recalculate statistics; otherwise they're already done...
		if ( this->n > 0 ) {
			this->m = this->xSum / this->n;
			if ( this->n > 1 ) {
				// from WikiPedia; checks with Excel
        if ( this->x2Sum  > ( this->xSum * this->xSum ) / this->n ) {
          this->v = ( this->x2Sum  - (this->xSum * this->xSum ) / this->n ) / ( this->n + this->bias );
          this->s = sqrt( this->v );
        } else {
          this->s = 0.0;
        }
			}
		}
		this->calculated = true;
	}
}

unsigned long Stats::num () {
	if ( ! this->calculated ) this->_calculate();
  return this->n;
}

double Stats::mean () {
	if ( ! this->calculated ) this->_calculate();
  return this->m;
}

double Stats::variance () {
	if ( ! this->calculated ) this->_calculate();
  return this->v;
}

double Stats::stDev () {
	if ( ! this->calculated ) this->_calculate();
  return this->s;
}

double Stats::rms () {
	if ( ! this->calculated ) this->_calculate();
  return sqrt ( this->x2Sum / this->n );
}

double Stats::zScore ( double v ) {
  if ( this->n < 4 ) return ( -1e16 );
	if ( ! this->calculated ) this->_calculate();
	if ( this->s < 1e-16 ) return ( -2e16 );
  return ( ( v - this->m ) / this->s );
}


double *Stats::results() {
	/*
	  stdev is defined as square root of (E(x2) - (E(x))2)
	*/
	static double ret[2];  // mean, stDev
	if ( ! this->calculated ) this->_calculate();
	ret[0] = this->m;
	ret[1] = this->s;
	return ret;
}

/*
char *Stats::resultString() {
	static char res[60];
	this->_calculate();
	sprintf(res, "[%2.4f, %2.4f]", this->m, this->s);
	return res;
}
*/
