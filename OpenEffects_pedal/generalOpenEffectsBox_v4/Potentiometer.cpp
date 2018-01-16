#include "Potentiometer.h"

const int VERBOSE = 2;

Potentiometer::Potentiometer () {
}

void Potentiometer::init ( int potID, int pin ) {

  _id = potID;
  _pin = pin;  // *analog* pin
  
  _lastValueChangeAt_ms = millis();
    
  if ( VERBOSE >= 10 ) {
    Serial.print ( "Potentiometer " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.println ( ") inited" );
  }
}

void Potentiometer::update () {

  int val = constrain ( map ( analogRead ( _pin ), 1023, 5, 0, 1023 ), 0, 1023 );
  _value = val * ( 1.0 - _alpha ) + _value * _alpha;
  if ( abs ( _value - _oldValue ) > _hysteresis ) {
    _oldValue = _value;
    _lastValueChangeAt_ms = millis();
  }
  if ( ( millis() - _lastValueChangeAt_ms ) < _settlingTime_ms ) {
    _changed = true;
  }

}

bool Potentiometer::changed () {
  return ( _changed );
}

int Potentiometer::getValue () {
  return ( _value );
}

void Potentiometer::clearState () {
  _changed = false;
}
