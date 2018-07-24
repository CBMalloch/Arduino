#define PROGNAME  "ESP_text_to_MQTT"
#define VERSION   "0.6.11"
#define VERDATE   "2018-07-24"

/*
  to be loaded onto ESP-01
  receive text commands to connect to network and then to send data to MQTT server
  NOTE: the ESP-01 is a 3.3V device, intolerant of 5V
  
  all commands and responses are JSON
  see <http://arduinojson.org>
    			
  implemented:
  
    > { "command": "setAssignment",
        "assignment": "Backup_indicator v4.0.1 2018-07-04 cbm"
        [, "retain": true ]
      }

    >	{ "command": "connect",
        "networkSSID": "blah_blah",
        "networkPW": "more blah",
        "MQTThost": "<url_or_IP>",
        "MQTTuserID": "whoever",
        "MQTTuserPW": "whatever"
      }
      e.g. { "command": "connect", "networkSSID": "cbm_IoT_MQTT", "networkPW": "SECRET",
             "MQTTuserID": "cbmalloch", "MQTTuserPW": "" }
             
    <	{ "connectionResult": "OK",
        "ip": "<assigned_ip_address>" }
    <	{ "connectionResult": "failed"
        "reason": "Timeout" }


    > { "command": "send",
        "topic": "<topic_string>",
        "value": <float>|<string>
        [, "retain": true ]
      }
      
    < { "sendResult": "OK" }
    < { "sendResult": "failed",
        "reason": "Packet too big" }
    < { "sendResult": "failed",
        "reason": "Empty payload" }
    < { "sendResult": "failed",
        "reason": "Unknown" }
  
    
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
    
      used to wait for a connect command
      now waits only if cannot connect with retained values
      
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
    
    
    
    No web interface at the moment
    
    Needs to retain interface data in case of a glitch???
    Or other program needs to get feedback to reinit


  0.0.1 2018-02-26 cbm cloned from backup_indicator_feed_to_Mosquitto
  0.3.0            cbm added conditional for hosting of html page
  0.4.0 2018-05-21 cbm added network reconnection into loop
  0.5.0 2018-07-02 cbm added retention of network parameters
  0.6.6 2018-07-15 cbm check for "connect" command replaced by check for 
                       comm parameters stored in EEPROM
  0.6.11 2018-07-24 cbm fixed static timing start in networkConnect;
                        had fixed timeouts, LED indicator activations;
                        found errors using OpticSpy!
                       
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

// Note: also including <Ethernet.h> broke WiFi.connect!
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
#include <WiFiUDP.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <cont.h>

#include <ArduinoJson.h>


// ***************************************
// ***************************************

// not a #define so we can (later) implement auto-detect baud rate...
// #define BAUDRATE       115200
unsigned long BAUDRATE = 115200UL;

const int  VERBOSE                    = 6;
const bool ALLOW_PRINTING_OF_PASSWORD = false;
// telemetry for the purpose of diagnosing network or MQTT connection issues is 
// less useful since we can't transmit the telemetry data unless we're connected!
// With telemetry, we found that available memory was not the problem... 
//   always over 35K free
const bool TELEMETRY_ON               = false;

const unsigned long sendDataToCloudInterval_ms =  10UL * 1000UL;
const int maxSendings = 5;

#define HOSTWEBPAGE

// ***************************************
// ***************************************



/********************************** GPIO **************************************/

/*
  Blue onboard LED is inverse logic, connected as:
    ESP-01:          GPIO1 = TX
      so use GPIO2 also (next to GND); it has a 1K pullup on my boards
    Adafruit Huzzah: GPIO2
*/

const int pdConnecting    = 2;
#define INVERSE_LOGIC_ON    0
#define INVERSE_LOGIC_OFF   1

/********************************* Network ************************************/

#ifdef cbmnetworkinfo_h
  int WIFI_LOCALE;
  cbmNetworkInfo Network;
#endif
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient conn_TCP;
const unsigned int port_UDP = 9250;
WiFiUDP conn_UDP;
// Set up the MQTT client by passing in the WiFi client and MQTT server and login details.
PubSubClient conn_MQTT ( conn_TCP );
#ifdef HOSTWEBPAGE
  ESP8266WebServer htmlServer ( 80 );
#endif

uint32_t chipID;
bool connection_initedP = false;

/********************************** mDNS **************************************/

const int mdnsIdLen = 40;
char mdnsId [ mdnsIdLen ];

/********************************* MQTT ***************************************/

// note that mqtt_clientID is independent of communications settings, so not stored in EEPROM
const size_t mqttClientIDLen = 50;
char mqtt_clientID [ mqttClientIDLen ] = "mqtt_clientID_not_initialized";

const size_t mqttHostLen = 50;
char mqtt_host [ mqttHostLen ] = "third_base";
const size_t mqttUserIDLen = 50;
char mqtt_userID [ mqttUserIDLen ] = "nobody";
const size_t mqttUserPWLen = 50;
char mqtt_userPW [ mqttUserPWLen ] = "nothing";
const int mqtt_serverPort = 1883;
// int mqtt_qos = 0;
// int mqtt_retain = 0;

/********************************** NTP ***************************************/

