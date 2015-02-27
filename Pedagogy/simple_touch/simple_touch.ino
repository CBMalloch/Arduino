/*
  simple_touch - demonstration of the touch sensor
  as developed for the Introduction to Arduino class
  with some minor enhancements
  Charles B. Malloch, PhD  2015-02-26
*/

#define BAUDRATE 115200
#define pdLED 13
// one can specify a touch pin by digital pin number with the bare number (e.g. 15)
#define ptTouch 15

void setup () {
  pinMode ( pdLED, OUTPUT );
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 10000 ) {
    digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
    delay ( 50 );
  }
}

void loop () {
  int counts, nSpaces;
  
  counts = touchRead ( ptTouch );
    
  // 'P' will print the results every half-second, and 
  // 'B' will do the bouncing ball thing.
  
  char myChoice = 'P';
  
  switch ( myChoice ) {
  
    case 'B':
    
      // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      //    print results every half-second
      // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      Serial.print ( "cts: " );
      Serial.print ( counts );
      Serial.println ();
      
      delay ( 500 );
      break;
      
    case 'B':
  
      // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      //    display results as bouncing ball
      // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

      const int line_length = 60;
      int nSpaces;
      nSpaces = map ( counts, 0, 1023, 0, 60 );
      
      // create the space to the left of the asterisk
      for ( int i = 0; i < nSpaces; i++ ) {
        Serial.print ( " " );
      }
      Serial.println ( "*" );
      delay ( 200 );
      break;
   
  }
  
  digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
  
}