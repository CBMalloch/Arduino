#define PROGNAME "ESP-01_MQTT_color_tree_TX"
#define VERSION  "1.1.6"
#define VERDATE  "2018-05-09"

/*
    ESP-01_MQTT_color_tree_TX.ino
    2018-03-24
    Charles B. Malloch, PhD

    Program to drive the ESP-01 RX output pins using MQTT
    to drive a 5V string of WS2812 3-color LEDs
    
    Target hardware: Adafruit ESP8266 Huzzah (at M5 or at Chuck's house) 
    WITHOUT an RCWL0516 microwave motion sensor connected at A0
    (receives its commands via MQTT)
    
    Interesting to use TX = GPIO1 after wresting it away from Serial
    
    With version 0.2, I'm starting to use OTA update
    Note: Error[1]: Begin Failed -> not enough room on device for new sketch 
      along with old one!
    This admittedly simple sketch takes 258787 byte, or 51% of a 
      512K-no-SPIFFS ESP-01...
    But the 25Q80 flash chip is 8Mbits or 1M bytes...
    
    With ALLOW_OTA, 258227 bytes; without, 243913
    
    Note: With auto-sensing, the device will connect (at home) to 
    cbm_IoT_MQTT, not to dd-wrt-03.
    The Arduino IDE needs to be on the IoT network in order 
    to push an update!
    
    OTA is currently working at my house, but I haven't tried it at M5. It 
    requires the developer to be co-located on a LAN (since the MQTT server doesn't
    pass through to the Internet). Thus you must be on the M5_IoT_MQTT network.
    
    
    With version 0.3, I've now replaced the standard color wheel with an
    HSV-space color spiral; the hues rotate through 360 degrees 5 times in 
    on large cycle; each 360 degrees gets 15% greater saturation
    
    ===========================================================================
    
    If you want to compile this program and you're not me, you need to change 
    <super secret password> to something that will work at M5
    == I refuse to put anything on my GitHub that contains any password ==
    
    2018-05-09 v1.1.0 cbm - operation with small tVelocity is jumpy.
      Substantially rewrite pixel functions using getPixelColor to move the
      pattern along by one pixel at each (calculated) clock tick; then the 
      creation of the pattern requires only the calculation of one pixel
      at a time.
      
*/


/* 
  For users other than Chuck, comment out the following #include.
  In the absence of the include, cbmnetworkinfo_h will remain undefined
  and this will eliminate Chuck-only aspects of the program
*/
#include <cbmNetworkInfo.h>

#define ALLOW_OTA
#ifdef ALLOW_OTA
  #include <ArduinoOTA.h>
#endif
#include <ESP8266WiFi.h>
// WiFiClient supports TCP connection
#include <WiFiClient.h>
// PubSubClient supports MQTT
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
// see https://tttapa.github.io/ESP8266/Chap08%20-%20mDNS.html
// ESP8266mDNS is multicast DNS - responds to <whatever>.local
#include <ESP8266mDNS.h>
// ESP8266WiFiMulti looks at multiple access points and chooses the strongest
// #include <ESP8266WiFiMulti.h>

#include <Adafruit_NeoPixel.h>

// ***************************************
// ***************************************

const int BAUDRATE = 115200;
const int VERBOSE = 10;
#define HOME_WIFI_LOCALE ( 1 ? CBMDDWRT3 : CBMIoT )

// higher velocities => smaller/faster pattern progress
// bigger scales => bigger/slower pattern progress
const float dxScale = 100.0;
const float dtScale = 100.0;

// ---------------------------------------
// ---------------------------------------

const char * MQTT_FEED = "cbmalloch/tree/#";

// define ( to enable ) or undef
#undef SERIAL_DEBUG 

// ***************************************
// ***************************************

/*********************************** GPIO *************************************/

// 0, 2, 4, 5, 12, 13, 14, 15, 16
// <https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/pinouts>

// ESP-01 GPIO pins: 0 = GPIO0; 1 = GPIO1 = TX; 2 = GPIO2; 3 = GPIO3 = RX
/*
  Blue onboard LED is inverse logic, connected as:
    ESP-01:          GPIO1 = TX
    Adafruit Huzzah: GPIO2
*/
const int pdConnecting    = 1;
#define INVERSE_LOGIC_ON    0
#define INVERSE_LOGIC_OFF   1

const int            pdWS2812      =   3;

const unsigned short nPixels       =  100;

