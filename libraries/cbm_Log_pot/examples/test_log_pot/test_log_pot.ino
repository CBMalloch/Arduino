#include <log_pot.h>

#define BAUDRATE 115200

Log_pot lp;

void setup () {
  Serial.begin ( BAUDRATE );
  
  report_internals();
  for ( int i = 0; i <= 10; i++ ) {
    report ( 0.1 * i );
  }
  Serial.println ();
  
  lp.init ( 0.4 );
  report_internals();
  for ( int i = 0; i <= 10; i++ ) {
    report ( 0.1 * i );
  }
  Serial.println ();
  
  lp.init ( 0.6 );
  report_internals();
  for ( int i = 0; i <= 10; i++ ) {
    report ( 0.1 * i );
  }
  Serial.println ();
  
  lp.init ( 0.8 );
  report_internals();
  for ( int i = 0; i <= 10; i++ ) {
    report ( 0.1 * i );
  }

}

void report ( float x ) {
  Serial.print ( "  " );
  Serial.print ( x );
  Serial.print ( " -> " );
  Serial.print ( lp.value ( x ) );
  
  Serial.println ();
    
}

void report_internals() {
  double internals[5];
  lp._internals ( internals );
 
  Serial.print ( "      //       upper: " );
  Serial.print ( internals [ 0 ] );
  Serial.print ( "; p: " );
  Serial.print ( internals [ 1 ] );
  Serial.print ( "; r: " );
  Serial.print ( internals [ 2 ] );
  Serial.print ( "; a: " );
  Serial.print ( internals [ 3 ] );
  Serial.print ( "; b: " );
  Serial.print ( internals [ 4 ] );

  Serial.println ();
}

void loop () {
}
