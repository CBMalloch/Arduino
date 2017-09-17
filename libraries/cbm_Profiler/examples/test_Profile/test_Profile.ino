#include <Profiler.h>

#define BAUDRATE    115200
#define VERBOSE          4

const int ppEye = 12;
const int pdLED = 13;
const int pdPIR = 14;


Profile x;
Profile y;

void setup () {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 10000 ) ) {
    digitalWrite ( pdLED, ( millis() >> 7 ) & 0x01 );
    delay ( 200 );
  }
  
  pinMode ( pdPIR, INPUT );
  
  x.setup ( "x", 12, 20, 3 );
  y.setup ( "y", 10, 15 );
  for ( int i = 0; i < 100; i++ ) {
    x.start();
    y.start();
    delay ( rand() % 200 * 10 );
    y.stop();
    delay ( rand() % 200 * 10 );
    x.stop();
    Serial.println ( i );
  }
  x.report ();
  y.report ();
  
}

void loop () {
}

