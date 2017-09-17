#include <math.h>
#include <LogarithmicScale.h>


LogarithmicScale::LogarithmicScale(void) {
}

void LogarithmicScale::init(float aMin, float aMax, int iMin, int iMax) {
  _iMin = iMin;
  _aMin = aMin;
  // divide the logarithmic distance by the number of intervals
  _factor = log(aMax / aMin) / (iMax - iMin);
}

float LogarithmicScale::value(int iValue) {
  return _aMin * exp((iValue - _iMin) * _factor);
}
