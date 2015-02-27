/*

  LED_wave_w_touch
  Author: Charles B. Malloch, PhD  2014-02-24
  
  Program demonstrates a quick-sine function, using it to generate a wave of light
  from a 10-element LED array whose elements are each connected to PWM output pins.
  Independently from the propagation of the wave, the main blinky LED flashes at a 
  rate dependent on the reading from a capacitance sensor.
  
*/

const int BAUDRATE = 115200;

const int pdBlinkyLED = 13;
const int pdTouch = 15;

int nLEDs;
int pdLEDs[] = { 3, 4, 5, 6, 9, 10, 23, 22, 21, 20 };
int brightness [  sizeof ( pdLEDs ) / sizeof ( int ) ];  // size set to equal that of pdLEDs

float mean, variance, stdev;
int threshold;
  
void setup () {
  Serial.begin ( BAUDRATE );
  
  // figure out how many LEDs are defined
  nLEDs = sizeof ( pdLEDs ) / sizeof ( int );
  Serial.print ( "nLEDs: " ); Serial.println ( nLEDs );
  
  for ( int i = 0; i < nLEDs; i++ ) {
    pinMode ( pdLEDs [ i ] , OUTPUT );
    brightness [ i ] = 0;
  }
  pinMode ( pdBlinkyLED, OUTPUT );
  
  /* test mySin
  
  for ( int i = 0; i < 64; i += 4 ) {
    Serial.print ( i ); Serial.print ( " : " );
    Serial.print ( mySin ( i ) ); Serial.println ();
  }
  while ( 1 ) { };
  
  */
  
  // find out the normal range of readings of the touch sensor
  
  delay ( 500 );
  
  const int nLoops = 1000;
  unsigned long sum = 0, sum2 = 0;
  for ( int i = 0; i < nLoops; i++ ) {
    int v = touchRead ( pdTouch );
    sum += v;
    sum2 += v * v;
    digitalWrite ( pdBlinkyLED, ! digitalRead ( pdBlinkyLED ) );
    delay ( 2 );
  }
  
  mean = sum / nLoops;
  variance = sum2 / nLoops - mean * mean;
  stdev = sqrt ( variance );

  threshold = int ( mean + 2 * stdev );
  Serial.print ( "mean: " ); Serial.println ( mean );
  Serial.print ( "variance: " ); Serial.println ( variance );
  Serial.print ( "std dev: " ); Serial.println ( stdev );
  Serial.print ( "threshold: " ); Serial.println ( threshold );
  
  // Serial.println (); Serial.print ( F ("0.3 Mem: ") ); Serial.println ( availableMemory() );
  // Serial.println (); Serial.print ( F ("Free mem: ") ); Serial.println ( FreeRam() ); 
    // gives 12231, now 12528, now 12460
  
  while ( !Serial && millis() < 10000 ) {
    digitalWrite ( pdBlinkyLED, ! digitalRead ( pdBlinkyLED ) );
    delay ( 50 );
  }
  Serial.println ( "LED_wave_w_touch v1.1.0 2015-02-27" );

  Serial.println (); Serial.println ( "Done with initialization" );
  delay ( 2000 );
  
}

void loop () {
  static unsigned long lastBlinkAt_ms = 0;
  static int blinkRate_ms = 600;
  
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
  
  int t;
  static float tsmooth = 700;
  const float alpha = 0.996;

  t = touchRead ( pdTouch );
  tsmooth = t * ( 1.0 - alpha ) + tsmooth * alpha;
  float zscore = ( tsmooth - mean ) / stdev;
  blinkRate_ms = constrain ( 600 / log ( zscore + 1.2 ) , 1, 1000 ); // nominally 600
  
  if ( ( millis() - lastDisplayAt_ms ) > loopRate_ms ) {
  
    Serial.print ( F ( "t: " ) ); Serial.print ( t );
    Serial.print ( F ( "; tsmooth: " ) ); Serial.print ( tsmooth );
    Serial.print ( F ( "; zscore: " ) ); Serial.print ( zscore );
    Serial.print ( F ( "; blinkRate_ms: " ) ); Serial.println ( blinkRate_ms );
  
    for ( int l = 0; l < nLEDs; l++ ) {
      // the angle offset of the first LED is angleOffset
      // the angular delta from LED to LED is physicalStepSize
      int theta = ( angleOffset - l * physicalStepSize + period ) % period;
      
      // diff is the distance in LEDs from the center of the brightness wave
          
      brightness [ l ] = ( mySin ( theta ) + 255 ) / 2;
      /*
      if ( ( diff < ( sizeof ( wave ) / sizeof ( int ) ) ) ) {
        brightness [ l ] = wave [ diff ];
      } else {
        brightness [ l ] = 0;
      }
      */
      /*
      Serial.print ( l ); Serial.print ( "[" ); Serial.print ( pdLEDs [ l ] ); Serial.print ( "]: " );
      Serial.print ( theta ); Serial.print ( " -> " );
      Serial.print ( brightness [ l ] ); Serial.print ( "\n" );
      */
      analogWrite ( pdLEDs [ l ], brightness [ l ] );
    }  // end for
    
    angleOffset += angularStepSize;
    if ( angleOffset >= period ) angleOffset = 0;
  
    lastDisplayAt_ms = millis();
  }  // end new display
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkRate_ms ) {
    digitalWrite ( pdBlinkyLED, 1 - digitalRead ( pdBlinkyLED ) );
    lastBlinkAt_ms = millis();
  }
  
}

int mySin (int n2pi_64) {
  /* 
    returns (int) 255 * sin ( n2pi_64 * 2 * pi() / 64 )
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

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>     


// int availableMemory() {
  // extern int __heap_start, *__brkval;
  // int v;
  // return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
// }

// http://forum.pjrc.com/threads/23256-Get-Free-Memory-for-Teensy-3-0
// uint32_t FreeRam(){ // for Teensy 3.0
    // uint32_t stackTop;
    // uint32_t heapTop;

    // current position of the stack.
    // stackTop = (uint32_t) &stackTop;

    // current position of heap.
    // void* hTop = malloc(1);
    // heapTop = (uint32_t) hTop;
    // free(hTop);

    // The difference is the free, available ram.
    // return stackTop - heapTop;
// }