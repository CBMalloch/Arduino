#include "OpenEffectsBox.h"



OpenEffectsBox::OpenEffectsBox() {
}

void OpenEffectsBox::init ( ) {

  // AudioMemory ( 250 );

  _verbose = 2;
  
  hw.init ();
  
  
  for ( int i = 0; i < 5; i++ ) {
    hw.relay [ relayL ].setValue ( 0 );
    hw.setLED ( ledOnOff, 0x102010 );
    delay ( 100 );
    hw.relay [ relayL ].setValue ( 1 );
    hw.setLED ( ledOnOff, 0x201010 );
    delay ( 100 );
  }
  
  for ( int i = 0; i < 8; i++ ) {
    hw.relay [ relayR ].setValue ( 0 );
    hw.setLED ( ledBoost, 0x102010 );
    delay ( 100 );
    hw.relay [ relayR ].setValue ( 1 );
    hw.setLED ( ledBoost, 0x201010 );
    delay ( 100 );
  }
  
  for ( int i = 0; i < 3; i++ ) {
    for ( int j = 0; j < 8; j++ ) {
      hw.setVU ( j );
      delay ( 100 );
    }
    for ( int j = 0; j < 8; j++ ) {
      hw.setVU ( 7 - j );
      delay ( 100 );
    }
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
}

void OpenEffectsBox::tickle () {
  hw.tickle ();
  
  for ( int i = 0; i < nPots; i++ ) {
    if ( hw.pot [ i ].changed () )
      OpenEffectsBox::cbPots ( i, hw.pot [ i ].getValue () );
  }
  
  if ( hw.bat [ 0 ].changed () ) OpenEffectsBox::cbBat0 ( hw.bat [ 0 ].getValue () );
  if ( hw.bat [ 1 ].changed () ) OpenEffectsBox::cbBat1 ( hw.bat [ 1 ].getValue () );
  
  for ( int i = 0; i < nPBs; i++ ) {
    if ( hw.pb [ i ].changed () )
      OpenEffectsBox::cbPBs ( i, hw.pb [ i ].getValue () );
  }

  for ( int i = 0; i < nPedals; i++ ) {
    if ( hw.pedal [ i ].changed () )
      OpenEffectsBox::cbPedals ( i, hw.pedal [ i ].getValue () );
  }
  
}


    /*
  
  
  
    modes and all should not be addressed here; should be reported to Dad
  
    for ( int i = 0; i < 2; i++ ) {
      int val = analogRead ( pa_batSwitches [ i ] );
      if ( i == 1 ) val = 1023 - val;
      if ( val > ( 1024 / 3 ) && val < ( 1024 * 2 / 3 ) ) {
        // centered; no change
        _batSwitchLastCenteredAt_ms [ i ] = millis();
        _batSwitchLastRepeatAt_ms [ i ] = 0UL;
      } else {
        // have detected actuation
        // notice only if it's been here a while
        if ( ( millis() - _batSwitchLastCenteredAt_ms [ i ] ) > _batSwitchNoticePeriod_ms ) {
          // it's firmly actuated; hit it and then ignore until it's time to repeat
          if ( ( millis() - _batSwitchLastRepeatAt_ms [ i ] ) > batSwitchRepeatPeriod_ms ) {
            // do something
            _batSwitchStatus [ i ] += ( val > ( 1024 / 2 ) ) ? 1 : -1;
            _batSwitchStatus [ i ] = constrain ( _batSwitchStatus [ i ], 
                                                0,
                                                i == 0 ? nModes - 1 : nSubModes - 1 );
            _batSwitchLastRepeatAt_ms [ i ] = millis();
          }
        }
      }
    }
  
    if ( mode != _batSwitchStatus [ 0 ] ) {
      _displayIsStale = true;
      modeChanged = true;
      mode = _batSwitchStatus [ 0 ];
      _batSwitchStatus [ 1 ] = 0;   // reset submode when mode changes
    }

    if ( subMode != _batSwitchStatus [ 1 ] ) {
      _displayIsStale = true;
      subModeChanged = true;
      subMode = _batSwitchStatus [ 1 ];
    }
  
    */



void OpenEffectsBox::cbPots ( int pot, int newValue ) {
  Serial.print ( "Callback pot " );
  Serial.print ( pot );
  Serial.print ( ": new value " ); 
  Serial.println ( newValue );
  hw.pot [ pot ].clearState ();
}

void OpenEffectsBox::cbBat0 ( int newValue ) {
  Serial.print ( "Callback bat0: new value " ); Serial.println ( newValue );
  hw.bat [ 0 ].clearState ();
}

void OpenEffectsBox::cbBat1 ( int newValue ) {
  Serial.print ( "Callback bat1: new value " ); Serial.println ( newValue );
  hw.bat [ 1 ].clearState ();
}

void OpenEffectsBox::cbPBs ( int pb, int newValue ) {
  Serial.print ( "Callback pushbutton " );
  Serial.print ( pb );
  Serial.print ( ": new value " ); 
  Serial.println ( newValue );
  hw.pb [ pb ].clearState ();
}

void OpenEffectsBox::cbPedals ( int pedal, int newValue ) {
  Serial.print ( "Callback pedal " );
  Serial.print ( pedal );
  Serial.print ( ": new value " ); 
  Serial.println ( newValue );
  hw.pedal [ pedal ].clearState ();
}


// ========================================================================
