#include "OpenEffectsBoxHW.h"

// pots
// bat switches
// stomp "pb" pushbuttons
// relays
// pedals
// NeoPixels
// OLED screen


OpenEffectsBoxHW::OpenEffectsBoxHW() {
}

void OpenEffectsBoxHW::init ( ) {

  _verbose = 2;
  
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
  init_oled_display ();
  
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

void OpenEffectsBoxHW::setVU ( int n, int mode, unsigned long onColor, unsigned long offColor ) {
  // if ( n < 0 || n > nVUpixels ) {
  //   Serial.println ( "setVU: invalid bargraph value ( 0 - 7 ): " );
  //   Serial.println ( n );
  // }
  for ( int i = 0; i < nVUpixels; i++ ) {
    unsigned long theColor = offColor;
    switch ( mode ) {
      case -1:  // from right
        if ( i > n ) theColor = onColor;
        break;
      case 0:   // just one led on
        if ( i == n ) theColor = onColor;
        break;
      case 1:   // normal mode: from left
        if ( i < n ) theColor = onColor;
        break;
      default:
        break;
    }
    _strip.setPixelColor ( VUfirstPixel + i, theColor );
  }
  _strip.show ();
}

void OpenEffectsBoxHW::init_oled_display () {
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  // initialize with the I2C addr 0x3D (for the 128x64)
  // 0x3c on Open Effects box
  
  const byte addI2C = 0x3c;
  _oled.begin ( SSD1306_SWITCHCAPVCC, addI2C );  
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  _oled.display ();
  delay ( 500 );

  // Clear the buffer.
  _oled.clearDisplay ();
  _oled.display ();
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