/********************************* Network ************************************/

#ifdef cbmnetworkinfo_h
  int WIFI_LOCALE;
  cbmNetworkInfo Network;
#endif
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient conn_TCP;
ESP8266WebServer htmlServer ( 80 );
PubSubClient conn_MQTT ( conn_TCP );

/********************************** mDNS **************************************/

const int mdnsIdLen = 40;
char mdnsId [ mdnsIdLen ];

/*********************************** MQTT *************************************/

char * MQTT_SERVER = "192.168.5.1"; 
#define MQTT_SERVERPORT  1883
#ifdef cbmnetworkinfo_h
  #define MQTT_USERNAME    CBM_MQTT_USERNAME
  #define MQTT_KEY         CBM_MQTT_KEY
#else
  #define MQTT_USERNAME    "Random_M5_User"
  #define MQTT_KEY         "m5launch"
#endif

char mqtt_clientID [ 50 ] = "whatever";
// int mqtt_qos = 0;
// int mqtt_retain = 0;


/********************************** WS2812 ************************************/

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

/******************************* Global Vars **********************************/

const size_t pBufLen = 128;
char pBuf [ pBufLen ];
const int htmlMessageLen = 1024;
char htmlMessage [htmlMessageLen];

int colorDotLocation = -1;
unsigned long colorDotColors [] = { 0x080808, 0x202020, 0x208020, 0x202020, 0x080808 };
unsigned long lastPositionCommandAt_ms = 0UL;
unsigned long pointerDuration_ms = 5000UL;
unsigned long rainbowTick_ms = 20UL;
int progressionMode = 0;                  // hue, saturation | value
int progressionNonvaryingParameter = 25;  //   thus, value
// control speed of pattern progression in x and t
float tVelocity = 1.0;

long patternTickDuration_ms = dtScale / tVelocity;  // leave signed!
float xVelocity = 1.0;
float patternPixelDistance = xVelocity / dxScale;
unsigned long brightnessPct = 100;

char uniqueToken [ 9 ];

/************************** Function Prototypes *******************************/

void connect( void );
void htmlResponse_root ();
void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length );
int interpretNewCommandString ( char * theTopic, char * thePayload );
bool weAreAtM5 ();

// void newColor ( unsigned long theColor );
void showColors ();
void pointer ( int location );
uint32_t Wheel(byte WheelPos);
uint32_t HSVWheel ( int major, int minor );
uint32_t newHSVWheel ( int x, int t );
void setColor ( int n, unsigned long theColor );
unsigned long hsv_to_rgb ( float h, float s, float v );

// magic juju to return array size
// see http://forum.arduino.cc/index.php?topic=157398.0
template< typename T, size_t N > size_t ArraySize (T (&) [N]){ return N; }

/******************************************************************************/

