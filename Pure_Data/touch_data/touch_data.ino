/*

  touch_data
  Author: Charles B. Malloch, PhD  2014-11-04
  
  Program sends the reading from a capacitance sensor as an binary integer for 
  Pure Data to read.
  
*/

const int BAUDRATE = 115200;

const int pdBlinkyLED = 13;
const int pdTouch = 15;

float mean, variance, stdev;
int threshold;
  
void setup () {
  Serial.begin ( BAUDRATE );
  pinMode ( pdBlinkyLED, OUTPUT );
  delay ( 500 ); // while ( !Serial ) {};
  
  // Serial.println ( F ( "touch_data v0.0.1 2014-11-04" ) );
  
  // delay ( 500 );
    
}

void loop () {
  static unsigned long lastBlinkAt_ms = 0;
  static int blinkRate_ms = 500;
  
  static unsigned long lastDisplayAt_ms = 0;
  const int loopRate_ms = 80;
    
  int t;
  static float tsmooth = 700;
  const float alpha = 0.996;

  t = touchRead ( pdTouch );
  tsmooth = t * ( 1.0 - alpha ) + tsmooth * alpha;
  
  if ( ( millis() - lastDisplayAt_ms ) > loopRate_ms ) {
  
    Serial.write ( t );
    
    lastDisplayAt_ms = millis();
  }  // end new display
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkRate_ms ) {
    digitalWrite ( pdBlinkyLED, 1 - digitalRead ( pdBlinkyLED ) );
    lastBlinkAt_ms = millis();
  }
  
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