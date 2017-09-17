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

Stats::Stats()
{
	reset();
}

const char *Stats::version()
{
	return "1.001.000";
}

void Stats::reset()
{
	load(0, 0.0, 0.0);
}

void Stats::load(int n, double xSum, double x2Sum)
{
	this->n     = n;
	this->xSum  = xSum;
	this->x2Sum = x2Sum;
	this->calculated = false;    // force calculation of results
	this->m = -655.30;
	this->s = -655.30;
}

void Stats::record(double x)
{
  this->n++;
	this->xSum  += x;
	this->x2Sum += x * x;
	this->calculated = false;    // force calculation of results
}

double *Stats::_internals()
{
	static double ret[3];
	ret[0] = this->n;
	ret[1] = this->xSum;
	ret[2] = this->x2Sum;
	return ret;
}

void Stats::_calculate()
{
	if ( ! this->calculated ) {
		// recalculate statistics; otherwise they're already done...
		if ( this->n > 0 ) {
			this->m = this->xSum / this->n;
			if ( this->n > 1 ) {
				// from WikiPedia; checks with Excel
        if ( this->x2Sum  > ( this->xSum * this->xSum ) / this->n ) {
          this->s = sqrt( ( this->x2Sum  - (this->xSum * this->xSum ) / this->n ) / ( this->n - 1 ) );
        } else {
          this->s = 0.0;
        }
			}
		}
		this->calculated = true;
	}
}

int Stats::num ()
{
	if ( ! this->calculated ) this->_calculate();
  return this->n;
}

double Stats::mean ()
{
	if ( ! this->calculated ) this->_calculate();
  return this->m;
}

double Stats::stDev ()
{
	if ( ! this->calculated ) this->_calculate();
  return this->s;
}

double *Stats::results()
{
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
char *Stats::resultString()
{
	static char res[60];
	this->_calculate();
	sprintf(res, "[%2.4f, %2.4f]", this->m, this->s);
	return res;
}
*/
