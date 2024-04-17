/*
  The eventually-identical version used for development is under tests
  This version is under the library, in examples
*/

#include "cbmCircularBuffer.h"

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************

#define BAUDRATE 115200

CircularBuffer<int> vuhI ( 15 );
int linearIBuf [ 17 ];
CircularBuffer<float> vuhF ( 20 );
float linearFBuf [ 22 ];

void setup () {
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 );
  for ( int i = 0; i < 17; i++ ) {
    linearIBuf [ i ] = -3;
  }
  for ( int i = 0; i < 22; i++ ) {
    linearFBuf [ i ] = 99.1;
  }
  
  Serial.printf ( "I: count: %ul; size: %ul; firstIndex: %ul\n", 
    vuhI.count (), vuhI.bufSize (), vuhI.firstIndex () );
  Serial.printf ( "F: count: %ul; size: %ul; firstIndex: %ul\n", 
    vuhF.count (), vuhF.bufSize (), vuhF.firstIndex () );

  Serial.println ( "\nInitialization done" );

  vuhF.store ( 21.0 );
  vuhF.store ( 22 );
  vuhI.store ( 1 );
  vuhI.store ( 2 );
  Serial.println ( "dumping two values set" );
  dumpValues ();
  
  for ( int i = 0; i < 30 - 2; i++ ) {
    vuhI.store ( 3+i );
  }
  
  for ( int i = 0; i < 30 - 2; i++ ) {
    vuhF.store ( 23.1+i );
  }
  
  Serial.println ( "dumping 30 values set" );
  dumpValues ();
  
}
  
void loop () { yield(); }

void dumpValues () {

  // dumping one extra value from each array to verify non-overrunning
  
  Serial.printf ( "I: count: %ul; size: %ul; firstIndex: %ul\n", 
    vuhI.count (), vuhI.bufSize (), vuhI.firstIndex () );
  vuhI.entries ( linearIBuf );
  for ( int i = 0; ( i <= vuhI.count() ) && ( i <= vuhI.bufSize() + 1 ); i++ ) {
    Serial.printf ( "%4d ", linearIBuf [ i ] );
  }
  Serial.printf ( "\n" );
  
  Serial.printf ( "F: count: %ul; size: %ul; firstIndex: %ul\n", 
    vuhF.count (), vuhF.bufSize (), vuhF.firstIndex () );
  vuhF.entries ( linearFBuf );
  for ( int i = 0; ( i <= vuhF.count() ) && ( i <= vuhF.bufSize() + 1 ); i++ ) {
    Serial.printf ( "%4.1f ", linearFBuf [ i ] );
  }
  Serial.printf ( "\n" );
  
}
