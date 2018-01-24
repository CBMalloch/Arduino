#include "Flange.h"

Flange::Flange () {
  // constructor
}

void Flange::init ( int id, char *name, AudioEffectFlange *flange, OpenEffectsBoxHW *oebhw, int verbose ) {
                   
                   
  DisplayableModule::init ( id, name, oebhw, verbose );
  
  _flange = flange;
  _offset = 0;
  _depth = 0;
  _delayFreq = 0;
  _inited = 0;
  
  setOffset ( _offset );    
  setDepth ( _depth );    
  setDelayFreq ( _delayFreq ); 

  if ( _verbose >= 12 ) {
    Serial.print ( "Flange " );
    Serial.print ( _id );
    Serial.println ( " initalized" );
  }
}

void Flange::setOffset ( int offset ) {
  _offset = offset;
  if ( _verbose >= 12 ) {
    Serial.print ( "Flange " );
    Serial.print ( _id );
    Serial.print ( _inited ? "" : " not" );
    Serial.print ( " inited; offset: " );
    Serial.print ( _offset ); 
    Serial.print ( "\n" );\
  }
  go ();
}

void Flange::setDepth ( int depth ) {
  _depth = depth;
  if ( _verbose >= 12 ) {
    Serial.print ( "Flange " );
    Serial.print ( _id );
    Serial.print ( _inited ? "" : " not" );
    Serial.print ( " inited; depth: " );
    Serial.print ( _depth ); 
    Serial.print ( "\n" );\
  }
  go ();
}

void Flange::setDelayFreq ( float delayFreq ) {
  _delayFreq = delayFreq;
  if ( _verbose >= 12 ) {
    Serial.print ( "Flange " );
    Serial.print ( _id );
    Serial.print ( _inited ? "" : " not" );
    Serial.print ( " inited; delayFreq: " );
    Serial.print ( _delayFreq ); 
    Serial.print ( "\n" );\
  }
  go ();

}

void Flange::go () {
  if ( ! _inited ) {
    _flange->begin ( _flangeDelayLine, FLANGE_DELAY_LENGTH, _offset, _depth, _delayFreq );
    _inited = true;
  } else {
    _flange->voices ( _offset, _depth, _delayFreq );
  }
  _displayIsStale = true;
}

void Flange::notify ( int channel, float value ) {
  if ( _verbose >= 12 ) {
    Serial.print ( "Flange " );
    Serial.print ( _id );
    Serial.print ( ": " );
    Serial.println ( value );
  }
  
  if ( channel < 0 || channel >= 3 ) return;
  
  switch ( channel ) {
    case 0:  // offset, in number of samples
      setOffset ( round ( Utility::fmapc ( value, 0.0, 1.0, 0.0, s_idx * 2.0 ) ) );
      break;
    case 1:  // depth, in number of samples ( amplitude of offset sine )
      setDepth ( round ( Utility::fmapc ( value, 0.0, 1.0, 0.0, s_depth * 2.0 ) ) );
      break;
    case 2:  // delay frequency
      setDelayFreq ( Utility::fmapc ( Utility::expmap ( value ), 0.0, 1.0, 0.01, s_freq * 2.0 ) );
      break;
    default:
      break;
  }

}

void Flange::display ( int mode, int subMode, bool force ) {

  if ( ! force && ! _displayIsStale ) return;
  
  _oebhw->oled.displayCommon ( mode, subMode );

  
  if ( _verbose >= 12 ) {
    Serial.print ( "Flange " );
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
  _oebhw->oled.print ( "flange" );
    
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 16 );
  _oebhw->oled.print ( _offset );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 20 );
  _oebhw->oled.print ( "ofst" );

  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 32 );
  _oebhw->oled.print ( _depth );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 36 );
  _oebhw->oled.print ( "dpth" );

  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 48 );
  _oebhw->oled.print ( _delayFreq );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 52 );
  _oebhw->oled.print ( "rate" );

  _oebhw->oled.display();  // note takes about 100ms!!!
  
  _displayIsStale = false;
}