#define PROGNAME  "ESP_text_to_MQTT"
#define VERSION   "0.1.0"
#define VERDATE   "2018-02-27"

/*
  to be loaded onto ESP-01
  receive text commands to connect to network and then to send data to MQTT server
  
  all commands and responses will be JSON
  see <http://arduinojson.org>
    			
	>	{ "command": "connect",
		  "networkSSID": "blah_blah",
      "networkPW": "more blah",
      "MQTTuserID": "whoever",
      "MQTTuserPW": "whatever"
		}
	<	{ "connectionResult": "OK" }
	<	{ "connectionResult": "failed" }
		e.g. { "command": "connect", "networkSSID": "cbm_IoT_MQTT", "networkPW": "cbmLaunch",
           "MQTTuserID": "cbmalloch", "MQTTuserPW": "" }

  > { "command": "chipID" }
  < { "chipID_hex": <hex string> }

  > { "command": "retain",
      "value": 0|1
    }
    
  > { "command": "send",
      "topic": "<topic_string>",
      "value": <float>|<string>
    }
    
  > { "command": "subscribe",
      "topic": "<topic_string>",
      "QOS": 0|1|2
    }
    
  < { "topic": "<topic_string>",
      "value": <float>|<string>
    }
    
    
    
  ================================================
  
  structure:
    setup
    loop
      
      waits for input from serial-connected device
      calls handleSerialInput
        gathers serial input
        calls handleString
          may call
            networkConnect
      waits something from MQTT
      <will call something>
    
  
  
  0.0.1 2018-02-26 cbm cloned from backup_indicator_feed_to_Mosquitto
                       
*/

// Note: also including <Ethernet.h> broke WiFi.connect!
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


// ***************************************
// ***************************************

#define BAUDRATE       115200
#define VERBOSE            10

const unsigned long sendDataToCloudInterval_ms =  10UL * 1000UL;
const int maxSendings = 5;

// ***************************************
// ***************************************

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient conn_TCP;

// Set up the MQTT client by passing in the WiFi client and MQTT server and login details.
PubSubClient conn_MQTT ( conn_TCP );

//*************************

uint32_t chipID;

int mqtt_qos = 0;
int mqtt_retain = 0;
int nSendings = 0;

void MQTTconnect( void );
void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length );
bool handleSerialInput ( void );
bool handleString ( char strBuffer[] );  // broadcasts if there's a change
int sendIt ( const char * topic, char * value, const char * name );

const size_t ssidLen = 20;
char ssid [ ssidLen ];
const size_t passphraseLen = 20;
char passphrase [ passphraseLen ];
const size_t userIDLen = 20;
char userID [ userIDLen ];
const size_t userPWLen = 20;
char userPW [ userPWLen ];

const size_t pBufLen = 128;
char pBuf [ pBufLen ];
int strBufPtr = 0;

void setup ( void ) {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 10000 ) );
  
  Serial.print ( "\n\n" );
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
    
}

void loop () {
  
  static unsigned long lastSentToCloudAt_ms = 0;
  
  conn_MQTT.loop();
  delay ( 10 );  // fix some issues with WiFi stability
  
  if ( ! conn_TCP.connected() ) {
    handleSerialInput();
    return;
  }
  
  if ( ! conn_MQTT.connected() ) MQTTconnect ();
  yield();
 
  handleSerialInput();
  
  yield();
  delay ( 10 );
 
}

void MQTTconnect () {
  
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
    Serial.print ( "\n  WiFi reconnected as " ); 
    Serial.print ( WiFi.localIP() );
    Serial.print (" in ");
    Serial.print ( millis() - startTime_ms );
    Serial.println ( " ms" );    
  }
  
  startTime_ms = millis();
  printed = false;
  while ( ! conn_MQTT.connected ( ) ) {
    conn_MQTT.connect ( "" );  // wants client ID
    if ( VERBOSE > 4 && ! printed ) { 
      Serial.print ( "  Checking MQTT ( status: " );
      Serial.print ( conn_MQTT.state () );
      Serial.print ( ")..." );
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
    // conn_MQTT.subscribe ( MQTT_FEED );
  }
  // conn_MQTT.unsubscribe("/example");
  newConnection = false;
}

bool handleSerialInput() {

  // collect serial input and handle it when necessary
  const int strBufSize = 200;
  static char strBuf [ strBufSize ];
  static int strBufPtr = 0;
  
  // if no characters arrive for a while, reset the buffer
  static unsigned long lastCharArrivedAt_ms = 0;
  const unsigned long timeoutPeriod_ms = 200;
  
  if ( ( millis() - lastCharArrivedAt_ms ) > timeoutPeriod_ms ) {
    strBufPtr = 0;
  }
    
  
  while ( int len = Serial.available() ) {
    for ( int i = 0; i < len; i++ ) {
      
      if ( strBufPtr >= strBufSize - 1 ) {
        // -1 to leave room for null ( '\0' ) appendix
        Serial.println ( "BUFFER OVERRUN" );
        strBufPtr = 0;
        // flush Serial input buffer
        while ( Serial.available() ) Serial.read();
        return ( false );
      }
      
      strBuf [ strBufPtr ] = Serial.read();
      // LEDToggle();
      // Serial.println(buffer[strBufPtr], HEX);
      if ( strBufPtr > 5 && strBuf [ strBufPtr ] == '\r' ) {  // 0x0d
        // append terminating null
        strBuf [ ++strBufPtr ] = '\0';
        handleString ( strBuf );
        // clear the buffer
        strBufPtr = 0;
      } else if ( strBuf [ strBufPtr ] == '\n' ) {  // 0x0a
        // ignore LF
      } else {
        strBufPtr++;
      }  // handling linends
      
      yield ();
      
    }  // reading the input buffer
    lastCharArrivedAt_ms = millis();
  }  // Serial.available
  
  return ( true );
}

