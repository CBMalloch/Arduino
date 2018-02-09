#define PROGNAME "ESP-01_MQTT_switch"
#define VERSION  "0.0.1"
#define VERDATE  "2017-03-26"

/*
    ESP-01_MQTT_switch.ino
    2017-03-26
    Charles B. Malloch, PhD

    Program to drive an ESP-01 output pin using MQTT
    
*/


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUDP.h>
#include <PubSubClient.h>

#include <cbmNetworkInfo.h>

// ***************************************
// ***************************************

const int BAUDRATE = 115200;
// need to use CBMDDWRT3 for my own network access
// can use CBMM5 or CBMDDWRT3GUEST for Sparkfun etc.
#define WIFI_LOCALE M5
const int VERBOSE = 10;

const char * MQTT_FEED = "cbmalloch/feeds/onoff";
const int pdControl = 0;

// define ( to enable ) or undef
#undef DEBUG_ESP

// ***************************************
// ***************************************

cbmNetworkInfo Network;

#define WLAN_PASS       password
#define WLAN_SSID       ssid

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient conn_TCP;

WiFiUDP conn_UDP;

/************************* MQTT Setup *********************************/

#define MQTT_SERVER      "192.168.5.1"
// "M5_IoT_host"
// "m12.cloudmqtt.com"
#define MQTT_SERVERPORT  1883
// 14714
#define MQTT_USERNAME    CBM_MQTT_USERNAME
// cmq_username
#define MQTT_KEY         CBM_MQTT_KEY
// cmq_key

PubSubClient conn_MQTT ( conn_TCP );

void connect( void );
void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length );
int interpretNewCommandString ( char * theTopic, char * thePayload );


//*************************

void setup() {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 ) {
    // wait for serial port to open
    delay ( 20 );
  }
  
  pinMode ( pdControl, OUTPUT );
  
  // for security reasons, the network settings are stored in a private library
  Network.init ( WIFI_LOCALE );

  if ( ( VERBOSE >= 10 ) || ( ! strncmp ( Network.chipName, "unknown", 12 ) ) ) {
    Network.describeESP ( Network.chipName, &Serial );
  }
  
  // Connect to WiFi access point.
  Serial.print ("\nConnecting to "); Serial.println ( Network.ssid );
  yield ();
  
  WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
  WiFi.begin ( Network.ssid, Network.password );

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);  // implicitly yields but may not pet the nice doggy
  }
  Serial.println();

  if ( VERBOSE >= 4 ) Serial.println ( "WiFi connected" );

  conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  conn_MQTT.setServer ( MQTT_SERVER, MQTT_SERVERPORT );
  connect ();

  yield ();

  Serial.println();
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  
  if ( VERBOSE >= 15 ) conn_MQTT.publish ( MQTT_FEED, "elephant" );

}

void loop() {
  
  conn_MQTT.loop();
  delay ( 10 );  // fix some issues with WiFi stability

  if ( !conn_MQTT.connected () ) connect ();

  // if ( millis() % 2000 < 10 ) Serial.print ( "." );
  yield ();
}

//*************************
//*************************

void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length ) {

  const size_t pBufLen = 128;
  char pBuf [ pBufLen ];
  if ( length < pBufLen ) {
    memcpy ( pBuf, payload, length );
    pBuf [ length ] = '\0';
  } else {
    Serial.print ( "Error! MQTT received message too long at " ); Serial.print ( length ); Serial.println ( " bytes!" );
    return;
  }
  
  if ( VERBOSE >= 8 ) {
    Serial.print ( "Got: " );
    Serial.print ( topic );
    Serial.print ( ": " );
    Serial.print ( pBuf );
    Serial.println ();
  }
        
  int retval = interpretNewCommandString ( topic, pBuf );
  if ( retval < 0 ) {
    Serial.print ( "  --> Bad return from iNCS: " ); Serial.println ( retval );
    return;
  }
  
  if ( VERBOSE >= 4 ) {
    Serial.print ( "  --> Return from iNCS: " ); Serial.println ( retval );
    return;
  }
  
}

//*************************

int interpretNewCommandString ( char * theTopic, char * thePayload ) {
  
  int retVal = -1;
  
  const char * goodPrefix = "cbmalloch/feeds/onoff";
  size_t lenPrefix = strlen ( goodPrefix );
  
  if ( strncmp ( theTopic, goodPrefix, lenPrefix ) ) return ( -1 );
  char * theRest = theTopic + lenPrefix;
  
  // thePayload is "ON" or "OFF"
  
  if ( ! strncmp ( thePayload, "ON", 2 ) ) {
    digitalWrite ( pdControl, 1 );
    retVal = 1;
  } else if ( ! strncmp ( thePayload, "OFF", 3 ) ) {
    digitalWrite ( pdControl, 0 );
    retVal = 0;
  }

  return ( retVal );
}

//*************************

void connect () {
  
  unsigned long startTime_ms = millis();
  static bool newConnection = true;
  bool printed;
  
  printed = false;
  while ( WiFi.status() != WL_CONNECTED ) {
    if ( VERBOSE > 4 && ! printed ) { 
      Serial.print ( "\nChecking wifi..." );
      printed = true;
      newConnection = true;
    }
    Serial.print ( "." );
    delay ( 1000 );
  }
  if ( newConnection && VERBOSE > 4 ) {
    Serial.print ( "\n  WiFi connected as " ); 
    Serial.print ( WiFi.localIP() );
    Serial.print (" in ");
    Serial.print ( millis() - startTime_ms );
    Serial.println ( " ms" );    
  }
  
  startTime_ms = millis();
  printed = false;
  while ( ! conn_MQTT.connected ( ) ) {
    conn_MQTT.connect ( "cbm_MQTT_Client_Arduino", MQTT_USERNAME, MQTT_KEY );  // first arg is client ID; required
    if ( VERBOSE > 4 && ! printed ) { 
      // Serial.print ( "  MQTT user: \"" ); Serial.print ( MQTT_USERNAME ); Serial.println ( "\"" );
      // Serial.print ( "  MQTT key: \"" ); Serial.print ( MQTT_KEY ); Serial.println ( "\"" );
      Serial.print ( "  Checking MQTT ( status: " );
      Serial.print ( conn_MQTT.state () );
      Serial.print ( " )..." );
      printed = true;
      newConnection = true;
    }
    Serial.print( "." );
    delay( 1000 );
  }
  
  if ( newConnection ) {
    if ( VERBOSE > 4 ) {
      Serial.print ("\n  MQTT connected in ");
      Serial.print ( millis() - startTime_ms );
      Serial.println ( " ms" );
    }
    conn_MQTT.subscribe ( MQTT_FEED );
  }
  // client.unsubscribe("/example");
  newConnection = false;
}

