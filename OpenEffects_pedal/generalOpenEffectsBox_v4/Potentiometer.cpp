#include "Potentiometer.h"

Potentiometer::Potentiometer () {
}

void Potentiometer::init ( int potID, int pin, int verbose ) {

  _id = potID;
  _pin = pin;  // *analog* pin
  _verbose = verbose;
  
  _value = constrain ( map ( analogRead ( _pin ), 1023, 5, 0, 1023 ), 0, 1023 );
  _oldValue = _value;
  _lastValueChangeAt_ms = millis();
    
  if ( _verbose >= 12 ) {
    Serial.print ( "Potentiometer " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.print ( ") initialized: " );
    Serial.println ( _value );
  }


}

void Potentiometer::update () {

  int val = constrain ( map ( analogRead ( _pin ), 1023, 5, 0, 1023 ), 0, 1023 );
  _value = val * ( 1.0 - _alpha ) + _value * _alpha;
  
  // if value is changing, we record that.
  // once value stops changing, we report the change
  if ( abs ( _value - _oldValue ) > _hysteresis ) {
    if ( _verbose >= 12 ) {
      Serial.print ( "Potentiometer " ); Serial.print ( _id );
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
      Serial.print ( "Potentiometer " ); Serial.print ( _id );
      Serial.print ( " (pin " ); Serial.print ( _pin );
      Serial.println ( ") reporting change" );
    }
    _changed = true;
    _changeReported = true;
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
