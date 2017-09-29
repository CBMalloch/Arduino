/*
  sprouted from seed at http://blog.rickk.com/tech/2014/11/nec-ir-remote-decoder-for-sparkcore.html
    thanks to Rick Kasguma
*/

enum { VERBOSE = 1 };

enum { BAUDRATE = 115200 };
enum { pdIRR = 12 };
enum { timeout_ms = 5 };

volatile unsigned long valueTime_us = 0;
volatile unsigned long valueTime_ms = 0;
volatile unsigned long value = 0;
volatile int valueBitCount = 0;

char decodeString ( char string [], int len );

void setup() {

  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 );
  Serial.println ( "tsop853_IR_receiver.ino  v1.0  2015-02-16  cbm" );
  
  pinMode( pdIRR, INPUT );
  attachInterrupt ( pdIRR, isr, CHANGE );
  
  resetValue();
    
}

void loop() {
  
  static char dataString [ 8 ];
  static int dsPointer = 0;
  static unsigned long valueTime_local_ms = millis();
  
  /*
  
  Note: after long debugging process, I realized that the (volatile) valueTime_us was
  often being updated after the call to micros() was initiated, giving a very large 
  ( like negative ) value for the elapsed time. What was necessary was to make a local 
  copy valueTime_local_us. I left all the debug code in so I could relearn this valuable 
  lesson.
  
  */
  
  unsigned long valueTime_local_us = valueTime_us;
  unsigned long now_us = micros ();
  unsigned long delta_us = ( unsigned long ) now_us - ( unsigned long ) valueTime_local_us;
  unsigned long timeout_us = ( unsigned long ) timeout_ms * 1000UL;
  int timeout = delta_us > timeout_us;
  
  if ( ( ( millis() - valueTime_local_ms ) > 50 ) && dsPointer ) {
    // got some data, which has now timed out
    if ( 0 ) {
      Serial.print ( " ==> 0x" ); 
      for ( int i = 0; i < dsPointer; i++ ) {
        int theValue = dataString [ i ];
        if ( theValue < 0x10 ) Serial.print ( "0" );
        Serial.print ( theValue, HEX );
      }
      Serial.println ();
    }
    Serial.print ( "===> " ); Serial.println ( decodeString ( dataString, dsPointer ) );
    delay ( 100 );  // wait to limit repeat rate
    resetValue();   // clear the receive buffer
    dsPointer = 0;  // and then delete repeats
  }
  
  if (valueBitCount >= 8) {
    // got a whole byte
    // Serial.print ( " ==> 0x" ); Serial.println(value, HEX);
    
    dataString [ dsPointer++ ] = value;
    valueTime_local_ms = millis();
    
    // Serial.print ( " :: " ); Serial.println ( dsPointer );
    
    // String hexValue = String(value, HEX);
    // Spark.publish("musicRemote", hexValue, 5, PRIVATE);

    resetValue();
    //   } else if ( deltaTime() > 10000L && valueBitCount != 0) {
  } else { 
    // if ( ( ( millis() - valueTime_ms ) > timeout_ms ) && ( valueBitCount != 0 ) ) {
    if ( timeout && ( valueBitCount != 0 ) ) {
      if ( 0 ) {
        Serial.print ( "Rrr. " );
        Serial.print ( millis() );
        Serial.print ( "ms - " );
        Serial.print ( valueTime_ms );
        Serial.print ( " ( = " );
        Serial.print ( millis() - valueTime_ms );
        Serial.print ( ") > " );
        Serial.print ( timeout_ms );
        Serial.print ( " (" );
        Serial.print ( ( millis() - valueTime_ms ) > timeout_ms );
        Serial.print ( ")" );
        Serial.print ( " ( whole: " );
        Serial.print ( ( ( millis() - valueTime_ms ) > timeout_ms ) && ( valueBitCount != 0 ) );
        Serial.print ( " )" );
        Serial.println ();
      } else if ( 1 && VERBOSE >= 10 ) {
        Serial.print ( "Rrr. " );
        Serial.print ( now_us );
        Serial.print ( " - " );
        Serial.print ( valueTime_local_us );
        Serial.print ( " ( = " );
        Serial.print ( delta_us );
        Serial.print ( "us ) > " );
        Serial.print ( timeout_us );
        Serial.print ( " ( " );
        Serial.print ( timeout );
        Serial.print ( " )" );
        Serial.print ( " ( whole: " );
        Serial.print ( timeout && ( valueBitCount != 0 ) );
        Serial.print ( " )" );
        Serial.println ();
      }
      if ( VERBOSE >= 5 ) {
        Serial.print ( "At " );
        Serial.print ( millis() );
        Serial.print(": lost data: 0b");
        for ( int i = valueBitCount - 1; i >= 0; i-- ) {
          Serial.print ( ( ( value >> i ) & 0x0001 ) ? "1" : "0" );
        }
        Serial.print ( " ( 0x" );
        Serial.print ( value, HEX );
        Serial.println ( "). Reset");
      }
      resetValue();
    }
  }
}

