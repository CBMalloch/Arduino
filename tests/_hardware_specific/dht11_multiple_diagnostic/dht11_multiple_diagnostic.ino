/*
  Program dht11_multiple_diagnostic
  Author: Charles B. Malloch, PhD
  Date:   2015-10-13
  
  Note that the cheap Chinese DHT-11 devices, while they return bits past the binary point,
  return all zeroes in them, so the values are whole numbers.
  
*/

#include <stdio.h>
#include <DHT.h>

#define pdThrobber 13

#define nDHTs 5
short pdDHT11s [ nDHTs ] = { 2, 3, 4, 5, 6 };

DHT dht0 ( pdDHT11s [ 0 ], DHT11);
DHT dht1 ( pdDHT11s [ 1 ], DHT11);
DHT dht2 ( pdDHT11s [ 2 ], DHT11);
DHT dht3 ( pdDHT11s [ 3 ], DHT11);
DHT dht4 ( pdDHT11s [ 4 ], DHT11);

DHT dhts [ nDHTs ] = { dht0, dht1, dht2, dht3, dht4 };

/* 
enum { len_histoT = 20,
       len_histoH = 40,
     };
const int offset_histoT_degC = 10;
const int offset_histoH = 40;
int histoT_degC [ len_histoT ];  // 10-30
int histoH      [ len_histoH ];  // 40-80
*/

enum { bufLen = 12 };
char strBuf [ bufLen ];

void setup() {
  Serial.begin(115200);
  
  for ( int i = 0; i < nDHTs; i++ ) {
    dhts [ i ].begin();
  }    
 
  Serial.println ( "DHT multiple test v1.0.1  2015-10-13  cbm" );
}

void loop() {
  
  static unsigned long lastReadingAt_ms = 0UL;
  static unsigned long lastBlinkAt_ms = 0UL;
  unsigned long readInterval_ms = 2000UL;
  unsigned long blinkInterval_ms = 500UL;
  
  float humidity, temp_degC;
  // float temp_degF;
  
  if ( ( millis() - lastReadingAt_ms ) > readInterval_ms ) {
    // Serial.print ( "At " ); Serial.print ( millis() ); Serial.println ( ": " );
    snprintf ( strBuf, bufLen, "%7lu", millis() );
    Serial.print ( strBuf );
    
    for ( int i = 0; i < nDHTs; i++ ) { 
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      
      humidity = dhts [ i ].readHumidity ();
      temp_degC = dhts [ i ].readTemperature ();
      
      /*
      temp_degF = dhts [ i ].readTemperature ( true );
      
      Serial.print ( "DHT11." ); Serial.print ( i ); 
      Serial.print ( ": H: " ); Serial.print ( humidity );
      Serial.print ( "; C: " ); Serial.print ( temp_degC ); 
      Serial.print ( "; F:" ); Serial.print ( temp_degF );
      */
      
      Serial.print ( " | " );
      snprintf ( strBuf, bufLen, "\t%4d", int ( humidity ) );
      Serial.print ( strBuf );
      snprintf ( strBuf, bufLen, "\t%4d", int ( temp_degC ) );
      Serial.print ( strBuf );
      
    }
    
    Serial.println ();
    
    lastReadingAt_ms = millis ();
  }

  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    digitalWrite ( pdThrobber, 1 - digitalRead ( pdThrobber ) );
    lastBlinkAt_ms = millis ();
  }
  
}
