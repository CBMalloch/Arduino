#define PROGNAME "ESP-01_M5_MQTT_switch"
#define VERSION  "0.2.0"
#define VERDATE  "2018-03-24"

/*
    ESP-01_M5_MQTT_switch.ino
    2018-02-24
    Charles B. Malloch, PhD

    Program to drive 2 ESP-01 output pins using MQTT.
    One pin drives a simple LED with 3.3V,
    and the other drives, via a 5LN01SP N-channel 1.5V MOSFET, 
      a 5V string of WS2812 3-color LEDs
    
*/


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#include <Adafruit_NeoPixel.h>

#include <cbmNetworkInfo.h>

// ***************************************
// ***************************************

const int BAUDRATE = 115200;
// #define atM5 true
// 
// // ---------------------------------------
// // ---------------------------------------
// 
// // need to use CBMDDWRT3 for my own network access
// // CBMDATACOL for other use
// // can use CBMM5 or CBMDDWRT3GUEST for Sparkfun etc.
// 
// 
// #if atM5
//   #define WIFI_LOCALE M5
// #else
//   #define WIFI_LOCALE CBMIoT
// #endif
int WIFI_LOCALE;

// ---------------------------------------
// ---------------------------------------

const int VERBOSE = 10;

const char * MQTT_FEED = "cbmalloch/feeds/onoff";

// define ( to enable ) or undef
#undef DEBUG_ESP

// ***************************************
// ***************************************

const int            pdControl     =   0;
const int            pdWS2812      =   2;   // is GPIO2
const unsigned short nPixels       =   1;

// ***************************************
// ***************************************

cbmNetworkInfo Network;

#define WLAN_PASS       password
#define WLAN_SSID       ssid

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient conn_TCP;

// /************************* MQTT Setup *********************************/
// 
// #if atM5
//   #define MQTT_SERVER      "192.168.5.1"
//   // "M5_IoT_MQTT"
// #else
//   #define MQTT_SERVER      "192.168.5.1"
// #endif
// #define MQTT_SERVERPORT  1883
// // 14714
// #define MQTT_USERNAME    CBM_MQTT_USERNAME
// // cmq_username
// #define MQTT_KEY         CBM_MQTT_KEY
// // cmq_key

#define MQTT_SERVER      "192.168.5.1"
#define MQTT_SERVERPORT  1883
#define MQTT_USERNAME    CBM_MQTT_USERNAME
#define MQTT_KEY         CBM_MQTT_KEY

PubSubClient conn_MQTT ( conn_TCP );

void connect( void );
void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length );
int interpretNewCommandString ( char * theTopic, char * thePayload );
bool weAreAtM5 ();

/************************* WS2812 Setup *********************************/

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel ( nPixels, ( uint8_t ) pdWS2812, NEO_RGB + NEO_KHZ800 );

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

/*
  The Huzzah outputs 3.3V, which isn't enough to drive some WS2812 pixel strips,
  so I level-shift that to 5V using an enhancement-mode n-channel MOSFET whose 
  gate is tied to 3.3V, whose source is connected to the ESP output pin 
  ( obviating the 10K recommended pull-up to 3.3V used for bidirectional level
  shifting ), and whose drain is pulled up to 5V with a ( 10K recommended,
  I'm using 330R for speed ) pull-up and also sent as the drive to the WS2812 strip.
  I took the plan for this level-shifter from 
    http://husstechlabs.com/support/tutorials/bi-directional-level-shifter/
*/