void resetValue() {
  // called from ISR, be careful what you put in this function
  value = 0;
  valueBitCount = 0;
}

char decodeString ( char string [], int len ) {
  /*
    These button encodings relate specifically to the KEYES IR remote
    kit item 26329 available via eBay.
    The IR decoders seem all to work the same 
    - no need to use the one from the kit.
  */
  enum { nButtons = 17 };
  static const char buttons[ nButtons ] = { 
      'U', 'R', 'D', 'L', 'K',
      '1', '2', '3', '4', '5',
      '6', '7', '8', '9', '0',
      '*', '#' };
  static const unsigned int strings [ nButtons ] = { 
      0x629d, 0xc23d, 0xa857, 0x22dd, 0x02fd,
      0x6897, 0x9867, 0xb04f, 0x30cf, 0x18e7, 
      0x7a85, 0x10ef, 0x38c7, 0x5aa5, 0x4ab5,
      0x42bd, 0x52ad };
  static char lastItem = 'W';
  
  unsigned int hdr   = string [ 0 ] << 8 | string [ 1 ];
  unsigned int value = string [ 2 ] << 8 | string [ 3 ];
  char rv = 'Y';
  
  if ( hdr == 0xffff ) {
    // repeat
    return ( lastItem );
  }
  
  if ( hdr != 0x00ff ) {
    Serial.print ( "Bad header: 0x" ); Serial.println ( hdr, HEX );
    rv = 'X';
  } else {
  
    for ( int i = 0; i < nButtons; i++ ) {
      if ( value == strings [ i ] ) {
        rv = ( buttons [ i ] );
      }
    }
  }
  lastItem = rv;
  return ( rv );
}

// Called whenever pdIRR changes state. This is an ISR; don't do much in this function!
unsigned long delta_us;  // global so the isr doesn't have to allocate it every time!
void isr() {
  
  // store bits into "value" and keep its bit count in valueBitCount
  
  if ( digitalRead( pdIRR ) ) {
    // Rising
    valueTime_us = micros();  // record time of rising edge
    valueTime_ms = millis();  // for timeout use
  } else {
    // Falling
    // delta = deltaTime();
    delta_us = micros() - valueTime_us;
    if ( delta_us < 800 ) {
        // 0 bit, 562.5 us
        value <<= 1;
        valueBitCount++;
    } else if (delta_us < 1900) {
      // 1 bit, 1687.5 us
      value <<= 1;
      value |= 0x01UL;
      valueBitCount++;
    } else if (delta_us < 2600) {
      // Repeat bit, 2250 us
      value = 0xffffffffUL;
      valueBitCount = 32;
    } else {
      // header is 4300 us. ignore and reset value
      resetValue();
    }
  }
}