void setup() {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 );
  
  // delay grabbing pins until we're done with Serial...
  // pinMode ( pdConnecting, OUTPUT );  
  // pinMode ( pdWS2812,  OUTPUT );
  
  /****************************** WiFi Setup **********************************/
  
  #ifdef cbmnetworkinfo_h
    if ( weAreAtM5 () ) {
      WIFI_LOCALE = M5;
      if ( VERBOSE >= 5 ) Serial.printf ( "At M5; WIFI_LOCALE = \"%d\"\n", WIFI_LOCALE );
    } else {
      WIFI_LOCALE =  HOME_WIFI_LOCALE;
      if ( VERBOSE >= 5 ) Serial.printf ( "Not at M5; WIFI_LOCALE = \"%d\"\n", WIFI_LOCALE );
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
    if ( VERBOSE >= 5 ) Serial.printf ( "No cbmNetworkInfo; WIFI_LOCALE = \"%d\"\n", WIFI_LOCALE );
  #endif
    
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay ( 500 );  // implicitly yields but may not pet the nice doggy
  }
  Serial.println();
  if ( VERBOSE >= 15 ) WiFi.printDiag ( Serial );

  if ( VERBOSE >= 15 ) {
    Serial.println ( "WiFi connected with IP address: " );
    Serial.println ( WiFi.localIP() );
  }

  /************************** Random Token Setup ******************************/
  
  // create a hopefully-unique string to identify this program instance
  // REQUIREMENT: must have initialized the network for chipName
  
  #ifdef cbmnetworkinfo_h
    // Chuck-only
    strncpy ( uniqueToken, Network.chipName, 9 );
  #else
    snprintf ( uniqueToken, 9, "%08x", ESP.getChipId() );
  #endif

  /****************************** MQTT Setup **********************************/

  snprintf ( mqtt_clientID, 50, "%s_%s", PROGNAME, uniqueToken );
  
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
    snprintf ( hostName, 50, "esp8266-%s", uniqueToken );
    ArduinoOTA.setHostname ( (const char *) hostName );

    // No authentication by default
    ArduinoOTA.setPassword ( (const char *) CBM_OTA_KEY );

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

  /**************************** HTML Server Setup *****************************/

  htmlServer.on ( "/", htmlResponse_root );
  // htmlServer.on ( "/form", htmlResponse_form );
  htmlServer.begin ();

  /**************************** mDNS Server Setup *****************************/

  snprintf ( mdnsId, mdnsIdLen, "%s_%s", PROGNAME, uniqueToken );
  if (!MDNS.begin ( mdnsId ) ) {             // Start the mDNS responder for <PROGNAME>.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.printf ( "mDNS responder '%s.local' started", mdnsId );
  
  /************************** Report successful init **************************/

  Serial.println();
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  
  if ( VERBOSE >= 15 ) conn_MQTT.publish ( MQTT_FEED, "elephant" );
  
  /******************************* GPIO Setup *********************************/
  
  #ifdef SERIAL_DEBUG
  
    Serial.println ( "Serial debug requires TX, which is the blue LED" );
    // pinMode ( pdConnecting, OUTPUT ); digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
    
  #else
  
  
    /********************* Prepare to reuse serial TX pin *********************/

    Serial.println ( "Stopping serial output" );
    delay ( 500 );
    Serial.end();
  
    // (finally) take control of desired GPIO pins
  
    pinMode ( pdConnecting, OUTPUT );  digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
    pinMode ( pdWS2812,  OUTPUT );
  
    if ( ! weAreAtM5 () ) brightnessPct = 32;  // strip.setBrightness ( 32 );
    for ( int i = 0; i < 10; i++ ) {
      digitalWrite ( pdConnecting, 1 - digitalRead ( pdConnecting ) );
      const unsigned long wh [] = { 0x000000, 0x100000, 0x101000, 0x001000, 0x001010, 0x000010 };
      for ( int j = 0; j < nPixels; j++ ) {
        strip.setPixelColor ( j, wh [ ( j + i ) % 6 ] );
      }
      strip.show();
      delay ( 200 );
    }
    digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
  #endif
}

void loop() {

  htmlServer.handleClient();  

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

// *****************************************************************************
// ********************************** WiFi *************************************
// *****************************************************************************

void connect () {
  
  unsigned long startTime_ms = millis();
  static bool newConnection = true;
  bool printed;
  
  digitalWrite ( pdConnecting, INVERSE_LOGIC_ON );
  
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
    conn_MQTT.connect ( mqtt_clientID, MQTT_USERNAME, MQTT_KEY );  // first arg is client ID; required
    if ( VERBOSE > 4 && ! printed ) { 
      // Serial.print ( "  MQTT client ID: \"" ); Serial.print ( mqttClientID ); Serial.println ( "\"" );
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
      Serial.printf ( "  and subscribed to %s as %s\n", MQTT_FEED, mqtt_clientID );
      Serial.println ( MQTT_FEED );
    }
  }
  // client.unsubscribe("/example");
  newConnection = false;
  
  digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );

}

// *****************************************************************************
// ********************************** MQTT *************************************
// *****************************************************************************

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

