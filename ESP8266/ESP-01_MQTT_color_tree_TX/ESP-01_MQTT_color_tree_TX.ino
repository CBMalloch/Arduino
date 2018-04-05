#define PROGNAME "ESP-01_MQTT_color_tree_TX"
#define VERSION  "0.2.0"
#define VERDATE  "2018-04-05"

/*
    ESP-01_MQTT_color_tree_TX.ino
    2018-03-24
    Charles B. Malloch, PhD

    Program to drive the ESP-01 RX output pins using MQTT
    to drive a 5V string of WS2812 3-color LEDs
    
    Interesting to use TX = GPIO1 after wresting it away from Serial
    
    With version 2, I'm starting to use OTA update
    Note: Error[1]: Begin Failed -> not enough room on device for new sketch 
      along with old one!
    This admittedly simple sketch takes 258787 byte, or 51% of a 
      512K-no-SPIFFS ESP-01...
    But the 25Q80 flash chip is 8Mbits or 1M bytes...
    
    With ALLOW_OTA, 258227 bytes; without, 243913
    
    Note: With auto-sensing, the device will connect (at home) to 
    cbm_IoT_MQTT, not to dd-wrt-03.
    This means that the Arduino IDE needs to be on the IoT network in order 
    to push an update...
    
    OTA is currently working at my house, but I haven't tried it at M5. It 
    requires the developer to be co-located on a LAN (since the MQTT server doesn't
    pass through to the Internet). Thus you must be on the M5_IoT_MQTT network.
    
    If you want to compile this program and you're not me, you don't have my
    network credentials "cbmNetworkInfo". So you need to change
      #define COMPILE_USING_CBMNETWORKINFO
    to
      #undef COMPILE_USING_CBMNETWORKINFO
    and to change <super secret password> to something that will work at M5
    == I refuse to put anything on my GitHub that contains any password ==
    
*/

#define COMPILE_USING_CBMNETWORKINFO
#ifdef COMPILE_USING_CBMNETWORKINFO
  #include <cbmNetworkInfo.h>
#endif

#undef ALLOW_OTA
#ifdef ALLOW_OTA
  #include <ArduinoOTA.h>
#endif
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
// see https://tttapa.github.io/ESP8266/Chap08%20-%20mDNS.html
// #include <ESP8266WebServer.h>
// ESP8266mDNS is multicast DNS - responds to <whatever>.local
// #include <ESP8266mDNS.h>
// ESP8266WiFiMulti looks at multiple access points and chooses the strongest
// #include <ESP8266WiFiMulti.h>

#include <Adafruit_NeoPixel.h>


// ***************************************
// ***************************************

const int BAUDRATE = 115200;
const int VERBOSE = 10;
#define HOME_WIFI_LOCALE ( 1 ? CBMDDWRT3 : CBMIoT )

// ---------------------------------------
// ---------------------------------------

const char * MQTT_FEED = "cbmalloch/tree";

// define ( to enable ) or undef
#undef DEBUG_ESP
#undef SERIAL_DEBUG 

// ***************************************
// ***************************************

// GPIO pins: 0 = GPIO0; 1 = GPIO1 = TX; 2 = GPIO2; 3 = GPIO3 = RX
const int            pdBlueLED     =   1;
const int            pdWS2812      =   3;

const unsigned short nPixels       =  100;

// ***************************************
// ***************************************

int colorDotLocation = -1;
unsigned long colorDotColors [] = { 0x080808, 0x202020, 0x208020, 0x202020, 0x080808 };
unsigned long lastPositionCommandAt_ms = 0UL;
unsigned long pointerDuration_ms = 5000UL;
unsigned long rainbowTick_ms = 20UL;

#ifdef COMPILE_USING_CBMNETWORKINFO
  int WIFI_LOCALE;
  cbmNetworkInfo Network;
#endif

#define WLAN_PASS       password
#define WLAN_SSID       ssid

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient conn_TCP;


// /************************* MQTT Setup *********************************/
// 

char * MQTT_SERVER = "192.168.5.1"; 
#define MQTT_SERVERPORT  1883
#ifdef COMPILE_USING_CBMNETWORKINFO
  int WIFI_LOCALE;
  cbmNetworkInfo Network;
  #define MQTT_USERNAME    CBM_MQTT_USERNAME
  #define MQTT_KEY         CBM_MQTT_KEY
#else
  #define MQTT_USERNAME    "Random_M5_User"
  #define MQTT_KEY         "m5launch"
#endif

PubSubClient conn_MQTT ( conn_TCP );

// ***************************************

// magic juju to return array size
// see http://forum.arduino.cc/index.php?topic=157398.0
template< typename T, size_t N > size_t ArraySize (T (&) [N]){ return N; }

void connect( void );
void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length );
int interpretNewCommandString ( char * theTopic, char * thePayload );
bool weAreAtM5 ();

