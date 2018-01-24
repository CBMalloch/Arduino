#include "Potentiometer.h"

Potentiometer::Potentiometer () {
}

float Potentiometer::read () {
  const int nReadRepetitions = 2;
  float value = 0;
  for ( int i = 0; i < nReadRepetitions; i++ ) {
    value += analogRead ( _pin );
    delay ( 1 );
  }
  value /= float ( nReadRepetitions );
  return ( Utility::fmapc ( value, 1023, 5, 0.0, 1.0 ) );
}

void Potentiometer::init ( int potID, int pin, int verbose ) {

  _id = potID;
  _pin = pin;  // *analog* pin
  _verbose = verbose;
  
  _value = read();
  _oldValue = _value;
  _lastValueChangeAt_ms = millis();
  _changeReported = true;
    
  if ( _verbose >= 12 ) {
    Serial.print ( "Potentiometer " ); Serial.print ( _id );
    Serial.print ( " (pin " ); Serial.print ( _pin );
    Serial.print ( ") initialized: " );
    Serial.println ( _value );
  }

}

void Potentiometer::update () {

  _value = read() * ( 1.0 - _alpha ) + _value * _alpha;
  
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
 //  } else 
  }
  if ( ! _changeReported ) {
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

float Potentiometer::getValue () {
  return ( _value );
}

void Potentiometer::clearState () {
  _changed = false;
}
