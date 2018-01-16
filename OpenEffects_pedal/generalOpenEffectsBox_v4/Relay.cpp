#include "Relay.h"

const int VERBOSE = 2;

Relay::Relay () {
}

void Relay::init ( int id, int pin ) {

  _id = id;
  _pin = pin;
  
  _lastValueChangeAt_ms = millis();
  
  pinMode ( _pin, OUTPUT );
  
  if ( VERBOSE >= 10 ) {
    Serial.print ( "Relay " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.println ( ") inited" );
  }
}

void Relay::setValue ( int val ) {
  _value = constrain ( val, 0, 1 );
  digitalWrite ( _pin, _value );
  _lastValueChangeAt_ms = millis();
  if ( VERBOSE >= 10 ) {
    Serial.print ( "Relay " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.print ( ") set to " ); Serial.println ( _value );
  }
}

int Relay::getValue () {
  return ( _value );
}
