#include <Arduino.h>
#include <Print.h>

#ifndef Potentiometer_h
#define Potentiometer_h

#define Potentiometer_VERSION "0.001.000"

const int potHysteresis = 10;

class Potentiometer {

  public:
    
    Potentiometer ();  // constructor
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
    unsigned long _lastValueChangeAt_ms;
    bool _changed;
    
};

#endif