// void newColor ( unsigned long theColor );
void showColors ();
void pointer ( int location );
uint32_t Wheel(byte WheelPos);
void setColor ( int n, unsigned long theColor );

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
  
  // delay grabbing pins until we're done with Serial...
  // pinMode ( pdBlueLED, OUTPUT );  
  // pinMode ( pdWS2812,  OUTPUT );
  
  /**************************** Connect to network ****************************/
  
  #ifdef COMPILE_USING_CBMNETWORKINFO
    if ( weAreAtM5 () ) {
      WIFI_LOCALE =  M5;
    } else {
      WIFI_LOCALE =  HOME_WIFI_LOCALE;
      if ( HOME_WIFI_LOCALE != CBMIoT ) MQTT_SERVER = "mosquitto";
    }
    // for security reasons, the network settings are stored in a private library
    Network.init ( WIFI_LOCALE );
  
    // Connect to WiFi access point.
    Serial.print ("\nConnecting to "); Serial.println ( Network.ssid );
    yield ();
  
    WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
    WiFi.begin ( Network.ssid, Network.password );
  #else
    WiFi.begin ( "M5_IoT_MQTT", <super-secret-password> );  
  #endif
    
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);  // implicitly yields but may not pet the nice doggy
  }
  Serial.println();
  if ( VERBOSE >= 15 ) WiFi.printDiag(Serial);

  if ( VERBOSE >= 15 ) {
    Serial.println ( "WiFi connected with IP address: " );
    Serial.println ( WiFi.localIP() );
  }

  /****************************** MQTT Setup **********************************/

  conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  conn_MQTT.setServer ( MQTT_SERVER, MQTT_SERVERPORT );
  if ( VERBOSE >= 5 ) {
    Serial.print ( "Connecting to MQTT at " );
    Serial.println ( MQTT_SERVER );
  }
  connect ();

  yield ();
  
  /***************************** OTA Update Setup *****************************/
  
  #ifdef ALLOW_OTA
  
    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    char hostName [ 50 ];
    snprintf ( hostName, 50, "esp8266-%s", Network.chipName );
    ArduinoOTA.setHostname ( (const char *) hostName );

    // No authentication by default
    ArduinoOTA.setPassword ( (const char *) "cbmLaunch" );

    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
  
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
  
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  
    ArduinoOTA.begin();
    Serial.println ( "ArduinoOTA running" );
    
  #endif

  /************************** Report successful init **************************/

  Serial.println();
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  
  if ( VERBOSE >= 15 ) conn_MQTT.publish ( MQTT_FEED, "elephant" );
  
  #ifndef SERIAL_DEBUG
    /********************** Prepare to reuse serial TX pin **********************/

    Serial.println ( "Stopping serial output" );
    delay ( 500 );
    Serial.end();
  
    // (finally) take control of desired GPIO pins
  
    pinMode ( pdBlueLED, OUTPUT );  
    pinMode ( pdWS2812,  OUTPUT );
  
    for ( int i = 0; i < 10; i++ ) {
      digitalWrite ( pdBlueLED, 1 - digitalRead ( pdBlueLED ) );
      delay ( 200 );
    }
  #endif
}

void loop() {

  // server.handleClient();
  #ifdef ALLOW_OTA
    ArduinoOTA.handle();
  #endif
  
  conn_MQTT.loop();
  delay ( 10 );  // fix some issues with WiFi stability

  if ( !conn_MQTT.connected () ) connect ();

  showColors ();
  
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
    #ifdef SERIAL_DEBUG
      Serial.print ( "Error! MQTT received message too long at " ); Serial.print ( length ); Serial.println ( " bytes!" );
    #endif
    return;
  }
  
  #ifdef SERIAL_DEBUG
    if ( VERBOSE >= 8 ) {
      Serial.print ( "Got: " );
      Serial.print ( topic );
      Serial.print ( ": " );
      Serial.print ( pBuf );
      Serial.println ();
    }
  #endif
        
  int retval = interpretNewCommandString ( topic, pBuf );
  if ( retval < 0 ) {
    #ifdef SERIAL_DEBUG
      Serial.print ( "  --> Bad return from iNCS: " ); Serial.println ( retval );
    #endif
    return;
  }
  
  if ( VERBOSE >= 4 ) {
    #ifdef SERIAL_DEBUG
      Serial.print ( "  --> Return from iNCS: " ); Serial.println ( retval );
    #endif
    return;
  }
  
}

//*************************