IPAddress timeServer ( 17, 253, 14, 253 );
const char* NTPServerName = "time.apple.com";

const int NTP_PACKET_SIZE = 48;   // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer [NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
  
const int timeZone = 0;   // GMT
// const int timeZone = -5;  // Eastern Standard Time (USA)
// const int timeZone = -4;  // Eastern Daylight Time (USA)
unsigned long lastNTPResponseAt_ms = millis();
// uint32_t time_unix_s               = 0;

/******************************* Global Vars **********************************/

const int assignmentStrLen = 64;
char assignment [assignmentStrLen] = "Unassigned";

int nSendings = 0;
const int lastMQTTtopicLen = 64;
char lastMQTTtopic [lastMQTTtopicLen] = "Topic not yet set";
const int lastMQTTmessageLen = 64;
char lastMQTTmessage [lastMQTTmessageLen] = "No message yet";

/*
  ... there may be retained values in EEPROM in the allowed 512-byte sector after SPIFFS
  0x00: 4 bytes "set\0" or something else
  0x04: network info: SSID, passphrase
    then MQTT client ID, MQTThost, then MQTT user ID, then MQTT PW
*/

const size_t networkSsidLen = 20;
char network_ssid [ networkSsidLen ];
const size_t networkPassphraseLen = 20;
char network_passphrase [ networkPassphraseLen ];

const size_t pBufLen = 128;
char pBuf [ pBufLen ];

const int htmlMessageLen = 1024;
char htmlMessage [htmlMessageLen];

const int timeStringLen = 32;
char timeString [ timeStringLen ];

char uniqueToken [ 9 ];

/************************** Function Prototypes *******************************/

int networkConnect ( const char network_ssid [], const char networkPW [] );
int MQTTconnect( void );
void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length );
bool handleSerialInput ( void );
bool handleString ( char strBuffer[] );  // broadcasts if there's a change
int sendMQTTvalue ( const char * topic, int value, const char * name, bool retainP = false );
int sendMQTTvalue ( const char * topic, const char * value, const char * name, bool retainP = false );
bool weAreAtM5 ();

int getEEPROMstring ( byte baseAdd, byte maxLen, char result [] );
void writeEEPROMstring ( byte baseAdd, char str [] );
bool persistentDataIsStoredToEEPROM_p ();
void storePersistentDataToEEPROM ();
bool getPersistentDataFromEEPROM ();

time_t getUnixTime();
void sendNTPpacket ( IPAddress& address );

#if TELEMETRY_ON
  int availableMemory();
#endif


/******************************************************************************/
/******************************************************************************/

