#include "Pedal.h"

const int VERBOSE = 2;

Pedal::Pedal () {
}

void Pedal::init ( int pedalID, int pin ) {

  _id = pedalID;
  _pin = pin;  // *analog* pin
  
  _lastValueChangeAt_ms = millis();
    
  if ( VERBOSE >= 10 ) {
    Serial.print ( "Pedal " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.println ( ") inited" );
  }
}

void Pedal::update () {

  int val = constrain ( map ( analogRead ( _pin ), 5, 192, 0, 1023 ), 0, 1023 );
  _value = val * ( 1.0 - _alpha ) + _value * _alpha;
  if ( abs ( _value - _oldValue ) > _hysteresis ) {
    _oldValue = _value;
    _lastValueChangeAt_ms = millis();
  }
  if ( ( millis() - _lastValueChangeAt_ms ) < _settlingTime_ms ) {
    _changed = true;
  }

}

bool Pedal::changed () {
  return ( _changed );
}

int Pedal::getValue () {
  return ( _value );
}

void Pedal::clearState () {
  _changed = false;
}
