#define PROGNAME  "ESP_text_to_MQTT"
#define VERSION   "0.1.1"
#define VERDATE   "2018-02-28"

/*
  to be loaded onto ESP-01
  receive text commands to connect to network and then to send data to MQTT server
  NOTE: the ESP-01 is a 3.3V device, intolerant of 5V
  
  all commands and responses are JSON
  see <http://arduinojson.org>
    			
  implemented:
    >	{ "command": "connect",
        "networkSSID": "blah_blah",
        "networkPW": "more blah",
        "MQTThost": "<url_or_IP>",
        "MQTTuserID": "whoever",
        "MQTTuserPW": "whatever"
      }
    <	{ "connectionResult": "OK",
        "ip": "<assigned_ip_address>" }
    <	{ "connectionResult": "failed" }
      e.g. { "command": "connect", "networkSSID": "cbm_IoT_MQTT", "networkPW": "SECRET",
             "MQTTuserID": "cbmalloch", "MQTTuserPW": "" }

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
    
  pending:
  
    > { "command": "chipID" }
    < { "chipID_hex": <hex string> }

    > { "command": "retain",
        "value": 0|1
      }
    
    
    
  ================================================
  
  structure:
    setup
    loop
      
      accepts input from serial-connected device (host)
      calls handleSerialInput
        gathers serial input
        calls handleString
          may call
            networkConnect
      waits something from MQTT
        sends it on to host
    
  
 Plans:
    add a "tick" option
    add options for QOS, retained values


  0.0.1 2018-02-26 cbm cloned from backup_indicator_feed_to_Mosquitto
                       
*/

// Note: also including <Ethernet.h> broke WiFi.connect!
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


// ***************************************
// ***************************************

// #define BAUDRATE       115200
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

char mqtt_host [ 50 ] = "third_base";
char mqtt_clientID [ 50 ] = "whatever";
char mqtt_userID [ 50 ] = "nobody";
char mqtt_userPW [ 50 ] = "nothing";
int mqtt_qos = 0;
int mqtt_retain = 0;
int nSendings = 0;

int networkConnect ( const char ssid [], const char networkPW [] );
int MQTTconnect( void );
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

long BAUDRATE = 115200L;

void setup ( void ) {
  
  #if false
    // first feeble attempt at autobaud
  
    const int nBaudrates = 4;
    const long baudrates [ nBaudrates ] = { 115200, 57600, 19200, 9600 };
    while ( BAUDRATE == 0L ) {
      for ( int i = 0; i < nBaudrates; i++ ) {
        Serial.begin ( baudrates [ i ] );
        // delay ( 2 );
        if ( Serial.available () ) Serial.print ( Serial.peek() );
        if ( Serial.available () && Serial.read() == '.' ) {
          BAUDRATE = baudrates [ i ];
          break;
        }  // got a good character
        Serial.print ( i ); delay ( 300 );
      }  // 
    }
    Serial.print ( "Detected baud rate: " ); Serial.println ( BAUDRATE );
  #else
    Serial.begin ( BAUDRATE );
    while ( !Serial && ( millis() < 10000 ) );
  #endif
  
  
  Serial.print ( "\n\n" );
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  
  // generate random client ID value
  randomSeed ( analogRead(0) + analogRead(3) + millis() );
  snprintf ( mqtt_clientID, 50, "ESP_text_to_MQTT_%ld", random ( 2147483647L ) );

}