void setup ( void ) {

  EEPROM.begin ( 512 );
  
  // GPIO2 is pdConnecting; it has a pullup on my boards
  pinMode ( pdConnecting, OUTPUT );
  
  #if false
    // first feeble attempt at autobaud
  
    const int nBaudrates = 4;
    const unsigned long baudrates [ nBaudrates ] = { 115200UL, 57600UL, 19200UL, 9600UL };
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
    while ( !Serial && ( millis() < 5000 ) );
  #endif
  
  Serial.println ();
  
  /****************************** Comm Setup **********************************/
  /* note WiFi Setup may have to be deferred until we have connection data from master
  /****************************************************************************/
    
  if ( getPersistentDataFromEEPROM () ) {
    // we think we have good connection data; is this network available?
    networkConnect ( network_ssid, network_passphrase );
    // all in good time...
    // MQTTconnect ();
  };
  
  if  ( ! connection_initedP ) 
    Serial.println ( "Waiting for connect string from master..." );
  while ( ! connection_initedP ) {
    handleSerialInput();
  
    yield();
    delay ( 10 );
  }

  /************************** Unique Token Setup ******************************/
  
  // create a hopefully-unique string to identify this program instance
  // REQUIREMENT: must have initialized the network for chipName
  
  Network.identifyESPchip ();
  
  #ifdef cbmnetworkinfo_h
    // Chuck-only
    strncpy ( uniqueToken, Network.chipName, 9 );
  #else
    snprintf ( uniqueToken, 9, "%08x", ESP.getChipId() );
  #endif
  
  snprintf ( mqtt_clientID, 50, "%s_%s", PROGNAME, uniqueToken );

  /******************************* UDP Setup **********************************/
  
  conn_UDP.begin ( port_UDP );
  
  if ( VERBOSE >= 6 ) Serial.println ( "UDP connected" );
  delay ( 50 );

 /****************************** MQTT Setup **********************************/
  
  conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  conn_MQTT.setServer ( mqtt_host, mqtt_serverPort );
  if ( VERBOSE >= 6 ) {
    Serial.print ( "Connecting to MQTT at " );
    Serial.println ( mqtt_host );
  }
  MQTTconnect ();

  yield ();
  
  /******************************* NTP Setup **********************************/
  
  setSyncProvider ( getUnixTime );
  setSyncInterval ( 300 );          // seconds
  
  while ( ( timeStatus() != timeSet ) && ( millis () < 30000UL ) ) {
    now ();
    Serial.print ( "t" );
    yield();
  }
  
  char topic [ 64 ];
  snprintf ( topic, 64, "%s/%s/time/startup", PROGNAME, uniqueToken );
  if ( timeStatus() == timeSet ) {
    unsigned long timeNow = now ();
    snprintf ( timeString, timeStringLen, "%04d-%02d-%02d %02d:%02d:%02dZ",
      year ( timeNow ), month ( timeNow ), day ( timeNow ), hour ( timeNow ), minute ( timeNow ), second ( timeNow ) );
    sendMQTTvalue ( topic, timeString, "Startup Time", true );
    Serial.printf ( "Starting at %s\n", timeString );
  } else {
    Serial.println ( "WARNING: No NTP response within 30 seconds..." );
    sendMQTTvalue ( topic, "WARNING no NTP within 30 seconds", "Startup Time", true );
  }
  yield();

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
      Serial.println("OTA Start");
    });
  
    ArduinoOTA.onEnd([]() {
      Serial.println("\nOTA End");
    });
  
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
           if (error == OTA_AUTH_ERROR)    Serial.println("OTA Auth Failed");
      else if (error == OTA_BEGIN_ERROR)   Serial.println("OTA Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("OTA Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA Receive Failed");
      else if (error == OTA_END_ERROR)     Serial.println("OTA End Failed");
    });
  
    ArduinoOTA.begin();
    Serial.println ( "ArduinoOTA running" );
    
  #endif

  /**************************** HTML Server Setup *****************************/

  #ifdef HOSTWEBPAGE
    htmlServer.on ( "/", htmlResponse_root );
    // htmlServer.on ( "/form", htmlResponse_form );
    htmlServer.begin ();
    Serial.println ( "Web page enabled" );
  #else
    Serial.println ( "Web page disabled" );
  #endif

  /**************************** mDNS Server Setup *****************************/

  snprintf ( mdnsId, mdnsIdLen, "%s_%s", PROGNAME, uniqueToken );
  if ( ! MDNS.begin ( mdnsId ) ) {             // Start the mDNS responder for <PROGNAME>.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.printf ( "mDNS responder '%s.local' started", mdnsId );
  
  /************************** Report successful init **************************/
   
  Serial.println();
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  
  #if TELEMETRY_ON
    {
      char topic [ 64 ];
      snprintf ( topic, 64, "%s/%s/telemetry/program_init", PROGNAME, uniqueToken );
      unsigned long timeNow = now ();
      snprintf ( timeString, timeStringLen, "%04d-%02d-%02d %02d:%02d:%02dZ",
        year ( timeNow ), month ( timeNow ), day ( timeNow ), hour ( timeNow ), minute ( timeNow ), second ( timeNow ) );
      sendMQTTvalue ( topic, timeString, "Telemetry program init", true );
    }
  #endif
  
  // turn on indicator that we're not yet connected
  digitalWrite ( pdConnecting, INVERSE_LOGIC_ON );
}

void loop () {
  
  static unsigned long lastSentToCloudAt_ms = 0;
  
  // just handle serial input until a good connect command is received
  /*
    conn_TCP_connected may be out of sync when connection parameters are used
    that came from EEPROM
    if ( ! conn_TCP.connected() ) is the wrong test, perhaps
  */
  
  // We won't reach loop until we've got an initial network connection established
  // which implies that we've got valid communication parameters
  
  // no sense continuing if we don't yet have communications parameters
  // if ( ! persistentDataIsStoredToEEPROM_p () ) return;
  
  // reconnect WiFi if necessary
  if ( WiFi.status() != WL_CONNECTED ) {
    if ( networkConnect ( network_ssid, network_passphrase ) <= 0 ) {
      return;
    }
  }
  
  // keep retrying to connect to MQTT until success
  if ( ! conn_MQTT.connected() ) {
    if ( MQTTconnect () != 1 ) {
      return;
    }
  }
  yield();
 
  conn_MQTT.loop();
  delay ( 10 );  // fix some issues with WiFi stability
  
  handleSerialInput();
  delay ( 10 );
 
  #ifdef HOSTWEBPAGE
    htmlServer.handleClient();  
  #endif
  
  #ifdef ALLOW_OTA
    ArduinoOTA.handle();
  #endif
  
  yield();
  
  #if TELEMETRY_ON
    static unsigned long lastWatchdogAt_ms = 0;
    const unsigned long watchdogInterval_ms = 30UL * 1000UL;
    const int topicLen = 50;
    char topic [ topicLen ];
    if ( ( millis() - lastWatchdogAt_ms ) > watchdogInterval_ms ) {
      snprintf ( topic, topicLen, "ESP_text_to_MQTT/%s/debug/%s", uniqueToken, "watchdog" );
      sendMQTTvalue ( topic, PROGNAME " v" VERSION " " VERDATE " cbm", "watchdog" );
      snprintf ( topic, topicLen, "ESP_text_to_MQTT/%s/debug/%s", uniqueToken, "available_memory" );
      sendMQTTvalue ( topic, availableMemory (), "available memory" );
      lastWatchdogAt_ms = millis();
    }
  #endif

  
}

// *****************************************************************************
// ********************************** WiFi *************************************
// *****************************************************************************

