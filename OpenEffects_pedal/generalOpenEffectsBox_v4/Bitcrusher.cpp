#include "Bitcrusher.h"

Bitcrusher::Bitcrusher () {
  // constructor
}

void Bitcrusher::init ( int id, char *name, AudioEffectBitcrusher *bitcrusher, OpenEffectsBoxHW *oebhw, int verbose ) {
                   
  DisplayableModule::init ( id, name, oebhw, verbose );
  
  _bitcrusher = bitcrusher;
  
  _nBits = 16;
  _sampleRate = 44100;
  
  if ( _verbose >= 12 ) {
    Serial.print ( "Bitcrusher " );
    Serial.print ( _id );
    Serial.print ( " nBits: " );
    Serial.print ( _nBits ); 
    Serial.print ( "; sample rate: " );
    Serial.print ( _sampleRate ); 
  }
  
  if ( _verbose >= 12 ) {
    Serial.print ( "Bitcrusher " );
    Serial.print ( _id );
    Serial.println ( " initalized" );
  }
}

void Bitcrusher::setNBits ( int nBits ) {
  _nBits = nBits;
  if ( _verbose >= 12 ) {
    Serial.print ( "Bitcrusher " );
    Serial.print ( _id );
    Serial.print ( " nBits: " );
    Serial.println ( _nBits );
  }
  _bitcrusher->bits ( _nBits );
  _displayIsStale = true;
}

void Bitcrusher::setSampleRate ( unsigned int sampleRate ) {
  _sampleRate = sampleRate;
  if ( _verbose >= 12 ) {
    Serial.print ( "Bitcrusher " );
    Serial.print ( _id );
    Serial.print ( " sampleRate: " );
    Serial.println ( _sampleRate );
  }
  _bitcrusher->sampleRate ( _sampleRate );
  _displayIsStale = true;
}

void Bitcrusher::notify ( int channel, float value ) {
  if ( channel < 0 || channel >= nChannels ) return;
  switch ( channel ) {
    case 0:  // bits
      {
        float nBits = round ( Utility::fmapc ( value, 0.0, 1.0, 16.2, 6.0 ) );
        setNBits ( round ( nBits ) );
      };
      break;
    case 1:  // sample rate
      {
        int index = round ( Utility::fmapc ( value, 0.0, 1.0, 0.0, 6.2 ) );
        unsigned int sampleRates [] = { 44100, 22050, 11025, 5512, 2756, 1378, 689 };
        unsigned int sampleRate = sampleRates [ index ];
        setSampleRate ( sampleRate );
      }
      break;
    default:
      break;
  }
  
}

void Bitcrusher::display ( int mode, int subMode, bool force ) {

  if ( ! force && ! _displayIsStale ) return;
  
  _oebhw->oled.displayCommon ( mode, subMode );

  
  if ( _verbose >= 12 ) {
    Serial.print ( "Bitcrusher " );
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
  _oebhw->oled.print ( "bitcrush" );
  
  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 16 );
  _oebhw->oled.print ( _nBits );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 20 );
  _oebhw->oled.print ( "bits" );

  _oebhw->oled.setTextSize ( 2 );
  _oebhw->oled.setCursor ( 30, 32 );
  _oebhw->oled.print ( _sampleRate );
  _oebhw->oled.setTextSize ( 1 );
  _oebhw->oled.setCursor ( 0, 36 );
  _oebhw->oled.print ( "samp" );
    
  _oebhw->oled.display();  // note takes about 100ms!!!
  
  _displayIsStale = false;
  
}