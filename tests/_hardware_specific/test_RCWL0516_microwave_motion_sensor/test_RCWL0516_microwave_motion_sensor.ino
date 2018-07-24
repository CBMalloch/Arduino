#define PROGNAME "test_RCWL0516_microwave_motion_sensor"
#define VERSION  "0.1.5"
#define VERDATE  "2018-07-24"

/*

      cbmalloch/tree          : mode for tree
      cbmalloch/tree/set      : third variable (HSV) for tree
      cbmalloch/tree/xVelocity: spatial velocity of pattern
      cbmalloch/tree/tVelocity: temporal velocity of pattern

*/

#include <ArduinoJson.h>

#include <cbmNetworkInfo.h>
#include <EWMA.h>

// ***************************************
// ***************************************

#define BAUDRATE      115200
#define VERBOSE           10

#define SEND_TO_MQTT

// ***************************************
// ***************************************

/********************************** GPIO **************************************/

#define paSensor  0
#define pdLED    13

/********************************* Network ************************************/

// The cbmNetworkInfo object has (at least) the following instance variables:
// .ip
// .gateway
// .mask
// .ssid
// .password
// .chipID    ( GUID assigned by manufacturer )
// .chipName  ( globally-unique name assigned by cbm and marked on PCB

cbmNetworkInfo Network;

// locales include M5, CBMIoT, CBMDATACOL, CBMDDWRT, CBMDDWRT3, CBMDDWRT3GUEST,
//   CBMBELKIN, CBM_RASPI_MOSQUITTO
#define WIFI_LOCALE CBMDATACOL

/******************************* Global Vars **********************************/

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

/************************** Function Prototypes *******************************/

#ifdef SEND_TO_MQTT
  void identifySelfToESP ();
  void sendNetworkCredentialsToESP ();
  void dispatchMQTTvalue ( const char * topic, float value, const char * name );
  void dispatchMQTTvalue ( const char * topic, int value, const char * name );
  void dispatchMQTTvalue ( const char * topic, char * value, const char * name );
  void initialize_MQTT_connection ();
#endif

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
  
  // /************************** Connect MQTT pass-thru **************************/
  
  // connection (at least for now) in loop, every 2 hours
  
  // #ifdef SEND_TO_MQTT
  //   initialize_MQTT_connection ();
  // #endif
  
  identifySelfToESP ();
  
  // connect the ESP to the network
  sendNetworkCredentialsToESP ();
    

  /************************** Report successful init **************************/

  Serial.println();
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
}

void loop() {
  
  int counts;
  
  static unsigned long lastMqttInitializationAt_ms = 0UL;
  const unsigned long mqttInitializationInterval_ms = 2UL * 60UL * 60UL * 1000UL;

  static unsigned long lastMqttSendAt_ms = 0UL;
  const unsigned long mqttSendInterval_ms = 500UL;
  const float hysteresis = 0.10;
  
  // #ifdef SEND_TO_MQTT
  //   if ( ( lastMqttInitializationAt_ms == 0UL )
  //        || ( ( millis() - lastMqttInitializationAt_ms ) > mqttInitializationInterval_ms ) ) {
  //     sendNetworkCredentialsToESP ();
  //     lastMqttInitializationAt_ms = millis();
  //     delay ( 5000 );
  //   }
  // #endif

  
  /*
    Note here that we're *not* metering the reading of the sensor nor the 
    rolling of the value into the EWMA calculations. We're currently only 
    rate-limiting the sending of values to the MQTT host.
  */
  
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
    static float oldValue = -1e12;
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
  
    if ( ( millis() - lastMqttSendAt_ms ) > mqttSendInterval_ms ) {
    
      if ( fabs ( value - oldValue ) > hysteresis ) {
    
        snprintf ( pBuf, pBufLen, "{ "
             " \"command\": \"send\", "
             " \"topic\": \"cbmalloch/tree/%s\", "
             " \"value\": %s "
             " }",
            itemStrings [ mode ],
            fBuf );
        Serial.println ( pBuf );
      
        oldValue = value;
        lastMqttSendAt_ms = millis();
      }
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

#ifdef SEND_TO_MQTT
    
  void identifySelfToESP () {

    StaticJsonBuffer<400> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject ();

    // { "command": "setAssignment", "assignment": <assignment>, "retain": true }
  
    root["command"]    = "setAssignment";
    root["assignment"] = PROGNAME " v" VERSION " " VERDATE " cbm";
    root["retain"]     = true;
  
    root.printTo ( Serial );
    Serial.println ();
  }

  void sendNetworkCredentialsToESP () {
  
    Network.setCredentials ( WIFI_LOCALE );
  
    /*
      CBM_MQTT_SERVER, CBM_MQTT_SERVERPORT, CBM_MQTT_USERNAME, CBM_MQTT_KEY are 
      globals set by cbmNetworkInfo.h
      now we also have .ssid and .password in the network object
    */
  
    StaticJsonBuffer<400> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject ();

    /*
      { "command": "connect", 
        "networkSSID": "cbmDataCollection", 
        "networkPW": "super_secret", 
        "MQTThost": "Mosquitto", 
        "MQTTuserID": "cbmalloch",
        "MQTTuserPW": "different_super_secret" }
    */
  
    root["command"]     = "connect";
    root["networkSSID"] = Network.ssid;
    root["networkPW"]   = Network.password;
    root["MQTThost"]    = CBM_MQTT_SERVER;
    root["MQTTuserID"]  = CBM_MQTT_USERNAME;
    root["MQTTuserPW"]  = CBM_MQTT_KEY;
  
    root.printTo ( Serial );
    Serial.println ();

  }

  void dispatchMQTTvalue ( const char * topic, float value, const char * name ) {
    char pBuf [ 12 ];
    dtostrf ( value, 3, 2, pBuf );
    dispatchMQTTvalue ( topic, (char *) &pBuf, name );
  }

  void dispatchMQTTvalue ( const char * topic, int value, const char * name ) {
    char pBuf [ 12 ];
    snprintf ( pBuf, 12, "%d", value );
    dispatchMQTTvalue ( topic, (char *) &pBuf, name );
  }

  void dispatchMQTTvalue ( const char * topic, char * value, const char * name ) {

    // Memory pool for JSON object tree.
    //
    // Inside the brackets, 200 is the size of the pool in bytes.
    // Don't forget to change this value to match your JSON document.
    // Use arduinojson.org/assistant to compute the capacity.

    StaticJsonBuffer<400> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject ();

    /*
  
      Send data via ESP-01 via JSON via MQTT to Mosquitto
    
      { "command": "send", 
        "topic": "cbmalloch/test", 
        "value": 12 }
    
    
    */
  
    int tLen = strlen ( topic );
    int vLen = strlen ( value );
  
    if ( VERBOSE >= 10 ) {
      Serial.print( "Sending " );
      Serial.print ( topic );
      Serial.print ( " ( name = " );
      Serial.print ( name );
      Serial.print ( ", len = " );
      Serial.print ( tLen + vLen );
      Serial.print ( " ): " );
      Serial.print ( value );
      Serial.print ( " ... " );
    }
  
    root["command"] = "send";
    root["topic"] = topic;
    root["value"] = value;
  
    root.printTo ( Serial );
    Serial.println ();
  
  };

#endif
