/*

  Author: Charles B. Malloch, PhD
  Date: 2012-12-08

  LogarithmicScale converts a linear scale into a logarthimic one,
  primarily for the purpose of simulating an audio-taper pot
  while physically using a linear one.
  
  The result range cannot include zero!!!!
  
  The result range is given in init by aMin and aMax;
  The optional input range is given by iMin and iMax;
    its default is (0, 1023).

*/

#ifndef LogarithmicScale_h
#define LogarithmicScale_h

// can define this after the include statement in the main program for debugging
#undef LSDEBUG

#define LOGARITHMIC_SCALE_VERSION 1.000000
// v1.000000 2012-12-08 cbm  created

class LogarithmicScale {
  public:
    LogarithmicScale(void);
    void init(float aMin, float aMax, int iMin = 0, int iMax = 1023);
    float value(int iValue);
  private:
    int _iMin;
    float _aMin;
    double _factor;
};

#endif
