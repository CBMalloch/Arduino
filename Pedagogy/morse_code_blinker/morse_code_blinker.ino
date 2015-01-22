// Morse Code Generator for Arduino / Teensy
// Charles B. Malloch, PhD
// 2015-01-13

enum { BAUDRATE = 115200 };
enum { pdLED = 13 };
enum { pdBUZZER = 10 };

enum { dotLen_ms = 200 };

enum { bufLen = 80 };
int bufPtr = 0;
char strBuf [bufLen];

void setup () {

  Serial.begin( BAUDRATE );
  
  pinMode ( pdLED, OUTPUT );
    
  for ( int i = 0; i < 5; i++ ) {
    digitalWrite ( pdLED, 1 );
    delay ( 200 );
    digitalWrite ( pdLED, 0 );
    delay ( 200 );
  }
  
  while ( ( millis() < 5000 ) && !Serial ) {
    delay ( 500 );
  }
  
  Serial.println ( "morse_code_blinker v2.10 2015-01-16" );
  
  sendMorseMessage ( 18, "morse code blinker" );
}

void loop () {
  checkSerial ();
  LEDToggle ();
  delay ( 10 );
}

void checkSerial () {
  const int BUFLEN = 200;
  static char buffer[BUFLEN];
  int i, len;
  
  if ( len = Serial.available() ) {
    for ( i = 0; i < len; i++ ) {
      if ( bufPtr >= BUFLEN ) {
        Serial.println ( "BUFFER OVERRUN" );
        bufPtr = 0;
      }
      buffer [ bufPtr ] = Serial.read ();
      // LEDToggle();
      Serial.println( buffer[bufPtr], HEX );
      if ( bufPtr > 0 && buffer[bufPtr] == 0x0d ) {
        buffer [ bufPtr+1 ] = 0x00;
        Serial.println ( "[Arduino] " );
        // delay ( 100 );
        Serial.print ("  >");
        Serial.println ( buffer );  // still has CRLF
        // Serial.println (DateTime.now());  // provides seconds since boot unless inited
        //    or look at millis() for milliseconds
        
        sendMorseMessage ( bufPtr, buffer );
        
        bufPtr = 0;
        break;
      } else if ( buffer[bufPtr] == 0x0a ) {
        // ignore LF
      } else {
        bufPtr++;
      }  // handling linends
    }  // reading the input buffer
  }  // Serial.available
}

