#include "DelayExt.h"

DelayExt::DelayExt () {
  // constructor
}

void DelayExt::init ( int id, char *name, AudioEffectDelayExternal *delayExt, OpenEffectsBoxHW *oebhw, int verbose ) {
  init ( id, name, delayExt, oebhw, 0.0, 0.0, 0.0, 0.0, verbose );
}

void DelayExt::init ( int id, char *name, AudioEffectDelayExternal *delayExt, OpenEffectsBoxHW *oebhw, 
                   int ch0, int ch1, int ch2, int ch3, int verbose ) {
                   
  DisplayableModule::init ( id, name, oebhw, verbose );
  
  _delayExt = delayExt;
  
  setDelay ( 0, ch0 );
  setDelay ( 1, ch1 );
  setDelay ( 2, ch2 );
  setDelay ( 3, ch3 );
  
  _delayExt->disable ( 4 );
  _delayExt->disable ( 5 );
  _delayExt->disable ( 6 );
  _delayExt->disable ( 7 );
  
  if ( _verbose >= 12 ) {
    Serial.print ( "DelayExt " );
    Serial.print ( _id );
    Serial.print ( " delays_ms: " );
    for ( int i = 0; i < nChannels; i++ ) {
      Serial.print ( _delays_ms [ i ] ); 
      Serial.print ( i < nChannels - 1 ? ", " : "\n" );
    }
  }
  
  if ( _verbose >= 12 ) {
    Serial.print ( "DelayExt " );
    Serial.print ( _id );
    Serial.println ( " initalized" );
  }
}

void DelayExt::setDelay ( int channel, int delay_ms ) {
  if ( channel < 0 || channel >= nChannels ) return;
  _delays_ms [ channel ] = delay_ms;
  if ( _verbose >= 12 ) {
    Serial.print ( "DelayExt " );
    Serial.print ( _id );
    Serial.print ( " delay " );
    Serial.print ( channel );
    Serial.print ( ": " );
    Serial.println ( _delays_ms [ channel ] );
  }
  _delayExt->delay ( channel, _delays_ms [ channel ] );
  _displayIsStale = true;
}

void DelayExt::notify ( int channel, float value ) {
  if ( channel < 0 || channel >= nChannels ) return;
  int delay_ms = Utility::fmapc ( value, 0.0, 1.0, 0.0, 1500.0 );
  setDelay ( channel, delay_ms );
}

void DelayExt::display ( int mode, int subMode, bool force ) {

  if ( ! force && ! _displayIsStale ) return;
  
  _oebhw->oled.displayCommon ( mode, subMode );

  
  if ( _verbose >= 12 ) {
    Serial.print ( "DelayExt " );
    Serial.print ( _id );
    Serial.print ( " (" );
    Serial.print ( _name );
    Serial.println ( ") display" );
  }

  _oebhw->oled.setTextSize ( 2 );  // 12x16
  _oebhw->oled.setCursor ( 0, 0 );
  if ( strlen ( _name ) > 0 ) {
    _oebhw->oled.print ( _name );
    _oebhw->oled.print ( " " );
  }
  _oebhw->oled.print ( "dly" );
  
  for ( int i = 0; i < nChannels; i++ ) {
    int y = i * 12 + 18;
    _oebhw->oled.setTextSize ( 1 );  // 6x8
    _oebhw->oled.setCursor ( 30, y );
    _oebhw->oled.print ( _delays_ms [ i ] );
  }
    
  _oebhw->oled.display();  // note takes about 100ms!!!
  
  _displayIsStale = false;
  
}