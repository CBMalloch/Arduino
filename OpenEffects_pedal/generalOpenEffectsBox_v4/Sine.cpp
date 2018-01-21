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
}

void Sine::display () {

  // somebody will have already issued oled.displayCommon ( mode, subMode )
  
  if ( _verbose >= 12 ) {
    Serial.print ( "Sine " );
    Serial.print ( _id );
    Serial.print ( " (" );
    Serial.print ( _name );
    Serial.println ( ") display" );
  }

  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 0, 0 );
  _oebhw->oled.print ( _name );
  _oebhw->oled.print ( " freq" );
  
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 16 );
  _oebhw->oled.print ( _frequency );
  _oebhw->oled.setTextSize ( 1 );

  _oebhw->oled.display();  // note takes about 100ms!!!
  
}