void sendMorseMessage ( int msgLen, char buffer[] ) {

  Serial.print (" 2>");
  Serial.println ( buffer );  // still has CRLF
  
  // store Morse alphabet as 2 bytes per character, base 4, high-order-bit justified
  //  00 = dot; 01 = dash; 10 = silence for 1 dot time; 11 = end-of-character
  //  so A = .- = 000111xx xxxxxxxx, and space = 10101010 1011xxxx ( 5 dot times + one before and one after = 7 dot times )
    static const long int morse [] =  { 
                                0b0101010101111111, // 0 -----
                                0b0001010101111111, // 1 .----
                                0b0000010101111111, // 2 ..---
                                0b0000000101111111, // 3 ...--
                                0b0000000001111111, // 4 ....-
                                0b0000000000111111, // 5 .....
                                0b0100000000111111, // 6 -....
                                0b0101000000111111, // 7 --...
                                0b0101010000111111, // 8 ---..
                                0b0101010100111111, // 9 ----.
                                0b0001111111111111, // A .-
                                0b0100000011111111, // B -...
                                0b0100010011111111, // C -.-.
                                0b0100001111111111, // D -..
                                0b0011111111111111, // E .
                                0b0000010011111111, // F ..-.
                                0b0101001111111111, // G --.
                                0b0000000011111111, // H ....
                                0b0000111111111111, // I ..
                                0b0001010111111111, // J .---
                                0b0100011111111111, // K -.-
                                0b0001000011111111, // L .-..
                                0b0101111111111111, // M --
                                0b0100111111111111, // N -.
                                0b0101011111111111, // O ---
                                0b0001010011111111, // P .--.
                                0b0101000111111111, // Q --.-
                                0b0001001111111111, // R .-.
                                0b0000001111111111, // S ...
                                0b0111111111111111, // T -
                                0b0000011111111111, // U ..-
                                0b0000000111111111, // V ...-
                                0b0001011111111111, // W .--
                                0b0100000111111111, // X -..-
                                0b0100010111111111, // Y -.--
                                0b0101000011111111, // Z --..
                                0b1010101010101111, // space adds 6 dot times silence
                              };

  
                      
  Serial.print (" 3>");
  Serial.println ( buffer );  // still has CRLF
  
  int theChar, theIndex;
  
  digitalWrite ( pdLED, 0 );
  delay ( 1000 );
  
  for ( int i = 0; i < msgLen; i++ ) {
    theChar = buffer [ i ];
    Serial.print ( " '" ); Serial.print ( toascii ( theChar ) ); Serial.print ( "' " );
    if ( ( theChar >= int ( '0' ) ) && ( theChar <= int ( '9' ) ) ) {
      theIndex = theChar - int ( '0' );;
    } else if ( ( theChar >= int ( 'A' ) ) && ( theChar <= int ( 'Z' ) ) ) {
      theIndex = theChar - int ( 'A' ) + 10;
    } else if ( ( theChar >= int ( 'a' ) ) && ( theChar <= int ( 'z' ) ) ) {
      theIndex = theChar - int ( 'a' ) + 10;
    } else if ( theChar == ' ' ) {
      theIndex = 0 + 26 + 10;
    } else {
      Serial.print ( "Unknown character " ); Serial.println ( theChar );
      // Serial.println ( int ( 'A' ) );
      theIndex = -1;
    }
      
    if ( theIndex >= 0 ) {
      Serial.print ( " ( " ); Serial.print ( theIndex ); Serial.print ( " ) " );
      long int theMorse = morse [ theIndex ];
      sendMorseCharacter ( theMorse );
      delay ( dotLen_ms );
    } else {
      Serial.println ( "<space>" );
    }
  }
  
  digitalWrite ( pdLED, 0 );
  delay ( 1000 );
}

void sendMorseCharacter ( long int theMorseChar ) {
  Serial.print ( "-->" );
  for ( int i = 0; i < 8; i++ ) {
    // send bit i
    // time for dash is 3x time for dot
    // Serial.print ( "0x" ); Serial.println ( theMorseChar, HEX );
    int theSymbol = ( theMorseChar >> 14 ) & 0x03;
    
    if ( theSymbol == 0b11 ) { // end of character
      // Serial.println ( "done" );
      break;  // exit for loop; cannot break the for loop from within the switch
    }
    
    switch ( theSymbol ) {
      case 0b00:  // dot
        Serial.print ( "." );
        digitalWrite ( pdLED, 1 );
        tone ( pdBUZZER, 440 );
        delay ( 1 * dotLen_ms );
        digitalWrite ( pdLED, 0 );
        noTone ( pdBUZZER );
        break;
      case 0b01:  // dash
        Serial.print ( "-" );
        digitalWrite ( pdLED, 1 );
        tone ( pdBUZZER, 440 );
        delay ( 3 * dotLen_ms );
        digitalWrite ( pdLED, 0 );
        noTone ( pdBUZZER );
        break;
      case 0b10:  // silence for 1 dot time
        Serial.print ( "b" );
        digitalWrite ( pdLED, 0 );
        noTone ( pdBUZZER );
        delay ( 1 * dotLen_ms );
        break;
    }
    theMorseChar <<= 2;
    delay ( dotLen_ms );  // pause between symbols is one dot time
  }
  Serial.println ();
}

void LEDToggle() {
  digitalWrite( pdLED, 1 - digitalRead ( pdLED ) );
}
