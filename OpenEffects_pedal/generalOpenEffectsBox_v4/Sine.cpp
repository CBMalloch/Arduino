#include "Sine.h"

Sine::Sine () {
  // constructor
}

void Sine::init ( int id, char *name, AudioSynthWaveformSine *sine, OpenEffectsBoxHW *oebhw, int verbose ) {
                   
                   
  DisplayableModule::init ( id, name, oebhw, verbose );
  
  _sine = sine;
  _frequency = 440.0;
  
  setFrequency ( _frequency );
  _sine->amplitude ( 0.2 );
  
  if ( _verbose >= 12 ) {
    Serial.print ( "Sine " );
    Serial.print ( _id );
    Serial.println ( " initalized" );
  }
}

void Sine::setFrequency ( float frequency ) {
  _frequency = frequency;
  if ( _verbose >= 12 ) {
    Serial.print ( "Sine " );
    Serial.print ( _id );
    Serial.print ( " frequency: " );
    Serial.print ( _frequency ); 
    Serial.print ( "\n" );\
  }
  _sine->frequency ( _frequency );
  
  /*
    Normally _displayIsStale would be unconditionally set by setFrequency, but 
    I needed to be able to set the gain without incurring the delay of updating
    the display. We'll use the _isActive flag to avoid updating a non-showing
    display!
  */
  if ( _isActive ) {
    _displayIsStale = true;
  } else {
    if ( _verbose >= 12 ) {
      Serial.print ( "Sine " );
      Serial.print ( _id );
      Serial.println ( " skipping display of non-active display" );
    }
  }

}

void Sine::notify ( int channel, float value ) {
  if ( _verbose >= 12 ) {
    Serial.print ( "Sine " );
    Serial.print ( _id );
    Serial.print ( " / " );
    Serial.print ( channel );
    Serial.print ( ": " );
    Serial.println ( value );
  }

  if ( channel != 0 ) return;
  
  float frequency = Utility::fmapc ( Utility::expmap ( value ), 0.0, 1.0, 55.0, 440.0 * 16.0 );
   
  setFrequency ( frequency );
  
}

void Sine::display ( int mode, int subMode, bool force ) {

  if ( ! force && ! _displayIsStale ) return;
  
  _oebhw->oled.displayCommon ( mode, subMode );

  
  if ( _verbose >= 12 ) {
    Serial.print ( "Sine " );
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
  _oebhw->oled.print ( "freq" );
  
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 16 );
  _oebhw->oled.print ( _frequency );
  _oebhw->oled.setTextSize ( 1 );

  _oebhw->oled.display();  // note takes about 100ms!!!
  
  _displayIsStale = false;
}