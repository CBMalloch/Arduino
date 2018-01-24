#include <Arduino.h>
#include <Print.h>

#include "Utility.h"

#ifndef Pedal_h
#define Pedal_h

#define Pedal_VERSION "0.001.000"


class Pedal {

  public:
    
    void setVerbose ( int i );
    
    #define _Pedal_VERBOSE_DEFAULT 12
    
    Pedal ();  // constructor
    void init ( int id, int pin, int verbose = _Pedal_VERBOSE_DEFAULT );
    
    void update ();
    
    bool changed ();
    float getValue ();
    void clearState ();


  protected:
  
  private:
  
    float read ();
    
    // LSB is 1 / ( 172 - 12 )
    const float _hysteresis = 2.5 / ( 172.0 - 12.0 );
    const float _alpha = 0.6;
    // const unsigned long _settlingTime_ms = 2;
    // const int _nReadRepetitions = 10;
    
    int _id;
    int _pin;
    int _verbose;
    float _value, _oldValue;
    unsigned long _lastValueChangeAt_ms;
    bool _changed;
    bool _changeReported = false;
    
};

#endif
