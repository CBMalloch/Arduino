/*
  Program ESP-01_sleep_and_ADC
  Author: Charles B. Malloch, PhD
  Date:   2016-02-27
  
  Sleeping: the sleep function RESETS to wake back up.
  
  No WiFi in this program
  
*/

// extern "C" {
//   #include "include/wl_definitions.h"
// }

#include <stdio.h>
// #include <user_interface.h>
// #include <ESP8266WiFi.h>
#include <math.h>
#include <Stats.h>

Stats adcCounts;


/*
 D   GPIO
 0    16
 1     5
 2     4
 3     0  <-  ESP-01 PCB pin 6
 4     2  <-  ESP-01 PCB pin 4
 5    14
 6    12
 7    13  <- maybe BUILTIN_LED for Arnica boards
 8    15
 9     3
10     1 
11     9
12    10

ADC is ESP8266 pin 6; it accepts 0-1V -> 0-1023 counts

*/

unsigned long sleepPeriod_s = 4;

// ***************************************
// ***************************************

const int VERBOSE    =           4 ;
const int BAUDRATE   =      115200 ;
const int pdThrobber = BUILTIN_LED ;   // value is GPIO number; note GPIO 0 doubles as FLASH

// ***************************************
// ***************************************


int initialCounts = -1000;

void setup() {
  
  initialCounts = analogRead ( A0 );
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 10000 ) ) {
    digitalWrite ( pdThrobber, ( millis() >> 7 ) & 0x01 );
  }
  
  // the pin used for this is also TX, so need to call this AFTER Serial.begin
  pinMode ( pdThrobber, FUNCTION_0 ); digitalWrite ( pdThrobber, 0 );  // turns blue light ON
  
  // yield();
    
  Serial.println ( "\n\n\nESP-01_sleep_and_ADC v0.0.1  2016-02-27  cbm" );
  
  Serial.print ( "Sleep period: " ); Serial.print ( sleepPeriod_s ); Serial.println ( " sec" );
  
  Serial.print ( "Initial reading of analog counts: " ); Serial.println ( initialCounts );
   
}

void loop() {
  
  adcCounts.reset();
  for ( int i = 0; i < 10; i++ ) {
    int counts = analogRead ( A0 );
    adcCounts.record ( (double) counts );
    if ( VERBOSE > 2 ) Serial.print ( counts ); Serial.print ( " " );
    delay ( 10 ) ;
    yield ();
  }
  if ( VERBOSE > 2 ) Serial.println ();
  
  Serial.print ( millis() ); 
  Serial.print ( " -> counts: " ); Serial.print ( adcCounts.mean() );
  Serial.print ( " with s.d. " ); Serial.print ( adcCounts.stDev() );
  if ( VERBOSE > 1 ) {
    const double voltageDividerRatio = 1.0 / 6.0;
    double Vpin = adcCounts.mean() / 1024.0;
    double Vsupply = Vpin / voltageDividerRatio;
    // on ESP-01 when powered by BUB we're getting a Vpin of 0.861
    // ( although that drops a bunch when chip is awake )
    // but reporting 124 counts and Vpin of 0.12
    // after initial counts of 115
    // that's a factor of ( 0.861 * 1024 = 882, which / 124 = 7.11 )
    // reloading as if the ESP-01 were an NodeMCU 1.0; no change
    // was thinking that there is a Vref value somewhere...
    Serial.print ( "; Vpin = " ); Serial.print ( Vpin );
    Serial.print ( "; Vsupply = " ); Serial.print ( Vsupply );
  }
  
  Serial.println ();
  
  Serial.println ( "Entering sleep mode" );
  digitalWrite ( pdThrobber, 1 );  // turns blue light OFF

  ESP.deepSleep ( sleepPeriod_s * 1000000, WAKE_RF_DEFAULT ); // Sleep (usec)
  
  while ( 0 ) {
    Serial.println ( millis() );
    delay ( 1 );
  }
  
  // it apparently takes precisely 100ms to go to sleep...
  delay ( 100 );
  
  // should never reach here; when deepSleep wakes back up, it does a reset.
  // but it actually keeps going for a little bit 
  //   -- probably deepSleep initiates sleep asynchronously and takes a little time

  for ( int i = 0; i < 20; i++ ) {
    Serial.println ( "WAH! Didn't fall asleep!" );
  }
  delay ( 5 * 1000 );

  
}
