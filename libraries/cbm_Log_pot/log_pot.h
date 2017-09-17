/*
	Log_pot.h - library for doing simple log- or exponentially-scaled transformations
	Created by Charles B. Malloch, PhD, September 13, 2014
	Released into the public domain
  
  Scale a curve so that it passes through ( 0, 0 ), ( 1, 1 ), and ( 1/2, p ).
  If p < 1/2, and more particularly if p = 0.2, this would simulate an audio-taper 
  potentiometer's response curve.
  
  Ref 
	
*/

#ifndef LOG_POT_h
#define LOG_POT_h

#define Log_pot_VERSION "0.001.000"

class Log_pot
{
	public:
		Log_pot();
		Log_pot ( double fifty_pct_value );
		void init( double fifty_pct_value );

    double value ( double x );
	
    void _internals( double * internals );
    
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> reportedly not needed
    
		
	private:
		
		double p, r, a, b;
		short upper;
		
};

#endif
