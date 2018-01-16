#include <Arduino.h>
#include <Print.h>

#ifndef Pedal_h
#define Pedal_h

#define Pedal_VERSION "0.001.000"


class Pedal {

  public:
    
    Pedal ();  // constructor
    void init ( int id, int pin );
    void update ();
    
    bool changed ();
    int getValue ();
    void clearState ();


  protected:
  
  private:
  
    const int _hysteresis = 5;
    const float _alpha = 0.9;
    const unsigned long _settlingTime_ms = 2;
    
    int _id;
    int _pin;
    int _value, _oldValue;
    unsigned long _lastValueChangeAt_ms;
    bool _changed;
    
};

#endif