bool handleString ( char strBuffer[] ) {
  
  /* 
    Respond to JSON commands like
      { "command": "connect", "networkSSID": "cbm_IoT_MQTT", "networkPW": "secret", 
        "MQTTuserID": "cbmalloch", "MQTTuserPW": "" }
      { "command": "send", "topic": "cbmalloch/feeds/onoff", "value": 1 }
  */
  
  // is the string valid JSON
  
  if ( strlen ( strBuffer ) ) {
    // there's at least a partial string     
    Serial.print ( "about to parse: " ); Serial.println ( strBuffer );
  }
    
  // Memory pool for JSON object tree.
  //
  // Inside the brackets, 200 is the size of the pool in bytes.
  // Don't forget to change this value to match your JSON document.
  // Use arduinojson.org/assistant to compute the capacity.

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject ( strBuffer );

  // Test if parsing succeeds.
  if ( root.success () && root [ "command" ].success() ) {
    // handle whatever it was
    const char* cmd = root [ "command" ];
    if ( ! strcmp ( cmd, "connect" ) ) {
      int result = networkConnect ( 
              (const char *) root [ "networkSSID" ],
              (const char *) root [ "networkPW" ],
              (const char *) root [ "MQTTuserID" ],
              (const char *) root [ "MQTTuserPW" ] );
      if ( result == 1 ) {
        Serial.println ( "{ \"connectionResult\": \"OK\" }" );
        return ( true );
      } else {
        Serial.printf ( "{ \"connectionResult\": \"failed %d\" }\n", result );
        return ( false );
      }
      
    } else if (  ! strcmp ( cmd, "send" ) ) {
      sendIt ( (const char *) root [ "topic" ],
               (const char *) root [ "value" ] );
    } else if (  ! strcmp ( cmd, "subscribe" ) ) {
    
    
    
      conn_MQTT.subscribe ( root [ "topic" ] );
      conn_MQTT.setCallback ( handleReceivedMQTTMessage );

               
               
               
               
               
    } else if ( false ) {
    } else {
      Serial.print ( "JSON msg from Serial: bad command: " );
      Serial.println ( cmd );
    }
    return ( true );
  } else {
    Serial.println ( "Received string was not valid JSON" );
    return ( false );
  }
 
  yield ();
  return true;

}

int networkConnect ( const char ssid [], const char networkPW [], const char user [], const char userPW [] ) {

  if ( VERBOSE >= 2 ) {
    byte macAddress [ 6 ];
    WiFi.macAddress ( macAddress );
    Serial.print ( "MAC address: " );
    for ( int i = 0; i < 6; i++ ) {
      Serial.print ( macAddress [ i ], HEX ); 
      if ( i < 5 ) Serial.print ( ":" );
    }
    Serial.println ();
  }
  
  int status = -1000;
  // Connect to WiFi access point.
  Serial.print ("\nnetworkConnect: connecting to network '"); Serial.print ( ssid );
  if ( VERBOSE >= 8 ) {
    Serial.print ( "' with password '" );
    Serial.print ( networkPW );
    Serial.println ( "'" );
  }
  Serial.println ();
  
  yield ();
  
  /*
  
    0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
    1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
    3 : WL_CONNECTED after successful connection is established
    4 : WL_CONNECT_FAILED if password is incorrect
    6 : WL_DISCONNECTED if module is not configured in station mode

  */
  
  int nDots = 0;
  if ( 1 || WiFi.status() != WL_CONNECTED ) {
    WiFi.disconnect();
    WiFi.begin ( ssid, networkPW );
  }
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.print ( WiFi.status() );
    if ( ++nDots % 80 == 0 ) Serial.print ( "\n" );
    yield();
    delay ( 500 );  // implicitly yields but may not pet the nice doggy
    if ( nDots > 160 ) {
      Serial.print ( "\nTimeout!\n" );
      return ( -1001 );
    }
  }
    
  if ( VERBOSE >= 8 ) Serial.printf ( "\nConnected with RSS %d.\n", WiFi.RSSI() );

  conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  // assumption that the server is running on the gateway machine
  conn_MQTT.setServer ( WiFi.gatewayIP(), 1883 );
  MQTTconnect ();
  
  yield ();
  
  return 1;
}

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


// callback function name hardcoded in MQTTClient.h
void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length ) {
  memcpy ( pBuf, payload, length );
  pBuf [ length ] = '\0';
  
  if ( VERBOSE >= 8 ) {
    Serial.print ( "Got: " );
    Serial.print ( topic );
    Serial.print ( ": " );
    Serial.print ( pBuf );
    Serial.println ();
  }
  
  // Serialize this information
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["topic"] = topic;
  root["value"] = pBuf;
  
  char myBuf [ 200 ];
  
  root.printTo ( myBuf );
  
  if ( VERBOSE >= 2 ) {
    Serial.printf ( "Received MQTT msg: '%s'\n", myBuf );
  }
  
  // for connected computer:
  Serial.println ( myBuf );
  
}
