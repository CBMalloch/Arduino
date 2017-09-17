#include <Arduino.h>
#include <string.h>
#include <Profiler.h>

Profile::Profile()
{
	reset();
}

const char *Profile::version()
{
	return "0.001.000";
}

void Profile::reset()
{
	for ( int i = 0; i < maxBins; i++ ) {
    bins [ i ] = 0UL;
  }
  nHits = 0UL;
  startedAt_ms = 0UL;
  totalTime = 0UL;
}

void Profile::setup ( const char * name, int nBins, int binSize, unsigned long lowBinStart ) {
  reset();
  strncpy ( this -> name, name, nameLen );
  if ( nBins > maxBins ) {
    if ( PROFILE_VERBOSE >= 4 ) {
      Serial.print ( "Error: nBins " ); Serial.print ( nBins );
      Serial.print ( " exceeds max of " ); Serial.println ( maxBins );
    }
    nBins = maxBins;
  }
  this -> nBins = nBins;
  this -> binSize = binSize;
  this -> lowBinStart = lowBinStart;
}

void Profile::start () {
  startedAt_ms = millis();
}
  
void Profile::stop () {
  if ( startedAt_ms == 0UL ) {
    // was not running; ERROR
    if ( PROFILE_VERBOSE >= 4 ) {
      Serial.print ( "Error: stop without start in " ); Serial.println ( name );
    }
  } else {
    // timer was running
    unsigned int duration = millis() - startedAt_ms;
    startedAt_ms = 0UL;
    if ( duration < lowBinStart ) duration = lowBinStart;
    int bin = ( duration - lowBinStart ) / binSize;
    if ( bin < 0 ) bin = 0;
    if ( bin > nBins - 1 ) bin = nBins - 1;
    bins [ bin ] ++;
    nHits++;
    totalTime += duration;
  }
}


void Profile::report () {
  Serial.print   ( name ); Serial.print ( ": " ); 
  Serial.print   ( nHits ); Serial.println ( " hits" );
  Serial.print   ( "  nBins: " ); Serial.println ( nBins );
  Serial.print   ( "  binSize: " ); Serial.println ( binSize );
  Serial.print   ( "  lowBinStart: " ); Serial.println ( lowBinStart );
  Serial.println ( "  bin values:" );
  unsigned long binStart = lowBinStart;
  for ( int i = 0; i < nBins; i++ ) {
    const int bufLen = 40;
    char buf [ bufLen ];
    snprintf ( buf, bufLen, "    [%8lu - %8lu]: %8lu\n",
               binStart, binStart + binSize - 1, bins [ i ] );
    Serial.print ( buf );
    binStart += binSize;
  }
  Serial.print ( "  total time: " ); Serial.println ( totalTime );
}
