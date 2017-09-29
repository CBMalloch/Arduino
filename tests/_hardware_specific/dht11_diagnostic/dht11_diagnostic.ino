/*
  Program dht11_diagnostic
  Author: Charles B. Malloch, PhD
  Date:   2015-09-22
  
*/

// #include <stdio.h>
#include "DHT.h"

#define pdDHT11 5

DHT dht ( pdDHT11, DHT11);

/* 
enum { len_histoT = 20,
       len_histoH = 40,
     };
const int offset_histoT_degC = 10;
const int offset_histoH = 40;
int histoT_degC [ len_histoT ];  // 10-30
int histoH      [ len_histoH ];  // 40-80

enum { bufLen = 8 };
char strBuf [ bufLen ];
*/


void setup() {
  Serial.begin(115200);
  
  dht.begin();
  
/*   
  for ( int i = 0; i < len_histoH; i++ ) {
    histoH [ i ] = 0;
  }
  for ( int i = 0; i < len_histoT; i++ ) {
    histoT_degC [ i ] = 0;
  }
*/  
 
  Serial.println ( "DHT demo v1.0  2015-09-22  cbm" );
}

void loop() {
  float humidity, temp_degC, temp_degF;
  int d_humidity, d_temp_degC;
  static int humidity_prev = -100;
  static int temp_degC_prev = -100;
  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  
  humidity = dht.readHumidity();
  temp_degC = dht.readTemperature();
  temp_degF = dht.readTemperature(true);
  
  if ( humidity_prev == -100 ) {
    d_humidity = 0;
  } else {
    d_humidity = int ( humidity ) - humidity_prev;
  }
  humidity_prev = int ( humidity );
    
  if ( temp_degC_prev == -100 ) {
    d_temp_degC = 0;
  } else {
    d_temp_degC = int ( temp_degC ) - temp_degC_prev;
  }
  temp_degC_prev = int ( temp_degC );
      
 /*  
  histoH      [ int ( humidity  ) - offset_histoH      ] ++;
  histoT_degC [ int ( temp_degC ) - offset_histoT_degC ] ++;
 
  Serial.print ( "humidity: " );
  for ( int i = 0; i < len_histoH; i++ ) {
    snprintf ( strBuf, bufLen, "%3d ", histoH [ i ] );
    Serial.print ( strBuf ); 
  }
  Serial.println ();
  
  Serial.print ( "temp (degC): " );
  for ( int i = 0; i < len_histoT; i++ ) {
    snprintf ( strBuf, bufLen, "%3d ", histoT_degC [ i ] );
    Serial.print ( strBuf ); 
  }
  Serial.println ();
  
 */  
  
  Serial.print ( "E" );
  Serial.print ( humidity ); Serial.print ( "," );
  Serial.print ( temp_degC ); Serial.print ( "," );
  Serial.print ( temp_degF ); Serial.print ( "," );
  Serial.print ( d_humidity ); Serial.print ( "," );
  Serial.print ( d_temp_degC ); Serial.println ();

  delay(2000);
}