int networkConnect ( const char network_ssid [], const char networkPW [] ) {

  /*
  
    Tries for a couple minutes to connect to the network; eventually times out
    
    Return codes:
        1 -> OK
    -1000 -> 
    -1001 -> Network timeout
    -1002 -> No networks found
    -1003 -> unrecognized cbm network_ssid
  */
  
  unsigned long timerStartedAt_ms = millis();  // NOT static!
  const unsigned long timeoutPeriod_ms = 15UL * 1000UL;
  
  digitalWrite ( pdConnecting, INVERSE_LOGIC_ON );
  
  // won't ever run, since we're not network connected here...
  #if TELEMETRY_ON
    {
      char topic [ 64 ];
      snprintf ( topic, 64, "%s/%s/telemetry/network_connect/begin", PROGNAME, uniqueToken );
      unsigned long timeNow = now ();
      snprintf ( timeString, timeStringLen, "%04d-%02d-%02d %02d:%02d:%02dZ",
        year ( timeNow ), month ( timeNow ), day ( timeNow ), hour ( timeNow ), minute ( timeNow ), second ( timeNow ) );
      sendMQTTvalue ( topic, timeString, "Telemetry network connect begin", true );
    }
  #endif

  
  #ifdef cbmnetworkinfo_h
    // Chuck-only convenience functions for fixed IP assignment
    if ( ! strncmp ( network_ssid, "dd-wrt-03", 20 ) ) {
      Network.init ( CBMDDWRT3 );
    } else if ( ! strncmp ( network_ssid, "cbmDataCollection", 20 ) ) {
      Network.init ( CBMDATACOL );
    } else if ( ! strncmp ( network_ssid, "cbm_IoT_MQTT", 20 ) ) {
      Network.init ( CBMIoT );
    } else {
      Serial.print ( "Unrecognized network SSID: " ); 
      Serial.println ( network_ssid );
      return ( -1003 );
    }
    WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
    Serial.printf ( "Chuck-only in handleString: %u.%u.%u.%u gw %u.%u.%u.%u\n",
       Network.ip [ 0 ], Network.ip [ 1 ], Network.ip [ 2 ], Network.ip [ 3 ],
       Network.gw [ 0 ], Network.gw [ 1 ], Network.gw [ 2 ], Network.gw [ 3 ] );
    delay ( 20 );
  #else
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
  #endif
  
  connection_initedP = true;  // indicate we've got our communications params
  
  int status = -1000;
  // Connect to WiFi access point.
  Serial.print ("\nnetworkConnect: connecting to network '"); 
  Serial.print ( network_ssid );
  if ( ( VERBOSE >= 4 ) && ALLOW_PRINTING_OF_PASSWORD ) {
    Serial.print ( "' with password '" );
    Serial.print ( networkPW );
    Serial.println ( "'" );
  } else {
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
  
  if ( WiFi.status() != WL_CONNECTED ) {
  
    // so indicate
    delay ( 500 );
    digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
    delay ( 500 );
    digitalWrite ( pdConnecting, INVERSE_LOGIC_ON );

    // WiFi.disconnect();
    // delay ( 1000 );
    WiFi.begin ( network_ssid, networkPW );
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
  
  int nDots = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.print ( WiFi.status() );
    if ( ++nDots % 80 == 0 ) Serial.print ( "\n" );
    if ( ( millis() - timerStartedAt_ms ) >= timeoutPeriod_ms ) {
    
      // timed out!
      
      Serial.println ( "{ \"connectionResult\": \"failed\", \"reason\": \"Timeout\" }" );
      Serial.println ( "networkConnect: connection timeout!" );
      
      // won't ever run, since we're not network connected here...
      #if TELEMETRY_ON
        {
          char topic [ 64 ];
          snprintf ( topic, 64, "%s/%s/telemetry/network_connect/failure", PROGNAME, uniqueToken );
          sendMQTTvalue ( topic, WiFi.status(), "Telemetry network connect failure", true );
        }
      #endif

      digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
      return ( -1001 );
    }
    
    digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
    delay ( 100 );
    digitalWrite ( pdConnecting, INVERSE_LOGIC_ON );

    yield();
    delay ( 400 );  // implicitly yields but may not pet the nice doggy
    
  }
  Serial.println ();
    
  Serial.printf ( "{ \"connectionResult\": \"OK\", \"ssid\": \"%d.%d.%d.%d\" }\n",
                  WiFi.localIP() [ 0 ], WiFi.localIP() [ 1 ], WiFi.localIP() [ 2 ], WiFi.localIP() [ 3 ] );
  if ( VERBOSE >= 8 ) Serial.printf ( "\nConnected; RSS: %d.\n", WiFi.RSSI() );
  
  digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
  
  if ( strncmp ( mqtt_clientID, "mqtt_clientID_not_initialized", 30 ) ) {
  
    // MQTT *has* been initialized, so we have inited its connection parameters.
    // try to reconnect

    conn_MQTT.setCallback ( handleReceivedMQTTMessage );
    conn_MQTT.disconnect ();
    delay ( 200 );
    int MQTT_connection_result = MQTTconnect ();
    if ( MQTT_connection_result != 1 ) {
      // failure...
      return MQTT_connection_result;
    }
  
    // indicate NETWORK reconnection, but only if MQTT is also connected!
    
    #if TELEMETRY_ON
      {
        char topic [ 64 ];
        snprintf ( topic, 64, "%s/%s/telemetry/network_connect/success", PROGNAME, uniqueToken );
        unsigned long timeNow = now ();
        snprintf ( timeString, timeStringLen, "%04d-%02d-%02d %02d:%02d:%02dZ",
          year ( timeNow ), month ( timeNow ), day ( timeNow ), hour ( timeNow ), minute ( timeNow ), second ( timeNow ) );
        sendMQTTvalue ( topic, timeString, "Telemetry network connect success", true );
      }
    #endif

  }
  
  yield ();

  return 1;
}

// *****************************************************************************
// *********************************** MQTT ************************************
// *****************************************************************************

int MQTTconnect () {

  static unsigned long timerStartedAt_ms = millis();
  const unsigned long timeoutPeriod_ms = 10UL * 1000UL;

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

  // won't ever run, since we're not MQTT connected here...
  #if TELEMETRY_ON
    {
      char topic [ 64 ];
      snprintf ( topic, 64, "%s/%s/telemetry/MQTT_connect/begin", PROGNAME, uniqueToken );
      unsigned long timeNow = now ();
      snprintf ( timeString, timeStringLen, "%04d-%02d-%02d %02d:%02d:%02dZ",
        year ( timeNow ), month ( timeNow ), day ( timeNow ), hour ( timeNow ), minute ( timeNow ), second ( timeNow ) );
      sendMQTTvalue ( topic, timeString, "Telemetry MQTT connect begin", true );
    }
  #endif

  digitalWrite ( pdConnecting, INVERSE_LOGIC_ON );  
  delay ( 500 );
  digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
  delay ( 500 );
  digitalWrite ( pdConnecting, INVERSE_LOGIC_ON );
  delay ( 500 );
  digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
  delay ( 500 );
  digitalWrite ( pdConnecting, INVERSE_LOGIC_ON );
  
  /*
  
  
     Candidate for removal!!
     
     
  */
  
  /*
  
  
  printed = false;
  nDots = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    if ( VERBOSE >= 4 && ! printed ) { 
      Serial.print ( "\nChecking wifi..." );
      printed = true;
      newConnection = true;
    }
    Serial.print ( "." );
    if ( ++nDots % 80 == 0 ) Serial.print ( "\n" );
    if ( nDots >= 25 ) {
      newConnection = true;
      Serial.println ( "MQTTconnect: network reconnection timeout!" );
      
      // won't ever run, since we're not MQTT connected here...
      #if TELEMETRY_ON
        {
          char topic [ 64 ];
          snprintf ( topic, 64, "%s/%s/telemetry/MQTT_network_connect/failure", PROGNAME, uniqueToken );
          unsigned long timeNow = now ();
          snprintf ( timeString, timeStringLen, "%04d-%02d-%02d %02d:%02d:%02dZ",
            year ( timeNow ), month ( timeNow ), day ( timeNow ), hour ( timeNow ), minute ( timeNow ), second ( timeNow ) );
          sendMQTTvalue ( topic, timeString, "Telemetry MQTT network connect failure", true );
        }
      #endif

      return -1011;
    }
    delay ( 1000 );
  }
  if ( newConnection && VERBOSE >= 4 ) {
    Serial.print ( "\n  WiFi reconnected as " ); 
    Serial.print ( WiFi.localIP() );
    Serial.print (" in ");
    Serial.print ( millis() - startTime_ms );
    Serial.println ( " ms" );    
  }
  
  */
  
  startTime_ms = millis();
  printed = false;
  nDots = 0;
  while ( ! conn_MQTT.connected ( ) ) {
    conn_MQTT.connect ( mqtt_clientID, mqtt_userID, mqtt_userPW );
    if ( VERBOSE >= 4 && ! printed ) { 
      Serial.print ( "\n  MQTTconnect: connecting to " ); Serial.print ( mqtt_host );
      Serial.print ( " as '"); Serial.print ( mqtt_clientID );
      if ( ALLOW_PRINTING_OF_PASSWORD ) {
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

    if ( ( millis() - timerStartedAt_ms ) >= timeoutPeriod_ms ) {
      newConnection = true;
      Serial.println ( "MQTTconnect: connection timeout!" );
      // won't ever run, since we're not MQTT connected here...
      #if TELEMETRY_ON
        {
          char topic [ 64 ];
          snprintf ( topic, 64, "%s/%s/telemetry/MQTT_connect/failure", PROGNAME, uniqueToken );
          sendMQTTvalue ( topic, conn_MQTT.state (), "Telemetry MQTT connect failure", true );
        }
      #endif
      digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
      return -1012;
    }
    
    yield();
    delay( 500 );
  }
  
  if ( newConnection ) {
    if ( VERBOSE >= 4 ) {
      Serial.print ("\n  MQTT connected in ");
      Serial.print ( millis() - startTime_ms );
      Serial.println ( " ms" );
    }
    // conn_MQTT.subscribe ( MQTT_FEED );
  }
  // conn_MQTT.unsubscribe("/example");
  newConnection = false;
  
  #if TELEMETRY_ON
    {
      char topic [ 64 ];
      snprintf ( topic, 64, "%s/%s/telemetry/MQTT_connect/success", PROGNAME, uniqueToken );
      unsigned long timeNow = now ();
      snprintf ( timeString, timeStringLen, "%04d-%02d-%02d %02d:%02d:%02dZ",
        year ( timeNow ), month ( timeNow ), day ( timeNow ), hour ( timeNow ), minute ( timeNow ), second ( timeNow ) );
      sendMQTTvalue ( topic, timeString, "Telemetry MQTT connect success", true );
    }
  #endif

  
  digitalWrite ( pdConnecting, INVERSE_LOGIC_OFF );
  
  return 1;
}

// callback function name hardcoded in MQTTClient.h ??
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
  StaticJsonBuffer<400> jsonBuffer;
  
  JsonObject& root = jsonBuffer.createObject();
  root["topic"] = topic;
  root["value"] = pBuf;
  
  char myBuf [ 400 ];
  
  root.printTo ( myBuf );
  
  if ( VERBOSE >= 2 ) {
    Serial.printf ( "Received MQTT msg: '%s'\n", myBuf );
  }
  
  // for connected computer:
  Serial.println ( myBuf );
  
}

int sendMQTTvalue ( const char * topic, int value, const char * name, bool retainP ) {
  const int valLen = 10;
  char val [ valLen ];
  snprintf ( val, valLen, "%d", value );
  return sendMQTTvalue ( topic,  val,  name, retainP );
}

int sendMQTTvalue ( const char * topic, const char * value, const char * name, bool retainP ) {

  /*
  
    Send data via MQTT to Mosquitto
    
  */
  
  int tLen = strlen ( topic );
  int vLen = strlen ( (const char *) value );
  if ( vLen < 1 ) {
    if ( VERBOSE >= 10 ) {
      Serial.print ( "\nNot sending null payload " );
      Serial.println ( name );
    }
    Serial.println ( "{ \"sendResult\": \"failed\", \"reason\": \"Empty payload\" }" );
    return 0;
  }
  
  if ( ( tLen + vLen ) > MQTT_MAX_PACKET_SIZE ) {
    if ( VERBOSE >= 10 ) {
      Serial.print ( "\nNot sending oversized ( " );
      Serial.print ( tLen + vLen );
      Serial.print ( " > 128 ) packet " );
      Serial.println ( name );
    }
    Serial.println ( "{ \"sendResult\": \"failed\", \"reason\": \"Packet too big\" }" );
    return 0;
  }
  
  if ( VERBOSE >= 10 ) {
    Serial.print( "Sending " );
    Serial.print ( topic );
    Serial.print ( " ( name = " );
    Serial.print ( name );
    Serial.print ( ", len = " );
    Serial.print ( tLen + vLen );
    Serial.print ( " ): " );
    Serial.print ( (const char *) value );
    Serial.print ( " ... " );
  }
  
  /*
  
    Note: each publish call seems to take about a second
    Even though connection remains good
    But only when the MQTT server is overtaxed - probably in need of restart!
  
  */
  
  bool success = conn_MQTT.publish ( topic, value, retainP );
  if ( success ) {
    if ( VERBOSE >= 10 ) Serial.println ( "{ \"sendResult\": \"OK\" }" );
  } else {
    if ( VERBOSE >= 10 ) Serial.println ( 
      "{ \"sendResult\": \"failed\", \"reason\": \"Unknown\" }" );
  }
  
  lastMQTTtopic [ 0 ] = '\0';
  lastMQTTmessage [ 0 ] = '\0';
  if ( ! success ) strncat ( lastMQTTtopic, "<FAILURE>", lastMQTTtopicLen );
  strncat ( lastMQTTtopic, topic, lastMQTTtopicLen );
  strncat ( lastMQTTmessage, (const char *) value, lastMQTTmessageLen );
  
  #if 0
    static unsigned long nPackets = 0;
    nPackets++;
    const int valLen = 30;
    char val [ valLen ] = "\0";
    snprintf ( val, valLen, "%lu", nPackets );
    Serial.print ( "Value: " ); Serial.print ( val ); Serial.println ();
    conn_MQTT.publish ( "ESP_text_to_MQTT/debug/nPackets", val, true );
    nPackets++;
  #endif
  
  return success ? ( tLen + vLen ) : -1;
};

// *****************************************************************************
// *********************************** HTML ************************************
// *****************************************************************************

#ifdef HOSTWEBPAGE

  void htmlResponse_root () {

    htmlMessage [ 0 ] = '\0';

    strncat ( htmlMessage, "<!DOCTYPE html>\n", htmlMessageLen );
    strncat ( htmlMessage, "  <html>\n", htmlMessageLen );
    strncat ( htmlMessage, "    <head>\n", htmlMessageLen );
    strncat ( htmlMessage, "      <h1>Status:</h1>\n", htmlMessageLen );
    snprintf ( pBuf, pBufLen, "      <h2>    %s v%s %s cbm</h2>\n",
                  PROGNAME, VERSION, VERDATE );
    strncat ( htmlMessage, pBuf, htmlMessageLen );
    
    snprintf ( pBuf, pBufLen, "      <h2><p>    Assignment: %s</h2>\n",
                  assignment );
    strncat ( htmlMessage, pBuf, htmlMessageLen );
    
    IPAddress ip = WiFi.localIP();
    snprintf ( pBuf, pBufLen, "      <h3>    Running as %s at %u.%u.%u.%u / %s.local</h3>\n",
                  uniqueToken, ip[0], ip[1], ip[2], ip[3], mdnsId );
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
    
    
  
    strncat ( htmlMessage, "      <p>Last MQTT message was:<br>\n", htmlMessageLen );
  
    snprintf ( pBuf, pBufLen, "<pre>        topic: %s<br>\n", lastMQTTtopic );
    strncat ( htmlMessage, pBuf, htmlMessageLen );
    snprintf ( pBuf, pBufLen, "        message: %s<br>\n", lastMQTTmessage );
    strncat ( htmlMessage, pBuf, htmlMessageLen );
    unsigned long timeNow = now ();
    snprintf ( timeString, timeStringLen, "%04d-%02d-%02d %02d:%02d:%02dZ",
      year ( timeNow ), month ( timeNow ), day ( timeNow ), hour ( timeNow ), minute ( timeNow ), second ( timeNow ) );
    snprintf ( pBuf, pBufLen, "        at: %s<br></pre></p>\n", timeString );
    strncat ( htmlMessage, pBuf, htmlMessageLen );
  
    // snprintf ( pBuf, pBufLen, "<br><br>Len so far %d<br>", strlen ( htmlMessage ) );
    // strncat ( htmlMessage, pBuf, htmlMessageLen );
   
    strncat ( htmlMessage, "    </body>\n", htmlMessageLen );
    strncat ( htmlMessage, "  </html>\n", htmlMessageLen );

    htmlServer.send ( 200, "text/html", htmlMessage );
  }

#endif

// *****************************************************************************
// ********************************** serial ***********************************
// *****************************************************************************

bool handleSerialInput() {

  // collect serial input and handle it when necessary
  const int strBufSize = 500;
  static char strBuf [ strBufSize ];
  static int strBufPtr = 0;
  
  // if no characters arrive for a while, reset the buffer
  static unsigned long lastCharArrivedAt_ms = 0;
  const unsigned long timeoutPeriod_ms = 500;
  
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
      if ( strBufPtr > 5 && strBuf [ strBufPtr ] == '\r' ) { // 0x0d
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

  bool retain = false;
  
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

  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject ( strBuffer );

  // Test if parsing succeeds.
  if ( root.success () && root [ "command" ].success() ) {
    // handle whatever it was
    
    if ( root.containsKey ( "retain" ) ) {
      retain = root [ "retain" ];
    }
      
    const char* cmd = root [ "command" ];
    if ( ! strncmp ( cmd, "setAssignment", 20 ) ) {
      strncpy ( assignment, (const char *) root [ "assignment" ], assignmentStrLen );
      
      storePersistentDataToEEPROM ();
      
      char topic [ 92 ];
      snprintf ( topic, 92, "%s/%s/assignment", PROGNAME, uniqueToken );
       
      sendMQTTvalue ( (const char *) topic,
                      (const char *) assignment,
                      (const char *) topic,
                      retain );
    
    } else if (  ! strncmp ( cmd, "connect", 10 ) ) {
    
      if ( root.containsKey( "MQTThost" ) &&
           root.containsKey( "MQTTuserID" ) &&
           root.containsKey( "MQTTuserPW" ) &&
           root.containsKey( "networkSSID" ) &&
           root.containsKey( "networkPW" ) ) {

        strncpy ( mqtt_host, (const char *) root [ "MQTThost" ], mqttHostLen );
        conn_MQTT.setServer ( mqtt_host, 1883 );
        strncpy ( mqtt_userID, (const char *) root [ "MQTTuserID" ], mqttUserIDLen );
        strncpy ( mqtt_userPW, (const char *) root [ "MQTTuserPW" ], mqttUserPWLen );
      
        strncpy ( network_ssid, (const char *) root [ "networkSSID" ], networkSsidLen );
        strncpy ( network_passphrase, (const char *) root [ "networkPW" ], networkPassphraseLen );
      
        storePersistentDataToEEPROM ();      
        Serial.println ( "OK stored to EEPROM" );
      
        return ( networkConnect ( network_ssid, network_passphrase ) == 1 );
        
      } else {
        // not all necessary keys were present
        Serial.println ( "Some keys were missing from the connect command" );
        return ( false );
      }
      
    } else if (  ! strcmp ( cmd, "send" ) ) {
      sendMQTTvalue ( (const char *) root [ "topic" ],
                      (const char *) root [ "value" ],
                      (const char *) root [ "topic" ],
                      retain );
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

// *****************************************************************************
// *********************************** time ************************************
// *****************************************************************************

time_t getUnixTime() {
  while (conn_UDP.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
   int size = conn_UDP.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      conn_UDP.read(NTPBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)NTPBuffer[40] << 24;
      secsSince1900 |= (unsigned long)NTPBuffer[41] << 16;
      secsSince1900 |= (unsigned long)NTPBuffer[42] << 8;
      secsSince1900 |= (unsigned long)NTPBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * 1UL * 60UL * 60UL;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

void sendNTPpacket ( IPAddress& address ) {
  Serial.printf ( "Sending NTP request to %2u.%2u.%2u.%2u\n", 
    address[0], address[1], address[2], address[3] );
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  NTPBuffer[1] = 0;     // Stratum, or type of clock
  NTPBuffer[2] = 6;     // Polling Interval
  NTPBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  NTPBuffer[12]  = 49;
  NTPBuffer[13]  = 0x4E;
  NTPBuffer[14]  = 49;
  NTPBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
   // send a packet requesting a timestamp:
  conn_UDP.beginPacket ( address, 123 ); // NTP requests are to port 123
  conn_UDP.write ( NTPBuffer, NTP_PACKET_SIZE );
  conn_UDP.endPacket();
}

// *****************************************************************************
// *********************************** util ************************************
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

int getEEPROMstring ( byte baseAdd, byte maxLen, char result [] ) {
  int nChars = -1;
  for ( int i = 0; i < maxLen; i++ ) {
    result [ i ] = (char) EEPROM.read ( baseAdd + i );
    if ( result [ i ] == '\0' ) {
      nChars = i;
      break;
    }
  }
  
  if ( VERBOSE >= 15 ) {
    Serial.print ( "Read at " ); Serial.print ( baseAdd ); 
    Serial.print ( " ( len = " ); Serial.print ( nChars ); 
    Serial.print ( " ): " ); Serial.print ( result );
    Serial.print ( "\n" );
  }
  
  return ( nChars );
}
    
void writeEEPROMstring ( byte baseAdd, char str [] ) {

  if ( VERBOSE >= 15 ) {
    Serial.print ( "Write at " ); Serial.print ( baseAdd ); 
    Serial.print ( " ( len = " ); Serial.print ( strlen ( str ) ); 
    Serial.print ( " ): " ); Serial.print ( str );
    Serial.print ( "\n" );
  }

	for ( int i = 0; i < strlen ( str ); i++ ) {
	  // Serial.print ( (char) str [ i ] );
	  EEPROM.write ( baseAdd + i, str [ i ] );
	}
	EEPROM.write ( baseAdd + strlen ( str ), 0x00 );
	EEPROM.commit ();                                   // NOTE
	
}

bool persistentDataIsStoredToEEPROM_p () {
  bool good;
  
  char tmp [ 4 ];
  good = getEEPROMstring ( 0, 4, tmp ) == 3;

  if ( good &&  ! strncmp ( tmp, "set", 3 ) ) {
    return ( true );
  } else {
    // Serial.print ( "EEPROM 'set' match failed: " ); Serial.println ( tmp );
    return ( false );
  }
}

void storePersistentDataToEEPROM () {
  
  size_t add = 0;
  writeEEPROMstring ( add, "set" ); add += 4;
  writeEEPROMstring ( add, network_ssid ); add += networkSsidLen;
  writeEEPROMstring ( add, network_passphrase ); add += networkPassphraseLen;
  
  writeEEPROMstring ( add, mqtt_host ); add += mqttHostLen;
  writeEEPROMstring ( add, mqtt_userID ); add += mqttUserIDLen;
  writeEEPROMstring ( add, mqtt_userPW ); add += mqttUserPWLen;
  
  writeEEPROMstring ( add, assignment ); add += assignmentStrLen;

}

bool getPersistentDataFromEEPROM () {

  // see if we have good data in EEPROM
  // the first 4 bytes of EEPROM will be "set\0" if we have set the data
  
  bool good = persistentDataIsStoredToEEPROM_p ();
  
  if ( good ) {
    
    // we think there's valid data in EEPROM
    // but it still may not point to a network that is currently available!
    size_t add = 4;
    if ( getEEPROMstring ( add, networkSsidLen, network_ssid ) < 0 ) good = false;
    add += networkSsidLen;
    if ( getEEPROMstring ( add, networkPassphraseLen, network_passphrase ) < 0 ) good = false;
    add += networkPassphraseLen;
  
    if ( getEEPROMstring ( add, mqttHostLen, mqtt_host ) < 0 ) good = false;
    add += mqttHostLen;
    if ( getEEPROMstring ( add, mqttUserIDLen, mqtt_userID ) < 0 ) good = false;
    add += mqttUserIDLen;
    if ( getEEPROMstring ( add, mqttUserPWLen, mqtt_userPW ) < 0 ) good = false;
    add += mqttUserPWLen;

    if ( getEEPROMstring ( add, assignmentStrLen, assignment ) < 0 ) good = false;
    add += assignmentStrLen;    
    
  } else {
    good = false;
  }
  
  // else {
  //   Serial.print ( "getEEPROMstring: " ); Serial.print ( getEEPROMstring ( 0, 4, tmp ) );
  //   Serial.print ( " --> " ); Serial.println ( tmp );
  // }
    
  return ( good );
  
}

// *****************************************************************************
// ********************************** debug ************************************
// *****************************************************************************

#if TELEMETRY_ON
  int availableMemory() {
    #if false 
      // arduino
      extern int __heap_start, *__brkval;
      int v;
      return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
    #else 
      // esp8266
      // extern "C" {
      //   #include "user_interface.h"
      // }
      // uint32_t free = system_get_free_heap_size();
      return ESP.getFreeHeap();
    #endif
  }
#endif


// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