int interpretNewCommandString ( char * theTopic, char * thePayload ) {
  
  int retVal = -1;
  
  const char * goodPrefix = "cbmalloch/tree";
  // strncpy ( goodPrefix, MQTT_FEED, 25 );;
  // strncat ( goodPrefix, "/", 25 );
  size_t lenPrefix = strlen ( goodPrefix );
  
  Serial.print ( "goodPrefix: \"" ); Serial.print ( goodPrefix ); Serial.println ( "\"" );
  Serial.print ( "theTopic: \"" ); Serial.print ( theTopic ); Serial.println ( "\"" );
  
  if ( strncmp ( theTopic, goodPrefix, lenPrefix ) ) return ( -1 );
  // char * theRest = theTopic + lenPrefix;
  
  // interpret thePayload ( "ON", "OFF", 0xrrggbb, or number (pointer) )
  
  if ( ! strncmp ( thePayload, "ON", 2 ) && ( strlen ( thePayload ) == 2 ) ) {
    digitalWrite ( pdBlueLED, 1 );
    #ifdef SERIAL_DEBUG
      Serial.println ( "pdBlueLED ON" );
    #endif
    retVal = 1;
  } else if ( ! strncmp ( thePayload, "OFF", 3 ) && ( strlen ( thePayload ) == 3 ) ) {
    digitalWrite ( pdBlueLED, 0 );
    #ifdef SERIAL_DEBUG
      Serial.println ( "pdBlueLED OFF" );
    #endif
    retVal = 0;
  } else if ( ! strncmp ( thePayload, "0x", 2 ) && ( strlen ( thePayload ) == 8 ) ) {
    // it's a hex string
    unsigned long theColor = strtoul ( thePayload + 2, NULL, 16 );  // it's hex, starting in char 2
    setColor ( 0, theColor );
    retVal = 0;
  } else if ( ! strncmp ( thePayload, "#", 1 ) && ( strlen ( thePayload ) == 7 ) ) {
    // it's a hex string
    unsigned long theColor = strtoul ( thePayload + 1, NULL, 16 );  // it's hex, starting in char 2
    setColor ( 0, theColor );
    retVal = 0;
  } else if ( ( ! strncmp ( thePayload, "0", 2 ) ) || ( ( strlen ( thePayload ) >= 1 ) && atoi ( thePayload ) ) ) {
    // position a pointer at the given LED location
    colorDotLocation = atoi ( thePayload );
    #ifdef SERIAL_DEBUG
      Serial.print ( "Pointer at " ); Serial.println ( colorDotLocation );
    #endif
    pointer ( colorDotLocation );    
    lastPositionCommandAt_ms = millis();
    retVal = 0;
  } else {
    Serial.print ( "Unknown message: " ); Serial.println ( thePayload );
    retVal = -2;
  }

  return ( retVal );
}

void showColors () {
  const int nCycles = 1;
  unsigned long timeSincePositionCommand_ms = millis() - lastPositionCommandAt_ms;
  if ( timeSincePositionCommand_ms < pointerDuration_ms ) return;
  // offset rainbow by ticks
  int offset = timeSincePositionCommand_ms / rainbowTick_ms;
  for ( int i = 0; i < nPixels; i++ ) {
    byte wheelPos = ( i * nCycles + offset ) & 0x00ff;
    strip.setPixelColor ( i, Wheel ( wheelPos ) );
  }
  strip.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


void pointer ( int loc ) {
  if ( VERBOSE >= 10 ) {
    Serial.print ( "Dot at " ); Serial.println ( loc );
  }
  int sideBandWidth = ( ArraySize ( colorDotColors ) - 1 ) / 2;
  for ( int i = 0; i < nPixels; i++ ) {
    if ( ( i < loc - sideBandWidth ) || ( i > loc + sideBandWidth ) ) {
      strip.setPixelColor ( i, 0UL );
    } else {
      // within the dot
      int locWithinDot = ( loc - i ) + sideBandWidth;
      if ( VERBOSE >= 10 ) {
        Serial.print ( "  Within at " ); Serial.print ( locWithinDot );
        Serial.print ( ": 0x" ); Serial.println ( colorDotColors [ locWithinDot ], HEX );
      }
      strip.setPixelColor ( i, colorDotColors [ locWithinDot ] );
    }
  }
  strip.show();
}

void setColor ( int n, unsigned long theColor ) {
  strip.setPixelColor ( n, theColor );
  strip.show();
  #ifdef SERIAL_DEBUG
    Serial.print ( "strip 0 0x" ); Serial.println ( theColor, HEX );
  #endif
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
    conn_MQTT.connect ( "cbm_MQTT_Client_Arduino_M5_MQTT_color_tree_TX", MQTT_USERNAME, MQTT_KEY );  // first arg is client ID; required
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
    if (  VERBOSE > 4 ) {
      Serial.print ( "  and subscribed to " );
      Serial.println ( MQTT_FEED );
    }
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
  if ( VERBOSE >= 2 ) {
    Serial.print ( "We are " ); 
    Serial.print ( atM5 ? "" : "not " ); 
    Serial.println ( "at M5" );
  }
  return atM5;
}

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
