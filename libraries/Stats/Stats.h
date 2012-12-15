/*
	Stats.h - library for doing simple statistics
	Created by Charles B. Malloch, PhD, April 3, 2009
	Released into the public domain
*/

#ifndef Stats_h
#define Stats_h

class Stats
{

public:
	Stats();
	const char *version();
  void reset();
  void load(int n, double xSum, double x2Sum);
  void record(double x);
	double *results();
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
	double m, stDev;
	void _calculate();
};

#endif