int interpretNewCommandString ( char * theTopic, char * thePayload ) {
  
  int retVal = -1;
  
  const char * goodPrefix = "cbmalloch/tree";
  // strncpy ( goodPrefix, MQTT_FEED, 25 );;
  // strncat ( goodPrefix, "/", 25 );
  size_t lenPrefix = strlen ( goodPrefix );
  
  Serial.print ( "goodPrefix: \"" ); Serial.print ( goodPrefix ); Serial.println ( "\"" );
  Serial.print ( "theTopic: \"" ); Serial.print ( theTopic ); Serial.println ( "\"" );
  
  if ( strncmp ( theTopic, goodPrefix, lenPrefix ) ) return ( -1 );
  char * theRest = theTopic + lenPrefix;
  
  /*
    Accepted items:
      cbmalloch/tree: ON, OFF - turns on or off the blue LED on the ESP-01
      cbmalloch/tree: 0xrrggbb or #rrggbb - sets first LED to that color ( for a few seconds )
      cbmalloch/tree: <integer> - sets "pointer" to the designated LED, for a few seconds
      
      cbmalloch/tree/mode:
        0 - x-t is hue, step through saturations; set will set value
        1 - x-t is hue, step through values; set will set saturation
        2 - x-t is saturation, step through hues; set will set value
        3 - x-t is saturation, step through values; set will set hue
        4 - x-t is value, step through hues; set will set saturation
        5 - x-t is value, step through saturations; set will set hue
      cbmalloch/tree/set: <integer> 
        hue [0,360) degrees
        saturation, value [0,100] percent
        
      cbmalloch/tree/xVelocity: spatial velocity of pattern
      cbmalloch/tree/tVelocity: temporal velocity of pattern
      
  */
  
  // interpret thePayload ( "ON", "OFF", 0xrrggbb, or number (pointer) )
  
  if ( ! strncmp ( theRest, "/mode", 6 ) ) {
    // set the progression mode
    progressionMode = atoi ( thePayload );
    #ifdef SERIAL_DEBUG
      Serial.print ( "Mode " ); Serial.println ( progressionMode );
    #endif
    pointer ( progressionMode );    
    lastPositionCommandAt_ms = millis() - 3000UL;
    retVal = 0;
  } else if ( ! strncmp ( theRest, "/set", 5 ) ) {
    // set the nonvarying parameter for the progression
    progressionNonvaryingParameter = atoi ( thePayload );
    #ifdef SERIAL_DEBUG
      Serial.print ( "Set " ); Serial.println ( progressionNonvaryingParameter );
    #endif
    retVal = 0;
  } else if ( ! strncmp ( theRest, "/xVelocity", 5 ) ) {
    // set the spatial velocity of  the pattern
    xVelocity = atof ( thePayload );
    #ifdef SERIAL_DEBUG
      Serial.print ( "xVelocity " ); Serial.println ( xVelocity );
    #endif
    patternPixelDistance = xVelocity / dxScale;
    retVal = 0;
  } else if ( ! strncmp ( theRest, "/tVelocity", 5 ) ) {
    // set the temporal velocity of  the pattern
    tVelocity = atof ( thePayload );
    #ifdef SERIAL_DEBUG
      Serial.print ( "tVelocity " ); Serial.println ( tVelocity );
    #endif
    // patternTickDuration_ms is inversely related to tVelocity
    patternTickDuration_ms = dtScale / tVelocity;
    retVal = 0;
  } else if ( strlen ( theRest ) > 0 ) {
    // any other "theRest" is invalid; don't test payloads
    retVal = -1;
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
    Serial.printf ( "Unknown message: \"%s\" (%d chars)\n", thePayload, strlen ( thePayload ) );
    retVal = -2;
  }

  return ( retVal );
}

// *****************************************************************************
// ********************************* WS2812 ************************************
// *****************************************************************************

double fMapToInterval01 ( double x ) {
  if ( x < 0.0 ) {
    x += (ceil) (x);  // workaround to avoid (incorrect) macro
  }
  return ( fmod ( x, 1.0 ) );
}

