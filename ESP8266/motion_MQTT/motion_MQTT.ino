#define PROGNAME "motion_MQTT"
#define VERSION  "0.1.1"
#define VERDATE  "2018-04-06"

/*
    motion_MQTT.ino
    2018-04-06
    Charles B. Malloch, PhD

    Read analog signal from RCWL-0516 microwave motion sensor
    (which involved wiretapping into pin 12)
    
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
#include <WiFiClient.h>
#include <PubSubClient.h>
// see https://tttapa.github.io/ESP8266/Chap08%20-%20mDNS.html
// #include <ESP8266WebServer.h>
// ESP8266mDNS is multicast DNS - responds to <whatever>.local
// #include <ESP8266mDNS.h>
// ESP8266WiFiMulti looks at multiple access points and chooses the strongest
// #include <ESP8266WiFiMulti.h>


// ***************************************
// ***************************************

const int BAUDRATE         = 115200;
const int VERBOSE          = 10;
#define   HOME_WIFI_LOCALE ( 1 ? CBMDDWRT3 : CBMIoT )

// ---------------------------------------
// ---------------------------------------

const char * MQTT_FEED = "cbmalloch/tree";

// define ( to enable ) or undef
#undef DEBUG_ESP
#undef SERIAL_DEBUG 

// ***************************************
// ***************************************

// const int            pdLED0        =    4;  // GPIO2; n'existe pas
const int            pdLED         =    0;  // GPIO16
const int            paMotion      =   A0;
float                scaleFactor   =  0.0;

// ***************************************
// ***************************************

#ifdef cbmnetworkinfo_h
  int WIFI_LOCALE;
  cbmNetworkInfo Network;
#endif

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient conn_TCP;

bool atM5 = true;


// /************************* MQTT Setup *********************************/
// 

char * MQTT_SERVER = "192.168.5.1"; 
#define MQTT_SERVERPORT  1883
#ifdef cbmnetworkinfo_h
  #define MQTT_USERNAME    CBM_MQTT_USERNAME "Motion_MQTT"
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
int sendIt ( const char * topic, const char * value );
bool weAreAtM5 ();

void setup() {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 ) {
    // wait for serial port to open
    delay ( 20 );
  }
    
  pinMode ( pdLED, OUTPUT );

  /**************************** Connect to network ****************************/
  
  #ifdef cbmnetworkinfo_h
    if ( weAreAtM5 () ) {
      WIFI_LOCALE =  M5;
    } else {
      WIFI_LOCALE =  HOME_WIFI_LOCALE;
      if ( HOME_WIFI_LOCALE != CBMIoT ) MQTT_SERVER = "mosquitto";
      atM5 = false;
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
  
  if ( VERBOSE >= 20 ) WiFi.printDiag ( Serial );

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

  /************************** Report successful init **************************/  
  
  scaleFactor = atM5 ? 100.0 : 16;

  Serial.println();
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  
  if ( VERBOSE >= 15 ) conn_MQTT.publish ( MQTT_FEED, "elephant" );
  
}

void loop() {
  
  const unsigned long readInterval_ms = 500UL;
  static unsigned long lastReadAt_ms  = 0UL;
  
  static unsigned long lastBlinkAt_ms = 0UL;
  const unsigned long blinkDelay_ms   = 500UL;
          
  int counts;  
  const float alphaLong               = 0.999;
  const float alphaShort              = 0.96;
  static float mean                   = 6110.0; // empirically measured value
  static float current                = 611.0;  // empirically measured value
  const float countsHysteresis        = 10.0;
  

  // htmlServer.handleClient();  

  #ifdef ALLOW_OTA
    ArduinoOTA.handle();
  #endif

  
  conn_MQTT.loop();
  delay ( 10 );  // fix some issues with WiFi stability

  if ( !conn_MQTT.connected () ) connect ();
  
  counts = analogRead ( paMotion );
  mean = ( 1.0 - alphaLong ) * float ( counts ) + alphaLong * mean;
  // current = ( 1.0 - alphaShort ) * fabs ( counts - mean ) + alphaShort * current;
  current = ( 1.0 - alphaShort ) * ( counts - mean ) + alphaShort * current;
  float sensorValue = current;
  if ( ( millis() - lastReadAt_ms ) > readInterval_ms ) {
  
    if ( abs ( current ) >= countsHysteresis ) {
      const int bufLen = 12;
      char strBuf [ bufLen ];
      int dotPosition = int ( ( sensorValue / 100.0 + 0.5 ) * scaleFactor / 2.0 );
      snprintf ( strBuf, bufLen, "%d", dotPosition );
      sendIt ( MQTT_FEED, strBuf );
    }
    
    Serial.printf ( "counts: %d; sensorValue: %2.0f; mean: %2.2f \n", counts, sensorValue, mean );
    lastReadAt_ms = millis();
  }

  if ( ( millis() - lastBlinkAt_ms ) > blinkDelay_ms ) {
    digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    lastBlinkAt_ms = millis();
  }


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
  
  /*
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
  */

  return ( retVal );
}

//*************************

int sendIt ( const char * topic, const char * value ) {
  
  int length = strlen ( value );
  
  if ( length < 1 ) {
    if ( VERBOSE >= 10 ) {
      Serial.print ( "\nNot sending null payload: " );
      Serial.println ( topic );
    }
    return 0;
  }
  
  if ( VERBOSE >= 10 ) {
    Serial.print( "\nSending " );
    Serial.print ( topic );
    Serial.print ( ": " );
    Serial.print ( value );
    Serial.print ( " ... " );
  }
  
  bool success = conn_MQTT.publish ( topic, value );
  if ( VERBOSE >= 10 ) Serial.println ( success ? "OK!" : "Failed" );
  
  return success ? length : -1;
};




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
    // first arg is client ID; required to be present and unique
    conn_MQTT.connect ( "cbm_MQTT_Client_motion_MQTT", MQTT_USERNAME, MQTT_KEY );  
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
