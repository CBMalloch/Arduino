#include "Pedal.h"

Pedal::Pedal () {
}

void Pedal::init ( int pedalID, int pin, int verbose ) {

  _id = pedalID;
  _pin = pin;  // *analog* pin
  _verbose = verbose;
  
  _value = constrain ( map ( analogRead ( _pin ), 5, 192, 0, 1023 ), 0, 1023 );
  _oldValue = _value;
  _lastValueChangeAt_ms = millis();
    
  if ( _verbose >= 10 ) {
    Serial.print ( "Pedal " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.println ( ") inited" );
  }
}

void Pedal::update () {

  int val = constrain ( map ( analogRead ( _pin ), 8, 180, 0, 1023 ), 0, 1023 );
  _value = val * ( 1.0 - _alpha ) + _value * _alpha;
  if ( abs ( _value - _oldValue ) > _hysteresis ) {
    if ( _verbose >= 12 ) {
      Serial.print ( "Pedal " ); Serial.print ( _id );
      Serial.print ( " (pin " ); Serial.print ( _pin );
      Serial.print ( ") new value: " );
      Serial.println ( _value );
    }
    _oldValue = _value;
    _lastValueChangeAt_ms = millis();
    _changeReported = false;
//  } else if ( ( millis() - _lastValueChangeAt_ms ) < _settlingTime_ms ) {
  } else if ( ! _changeReported ) {
    if ( _verbose >= 12 ) {
      Serial.print ( "Pedal " ); Serial.print ( _id );
      Serial.print ( " (pin " ); Serial.print ( _pin );
      Serial.println ( ") reporting change" );
    }
    _changed = true;
    _changeReported = true;
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
