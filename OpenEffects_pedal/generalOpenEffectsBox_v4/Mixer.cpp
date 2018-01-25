#include "Mixer.h"

Mixer::Mixer () {
  // constructor
}

void Mixer::init ( int id, char *name, AudioMixer4 *mixer, OpenEffectsBoxHW *oebhw, int verbose ) {
  init ( id, name, mixer, oebhw, 0.0, 0.0, 0.0, 0.0, verbose );
}

void Mixer::init ( int id, char *name, AudioMixer4 *mixer, OpenEffectsBoxHW *oebhw, 
                   int ch0, int ch1, int ch2, int ch3, int verbose ) {
                   
  DisplayableModule::init ( id, name, oebhw, verbose );
  
  _mixer = mixer;
  
  setGain ( 0, ch0 );
  setGain ( 1, ch1 );
  setGain ( 2, ch2 );
  setGain ( 3, ch3 );
  
  if ( _verbose >= 12 ) {
    Serial.print ( "Mixer " );
    Serial.print ( _id );
    Serial.print ( " gains: " );
    for ( int i = 0; i < nChannels; i++ ) {
      Serial.print ( _gains [ i ] ); 
      Serial.print ( i < nChannels - 1 ? ", " : "\n" );
    }
  }
  
  if ( _verbose >= 12 ) {
    Serial.print ( "Mixer " );
    Serial.print ( _id );
    Serial.println ( " initalized" );
  }
}

void Mixer::setGain ( int channel, float gain ) {
  if ( channel < 0 || channel >= nChannels ) return;
  _gains [ channel ] = gain;
  if ( _verbose >= 12 ) {
    Serial.print ( "Mixer " );
    Serial.print ( _id );
    Serial.print ( " gain " );
    Serial.print ( channel );
    Serial.print ( ": " );
    Serial.println ( _gains [ channel ] );
  }
  _mixer->gain ( channel, _gains [ channel ] );
  _displayIsStale = true;
}

void Mixer::notify ( int channel, float value ) {
  if ( channel < 0 || channel >= nChannels ) return;
  float gain = Utility::fmapc ( value, 0.0, 1.0, 0.0, 2.0 );
  setGain ( channel, gain );
}

void Mixer::display ( int mode, int subMode, bool force ) {

  if ( ! force && ! _displayIsStale ) return;
  
  _oebhw->oled.displayCommon ( mode, subMode );

  
  if ( _verbose >= 12 ) {
    Serial.print ( "Mixer " );
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
  _oebhw->oled.print ( "gain" );
  
  for ( int i = 0; i < nChannels; i++ ) {
    int y = i * 12 + 18;
    _oebhw->oled.setTextSize ( 1 );  // 6x8
    _oebhw->oled.setCursor ( 30, y );
    _oebhw->oled.print ( _gains [ i ] );
  }
    
  _oebhw->oled.display();  // note takes about 100ms!!!
  
  _displayIsStale = false;
  
}