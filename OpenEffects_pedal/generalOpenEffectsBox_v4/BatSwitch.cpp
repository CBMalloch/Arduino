#include "BatSwitch.h"

const int VERBOSE = 2;

BatSwitch::BatSwitch () {
}

void BatSwitch::init ( int id, int pin ) {

  _id = id;
  _pin = pin;  // *analog* pin
  
  _lastRepeatAt_ms = millis();
  _lastValueChangeAt_ms = millis();
  _lastCenteredAt_ms = millis();
    
  if ( VERBOSE >= 10 ) {
    Serial.print ( "BatSwitch " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.println ( ") inited" );
  }
}

void BatSwitch::update () {

  // NASTY read the state of the bat switch and update its status
  
  int val = map ( analogRead ( _pin ), 0, 1023, -1, 1 );
  if ( _id == 1 ) val = - val;  // KLUDGE!    
    
  if ( val == 0 ) {
    // centered; no change
    _lastCenteredAt_ms = millis();
    _lastRepeatAt_ms = 0UL;
  } else {
    // have detected actuation
    // notice only if it's been here a while
    if ( ( millis() - _lastCenteredAt_ms ) > batSwitchNoticePeriod_ms ) {
      // it's firmly actuated; hit it and then ignore until it's time to repeat
      if ( ( millis() - _lastRepeatAt_ms ) > batSwitchRepeatPeriod_ms ) {
        // ready to call actuated or do another repeat
        _value = val;
        _changed = true;
        _lastRepeatAt_ms = millis();
        
        // Serial.println ( "actuation..." );
        
      }  // call actuated or do another repeat
    }  // debounced
  }  // not centered  
}

bool BatSwitch::changed () {
  return ( _changed );
}

int BatSwitch::getValue () {
  return ( _value );
}

void BatSwitch::clearState () {
  _changed = false;
}
