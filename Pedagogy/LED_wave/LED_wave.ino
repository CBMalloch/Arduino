const int BAUDRATE = 115200;
const int paLED = 13;
int nLEDs;

int paLEDs[] = { 23, 22, 21, 20, 3, 4, 5, 6, 9, 10 };
int brightness [ 10 ];  // don't know how to dynamically set this array size... :(

void setup () {
  Serial.begin ( BAUDRATE );
  delay ( 500 ); // while ( !Serial ) {};
  Serial.println ( "LED_wave v2.0.5 2014-02-01" );
  
  // figure out how many LEDs are defined
  nLEDs = sizeof ( paLEDs ) / sizeof ( int );
  Serial.print ( "nLEDs: " ); Serial.println ( nLEDs );
  
  for ( int i = 0; i < nLEDs; i++ ) {
    pinMode ( paLEDs [ i ] , OUTPUT );
    brightness [ i ] = 0;
  }
  pinMode ( paLED, OUTPUT );
  
  /* test mySin
  
  for ( int i = 0; i < 64; i += 4 ) {
    Serial.print ( i ); Serial.print ( " : " );
    Serial.print ( mySin ( i ) ); Serial.println ();
  }
  while ( 1 ) { };
  
  */
  
  delay ( 2000 );
  Serial.println ( "Done with initialization" );
  
}

void loop () {
  static unsigned long lastBlinkAt_ms = 0;
  const int blinkRate_ms = 600;
  
  static unsigned long lastDisplayAt_ms = 0;
  const int loopRate_ms = 80;
  
  /*
    The LED wave has a physical width, measured in LEDs (full width, how many LEDs do
    we want to see lit at once), stored in variable "width".
    It also has a temporal width, related to how much time it takes for the wave / pulse 
    to pass through a single fixed LED. This is stored in variable "timeStepSize".
  */
  
  const int period = 64;  // the integer period of the sin wave function
  const int width = 12;  // the physical width of one full cycle of the sin wave
  const int physicalStepSize = period / width;
  
  const int timeStepSize = 6;
  // at each step, the wave should move period / timeStepSize of a full sin period
  const int angularStepSize = period / timeStepSize;
  static int angleOffset = 0;
  
  if ( ( millis() - lastDisplayAt_ms ) > loopRate_ms ) {
  
    for ( int l = 0; l < nLEDs; l++ ) {
      // the angle offset of the first LED is angleOffset
      // the angular delta from LED to LED is physicalStepSize
      int theta = ( angleOffset - l * physicalStepSize + period ) % period;
      
      // diff is the distance in LEDs from the center of the brightness wave
      
      Serial.print ( l ); Serial.print ( "[" ); Serial.print ( paLEDs [ l ] ); Serial.print ( "]: " );
          
      brightness [ l ] = ( mySin ( theta ) + 255 ) / 2;
      /*
      if ( ( diff < ( sizeof ( wave ) / sizeof ( int ) ) ) ) {
        brightness [ l ] = wave [ diff ];
      } else {
        brightness [ l ] = 0;
      }
      */
      Serial.print ( theta ); Serial.print ( " -> " );
      Serial.print ( brightness [ l ] ); Serial.print ( "\n" );
      analogWrite ( paLEDs [ l ], brightness [ l ] );
    }  // end for
    
    angleOffset += angularStepSize;
    if ( angleOffset >= period ) angleOffset = 0;
  
    lastDisplayAt_ms = millis();
  }  // end new display
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkRate_ms ) {
    digitalWrite ( paLED, 1 - digitalRead ( paLED ) );
    lastBlinkAt_ms = millis();
  }
  
}

int mySin (int n2pi_64) {
  /* returns 255 * sin ( n2pi_64 * 2 * pi() / 64 )
  */
  
  const int quarterSin[] = {
    0, 25, 50, 74, 98, 120, 142, 162, 
    180, 197, 212, 225, 236, 244, 250, 254, 
    255 };
    
  const int qsLen = ( sizeof ( quarterSin ) / sizeof ( int ) ) - 1;  // e.g. 16
    
  int sign = ( n2pi_64 > ( 2 * qsLen ) ) ? -1 : 1;
  int offset;
  if ( ( ( n2pi_64 / qsLen ) % 2 ) == 0 ) {
    // even quadrant
    offset = n2pi_64 % qsLen;
  } else {
    // odd quadrant
    offset = qsLen - ( n2pi_64 % qsLen );
  }
  
  /*
  Serial.print ( n2pi_64 ); Serial.print ( ": " );
  Serial.print ( sign ); Serial.print ( " | " ); 
  Serial.print ( offset ); Serial.println ();
  */
  
  return ( quarterSin [ offset ] * sign );
  
}
    


