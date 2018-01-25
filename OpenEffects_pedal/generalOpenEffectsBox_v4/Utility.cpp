#include "Utility.h"

Utility::Utility () {
  // constructor
}

void Utility::setVerbose ( int verbose ) {
  _verbose = verbose;
  Serial.print ( "Utility VERBOSE: " );
  Serial.println ( _verbose );
}

float Utility::fmap ( float n, float n0, float n9, float y1, float y9 ) {
  float p = ( n - n0 ) / ( n9 - n0 );
  return ( y1 + ( y9 - y1 ) * p );
}

float Utility::fconstrain ( float x, float y1, float y9 ) {
  bool inverse = y9 > y1;
  if ( ( x < y1 ) == inverse) return ( y1 );
  if ( ( x > y9 ) == inverse ) return ( y9 );
  return ( x );
}

float Utility::fmapc ( float n, float n0, float n9, float y1, float y9 ) {
  return fconstrain ( fmap ( n, n0, n9, y1, y9 ), y1, y9 );
}

/*!
  Simulation of the action of an audio-taper pot
*/

float Utility::expmap ( float x, float fifty_pct_value ) {
  // 0 -> small num; 0.5 -> fifty_pct_value; 1 -> 1
  // good for values of fifty_pct_value from >0 to about 0.4
  float scale = 1 / ( fifty_pct_value * fifty_pct_value );
  float base = log ( scale );  // log is natural log
  return ( exp ( x * base ) / scale );
}

