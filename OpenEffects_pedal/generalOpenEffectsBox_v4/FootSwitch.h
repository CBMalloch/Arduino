#include <Arduino.h>
#include <Print.h>
#include <Bounce2.h>

#ifndef FootSwitch_h
#define FootSwitch_h

#define FootSwitch_VERSION "0.001.002"

class FootSwitch {

  public:
    
    FootSwitch ();  // constructor
    void init ( int id, int pin );
    void update ();
    
    bool changed ();
    int getValue ();
    void clearState ();


  protected:
  
  private:
  
    const unsigned long _debouncePeriod_ms =  5UL;
    const unsigned long _repeatPeriod_ms = 500UL;

    Bounce _pb = Bounce();
    int _id;
    int _pin;
    int _value;
    unsigned long _lastRepeatAt_ms, _lastValueChangeAt_ms, _lastCenteredAt_ms;
    bool _changed;
    
};

#endif