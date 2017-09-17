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

class Stats {

public:
	Stats();
	const char *version();
  void reset();
  void load(int n, double xSum, double x2Sum);
  void record(double x);
	double *results();
  int num();
  double mean();
  double stDev();
//	char *resultString();
	double *_internals();

protected:
  // anything that needs to be available only to:
	//    the class itself
	//    friend functions and classes
	//    inheritors
	// this-> reportedly not needed
	
private:
  double xSum, x2Sum;
	int n;
	bool calculated;
	double m, s;
	void _calculate();
};

#endif
