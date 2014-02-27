/*
  Program: count 3 vs 2
  Author: Charles B. Malloch, PhD  2014-02-27
  Purpose: to demonstrate how to keep track of several things at once,
    seemingly simultaneously
    
*/

const int pdLEDs [] = { 4, 21 };                  // the digital pin number for each LED
const unsigned long fullCycleTakes_ms = 1000;     // how long is a musical measure

// bookkeeping for each LED
unsigned long lastTurnedOnAt_ms [ 2 ];
int myBeatsPerCycle [] = { 2, 3 };
unsigned long myCycleTakes_ms [ 2 ];              // how long is one of my beats
unsigned long myFlashLasts_ms [ 2 ];              // how long should I be on during each beat

void setup () {
  int i;
  
  for ( i = 0; i < 2; i++ ) {
    // set up the digital pin and turn it off to start
    pinMode ( pdLEDs [ i ], OUTPUT );
    digitalWrite ( pdLEDs [ i ], 0 );
    
    // initialize the timer
    lastTurnedOnAt_ms [ i ] = 0;
    
    // set up the timers
    myCycleTakes_ms [ i ] = fullCycleTakes_ms / myBeatsPerCycle [ i ];
    myFlashLasts_ms [ i ] = myCycleTakes_ms [ i ] / 2;
  }
}

void loop () {
  int i;
  
  for ( i = 0; i < 2; i++ ) {
    
    // Is LED i off?
    if ( digitalRead ( pdLEDs [ i ] ) == 0 ) {
      // It's off. Is it time to turn LED i on?
      if ( ( millis () - lastTurnedOnAt_ms [ i ] ) > myCycleTakes_ms [ i ] ) {
        digitalWrite ( pdLEDs [ i ], 1 );
        // Note when we did this
        lastTurnedOnAt_ms [ i ] = millis();
      }
    } else {
      // LED i is on. Is it time to turn it off?
      if ( ( millis () - lastTurnedOnAt_ms [ i ] ) > ( myFlashLasts_ms [ i ] ) ) {
        // It's been on long enough
        digitalWrite ( pdLEDs [ i ], 0 );
      }
    }
    
  }  // looping through each LED
  
}