#include <Arduino.h>
#include <Print.h>
#include <Bounce2.h>

#ifndef FootSwitch_h
#define FootSwitch_h

#define FootSwitch_VERSION "0.001.002"

/*! \brief Wrapper for hardware interface -FootSwitch-.
 
  The foot switches are momentary-contact switches. We use the Arduino library
  Bounce2 to debounce these.
  I wanted to allow autorepeat for these switches, in case I needed to sense
  when they were being held down. So instead of using the methods rose() and
  fell(), which are called exactly once, I used the read() method to poll 
  the status of the switch.

*/

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