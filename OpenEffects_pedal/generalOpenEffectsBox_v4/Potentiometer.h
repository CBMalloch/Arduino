#include <Arduino.h>
#include <Print.h>

#include "Utility.h"

#ifndef Potentiometer_h
#define Potentiometer_h

#define Potentiometer_VERSION "0.001.001"

class Potentiometer {

  public:
    
    #define _Potentiometer_VERBOSE_DEFAULT 12
    
    Potentiometer ();  // constructor
    void init ( int id, int pin, int verbose = _Potentiometer_VERBOSE_DEFAULT );
    void update ();
    
    bool changed ();
    float getValue ();
    void clearState ();


  protected:
  
  private:
  
    float read ();
    
    // LSB is about 1023 - 5
    const float _hysteresis = 2.5 / ( 1023.0 - 5.0 );
    const float _alpha = 0.6;
    // const unsigned long _settlingTime_ms = 2;
    
    int _id;
    int _pin;
    int _verbose;
    float _value, _oldValue;
    unsigned long _lastValueChangeAt_ms;
    bool _changed;
    bool _changeReported = false;

};

#endif