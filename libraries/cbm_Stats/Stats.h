/*
	Stats.h - library for doing simple statistics
	Created by Charles B. Malloch, PhD, April 3, 2009
	Released into the public domain
  
  #include <Stats.h>
  Stats theStats;
  
  ...
  theStats.record ( (double) value );
  ...
  Serial.print ( theStats.mean () );
*/

#ifndef Stats_h
#define Stats_h

#define BASIS_SAMPLE     -1
#define BASIS_POPULATION  0

class Stats {

public:
	Stats();
	void setBasis ( int bias );  // use BASIS_SAMPLE or BASIS_POPULATION
	const char *version();
  void reset();
  void load ( unsigned long n, double xSum, double x2Sum );
  void record ( double x );
	double *results();
  unsigned long num();
  double mean();
  double variance();
  double stDev();
  double rms ();
  double zScore ( double v );
//	char *resultString();
	double *_internals();

protected:
  // anything that needs to be available only to:
	//    the class itself
	//    friend functions and classes
	//    inheritors
	// this-> reportedly not needed
	
private:
  // for *sample* stats, use basis of n-1; for *population* stats, use n
  // the bias, which will be added to n, must be thus either 0 or -1
  // set using setBasis with BASIS_SAMPLE or BASIS_POPULATION
  int bias;
  double xSum, x2Sum;
	unsigned long n;
	bool calculated;
	double m, v, s;
	void _calculate();
};

#endif