void loop () {
  
  static unsigned long lastSentToCloudAt_ms = 0;
  
  conn_MQTT.loop();
  delay ( 10 );  // fix some issues with WiFi stability
  
  if ( ! conn_TCP.connected() ) {
    handleSerialInput();
    return;
  }
  
  if ( ! conn_MQTT.connected() ) {
    if ( MQTTconnect () != 1 ) {
      return;
    }
  }
  yield();
 
  handleSerialInput();
  
  yield();
  delay ( 10 );
 
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
        strBuf [ 0 ] = '\0';
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
      strncpy ( mqtt_host, (const char *) root [ "MQTThost" ], 50 );
      conn_MQTT.setServer ( mqtt_host, 1883 );
      strncpy ( mqtt_userID, (const char *) root [ "MQTTuserID" ], 50 );
      strncpy ( mqtt_userPW, (const char *) root [ "MQTTuserPW" ], 50 );
      
      int result = networkConnect ( 
              (const char *) root [ "networkSSID" ],
              (const char *) root [ "networkPW" ] );
      if ( result == 1 ) {
        Serial.printf ( "{ \"connectionResult\": \"OK\", \"ssid\": \"%d.%d.%d.%d\" }\n",
                        WiFi.localIP() [ 0 ], WiFi.localIP() [ 1 ], WiFi.localIP() [ 2 ], WiFi.localIP() [ 3 ] );
          
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

int networkConnect ( const char ssid [], const char networkPW [] ) {

  /*
    Return codes:
        1 -> OK
    -1000 ->
    -1001 -> Network timeout
    -1002 -> No networks found
  */

  if ( VERBOSE >= 2 ) {
    Serial.print ( " Chip ID: " ); Serial.print ( ESP.getChipId () );
    Serial.print ( " ( 0x" ); Serial.print ( ESP.getChipId (), HEX );
    Serial.print ( " ); " ); // Serial.println ();
    
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
  Serial.println ( "Available networks:" );
  if ( ! listNetworks() ) return -1002;
  */
  
  /*
  
    0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
    1 : WL_NO_SSID_AVAIL in case configured SSID cannot be reached
    3 : WL_CONNECTED after successful connection is established
    4 : WL_CONNECT_FAILED if password is incorrect
    6 : WL_DISCONNECTED if module is not configured in station mode

  */
  
  int nDots = 0;
  if ( WiFi.status() != WL_CONNECTED ) {
    // WiFi.disconnect();
    // delay ( 1000 );
    WiFi.begin ( ssid, networkPW );
    #define DNS_IS_WORKING false
    #if DNS_IS_WORKING
      // we'd like to be able to use DNS
      // NOPE WiFi.setDNS ( "172.16.68.3", "9.9.9.9" );
      // maybe now 
      IPAddress dns1 = { 172, 16, 68, 3 };
      IPAddress dns2 = { 9, 9, 9, 9 };
      WiFi.config ( WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), dns1, dns2 );
    #endif
    delay ( 250 );
  }
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.print ( WiFi.status() );
    if ( ++nDots % 80 == 0 ) Serial.print ( "\n" );
    yield();
    delay ( 500 );  // implicitly yields but may not pet the nice doggy
    if ( nDots >= 160 ) {
      Serial.println ( "networkConnect: connection timeout!" );
      return ( -1001 );
    }
  }
    
  if ( VERBOSE >= 8 ) Serial.printf ( "\nConnected; RSS: %d.\n", WiFi.RSSI() );

  conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  conn_MQTT.disconnect ();
  int MQTT_ok = MQTTconnect ();
  if ( MQTT_ok != 1 ) {
    return MQTT_ok;
  }
  
  yield ();
  
  return 1;
}

int MQTTconnect () {

  /*
    Return values:
          1: OK
      -1011: WiFi reconnection timeout
      -1012: MQTT timeout
  */
  
  unsigned long startTime_ms = millis();
  static bool newConnection = true;
  bool printed;
  
  int nDots;
  
  printed = false;
  nDots = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    if ( VERBOSE > 4 && ! printed ) { 
      Serial.print ( "\nChecking wifi..." );
      printed = true;
      newConnection = true;
    }
    Serial.print ( "." );
    if ( ++nDots % 80 == 0 ) Serial.print ( "\n" );
    if ( nDots >= 25 ) {
      newConnection = true;
      Serial.println ( "MQTTconnect: network reconnection timeout!" );
      return -1011;
    }
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
  nDots = 0;
  while ( ! conn_MQTT.connected ( ) ) {
    conn_MQTT.connect ( mqtt_clientID, mqtt_userID, mqtt_userPW );
    if ( VERBOSE > 4 && ! printed ) { 
      Serial.print ( "\n  MQTTconnect: connecting to " ); Serial.print ( mqtt_host );
      Serial.print ( " as '"); Serial.print ( mqtt_clientID );
      if ( VERBOSE >= 8 ) {
        Serial.print ( "' user '" );
        Serial.print ( mqtt_userID );
        Serial.print ( "' / '" );
        Serial.print ( mqtt_userPW );
        Serial.println ( "'" );
      }
      Serial.println ();

      Serial.print ( "  Checking MQTT ( status: " );
      Serial.print ( conn_MQTT.state () );
      Serial.print ( " ) ..." );
      printed = true;
      newConnection = true;
    }
    Serial.print( "." );
    if ( ++nDots % 80 == 0 ) Serial.print ( "\n  " );
    if ( nDots >= 25 ) {
      newConnection = true;
      Serial.println ( "MQTTconnect: connection timeout!" );
      return -1012;
    }
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

int listNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println("Couldn't get a wifi connection");
    while (true);
  }

  // print the list of networks seen:
  Serial.print("Number of available networks: ");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    switch (WiFi.encryptionType(thisNet)) {
      case ENC_TYPE_WEP:
        Serial.println("WEP");
        break;
      case ENC_TYPE_TKIP:
        Serial.println("WPA");
        break;
      case ENC_TYPE_CCMP:
        Serial.println("WPA2");
        break;
      case ENC_TYPE_NONE:
        Serial.println("None");
        break;
      case ENC_TYPE_AUTO:
        Serial.println("Auto");
        break;
    }
  }
  return numSsid;
}
