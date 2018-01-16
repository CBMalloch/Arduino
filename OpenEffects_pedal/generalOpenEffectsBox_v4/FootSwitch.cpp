#include "FootSwitch.h"

const int VERBOSE = 2;

FootSwitch::FootSwitch () {
}

void FootSwitch::init ( int id, int pin ) {

  _id = id;
  _pin = pin;
  
  _lastValueChangeAt_ms = millis();
  
  pinMode ( _pin, INPUT );
  _pb.attach ( _pin );
  _pb.interval ( debouncePeriod_ms ); // debounce interval
  
  if ( VERBOSE >= 10 ) {
    Serial.print ( "Foot switch " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.println ( ") inited" );
  }
}

void FootSwitch::update () {

  _pb.update();
  
  if ( _pb.rose() ) {
    _value = 0;  // answers "is button pushed?", opposite of switch state 1=not pushed
    _changed = true;
    _lastValueChangeAt_ms = millis();
    if ( VERBOSE >= 10 ) {
      Serial.print ( "Foot switch " ); Serial.print ( _id );
      Serial.print ( " (pin " ); Serial.print ( _pin );
      Serial.println ( ") rose" );
    }
  } else if ( _pb.fell() ) {
    _value = 1;
    _changed = true;
    _lastValueChangeAt_ms = millis();
    if ( VERBOSE >= 10 ) {
      Serial.print ( "Foot switch " ); Serial.print ( _id );
      Serial.print ( " (pin " ); Serial.print ( _pin );
      Serial.println ( ") fell" );
    }
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