void showColors () {

  // xVelocity and tVelocity are globally the speed of the pattern,
  // measured on (scaled) pixels and milliseconds, respectively
  
  static bool patternIsStale = true;
  static float x0 = 0.0;
  
  unsigned long timeSincePositionCommand_ms = millis() - lastPositionCommandAt_ms;
  if ( timeSincePositionCommand_ms < pointerDuration_ms ) {
    patternIsStale = true;
    return;
  }
  
  static unsigned long lastTickAt_ms = 0UL;
  
  // patternTickDuration_ms is set in interpretNewCommandString to be 
  // 100 / tVelocity
  
  if ( ( millis() - lastTickAt_ms ) < abs ( patternTickDuration_ms ) ) return;
    
  if ( patternIsStale ) {
    #ifdef SERIAL_DEBUG
      Serial.println ( "Stale" );
    #endif
    x0 = 0.0;
    for ( int i = 0; i < nPixels; i++ ) {
      x0 -= patternPixelDistance;
      strip.setPixelColor ( i, newHSVWheel ( x0 ) );
    }
    patternIsStale = false;
  } else {
  
    // tick the pattern; begin by moving everybody up one pixel
    
    if ( patternTickDuration_ms > 0 ) {
      // time is flowing forwards; loop backwards
      for ( int i = nPixels - 1; i >= 1; i-- ) {
        #if defined ( SERIAL_DEBUG ) && VERBOSE >= 12
          Serial.printf ( " %d:%#08x", i, strip.getPixelColor ( i - 1 ) );
        #endif
        strip.setPixelColor ( i, strip.getPixelColor ( i - 1 ) );
      }
      x0 = fMapToInterval01 ( x0 - patternPixelDistance);
      strip.setPixelColor ( 0, newHSVWheel ( x0 ) );
    } else {
      // time is flowing backwards; loop forward
      for ( int i = 0; i <= nPixels - 2; i++ ) {
        #if defined ( SERIAL_DEBUG ) && VERBOSE >= 12
          Serial.printf ( " %d:%#08x", i, strip.getPixelColor ( i + 1 ) );
        #endif
        strip.setPixelColor ( i, strip.getPixelColor ( i + 1 ) );
      }
      x0 = fMapToInterval01 ( x0 + patternPixelDistance);
      strip.setPixelColor ( nPixels - 1, newHSVWheel ( x0 ) );
    }
  
    #if defined ( SERIAL_DEBUG ) && VERBOSE >= 12
      Serial.printf ( "  x0: %4.2f\n", x0 );
    #endif

  }
  
  strip.show();
  lastTickAt_ms = millis();
}

