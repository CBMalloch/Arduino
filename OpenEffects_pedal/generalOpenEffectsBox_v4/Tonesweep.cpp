#include "Tonesweep.h"

Tonesweep::Tonesweep () {
  // constructor
}

void Tonesweep::init ( int id, 
                       char *name, 
                       AudioSynthToneSweep *tonesweep, 
                       OpenEffectsBoxHW *oebhw, 
                       int verbose ) {
                   
                   
  DisplayableModule::init ( id, name, oebhw, verbose );
  
  _tonesweep = tonesweep;
  _lFrequency = 440.0;
  _hFrequency = 1760.0;
  _sweepTime = 1.0;
  
  if ( _verbose >= 12 ) {
    Serial.print ( "Tonesweep " );
    Serial.print ( _id );
    Serial.println ( " initalized" );
  }
}

void Tonesweep::setLFrequency ( float frequency ) {
  _lFrequency = frequency;
  if ( _verbose >= 12 ) {
    Serial.print ( "Tonesweep " );
    Serial.print ( _id );
    Serial.print ( " lower frequency: " );
    Serial.print ( _lFrequency ); 
    Serial.print ( "\n" );\
  }
}
  
void Tonesweep::setHFrequency ( float frequency ) {
  _hFrequency = frequency;
  if ( _verbose >= 12 ) {
    Serial.print ( "Tonesweep " );
    Serial.print ( _id );
    Serial.print ( " upper frequency: " );
    Serial.print ( _hFrequency ); 
    Serial.print ( "\n" );\
  }
}
  
void Tonesweep::setSweepTime ( float sweep_time ) {
  _sweepTime = sweep_time;
  if ( _verbose >= 12 ) {
    Serial.print ( "Tonesweep " );
    Serial.print ( _id );
    Serial.print ( " sweep time: " );
    Serial.print ( _sweepTime ); 
    Serial.print ( "\n" );\
  }
}

void Tonesweep::notify ( int channel, float value ) {
  if ( _verbose >= 12 ) {
    Serial.print ( "Tonesweep " );
    Serial.print ( _id );
    Serial.print ( " / " );
    Serial.print ( channel );
    Serial.print ( ": " );
    Serial.println ( value );
  }

  if ( channel < 0 || channel >= 3 ) return;
  
  switch ( channel ) {
    case 0:  // tone sweep lower frequency
      setLFrequency ( Utility::fmapc ( Utility::expmap ( value ), 0.0, 1.0, 1.0, 440.0 ) );
      break;
    case 1:  // tone sweep upper frequency
      setHFrequency ( Utility::fmapc ( Utility::expmap ( value ), 0.0, 1.0, 10.0, 10000.0 ) );
      break;
    case 2:  // tone sweep time
      setSweepTime ( Utility::fmapc ( Utility::expmap ( value ), 0.0, 1.0, 0.01, 2.0 ) );
      break;
    default:
      break;
  }
  
  tickle ();
  if ( _verbose >= 10 ) _oebhw->setVU ( map ( value, 0, 1023, 0, 7 ) );   // debug

}

void Tonesweep::tickle () {
  if ( ! _tonesweep->isPlaying() ) {
    _tonesweep->play ( 1.0, _lFrequency, _hFrequency, _sweepTime );
  }
}

void Tonesweep::display ( int mode, int subMode, bool force ) {

  if ( ! force && ! _displayIsStale ) return;
  
  _oebhw->oled.displayCommon ( mode, subMode );

  
  if ( _verbose >= 12 ) {
    Serial.print ( "Tonesweep " );
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
  _oebhw->oled.print ( "toneswp" );
  
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 16 );
  _oebhw->oled.print ( _lFrequency );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 20 );
  _oebhw->oled.print ( "lo f" );
  
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 32 );
  _oebhw->oled.print ( _hFrequency );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 36 );
  _oebhw->oled.print ( "hi f" );
  
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 48 );
  _oebhw->oled.print ( _sweepTime );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 52 );
  _oebhw->oled.print ( "s" );

  _oebhw->oled.display();  // note takes about 100ms!!!
  
}