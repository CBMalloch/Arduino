#include <Arduino.h>
#include <Print.h>

#ifndef Relay_h
#define Relay_h

#define Relay_VERSION "0.001.000"

class Relay {

  public:
    
    Relay ();  // constructor
    void init ( int id, int pin );
    void setValue ( int val );
    int getValue ();


  protected:
  
  private:
  
    int _id;
    int _pin;
    int _value;
    unsigned long _lastValueChangeAt_ms;
    
};

#endif