#include <Arduino.h>
#include <Print.h>

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
    int getValue ();
    void clearState ();


  protected:
  
  private:
  
    const int _hysteresis = 2;
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