#include "OpenEffectsBoxHW.h"

OpenEffectsBoxHW::OpenEffectsBoxHW() {
}

void OpenEffectsBoxHW::setVerbose ( int verbose ) {
  _verbose = verbose;
  Serial.print ( "OEBHW VERBOSE: " );
  Serial.println ( _verbose );
}

void OpenEffectsBoxHW::init ( int verbose ) {

  _verbose = verbose;
  
  // callbacks
  // register_cbOnBatChanged = NULL;
  
  // pots
  for ( int i = 0; i < nPots; i++ ) {
    pot [ i ].init ( i, pa_pots [ i ] );
  }
  
  // bat switches
  
  for ( int i = 0; i < nBats; i++ ) {
    bat [ i ].init ( i, pa_bats [ i ] );
  }

  // stomp "pb" pushbuttons
  for ( int i = 0; i < nPBs; i++ ) {
    pb [ i ].init ( i, pd_pbs [ i ] );
  }
  
  // relays
  for ( int i = 0; i < nRelays; i++ ) {
    relay [ i ].init ( i, pd_relays [ i ] );
  }
  
  // pedals
  for ( int i = 0; i < nPedals; i++ ) {
    pedal [ i ].init ( i, pa_pedals [ i ] );
  } 
  
  // NeoPixels
  init_NeoPixel_strip ();
  
  // OLED screen
  oled.init ();
  
}

void OpenEffectsBoxHW::init_NeoPixel_strip () {
  _strip.begin();
  _strip.setPixelColor ( ledSingleton, 0x102010 );
  _strip.setPixelColor ( ledOnOff, 0x101020 );
  _strip.setPixelColor ( ledBoost, 0x201010 );
  _strip.show();
}

void OpenEffectsBoxHW::setLED ( int led, unsigned long color ) {
  if ( led < 0 || led >= nNeoPixels ) return;
  _strip.setPixelColor ( led, color );
  _strip.show ();
}

void OpenEffectsBoxHW::setVU ( int n, int mode, int warnAt, 
                               unsigned long onColor, unsigned long offColor,
                               unsigned long warnOnColor, unsigned long warnOffColor ) {
  // if ( n < 0 || n > nVUpixels ) {
  //   Serial.println ( "setVU: invalid bargraph value ( 0 - 7 ): " );
  //   Serial.println ( n );
  // }
  for ( int i = 0; i < nVUpixels; i++ ) {
    bool warn = i >= warnAt;
    unsigned long on = warn ? warnOnColor : onColor;
    unsigned long off = warn ? warnOffColor : offColor;
    unsigned long theColor = off;
    if ( mode == 0 ) {
      // just one led on
      if ( i == n ) theColor = on;
    } else {
      // normal mode: bar
      if ( i < n ) theColor = on;
    }
    int thePixel = ( mode == -1 ) ? nVUpixels - 1 - i : i;
    _strip.setPixelColor ( VUfirstPixel + thePixel, theColor );
  }
  _strip.show ();
}

void OpenEffectsBoxHW::tickle () {
  // read everything, create events for changes
  
  updateInputs ();
  
  
  // for debugging
  static unsigned long lastBlinkAt_ms = 0UL;
  const unsigned long blinkDelay_ms = 500UL;
  static int blinkStatus = 0;
        
  if ( ( millis() - lastBlinkAt_ms ) > blinkDelay_ms ) {
    _strip.setPixelColor ( ledSingleton, blinkStatus ? 0x102010 : 0x100010 );
    _strip.show();
    blinkStatus = 1 - blinkStatus;
    lastBlinkAt_ms = millis();
  }
  
}

void OpenEffectsBoxHW::updateInputs () {
  
  // ----------------------
  // read switches and pots
  // ----------------------
  
  for ( int i = 0; i < nPots; i++ ) {
    pot [ i ].update ();
  }  // cycle through pots
  
  for ( int i = 0; i < nBats; i++ ) {
    bat [ i ].update ();
  }  // cycle through bats
  
  for ( int i = 0; i < nPBs; i++ ) {
    pb [ i ].update();
  }
  
  for ( int i = 0; i < nPedals; i++ ) {
    pedal [ i ].update();
  }
  
}


// void OpenEffectsBoxHW::register_cbOnBatChanged ( int which, void ( OpenEffectsBox::*cb ) ( int * newValue ) ) {
/* 
void OpenEffectsBoxHW:: ( int which, void ( * cb ) ( int * newValue ) ) {
  if ( which < 0 || which >= nBats ) return;
  Serial.print ( "About to register callback " ); Serial.print ( which ); Serial.println ( "!" );
  _cbBats [ which ] = *cb;
}
*/
