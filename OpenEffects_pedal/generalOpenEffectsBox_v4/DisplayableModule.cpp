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
}

bool DisplayableModule::isActive () {
  return ( _isActive );
}

void DisplayableModule::activate ( bool val ) {
  _isActive = val;
}

void DisplayableModule::display () {
}