uint32_t newHSVWheel ( float z ) {
  /*
  Rotate through hues (0-360), saturations (0-1), and values (0-1)
  single-valued function of x - t, so that a later t moves the pattern
  to larger x
  
  large cycle goes through 5 small cycles
  each small cycle is once through the first dimension
  each successive small cycle increases the second dimension by 15%
  from 25% to 100% or by 60deg from 0deg to 300deg
  */

  // the large cycle pattern consists of smallCyclesInLargeCycle small cycles
  const int smallCyclesInLargeCycle = 5;
  
  // const int smallCycleSteps = smallCyclesInLargeCycle - 1;
  // const float cycleRatio = 1.0 / float ( smallCyclesInLargeCycle );
  // const float msInSmallCycle = 1000.0;
  // // small cycle should tick this fast
  // // units value / time;  1/smallCyclesInLargeCycle  /  msInSmallCycle
  // //  1 * 1/6 / ( 1000 / 20 ) -> 1/300 or so
  // const float smallCycleTimeScale = 1.0 * cycleRatio / ( msInSmallCycle / float ( rainbowTick_ms ) );// / 10.0;
  // 
  // // small cycle should be this many pixels
  // const float smallCyclePixelScale = 1.0 / 100.0 / smallCyclesInLargeCycle;
  
  // small cycle should go 360 deg
  const float hue_int = 0.0;
  const float hue_slope = 360.0;
  const float saturation_int = 0.25;
  const float saturation_slope = 1.0 - saturation_int;
  const float value_int = 0.25;
  const float value_slope = 1.0 - value_int;
  
  // // we see 35 or 36 ms change in t between minor-cycle calls
  // 
  // // constrain to [ 0, 1 )
  // // first term [0, 100) * 0.01; second term could be big - t changes by order of 35
  // // want second term to be on order of 1e-3
  // float z = fmod ( float ( x ) * smallCyclePixelScale - float ( t ) * smallCycleTimeScale, 1.0 );
  
  if ( z < 0 ) z += 1.0;
  
  float hue, saturation, value;
  
  // f ( z ) = ( r, theta ) is to be a spiral in HSV space
  switch ( progressionMode ) {
    case 0:
      // hue, saturation | value
      hue = fmod ( z * hue_slope * smallCyclesInLargeCycle + hue_int, 360.0 );
      saturation = fmod ( z * saturation_slope + saturation_int, 1.0 );
      value = progressionNonvaryingParameter / 100.0 * brightnessPct / 100.0;
      break;
    case 1:
      // hue, value | saturation 
      hue = fmod ( z * hue_slope * smallCyclesInLargeCycle + hue_int, 360.0 );
      value = fmod ( z * value_slope + value_int, 1.0 ) * brightnessPct / 100.0;
      saturation = progressionNonvaryingParameter / 100.0;
      break;
    case 2:
      // saturation, hue | value 
      saturation = fmod ( z * saturation_slope * smallCyclesInLargeCycle + saturation_int, 1.0 );
      hue = fmod ( z * hue_slope + hue_int, 360.0 );
      value = progressionNonvaryingParameter / 100.0 * brightnessPct / 100.0;
      break;
    case 3:
      // saturation, value | hue
      saturation = fmod ( z * saturation_slope * smallCyclesInLargeCycle + saturation_int, 1.0 );
      value = fmod ( z * value_slope + value_int, 1.0 ) * brightnessPct / 100.0;
      hue = progressionNonvaryingParameter * 360.0 / 100.0;
      break;
    case 4:
      // value, hue | saturation
      value = fmod ( z * value_slope * smallCyclesInLargeCycle + value_int, 1.0 ) * brightnessPct / 100.0;
      hue = fmod ( z * hue_slope + hue_int, 360.0 );
      saturation = progressionNonvaryingParameter / 100.0;
      break;
    case 5:
      // value, saturation | hue
      value = fmod ( z * value_slope * smallCyclesInLargeCycle + value_int, 1.0 ) * brightnessPct / 100.0;
      saturation = fmod ( z * saturation_slope + saturation_int, 1.0 );
      hue = progressionNonvaryingParameter * 360.0 / 100.0;
      break;
    default:
      break;
  }
  
  #ifdef SERIAL_DEBUG
    if ( VERBOSE >= 12 ) {
      Serial.printf ( "z: %5.3f; progMode: %1d; hsv: ( %5.1f, %5.3f, %5.3f )  : ", 
        z, progressionMode, hue, saturation, value );
      if ( VERBOSE < 20 ) Serial.println ();
    }
  #endif
    
  return ( hsv_to_rgb ( hue, saturation, value ) );
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

/* 
  adapted from <https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both/>
  math adapted from: http://www.rapidtables.com/convert/color/rgb-to-hsl.htm     
  reasonably optimized for speed, without going crazy 
*/

// void hsv_to_rgb ( float h, float s, float v, int *r_r, int *r_g, int *r_b ) {
unsigned long hsv_to_rgb ( float h, float s, float v ) {
  // 0-359, 0-1, 0-1
  h = fmod (h, 360.0);
  s = constrain (s, 0.0, 1.0);
  v = constrain (v, 0.0, 1.0);
  
  // HSL note: V = 1 - | 2*L - 1 |
  float c = v * s;
  float x = c * (1 - fabsf (fmod ((h / 60.0), 2) - 1));
  float m = v - c;
  
  float rp, gp, bp;

  switch ( int ( h / 60 ) ) {
    case 0:
      rp = c;
      gp = x;
      bp = 0;
    break;

    case 1:
      rp = x;
      gp = c;
      bp = 0;
    break;

    case 2:
      rp = 0;
      gp = c;
      bp = x;
    break;

    case 3:
      rp = 0;
      gp = x;
      bp = c;
    break;

    case 4:
      rp = x;
      gp = 0;
      bp = c;
    break;

    default: // case 5:
      rp = c;
      gp = 0;
      bp = x;
    break;
  }

  int r = int ( ( rp + m ) * 256.0 ) & 255;
  int g = int ( ( gp + m ) * 256.0 ) & 255;
  int b = int ( ( bp + m ) * 256.0 ) & 255;
  
  // *r_g = (gp + m) * 255;
  // *r_b = (bp + m) * 255;

  #ifdef SERIAL_DEBUG
    if ( VERBOSE >= 20 ) Serial.printf ( 
        "hsv = %5.1f, %5.3f, %4.2f ( via %5.3f, %5.3f, %5.3f with m = %5.3f )"
        " ---> rgb = %d, %d, %d\n", 
      h, s, v, 
      rp, gp, bp,  m,
      r, g, b);
  #endif
  
  return ( ( r << 16 ) + ( g << 8 ) + ( b << 0 ) );
}

// *****************************************************************************
// ********************************** HTML *************************************
// *****************************************************************************

void htmlResponse_root () {

  htmlMessage [ 0 ] = '\0';

  strncat ( htmlMessage, "<!DOCTYPE html>\n", htmlMessageLen );
  strncat ( htmlMessage, "  <html>\n", htmlMessageLen );
  strncat ( htmlMessage, "    <head>\n", htmlMessageLen );
  strncat ( htmlMessage, "      <h1>Status: Color Tree</h1>\n", htmlMessageLen );
  snprintf ( pBuf, pBufLen, "      <h2>    %s v%s %s cbm</h2>\n",
                PROGNAME, VERSION, VERDATE );
  strncat ( htmlMessage, pBuf, htmlMessageLen );
  IPAddress ip = WiFi.localIP();
  snprintf ( pBuf, pBufLen, "      <h3>    Running on %s at %u.%u.%u.%u / %s.local</h3>\n",
                uniqueToken, ip[0], ip[1], ip[2], ip[3], PROGNAME );
  strncat ( htmlMessage, pBuf, htmlMessageLen );
  strncat ( htmlMessage, "    </head>\n", htmlMessageLen );
  strncat ( htmlMessage, "    <body>\n", htmlMessageLen );
  
  /*
                  <p>Uptime: %02d:%02d:%02d</p>\
                  <form action='http://%s/form' method='post'>\
                    Network: <input type='text' name='nw'><br>\
                    <input type='submit' value='Submit'>\
                  </form>\
*/
  
  if ( ( millis() - lastPositionCommandAt_ms ) < pointerDuration_ms ) {
    snprintf ( pBuf, pBufLen, "Indicator dot at pixel %d<br>\n",
                colorDotLocation );
  } else {

    switch ( progressionMode ) {
      case 0:
        snprintf ( pBuf, pBufLen, "%s<br>\n",
                    "0 - x-t is hue, step through saturations; set will set value" );
        break;
      case 1:
        snprintf ( pBuf, pBufLen, "%s<br>\n",
                    "1 - x-t is hue, step through values; set will set saturation" );
        break;
      case 2:
        snprintf ( pBuf, pBufLen, "%s<br>\n",
                    "2 - x-t is saturation, step through hues; set will set value" );
        break;
      case 3:
        snprintf ( pBuf, pBufLen, "%s<br>\n",
                    "3 - x-t is saturation, step through values; set will set hue" );
        break;
      case 4:
        snprintf ( pBuf, pBufLen, "%s<br>\n",
                    "4 - x-t is value, step through hues; set will set saturation" );
        break;
      case 5:
        snprintf ( pBuf, pBufLen, "%s<br>\n",
                    "5 - x-t is value, step through saturations; set will set hue" );
        break;
      default:
        snprintf ( pBuf, pBufLen, "Unexpected progression mode %d<br>\n",
                    progressionMode );
        
    }
  }
  strncat ( htmlMessage, pBuf, htmlMessageLen );
  
  snprintf ( pBuf, pBufLen, "<br>\nTime direction: %s<br>\n", 
    patternTickDuration_ms > 0 ? "forwards" : "backwards" );
  strncat ( htmlMessage, pBuf, htmlMessageLen );
  snprintf ( pBuf, pBufLen, "tVelocity: %4.2f; Tick duration (ms): %d<br>\n", 
    tVelocity, abs ( patternTickDuration_ms ) );
  strncat ( htmlMessage, pBuf, htmlMessageLen );
  snprintf ( pBuf, pBufLen, "xVelocity: %4.2f; Pixel distance: %4.2f<br>\n\n", 
    xVelocity, patternPixelDistance );
  strncat ( htmlMessage, pBuf, htmlMessageLen );
   
  strncat ( htmlMessage, "    </body>\n", htmlMessageLen );
  strncat ( htmlMessage, "  </html>\n", htmlMessageLen );

  htmlServer.send ( 200, "text/html", htmlMessage );
}

// void htmlResponse_form () {
// 
//   for ( int i = 0; i < htmlServer.headers(); i++ ) {
//     Serial.print ( "arg " ); Serial.print ( i ); Serial.print ( ": " ); Serial.println ( htmlServer.header ( i ) );
//   }
//       
//   for ( int i = 0; i < htmlServer.args(); i++ ) {
//     Serial.print ( "arg " ); Serial.print ( i ); Serial.print ( ": " ); Serial.println ( htmlServer.arg ( i ) );
//   }
//       
//   if ( ! htmlServer.hasArg ( "nw" ) ) {
//     htmlServer.send ( 200, "text/plain", "Body not received" );
//     return;
//   }
//   
//   Serial.println ( htmlServer.arg ( "nw" ) );
//   
//   String message = "Body received:\n";
//   message += htmlServer.arg("nw");
//   message += "\n";
// 
//   htmlServer.send ( 200, "text/plain", message );
//   Serial.println ( message );
// 
// }

// *****************************************************************************
// ********************************** util *************************************
// *****************************************************************************

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
