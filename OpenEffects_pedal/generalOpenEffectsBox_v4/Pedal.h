#include <Arduino.h>
#include <Print.h>

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
    int getValue ();
    void clearState ();


  protected:
  
  private:
  
    const int _hysteresis = 8;
    const float _alpha = 0.9;
    const unsigned long _settlingTime_ms = 2;
    
    int _id;
    int _pin;
    int _verbose;
    int _value, _oldValue;
    unsigned long _lastValueChangeAt_ms;
    bool _changed;
    bool _changeReported = false;
    
};

#endif
