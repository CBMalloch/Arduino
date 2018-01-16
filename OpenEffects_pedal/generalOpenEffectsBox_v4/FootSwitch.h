#include <Arduino.h>
#include <Print.h>
#include <Bounce2.h>

#ifndef FootSwitch_h
#define FootSwitch_h

#define FootSwitch_VERSION "0.001.000"

const unsigned long debouncePeriod_ms =  5UL;

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
  
    Bounce _pb = Bounce();
    int _id;
    int _pin;
    int _value;
    unsigned long _lastValueChangeAt_ms;
    bool _changed;
    
};

#endif