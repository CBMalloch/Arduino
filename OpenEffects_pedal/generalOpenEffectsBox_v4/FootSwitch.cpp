#include "FootSwitch.h"

const int VERBOSE = 2;

FootSwitch::FootSwitch () {
}

void FootSwitch::init ( int id, int pin ) {

  _id = id;
  _pin = pin;
  
  _lastRepeatAt_ms = millis();
  _lastValueChangeAt_ms = millis();
  _lastCenteredAt_ms = millis();
  
  pinMode ( _pin, INPUT );
  _pb.attach ( _pin );
  _pb.interval ( _debouncePeriod_ms ); // debounce interval
  
  if ( VERBOSE >= 10 ) {
    Serial.print ( "Foot switch " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.println ( ") inited" );
  }
}

void FootSwitch::update () {

  _pb.update();
  
  if ( _pb.rose() ) {
    // foot switch is released
    _value = 0;  // answers "is button pushed?", opposite of switch state 1=not pushed
    _changed = true;
    _lastValueChangeAt_ms = millis();
    if ( VERBOSE >= 10 ) {
      Serial.print ( "Foot switch " ); Serial.print ( _id );
      Serial.print ( " (pin " ); Serial.print ( _pin );
      Serial.println ( ") rose" );
    }
  } else if ( _pb.read() == 0 ) {
    // Note: if we test _pb.fell(), we enter here once only. We need to test
    // something else  
    // foot switch is depressed
    // hit it at once and then ignore until it's time to repeat
    if ( ( millis() - _lastRepeatAt_ms ) > _repeatPeriod_ms ) {
      // ready to call actuated or do another repeat
      _value = 1;
      _changed = true;
      _lastValueChangeAt_ms = millis();
      _lastRepeatAt_ms = millis();
      
      if ( VERBOSE >= 10 ) {
        Serial.print ( "Foot switch " ); Serial.print ( _id );
        Serial.print ( " (pin " ); Serial.print ( _pin );
        Serial.println ( ") is depressed :(" );
      }
    }  // call actuated or do another repeat
  }
}

bool FootSwitch::changed () {
  return ( _changed );
}

int FootSwitch::getValue () {
  return ( _value );
}

void FootSwitch::clearState () {
  _changed = false;
}
