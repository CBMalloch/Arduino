/*
  Program hc-06_demo
  Author  Charles B. Malloch, PhD
  Date    2015-09-22
  
  Program reads the value of a sensor (proxied by a potentiometer)
  and serial-outputs its value for bluetooth HC-06 transmission
  
*/

const unsigned long BAUDRATE = 115200;

enum {
      paPot = 3,
      pdThrobber = 13,
     };

void setup () {
  
  // note that the HC-06 (respond "linvorV1.8" to AT+VERSION\n) bluetooth modules 
  // come configured at some other baud rate; some 9600, some 57600, some 115200
  Serial.begin ( BAUDRATE );
    
  pinMode ( pdThrobber, OUTPUT ); digitalWrite ( pdThrobber, 1 );
  
  Serial.println ( "HC-06 demo v1.0 2015-09-22 cbm" );
  
}

void loop () {
  
  static unsigned long lastReadingAt_ms = 0UL;
  static unsigned long lastBlinkAt_ms = 0UL;
  static unsigned long lastPrintAt_ms = 0UL;
  static unsigned long lastGraphAt_ms = 0UL;
  static unsigned long lastFlashChangeAt_ms = 0UL;
  
  static unsigned long readingInterval_ms = 100UL;
  static unsigned long printInterval_ms = 1000UL;
  static unsigned long graphInterval_ms = 100UL;
  static unsigned long flashPeriod_ms = 50UL;
  
  static int countsPot;
  static float fracPot = 0.0;
  static float voltsPot = 0.0; 
      
  if ( ( millis() - lastReadingAt_ms ) > readingInterval_ms ) {
  
    countsPot = analogRead ( paPot );
    fracPot = float ( countsPot ) / 1023.0;
    voltsPot = fracPot * 5.0;

    lastReadingAt_ms = millis();
  
  }  // read data

  // printing
    
  if ( ( millis() - lastPrintAt_ms ) > printInterval_ms ) {
    Serial.print ( millis() ); Serial.print ( ": " );
    Serial.print ( fracPot );
    Serial.println ();
    
    lastPrintAt_ms = millis();
  }

  // graphing on Android via bluetooth dongle
  
  if ( ( millis() - lastGraphAt_ms ) > graphInterval_ms ) {
    Serial.print ( "E" );
    Serial.print ( voltsPot ); // Serial.print ( "," );
    Serial.println ();
    
    lastGraphAt_ms = millis();
  }
  
  // lights
      
  if ( ( millis() - lastFlashChangeAt_ms ) < ( flashPeriod_ms * fracPot ) ) {
    // s.b. on
    digitalWrite ( pdThrobber, 1 );
  } else {
    // s.b. off
    digitalWrite ( pdThrobber, 0 );
  }

  if ( ( millis() - lastFlashChangeAt_ms ) > flashPeriod_ms ) {
    lastFlashChangeAt_ms = millis();
  }
  
}

