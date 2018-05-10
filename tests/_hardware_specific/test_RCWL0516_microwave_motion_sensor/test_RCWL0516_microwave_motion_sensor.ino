#define PROGNAME "test_RCWL0516_microwave_motion_sensor"
#define VERSION  "0.1.0"
#define VERDATE  "2018-04-28"

/*

      cbmalloch/tree          : mode for tree
      cbmalloch/tree/set      : third variable (HSV) for tree
      cbmalloch/tree/xVelocity: spatial velocity of pattern
      cbmalloch/tree/tVelocity: temporal velocity of pattern

*/

// ***************************************
// ***************************************

#define BAUDRATE      115200
#define SEND_TO_MQTT 

// ***************************************
// ***************************************

#include "EWMA.h"

#define paSensor 0

#define pdLED 13

EWMA slow, mediumAbs, fast, fastAbs;

const int lineLen   = 120;
const int maxCounts = 1024;

float firstAnalogValue = -1e6;

int mode = 2;  // 0 = set; 1 = xVelocity; 2 = tVelocity
char itemStrings [3][20] = { "set", "xVelocity", "tVelocity" };

const size_t pBufLen = 512;
char pBuf [ pBufLen ];

const size_t fBufLen = 12;
char fBuf [ fBufLen ];

void setup() {

  Serial.begin ( BAUDRATE ); while ( ! Serial && ( millis() < 1000 ) );

  pinMode ( pdLED, OUTPUT );
  slow.setAlpha ( slow.alpha ( 10000 ) );
  mediumAbs.setAlpha ( mediumAbs.alpha ( 2000 ) );
  fast.setAlpha ( fast.alpha ( 10 ) );
  fastAbs.setAlpha ( fastAbs.alpha ( 10 ) );
  
  for ( int i = 0; i < 1000; i++ ) {
    slow.record ( analogRead ( paSensor ) );
    delay ( 10 );
  }
  firstAnalogValue = slow.value();
  
  /************************** Connect MQTT pass-thru **************************/
    
  #ifdef SEND_TO_MQTT
    Serial.println ( "{ "
        "\"command\": \"connect\", "
        "\"networkSSID\": \"dd-wrt-03\", "
        "\"networkPW\": \"cbmAlaskas\", "
        "\"MQTThost\": \"mosquitto\", "
        "\"MQTTuserID\": \"test_RCWL9197_microwave_motion_sensor\", "
        "\"MQTTuserPW\": \"whatever\" "
      " }" );
  #endif
    
  /************************** Report successful init **************************/

  Serial.println();
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
}

void loop() {
  
  int counts;
  
  static unsigned long lastMqttAt_ms = 0UL;
  const unsigned long mqttDelay_ms = 200UL;
  
  counts = analogRead ( paSensor );
  
  // could scale the value: max counts = 1024; Vdd = 5.0V; VddSensor = 3.3V
  // but no particular need to do so

  slow.record ( float ( counts ) );
  float difference = float ( counts ) - slow.value();
  mediumAbs.record ( fabs ( difference ) );
  fast.record ( difference );
  fastAbs.record ( fabs ( difference ) );
  
  #ifdef SEND_TO_MQTT
    float value;
    switch ( mode ) {
      case 0:
        value = constrain ( int ( mediumAbs.value() * 250.0 / float ( maxCounts ) ), 0, 100 );
        break;
      case 1:
      case 2:
        value = mediumAbs.value() * 250.0 / float ( maxCounts ) / 20.0;
        break;
    }
    
    // formatFloat ( fBuf, fBufLen, value, 2);
    dtostrf ( value, 4, 2, fBuf );  // 4 = width = *minimum* width...
    
    if ( ( millis() - lastMqttAt_ms ) > mqttDelay_ms ) {
     snprintf ( pBuf, pBufLen, "{  "
           " \"command\": \"send\", "
           " \"topic\": \"cbmalloch/tree/%s\", "
           " \"value\": %s "
           " }",
          itemStrings [ mode ],
          fBuf );
      Serial.println ( pBuf );
      lastMqttAt_ms = millis();
    }
  
  #else

    int dotAt = fast.value() * float ( lineLen ) / float ( maxCounts );
  
    // Serial.print ( counts ); Serial.print ( "    | " );
    Serial.print ( slow.value() - firstAnalogValue ); Serial.print ( "    | " );
    Serial.print ( fast.value() ); Serial.print ( "    | " );
    Serial.print ( fastAbs.value() ); Serial.print ( "    | " );
    // Serial.print ( long ( counts * lineLen ) ); Serial.print ( "    | " );
    // Serial.print ( long ( counts ) * lineLen ); Serial.print ( "    | " );
    // Serial.print ( dotAt ); Serial.print ( "    | " );
  
    for ( int i = 1; i < lineLen; i++ ) {
      Serial.print ( ( abs ( i - dotAt ) <= 1 ) ? "#" : " " );
    }
  
    Serial.println ();
  
    // Serial.print ( "|| " );  
    // Serial.print ( counts );
  #endif

}

