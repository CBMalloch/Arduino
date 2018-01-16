#include <Arduino.h>
#include <Print.h>

#ifndef BatSwitch_h
#define BatSwitch_h

#define BatSwitch_VERSION "0.001.000"

const unsigned long batSwitchNoticePeriod_ms =  50UL;
const unsigned long batSwitchRepeatPeriod_ms = 500UL;

class BatSwitch {

  public:
    
    BatSwitch ();  // constructor
    void init ( int id, int pin );
    void update ();
    
    bool changed ();
    int getValue ();
    void clearState ();


  protected:
  
  private:
  
    int _id;
    int _pin;
    int _value, _oldValue;
    unsigned long _lastRepeatAt_ms, _lastValueChangeAt_ms, _lastCenteredAt_ms;
    bool _changed;
    
};

#endif

