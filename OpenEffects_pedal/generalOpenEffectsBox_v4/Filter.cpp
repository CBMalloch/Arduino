#include "Filter.h"

Filter::Filter () {
  // constructor
}

void Filter::init ( int id, char *name, AudioFilterStateVariable *filter, OpenEffectsBoxHW *oebhw, int verbose ) {
                   
                   
  DisplayableModule::init ( id, name, oebhw, verbose );
  
  _filter = filter;
  
  setFrequency ( 440.0 );    
  setResonance ( 1.0 );    
  setOctaveControl ( 2.0 ); 

  if ( _verbose >= 12 ) {
    Serial.print ( "Filter " );
    Serial.print ( _id );
    Serial.println ( " initalized" );
  }
}

void Filter::setFrequency ( float frequency ) {
  _frequency = frequency;
  if ( _verbose >= 12 ) {
    Serial.print ( "Filter " );
    Serial.print ( _id );
    Serial.print ( "; frequency: " );
    Serial.print ( _frequency ); 
    Serial.print ( "\n" );\
  }
  _filter->frequency ( _frequency );
  // guard against slowing the pedal down if the display isn't active
  if ( _isActive ) _displayIsStale = true;
}

void Filter::setResonance ( float resonance ) {
  _resonance = resonance;
  if ( _verbose >= 12 ) {
    Serial.print ( "Filter " );
    Serial.print ( _id );
    Serial.print ( "; resonance: " );
    Serial.print ( _resonance ); 
    Serial.print ( "\n" );\
  }
  _filter->resonance ( _resonance );
  _displayIsStale = true;
}

void Filter::setOctaveControl ( float octaveControl ) {
  _octaveControl = octaveControl;
  if ( _verbose >= 12 ) {
    Serial.print ( "Filter " );
    Serial.print ( _id );
    Serial.print ( "; octaveControl: " );
    Serial.print ( _octaveControl ); 
    Serial.print ( "\n" );\
  }
  _filter->octaveControl ( _octaveControl );
  _displayIsStale = true;
}

void Filter::notify ( int channel, float value ) {
  if ( _verbose >= 12 ) {
    Serial.print ( "Filter " );
    Serial.print ( _id );
    Serial.print ( ": " );
    Serial.println ( value );
  }
  
  if ( channel < 0 || channel >= 2 ) return;
  
  switch ( channel ) {
    case 0:  // center frequency
      setFrequency ( Utility::fmapc ( Utility::expmap ( value ), 0.0, 1.0, 55.0, 7040.0 ) );
      break;
    case 1:  // resonance (Q) 0.7 - 5.0
      setResonance ( Utility::fmapc ( value, 0.0, 1.0, 0.7, 5.0 ) );
      break;
    case 2:  // half-range of resonance frequency, for signal input only
      // setOctaveControl ( Utility::fmapc ( value, 0.0, 1.0, 0.5, 7.0 ) );
      if ( _verbose >= 6 ) {
        Serial.print ( "Filter " );
        Serial.print ( _id );
        Serial.println ( ": Octave Control applies only to signal-input configuration" );
      }
      break;
    default:
      break;
  }

}

void Filter::display ( int mode, int subMode, bool force ) {

  if ( ! force && ! _displayIsStale ) return;
  
  _oebhw->oled.displayCommon ( mode, subMode );

  
  if ( _verbose >= 12 ) {
    Serial.print ( "Filter " );
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
  _oebhw->oled.print ( "filter" );
    
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 16 );
  _oebhw->oled.print ( _frequency );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 20 );
  _oebhw->oled.print ( "ctr" );

  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 32 );
  _oebhw->oled.print ( _resonance );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 36 );
  _oebhw->oled.print ( "Q" );

  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 48 );
  _oebhw->oled.print ( _octaveControl );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 52 );
  _oebhw->oled.print ( "n/a" );

  _oebhw->oled.display();  // note takes about 100ms!!!
  
  _displayIsStale = false;
}