#include "DisplayableModule.h"

DisplayableModule::DisplayableModule () {
  // constructor
}

void DisplayableModule::setVerbose ( int verbose ) {
  _verbose = verbose;
  Serial.print ( "DisplayableModule VERBOSE: " );
  Serial.println ( _verbose );
}

void DisplayableModule::init ( int id, char *name, OpenEffectsBoxHW *oebhw, int verbose ) {

  _id = id;
  strncpy ( _name, name, nameStrLen );
  setVerbose ( verbose );
  _oebhw = oebhw;
  _isActive = false;
  _displayIsStale = true;
}

bool DisplayableModule::isActive () {
  return ( _isActive );
}

void DisplayableModule::activate ( bool val ) {
  _isActive = val;
}

// void DisplayableModule::notify ( int channel, int value ) {
// }
    
void DisplayableModule::notify ( int channel, float value ) {
}
    
void DisplayableModule::display ( int mode, int subMode, bool force ) {
}
