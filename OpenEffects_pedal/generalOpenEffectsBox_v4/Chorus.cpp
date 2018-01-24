#include "Chorus.h"

Chorus::Chorus () {
  // constructor
}

void Chorus::init ( int id, char *name, AudioEffectChorus *chorus, OpenEffectsBoxHW *oebhw, int verbose ) {
                   
                   
  DisplayableModule::init ( id, name, oebhw, verbose );
  
  _chorus = chorus;
  _nVoices = 1;
  _inited = 0;
  
  setNVoices ( _nVoices );
  
  if ( _verbose >= 12 ) {
    Serial.print ( "Chorus " );
    Serial.print ( _id );
    Serial.println ( " initalized" );
  }
}

void Chorus::setNVoices ( int nVoices ) {
  _nVoices = nVoices;
  if ( _verbose >= 12 ) {
    Serial.print ( "Chorus " );
    Serial.print ( _id );
    Serial.print ( _inited ? "" : " not" );
    Serial.print ( " inited; nVoices: " );
    Serial.print ( _nVoices ); 
    Serial.print ( "\n" );\
  }
  if ( ! _inited ) {
    _chorus->begin ( _chorusDelayLine, CHORUS_DELAY_LENGTH, _nVoices );  // 1 is passthru
    _inited = true;
  } else {
    _chorus->voices ( _nVoices );
  }
  _displayIsStale = true;

}

void Chorus::notify ( int channel, float value ) {
  if ( _verbose >= 12 ) {
    Serial.print ( "Chorus " );
    Serial.print ( _id );
    Serial.print ( ": " );
    Serial.println ( value );
  }

  if ( channel != 0 ) return;
  
  int nVoices = Utility::fmapc ( value, 0.0, 1.0, 1.0, 6.5 );
  setNVoices ( nVoices );
}

void Chorus::display ( int mode, int subMode, bool force ) {

  if ( ! force && ! _displayIsStale ) return;
  
  _oebhw->oled.displayCommon ( mode, subMode );

  
  if ( _verbose >= 12 ) {
    Serial.print ( "Chorus " );
    Serial.print ( _id );
    Serial.print ( " (" );
    Serial.print ( _name );
    Serial.println ( ") display" );
  }

  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 0, 0 );
  if ( strlen ( _name ) > 0 ) {
    _oebhw->oled.print ( _name );
    _oebhw->oled.print ( " " );
  }
  _oebhw->oled.print ( "chorus" );
    
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 16 );
  _oebhw->oled.print ( _nVoices );
  _oebhw->oled.setTextSize ( 1 );

  _oebhw->oled.display();  // note takes about 100ms!!!
  
  _displayIsStale = false;
}