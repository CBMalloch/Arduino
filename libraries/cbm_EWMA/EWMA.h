/*
	EWMA.h - library for doing simple Exponentially Weighted Moving Averages
	Created by Charles B. Malloch, PhD, May 11, 2009
	Released into the public domain
	
EWMA, or Exponentially Weighted Moving Average, is a very good tool to 
calculate and continually update an average. The normal averaging technique
of adding n numbers and dividing by n has several disadvantages when looking 
at a stream of numbers. The EWMA sidesteps most of these disadvantages.

Here are some links to articles on EWMA:
http://en.wikipedia.org/wiki/Moving_average
http://en.wikipedia.org/wiki/EWMA_chart

A simplified explanation of how EWMA works is that each time a new 
number arrives from the data stream, it is combined with the previous 
average value to result in a new average value. The only memory required 
is of the current average value.

The average calculated by EWMA can respond more quickly or more slowly to 
sudden changes in the data stream, depending on a parameter alpha, which 
determines the *weight* of each new value relative to the average. 
EWMA is calculated as 
    E(t) = (1 - alpha) * E(t-1) + alpha * new_reading
The persistence of a reading taken at t0 at a later time t1 is then
    p(t0, t1) = (1 - alpha) ^ (t1 - t0), 
since it will be diminished by the factor (1 - alpha) at each time step.
Thus the half-life (measured in time steps) of a reading is calculated by 
    1/2 = (1 - alpha) ^ half_life. Then
    ln(1/2) / ln(1 - alpha) = half_life
And so if we want a half life of n time steps, we get
    ln(1/2) / half_life = ln(1 - alpha)
    1/2 ^ (1 / half_life) = 1 - alpha
    alpha = 1 - (1/2 ^ (1 / half_life))

Note this is a discrete approximation to the exponential.
  
	For a given alpha, the number of periods to reach x from an old steady state
	to a new steady state is (-ln(x)/alpha). For example, from an initial state of 
	0, it will take (-ln(0.5)/0.2) or 3.46 periods after the input changes to 1 to 
	reach 0.5 if the alpha is (as is usual) 0.2
  WRONG!!!
  The correct formula is ln(1/2) / ln(1 - alpha)
		
	Synopsis
		double results[2], z;
		EWMA myEWMA;
		myEWMA.setAlpha ( myEWMA.alpha ( 200 ) );
		myEWMA = EWMA(0.693147 / 40.0);
		val = myEWMA.record(800.0);
		z = myEWMA.value();
		myEWMA.results(results);

*/

#ifndef EWMA_h
#define EWMA_h

#define EWMA_VERSION "1.002.000"

class EWMA
{
	public:
		EWMA();
		EWMA ( double alpha );
    double setAlpha ( double alpha );
		void reset ();
		void load ( int n, double EWMA );
		// void record( double x );
		double record ( double x );
    double value ();
		//double *results ();
		void results ( double *ret );  // returns n and currentvalue
	//	char *resultString ();
		void init ( double alpha );
		double periods ( int nPeriodsOfHalfLife );
    // periods deprecated; to be replaced by alpha
    double alpha ( int nPeriodsOfHalfLife );
    double alpha ( unsigned long nPeriodsOfHalfLife );
	
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> reportedly not needed
		
	private:
		
		double _alpha;
		int _n;
		double _currentValue;
    
		double *_internals();
		
};

#endif