void setup() {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 ) {
    // wait for serial port to open
    delay ( 20 );
  }
  
  pinMode ( pdControl, OUTPUT );
  pinMode ( pdWS2812,  OUTPUT );
  
  WIFI_LOCALE = weAreAtM5 () ? M5 : CBMIoT;
    
  // for security reasons, the network settings are stored in a private library
  Network.init ( WIFI_LOCALE );

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
  
  // interpret thePayload ( "ON", "OFF", 1, 0, or 0xrrggbb )
  
  if ( ! strncmp ( thePayload, "ON", 2 ) && ( strlen ( thePayload ) == 2 ) ) {
    digitalWrite ( pdControl, 1 );
    Serial.println ( "pdControl ON" );
    retVal = 1;
  } else if ( ! strncmp ( thePayload, "1", 1 ) && ( strlen ( thePayload ) == 1 ) ) {
    digitalWrite ( pdControl, 1 );
    Serial.println ( "pdControl 1" );
    retVal = 0;
  } else if ( ! strncmp ( thePayload, "OFF", 3 ) && ( strlen ( thePayload ) == 3 ) ) {
    digitalWrite ( pdControl, 0 );
    Serial.println ( "pdControl OFF" );
    retVal = 0;
  } else if ( ! strncmp ( thePayload, "0", 1 ) && ( strlen ( thePayload ) == 1 ) ) {
    digitalWrite ( pdControl, 0 );
    Serial.println ( "pdControl 0" );
    retVal = 0;
  } else if ( ! strncmp ( thePayload, "0x", 2 ) && ( strlen ( thePayload ) == 8 ) ) {
    // it's a hex string
    unsigned long theColor = strtoul ( thePayload + 2, NULL, 16 );  // it's hex, starting in char 2
    strip.setPixelColor ( 0, theColor );
    strip.show();
    Serial.print ( "strip 0 0x" ); Serial.println ( theColor, HEX );
    retVal = 0;
  } else if ( ! strncmp ( thePayload, "#", 1 ) && ( strlen ( thePayload ) == 7 ) ) {
    // it's a hex string
    unsigned long theColor = strtoul ( thePayload + 1, NULL, 16 );  // it's hex, starting in char 2
    strip.setPixelColor ( 0, theColor );
    strip.show();
    Serial.print ( "strip 0 0x" ); Serial.println ( theColor, HEX );
    retVal = 0;
  } else {
    Serial.print ( "Unknown message: " ); Serial.println ( thePayload );
    retVal = -2;
  }

  return ( retVal );
}

// ******************************************************************************
// ******************************************************************************

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
    conn_MQTT.connect ( "cbm_MQTT_Client_Arduino_M5_MQTT_switch", MQTT_USERNAME, MQTT_KEY );  // first arg is client ID; required
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

// ******************************************************************************

bool weAreAtM5 () {
  bool atM5 = false;
  const int VERBOSE = 2;
  // scan for nearby networks:
  if ( VERBOSE >= 5 ) Serial.println ( "** Scan Networks **" );
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println ( "No wifi connection" );
    return ( -1 );
  }

  if ( VERBOSE >= 5 ) {
    // print the list of networks seen:
    Serial.print ( "Number of available networks: " );
    Serial.println ( numSsid );
  }

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    if ( VERBOSE >= 5 ) {
      Serial.print ( thisNet );
      Serial.print ( ") " );
      Serial.print ( WiFi.SSID ( thisNet ) );
      Serial.print ( "\tSignal: " );
      Serial.print ( WiFi.RSSI ( thisNet ) );
      Serial.print ( " dBm" );
      Serial.print ( "\tEncryption: " );
      switch ( WiFi.encryptionType ( thisNet ) ) {
        case ENC_TYPE_WEP:
          Serial.println ( "WEP" );
          break;
        case ENC_TYPE_TKIP:
          Serial.println ( "WPA" );
          break;
        case ENC_TYPE_CCMP:
          Serial.println ( "WPA2" );
          break;
        case ENC_TYPE_NONE:
          Serial.println ( "None" );
          break;
        case ENC_TYPE_AUTO:
          Serial.println ( "Auto" );
          break;
        default:
          Serial.println ( "Unk" );
          break;
      }  // switch encryption type
    }  // VERBOSE print
    if ( WiFi.SSID ( thisNet ) == (String) "M5_IoT_MQTT" ) {
      atM5 = true;
    }
  }  // loop through networks
  
  // Serial.println ( "xyzzy" );
  // Serial.printf ( "->%s<-\n", "plugh" );
  if ( VERBOSE >= 2 ) Serial.printf ( "We are %sat M5" ), atM5 ? "" : "not ";
  return atM5;
}
