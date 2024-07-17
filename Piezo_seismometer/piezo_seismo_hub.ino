/*
                                                        
                  power 9VDC                            
                                                        
                                                        
                                      ┌──────────────┐  
   piezo ──► amp ───► ATmega ───────► │ESP-01 AP-STA │  
                        fast loop     │  MQTT        │  
                                      │  HTML        │  
                                      └──────────────┘  
                                                                                                                       
   ( asciiflow.com )
 */

#define PROGNAME "Seismo_hub"
#define VERSION  "0.1.5"
#define VERDATE  "2024-07-15"

/*
    seismo_hub.ino
    2024-06-24
    Charles B. Malloch, PhD

    cloned from
    ESP8266_basic.ino
    2019-01-23
    Charles B. Malloch, PhD
    
    Takes seismometry readings from an ATmega processor and makes them available
      o to MQTT
      o to a web browser pointed to esp8266-19.local
      o to a web browser pointed to esp8266-19.local on network "Seismometer"
      o a log file on an SD card ** is handled by the ATmega **
    
    To do:
      √ set up throbber
      √ adapt and move in web page from MEMS seismo program
      √ adapt to the fact that data comes in as JSON via the serial port
          rather than from a MEMS device connected by i2c
            Respond to JSON commands like
            o { "command": "send", "topic": "seismo/piezo/energy", "value": "0.8" }
            o { "command": "send", "topic": "seismo/piezo/threshold", "value": "101" }
            o { "command": "send", "topic": "seismo/piezo/triggered", "value": "1" }     // expect 1 or 0
            
            o { "command": "send", "topic": "seismo/piezo/energy", "value": "<value>"[, "retain": "<value>"] }
            o { "command": "send", "topic": "seismo/piezo/threshold", "value": "<value>"[, "retain": "<value>"] }
            o { "command": "send", "topic": "seismo/piezo/triggered", "value": "<value>"[, "retain": "<value>"] }
            as if they were
            o { "command": "send", "topic": "seismo/19/piezo/energy", "value": "0.2" } etc.

      √ allow text output for testing mode
      √ adapt and move in JSON handling from text-to-MQTT
      √ verify MQTT
      √ update web browser to chart values
      √ verify web browser
      √ verify OTA
      √ report offered access point on web page
      √ reposition, resize table
      √ timestamps in data
      √ EWMA 1h, 6m
      √ add Nell ESW network
      √ allow "chart explore" on pan and scroll
      √ allow for missing MQTT
      - allow for missing cbm favicon
      
*/

/* 
  Regarding websocket event debugging:
    https://github.com/Links2004/arduinoWebSockets/issues/274
    > I have this line enabled, is that what you mean?
      USE_SERIAL.setDebugOutput(true);
      There have not been any messages in serial monitor other than like my screenshot.
    > that is part of it, the IDE has a menu to enable build defines to get more logging.
      https://arduino-esp8266.readthedocs.io/en/latest/Troubleshooting/debugging.html
      if you not use the Arduino IDE then you need to set the defines yourself.
      
  Also see https://arduino-esp8266.readthedocs.io/en/latest/Troubleshooting/debugging.html
*/



#include <ESP8266WiFi.h>                  //https://github.com/esp8266/Arduino
#include <ArduinoJson.h>                  

#include <WiFiClient.h>                   // WiFiClient supports TCP connection
#include <PubSubClient.h>                 // PubSubClient supports MQTT
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
// mDNS is now furnished by ArduinoOTA
// see https://tttapa.github.io/ESP8266/Chap08%20-%20mDNS.html
// ESP8266mDNS is multicast DNS - responds to <whatever>.local
// #include <ESP8266mDNS.h>
// ESP8266WiFiMulti looks at multiple access points and chooses the strongest
// #include <ESP8266WiFiMulti.h>
#include <WiFiUDP.h>                      // needed for NTP
#include <TimeLib.h>

#define ALLOW_OTA
#ifdef ALLOW_OTA
  #include <ArduinoOTA.h>
#endif

#include <cbmThrobber.h>
#include <cbmNTP.h>
#include <cbmNetworkInfo.h>
#include <EWMA.h>

#define SECOND_ms  ( 1000UL )
#define MINUTE_ms  ( 60UL * SECOND_ms )
#define HOUR_ms    ( 60UL * MINUTE_ms )

// ***************************************
// ***************************************
#pragma mark -> vars MAIN PARAMETERS

// ARDUINO_ESP8266_GENERIC

#define mqtt_basebaseTopic "seismo"

#define BAUDRATE    115200
// VERBOSE <= 1 causes throbber instead of Serial output
#define VERBOSE          1

const unsigned long rebootInterval_ms         = 0UL;  // 24UL * HOUR_ms;

// either define ( to enable ) or undef
#define USE_GENERIC_MDNS_NAME
#define ALLOW_PRINTING_OF_PASSWORD false
#undef TELEMETRY_ON

const unsigned int port_UDP = 9250;

// ***************************************
// ***************************************

/********************************** GPIO **************************************/
#pragma mark -> vars GPIO

/*
  I2C cbm standard colors: yellow for SDA, blue for SCL
*/

#define INVERSE_LOGIC_OFF 1
#define INVERSE_LOGIC_ON  0
  
// Red LED is power; no control over it
// GPIO0 for SDA
// GPIO1 ( TX ) is blue, inverse logic
// GPIO2 (next to GND) for SCL; 1K pullup on my boards
// GPIO3 ( RX )

/*
  For this program, we need to use RX(3) to receive messages from the ATmega
  And TX(1), active low, for throbbing or alert
  Can we use GPIO0 after booting for output for alert?
*/
#undef pdConnecting
#define pdThrobber               1
#define throbberIsInverted_P  true
#define throbberPWM_P        false

#ifdef pdThrobber
  //                         pin            inverted_P         PWM_P
  cbmThrobber throbber ( pdThrobber, throbberIsInverted_P, throbberPWM_P );
#endif

/********************************* Network ************************************/
#pragma mark -> vars Network

/*
  The cbmNetworkInfo object has (at least) the following instance variables:
    .ip
    .gw (gateway)
    .mask
    .ssid
    .password
    .chipID    ( GUID assigned by manufacturer, often the last three octets
                 of the MAC address, e.g. 5c:cf:7f:xx:xx:xx )
    .chipName  ( globally-unique name assigned by cbm and marked on PCB
*/

cbmNetworkInfo Network;

// All that's necessary for access point is this and two lines in initializeNetwork
// The AP password is in cbmNetworkInfo
//   WiFi.mode(WIFI_AP_STA);
//   WiFi.softAP(AP_SSID, AP_PASS);

#define AP_SSID "Seismometer"

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient conn_TCP;
WiFiUDP conn_UDP;
PubSubClient conn_MQTT ( conn_TCP );
ESP8266WebServer htmlServer ( 80 );
WebSocketsServer webSocket = WebSocketsServer(81);

/************************************ mDNS ************************************/
#pragma mark -> vars mDNS

const int mdnsOtaIdLen = 40;
char mdnsOtaId [ mdnsOtaIdLen ];

/************************************ MQTT ************************************/
#pragma mark -> vars MQTT

bool MQTTdisabledP = false;

const size_t mqttClientIDLen                  = 50;
char mqtt_clientID [ mqttClientIDLen ]        = "<unset>";

const int mqttTopicLen                        = 50;
char mqtt_baseTopic [ mqttTopicLen ];
char mqttCmdTopic [ mqttTopicLen ];
char mqttStatusTopic [ mqttTopicLen ];
char mqttTimeoutTopic [ mqttTopicLen ];

const bool MQTT_RETAIN = true;

unsigned long nSendings = 0;
const int lastMQTTtopicLen = 64;
char lastMQTTtopic [lastMQTTtopicLen] = "No topic yet";
const int lastMQTTmessageLen = 64;
char lastMQTTmessage [lastMQTTmessageLen] = "No message yet";
unsigned long lastMQTTmessageAt_nts = 0;


/********************************** NTP ***************************************/
#pragma mark -> vars NTP

IPAddress timeServer ( 17, 253, 14, 253 );
const char* NTPServerName = "time.apple.com";

const int NTP_PACKET_SIZE = 48;   // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer [NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
  
const int timeZone = 0;   // GMT
// const int timeZone = -5;  // Eastern Standard Time (USA)
// const int timeZone = -4;  // Eastern Daylight Time (USA)
unsigned long lastNTPResponseAt_ms = millis();
// uint32_t time_unix_s               = 0;

/********************************** Web ***************************************/
#pragma mark -> vars Web

/******************************* Global Vars **********************************/
#pragma mark -> vars global

const int assignmentStrLen = 64;
char assignment [assignmentStrLen] = "Piezo Seismometer Hub";

char uniqueToken [ 9 ];
unsigned long lastRebootAt_nts                 = 0UL;

const size_t pBufLen = 128;
char pBuf [ pBufLen ];
const size_t fBufLen = 8;
char fBuf [ fBufLen ];

char topic [ mqttTopicLen ];

const int timeStringLen = 32;
char timeString [ timeStringLen ] = "<unset>";
char bootTimeString [ timeStringLen ] = "<unset>";

const size_t jsonStrSize = 1000; 
char jsonString [ jsonStrSize ];  // needs to be global to be on heap!

int relayIsOn = 0;

bool forceWSUpdate = false;

/**************************** App-Specific Vars *******************************/
#pragma mark -> vars app-specific

float energy;
float threshold;
bool triggered_P;
const unsigned long wsSendInterval_ms        =   5000UL;


EWMA EWMA_6m ( EWMA::alpha (        6.0 * 60.0 * 1000.0 / wsSendInterval_ms ) );
EWMA EWMA_1h ( EWMA::alpha ( 1.0 * 60.0 * 60.0 * 1000.0 / wsSendInterval_ms ) );

/**************************** Function Prototypes *****************************/
#pragma mark -> function prototypes

void initializeGPIO ();
void initializeNetwork ();
void initializeRandomToken ();
void initializeUDP ();
void initializeNTP ();
bool initializeMQTT ();
void reportBootTime ();
void initializeOTA ();
void initializeWebServer ();

bool connect_WiFi ();

bool connect_MQTT ();
// void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length );
int sendValueToMQTT ( const char * topic, long value, const char * name, bool retainP = false );
int sendValueToMQTT ( const char * topic, const char * value, const char * name, bool retainP = false );

void updateWebPage_root ();
void webSocketEvent ( uint8_t num, WStype_t type, uint8_t * payload, size_t length );
void update_WebSocket ();

bool handleSerialInput ();
// void handleSerialString ( char str[] );
// void broadcastString ( char buffer[] );
// void handle_UDP_conversation ();
// int handleCommand ( char packetBuf[] );
// void reply_to_message ( WiFiUDP conn_UDP, char* incoming_message );

void indicateConnecting ( int value );
void reportSystemStatus();
int availableMemory();

// magic juju to return array size
// see http://forum.arduino.cc/index.php?topic=157398.0
template< typename T, size_t N > size_t ArraySize (T (&) [N]){ return N; }

/******************************************************************************/

void setup() {

  initializeGPIO ();
  initializeNetwork ();
  connect_WiFi ();
  yield();
  initializeRandomToken ();
  initializeUDP ();
  yield ();
  initializeNTP ();
  initializeMQTT ();
  reportBootTime ();
  yield();
  initializeOTA ();
  initializeWebServer ();
  yield();
  
  /************************** Report successful init **************************/

  Serial.println();
  snprintf ( pBuf, pBufLen, "%s v%s %s", PROGNAME, VERSION, VERDATE );
  Serial.printf ( "\n%s cbm", pBuf );
  snprintf ( topic, mqttTopicLen, "firmware/%s", uniqueToken );
  sendValueToMQTT ( topic, pBuf, "firmware" );
  
  #ifdef TESTING
    Serial.printf ( " TESTING level %d\n", TESTING );
  #else
    Serial.println ();
  #endif
  Serial.printf ( "Compiled at %s on %s\n\n\n", __TIME__, __DATE__ );
  
  if ( VERBOSE <= 1 ) {
    // and now release throbber / alert pin from TX duty:
    Serial.printf ( "\n\nReleasing throbber pin now...\n" );
    delay ( 100 );
    Serial.begin ( BAUDRATE, SERIAL_8N1, SERIAL_RX_ONLY );
    pinMode ( pdThrobber, OUTPUT );
    // Serial.printf ( "Testing what happens when we try to print now...\n" );  // nothing bad
    // throbber still works
    // If we *don't* release the pin, Serial indeed works as before and throb doesn't work
  }
    
}

void loop() {

  static unsigned long lastWSSendAt_ms         =      0UL;
     
  const unsigned long commoCheckInterval_ms    =      2UL;
  static unsigned long lastCommoCheckAt_ms     =      0UL;
  
  const unsigned long GPIOUpdateInterval_ms    =     50UL;
  static unsigned long lastGPIOUpdateAt_ms     =      0UL;

  const unsigned long statusDelay_ms           = 300000UL;
  static unsigned long lastStatusAt_ms         =      0UL;
        
  const unsigned long timeCheckDelay_ms        = 300000UL;
  static unsigned long timeCheckStatusAt_ms    =      0UL;

  /****************************************************************************/

  if ( ! connect_WiFi () ) return;
  yield();
  
  #ifdef ALLOW_OTA
    ArduinoOTA.handle();
    yield ();
  #endif

  /****************************************************************************/

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition below.
  
  if ( ! connect_MQTT () ) return;
    
  conn_MQTT.loop();
  delay ( 10 );  // fix some issues with WiFi stability
  yield();
  
  /****************************************************************************/

  if ( ( millis() - timeCheckStatusAt_ms ) > timeCheckDelay_ms ) {
    if ( timeStatus() != timeSet ) {
      getUnixTime ();
    }
    timeCheckStatusAt_ms = millis();
  }

  /****************************************************************************/

  htmlServer.handleClient();
  
  webSocket.loop();
  yield();
    
  /***********/

  if ( ( forceWSUpdate ) || ( lastWSSendAt_ms == 0UL ) 
      || ( ( millis() - lastWSSendAt_ms ) > wsSendInterval_ms ) ) {
    
    if ( ( VERBOSE >= 15 ) && forceWSUpdate ) Serial.println ( F ( "forced WS update" ) );

    update_WebSocket ();
    lastWSSendAt_ms = millis();

  }

  yield ();

  /****************************************************************************/  
   
  if   ( ( lastCommoCheckAt_ms == 0UL ) 
    || ( ( millis() - lastCommoCheckAt_ms ) > commoCheckInterval_ms ) ) {
    handleSerialInput();
    lastCommoCheckAt_ms = millis();
  }
  
  if ( ( millis() - lastGPIOUpdateAt_ms ) > GPIOUpdateInterval_ms ) {
    // indicateConnecting ( relayIsOn );
    lastGPIOUpdateAt_ms = millis();
  }
    
  yield ();
  
  /****************************************************************************/

  if ( ( millis() - lastStatusAt_ms ) > statusDelay_ms ) {
    reportSystemStatus();
    lastStatusAt_ms = millis();
  }

  /****************************************************************************/

  if ( ( rebootInterval_ms > 0UL ) && ( millis() > rebootInterval_ms ) ) {
    
    snprintf ( topic, mqttTopicLen, "%s/telemetry/time/restart", mqtt_baseTopic );
    formatTimeString ( bootTimeString, timeStringLen, now () );
    sendValueToMQTT ( topic, bootTimeString, "Restart Time" );
    Serial.printf ( "Rebooting at %s\n", bootTimeString );
    delay ( 1000 );
    
    ESP.restart();  // soft reset;  ESP.reset() is a hard reset leaving regs unknown...
  
  }

  #ifdef pdThrobber
    if ( relayIsOn ) {
      throbber.throb ( 750UL, 250UL );
    } else {
      throbber.throb ( 250UL, 750UL );
    }
  #endif
  
  delay ( 10 );
  
}

// *****************************************************************************
// **************************** Initializations ********************************
// *****************************************************************************

void initializeGPIO () {

  #ifdef pdConnecting
    pinMode ( pdConnecting, OUTPUT );
    indicateConnecting ( 1 );
  #endif
  #ifdef pdThrobber
    pinMode ( pdThrobber, OUTPUT ); 
    digitalWrite ( pdThrobber, throbberIsInverted_P );
  #endif
  
  // #define SERIAL_FUNCTION FUNCTION_0
  // #define GPIO_FUNCTION   FUNCTION_3

  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 5000 ) ) {
    indicateConnecting ( -1 );
    delay ( 200 );  // wait a little more, if necessary, for Serial to come up
    yield ();
  }
  Serial.print ( F ( "\n\nGPIO initialized\n" ) );

  if ( VERBOSE >= 2 ) {
    Serial.setDebugOutput ( true );
    Serial.print ( F ( "\n\nsetDebugOutput set to true\n" ) );
  }

  indicateConnecting ( 0 );
}

void initializeNetwork () {
  
  int locale = Network.choosePreferredNetwork();
  Network.init ( locale );
  
  if ( ! strncmp ( Network.chipName, "unknown", 12 ) ) {
    Network.describeESP ( Network.chipName );
  }

  // Connect to WiFi access point.
  Serial.printf ( "\nESP8266 device '%s' connecting to %s\n", Network.chipName, Network.ssid );
  yield ();
  
  // does this need to be before .begin? before .config?
  // it's OK before .config
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
  WiFi.begin ( Network.ssid, Network.password );
  
  if ( ( VERBOSE >= 4 ) ) {
    Serial.printf ( "WiFi access point: %s\n", Network.ssid );
    Serial.printf ( "WiFi parameters: %u.%u.%u.%u gw %u.%u.%u.%u mask %u.%u.%u.%u dns %u.%u.%u.%u\n",
       Network.ip   [ 0 ], Network.ip   [ 1 ], Network.ip   [ 2 ], Network.ip   [ 3 ],
       Network.gw   [ 0 ], Network.gw   [ 1 ], Network.gw   [ 2 ], Network.gw   [ 3 ],
       Network.mask [ 0 ], Network.mask [ 1 ], Network.mask [ 2 ], Network.mask [ 3 ],
       Network.dns  [ 0 ], Network.dns  [ 1 ], Network.dns  [ 2 ], Network.dns  [ 3 ]  );
  }

  if ( ! strncmp ( Network.ssid, "Pinafore" ) ) {
    // we're at Nell's
    MQTTdisabledP = true;
  }
  Serial.printf ( "MQTT is %sabled\n", MQTTdisabledP ? "dis" : "" );

  return;
}

void initializeRandomToken () {  
  // create a hopefully-unique string to identify this program instance
  // PREREQUISITE: must have initialized the network for chipName
  // DEPENDENCY: uniqueToken is used by MQTT connection process
  
  #ifdef cbmnetworkinfo_h
    // Chuck-only
    strncpy ( uniqueToken, Network.chipName, 9 );
  #else
    snprintf ( uniqueToken, 9, "%08x", ESP.getChipId() );
  #endif
}

void initializeUDP () {

  conn_UDP.begin ( port_UDP );
  
  if ( VERBOSE >= 2 ) Serial.printf ( "UDP connected on port %d\n", port_UDP );
  delay ( 50 );
}

void initializeNTP () {

  // note that NTP uses UDP, which must by now have been initialized
  
  // setSyncProvider ( getUnixTime );
  // setSyncInterval ( 300 );          // seconds
  // unsigned long beganWaitingAt_ms = millis();
  // 
  // while ( ( timeStatus() != timeSet ) && ( ( millis () - beganWaitingAt_ms ) < 10000UL ) ) {
  //   now ();
  //   Serial.print ( "t" );
  //   delay ( 100 );
  //   yield();
  // }
  
  cbmNTP_init ( conn_UDP, timeZone );

  unsigned long beganWaitingAt_ms = millis();
  
  while ( ( timeStatus() != timeSet ) && ( ( millis () - beganWaitingAt_ms ) < 10000UL ) ) {
    now ();
    if ( VERBOSE >= 2 ) Serial.print ( F ( "t" ) );
    delay ( 100 );
    // yield();
  }
  if ( VERBOSE >= 2 ) Serial.println ();
  
}

bool initializeMQTT () {

  if ( MQTTdisabledP ) {
    // we're not at Chuck's
    return;
  }

  if ( ! strncmp ( uniqueToken, "<unset>", 9 ) ) {
    Serial.println ( F ( "ERROR: uniqueToken not yet set!" ) );
    while ( 1 );
  }
  snprintf ( mqtt_clientID, mqttClientIDLen, "%s_%s", PROGNAME, uniqueToken );
  
  snprintf ( mqtt_baseTopic, mqttTopicLen, "%s/%s", mqtt_basebaseTopic, uniqueToken );
  snprintf ( mqttCmdTopic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "command" );
  snprintf ( mqttStatusTopic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "status" );
  snprintf ( mqttTimeoutTopic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "timeout_ms" );
  
  // conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  
  
  conn_MQTT.setServer ( CBM_MQTT_SERVER, CBM_MQTT_SERVERPORT );  
  if ( VERBOSE >= 2 ) {
    Serial.print ( F ( "Connecting to MQTT at " ) );
    Serial.println ( CBM_MQTT_SERVER );
  }
  return connect_MQTT ();
}

void reportBootTime () {
  snprintf ( topic, mqttTopicLen, "%s/telemetry/time/startup", mqtt_baseTopic );
  if ( timeStatus() == timeSet ) {
    lastRebootAt_nts = now ();
    formatTimeString ( bootTimeString, timeStringLen, lastRebootAt_nts );
    sendValueToMQTT ( topic, bootTimeString, "Startup Time", MQTT_RETAIN );
    if ( VERBOSE >= 2 ) Serial.printf ( "Startup recorded at %s\n", bootTimeString );
  } else {
    if ( VERBOSE >= 2 ) Serial.println ( F ( "WARNING: No NTP response within 30 seconds..." ) );
    sendValueToMQTT ( topic, "WARNING no NTP within 30 seconds", "Startup Time", MQTT_RETAIN );
    snprintf ( bootTimeString, timeStringLen, "&lt;No NTS at boot&gt;" );
  }
}

void initializeOTA () {

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to ESP8266-[ChipID]
  // snprintf ( mdnsOtaId, mdnsOtaIdLen, "%s-%s", PROGNAME, uniqueToken );
  // setHostname needed to permit mDNS
  snprintf ( mdnsOtaId, 50, "ESP8266-%s", uniqueToken );
  ArduinoOTA.setHostname ( (const char *) mdnsOtaId );

  #ifdef ALLOW_OTA
  
    // No authentication by default
    ArduinoOTA.setPassword ( (const char *) CBM_OTA_KEY );

    ArduinoOTA.onStart([]() {
      if ( VERBOSE >= 2 ) Serial.println ( F ("OTA Start") );
    });
  
    ArduinoOTA.onEnd([]() {
      if ( VERBOSE >= 2 ) Serial.println ( F ("\nOTA End") );
    });
  
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      if ( VERBOSE >= 2 ) Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  
    ArduinoOTA.onError([](ota_error_t error) {
      if ( VERBOSE >= 2 ) {
        Serial.printf("Error[%u]: ", error);
             if (error == OTA_AUTH_ERROR)    Serial.println ( F ("OTA Auth Failed") );
        else if (error == OTA_BEGIN_ERROR)   Serial.println ( F ("OTA Begin Failed") );
        else if (error == OTA_CONNECT_ERROR) Serial.println ( F ("OTA Connect Failed") );
        else if (error == OTA_RECEIVE_ERROR) Serial.println ( F ("OTA Receive Failed") );
        else if (error == OTA_END_ERROR)     Serial.println ( F ("OTA End Failed") );
      }
    });
  
    ArduinoOTA.begin();
    if ( VERBOSE >= 1 ) Serial.printf ( "ArduinoOTA running with host name %s\n", mdnsOtaId );
    
    yield();
    
  #endif
}

void initializeWebServer () {

  /**************************** HTML Server Setup *****************************/

  // for html, do the on's before server begin

  htmlServer.on ( "/", updateWebPage_root );
  htmlServer.onNotFound([](){
    htmlServer.send(404, "text/plain", "404: Not found");
  });
  htmlServer.begin();  
  
  webSocket.begin();
  webSocket.onEvent ( webSocketEvent );
  // the below are applicable to a WebSocket CLIENT...
  // webSocket.setReconnectInterval ( 15000 );  // ms to reconnect after failure
  // ms; <ping interval> <response timeout> <disconnect if n failures>
  // webSocket.enableHeartbeat ( 30000, 3000, 2 );
    
}

// *****************************************************************************
// ********************************** WiFi *************************************
// *****************************************************************************

bool connect_WiFi () {
  
  // returns true on success
    
  if ( WiFi.status() == WL_CONNECTED ) return true;
  
  // is this the first time we're connecting?
  static bool newConnection = true;
  int nDots = 0;
  
  unsigned long startTime_ms                     = millis();
  const unsigned long connectionTimeoutPeriod_ms = 15UL * SECOND_ms;
  unsigned long lastDotAt_ms                     = millis();
  const unsigned long dotPeriod_ms               =            500UL;
  unsigned long lastBlinkAt_ms                   = millis();
  const unsigned long blinkPeriod_ms             =            500UL;
  bool printed = false;

  indicateConnecting ( 1 );
  
  if ( VERBOSE >= 1 ) {
    Serial.print ( F ( "\nconnect_WiFi: connecting to network '" ) ); Serial.print ( Network.ssid );
    Serial.printf ( "' at IP address " );
  }
  Network.printIP ( Network.ip );
  
  if ( ( VERBOSE >= 4 ) && ALLOW_PRINTING_OF_PASSWORD ) {
    Serial.print ( F ( "' with password '" ) );
    Serial.print ( Network.password );
  }
  Serial.println ( F ( "'" ) );
  
  yield ();
  
  // Serial.println ( F ( "Available networks:" ) );
  // if ( ! listNetworks() ) return -1002;
  
  while ( WiFi.status() != WL_CONNECTED ) {
  
  /*
  
    WiFi.status() return values:
        3 : WL_CONNECTED: assigned when connected to a WiFi network;
      255 : WL_NO_SHIELD: assigned when no WiFi shield is present;
        0 : WL_IDLE_STATUS: it is a temporary status assigned when WiFi.begin() is called and remains active until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a connection is established (resulting in WL_CONNECTED);
        1 : WL_NO_SSID_AVAIL: assigned when no SSID are available;
        2 : WL_SCAN_COMPLETED: assigned when the scan networks is completed;
        4 : WL_CONNECT_FAILED: assigned when the connection fails for all the attempts;
        5 : WL_CONNECTION_LOST: assigned when the connection is lost;
        6 : WL_DISCONNECTED: assigned when disconnected from a network; 

  */
  
    if ( VERBOSE > 4 && ! printed ) { 
      Serial.print ( "\nChecking wifi..." );
      printed = true;
      newConnection = true;
    }
    
    if ( ( millis() - lastDotAt_ms ) > dotPeriod_ms ) {
      if ( VERBOSE >= 2 ) Serial.print ( WiFi.status() );
      if ( ++nDots % 80 == 0 ) Serial.print ( F ( "\n" ) );
      lastDotAt_ms = millis();
    }
    
    if ( ( millis() - lastBlinkAt_ms ) > blinkPeriod_ms ) {
      indicateConnecting ( -1 );
      lastBlinkAt_ms = millis();
    }
    
    if ( ( millis() - startTime_ms ) > connectionTimeoutPeriod_ms ) {
      if ( VERBOSE >= 1 ) Serial.println ( F ( "connect_WiFi: connection timeout!" ) );
      #if ( 1 ) 
        // REBOOT!!
        delay ( 50 );
        ESP.restart();  // soft reset;  ESP.reset() is a hard reset leaving regs unknown...
        // won't ever run, since wifi is not connected here...
        #if ( defined TELEMETRY_ON ) && 0
          snprintf ( topic, mqttTopicLen, "%s/telemetry/wifi_%sconnect/failure", mqtt_baseTopic, newConnection ? "" : "re" );
          sendValueToMQTT ( topic, WiFi.status(), "WiFi (re)connect failure", MQTT_RETAIN );
        #endif
        return false;
      #else
        // don't start trying to restart network until oldConnectionTimeout_ms
          static unsigned long lastReconfigTime_ms   = millis();
          const unsigned long reconfigInterval_ms    = 10 * SECOND_ms;
   
          // repeat at intervals of reconfigInterval_ms
          if ( ( millis() - lastReconfigTime_ms ) > reconfigInterval_ms ) {
            initializeNetwork ();
            lastReconfigTime_ms = millis();
          }    
      #endif
    }
        
    delay ( 50 );
  }
  
  if ( newConnection && VERBOSE > 4 ) {
    Serial.print ( F ( "\n  WiFi connected as " ) ); 
    Serial.print ( WiFi.localIP() );
    Serial.print ( F ( " in " ) );
    Serial.print ( millis() - startTime_ms );
    Serial.println ( F ( " ms" ) );    
  }
    
  // won't ever run, since MQTT is not connected here...
  #if ( defined TELEMETRY_ON ) && 0
    snprintf ( topic, mqttTopicLen, "%s/telemetry/wifi_%sconnect/success", mqtt_baseTopic, newConnection ? "" : "re" );
    formatTimeString ( timeString, timeStringLen, now() );
    sendValueToMQTT ( topic, timeString, "WiFi (re)connect", MQTT_RETAIN );
    
  #endif

  yield();

  startTime_ms = millis();
  printed = false;
  
  // WiFi_connection_initedP = true;
  newConnection = false;

  indicateConnecting ( 0 );
  
  return true;
  
}

// *****************************************************************************
// ********************************** MQTT *************************************
// *****************************************************************************

bool connect_MQTT () {
  
  // returns true if success

  // immediately return if MQTT is already connected
  if ( conn_MQTT.connected() ) return true;
  
  // see if MQTT has been initialized
  // if so, its connection parameters are set
  if ( ! strncmp ( mqtt_clientID, "mqtt_clientID_not_initialized", 30 ) ) {
    Serial.println ( F ( "connect_MQTT: ERROR: MQTT client has not been initialized!" ) );
    return false;
  }
    
  indicateConnecting ( 1 );
    
  Serial.printf ( "connect_MQTT: Connecting to MQTT at %s:%d\n", CBM_MQTT_SERVER, CBM_MQTT_SERVERPORT );
  if ( VERBOSE >= 4 ) {
    Serial.print ( F ( " as '" ) ); Serial.print ( mqtt_clientID );
    if ( ALLOW_PRINTING_OF_PASSWORD ) {
      Serial.print ( F ( "' user '" ) );
      Serial.print ( CBM_MQTT_USERNAME );
      Serial.print ( F ( "' / '" ) );
      Serial.print ( CBM_MQTT_KEY );
    }
    Serial.println ( F ( "'" ) );
    Serial.print ( F ( "  Checking MQTT ( status: " ) );
    Serial.print ( conn_MQTT.state () );
    Serial.print ( F ( " ) ..." ) );
  }
  
  yield();
  
  static bool newConnection = true;
  int nDots = 0;
  bool printed = false;

  unsigned long startTime_ms                     = millis();
  const unsigned long connectionTimeoutPeriod_ms = 15UL * 1000UL;
  unsigned long lastDotAt_ms                     = millis();
  const unsigned long dotPeriod_ms               =         500UL;
  unsigned long lastBlinkAt_ms                   = millis();
  const unsigned long blinkPeriod_ms             =         500UL;
  
  while ( ! conn_MQTT.connected () ) {
  
    // conn_MQTT.disconnect();
    // delay ( 50 );
    
    conn_MQTT.connect ( mqtt_clientID, CBM_MQTT_USERNAME, CBM_MQTT_KEY );
    
    if ( VERBOSE > 4 && ! printed ) { 
      Serial.print ( "  Connecting to MQTT as '"); Serial.print ( mqtt_clientID );
      Serial.print ( "'; status: " );
      Serial.print ( conn_MQTT.state () );
      Serial.print ( "..." );
      /*
        -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
        -3 : MQTT_CONNECTION_LOST - the network connection was broken
        -2 : MQTT_CONNECT_FAILED - the network connection failed - perhaps the IP is bad!!!
        -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
         0 : MQTT_CONNECTED - the cient is connected
         1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
         2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
         3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
         4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
         5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect_WiFi
      */
      printed = true;
      newConnection = true;
    }
   
    if ( ( millis() - lastDotAt_ms ) > dotPeriod_ms ) {
      Serial.print ( conn_MQTT.state() );
      if ( ++nDots % 80 == 0 ) Serial.print ( F ( "\n" ) );
      lastDotAt_ms = millis();
    }
    
    if ( ( millis() - lastBlinkAt_ms ) > blinkPeriod_ms ) {
      indicateConnecting ( -1 );
      lastBlinkAt_ms = millis();
    }
    
    if ( ( millis() - startTime_ms ) > connectionTimeoutPeriod_ms ) {
      Serial.println ( F ( "connect_MQTT: connection timeout!" ) );
      
      // won't ever run, since MQTT is not connected here...
      #ifdef TELEMETRY_ON
        snprintf ( topic, mqttTopicLen, "%s/telemetry/MQTT_%sconnect/failure", mqtt_baseTopic, newConnection ? "" : "re" );
        sendValueToMQTT ( topic, conn_MQTT.state (), "Telemetry MQTT connect failure", MQTT_RETAIN );
      #endif
      return false;        
    }
      
    delay ( 50 );
  }
  
  if ( newConnection ) {
    if ( VERBOSE > 4 ) {
      Serial.print ("\n  MQTT connected in ");
      Serial.print ( millis() - startTime_ms );
      Serial.println ( " ms" );
    }
  }
  
  #ifdef TELEMETRY_ON
    snprintf ( topic, mqttTopicLen, "%s/telemetry/MQTT_%sconnect/success", mqtt_baseTopic, newConnection ? "" : "re" );
    formatTimeString ( timeString, timeStringLen, now() );
    sendValueToMQTT ( topic, timeString, "Telemetry MQTT connect success", MQTT_RETAIN );
  #endif
  
  //  + is one level; # is many levels
  {
    const size_t extendedMqttCmdTopicLen = 128; char extendedMqttCmdTopic [ extendedMqttCmdTopicLen ];
    snprintf ( extendedMqttCmdTopic, extendedMqttCmdTopicLen, "%s/#", mqttCmdTopic );
    conn_MQTT.subscribe ( extendedMqttCmdTopic, 1 );
    if ( VERBOSE >= 4 ) Serial.printf ( "\n  Subscribed to MQTT feed %s\n", extendedMqttCmdTopic );
  }
  // conn_MQTT.unsubscribe("/example");
  // conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  
  yield ();

  newConnection = false;
  indicateConnecting ( 0 );
  
  return true;
}

int sendValueToMQTT ( const char * topic, long value, const char * name, bool retainP ) {
  const size_t valLen = 10;
  char val [ valLen ];
  snprintf ( val, valLen, "%ld", value );
  return sendValueToMQTT ( topic,  val,  name, retainP );
}

int sendValueToMQTT ( const char * topic, const char * value, const char * name, bool retainP ) {

  /*
  
    Send data via MQTT to mosquitto
    
  */
  
  if ( MQTTdisabledP ) {
    // we're not at Chuck's
    return 0;
  }

  int tLen = strlen ( topic );
  int vLen = strlen ( value );
  if ( vLen < 1 ) {
    if ( VERBOSE >= 10 ) {
      Serial.print ( F ( "\nNot sending null payload " ) );
      Serial.println ( name );
    }
    return 0;
  }
  
  if ( ( tLen + vLen ) > MQTT_MAX_PACKET_SIZE ) {
    if ( VERBOSE >= 10 ) {
      Serial.print ( F ( "\nNot sending oversized ( " ) );
      Serial.print ( tLen + vLen );
      Serial.print ( F ( " > 128 ) packet " ) );
      Serial.println ( name );
    }
    return 0;
  }
  
  if ( VERBOSE >= 10 ) {
    Serial.print ( F ( "Sending " ) );
    Serial.print ( topic );
    Serial.print ( F ( " ( name = " ) );
    Serial.print ( name );
    Serial.print ( F ( ", len = " ) );
    Serial.print ( tLen + vLen );
    Serial.print ( F ( " ): " ) );
    Serial.print ( value );
    Serial.print ( F ( " ... " ) );
  }
  
  /*
  
    Note: each publish call seems to take about a second
    Even though connection remains good
    But only when the MQTT server is overtaxed - probably in need of restart!
  
  */

  bool success = conn_MQTT.publish ( topic, value, retainP );
  if ( success ) {
    if ( VERBOSE >= 10 ) Serial.println ( F ( "{ \"sendResult\": \"OK\" }" ) );
  } else {
    if ( VERBOSE >= 10 ) Serial.println ( F ( "{ \"sendResult\": \"failed\", \"reason\": \"Unknown\" }" ) );
  }
  
  return success ? ( tLen + vLen ) : -1;
};

// *****************************************************************************
// ********************************** serial ***********************************
// *****************************************************************************

bool handleSerialInput () {

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

  bool retain;
  
  /* 
    Respond to JSON commands like
    o { "command": "send", "topic": "seismo/piezo/energy", "value": "0.8" }
    o { "command": "send", "topic": "seismo/piezo/threshold", "value": "101" }
    o { "command": "send", "topic": "seismo/piezo/triggered", "value": "1" }     // expect 1 or 0
    
    o { "command": "send", "topic": "seismo/piezo/energy", "value": "<value>"[, "retain": "<value>"] }
    o { "command": "send", "topic": "seismo/piezo/threshold", "value": "<value>"[, "retain": "<value>"] }
    o { "command": "send", "topic": "seismo/piezo/triggered", "value": "<value>"[, "retain": "<value>"] }
    as if they were
    o { "command": "send", "topic": "seismo/19/piezo/energy", "value": "0.2" } etc.
  */
  
  #ifdef ECHO_INPUT_TO_UDP
    echoInput ( strBuffer );
  #endif
  
  // is the string valid JSON
  
  if ( strlen ( strBuffer ) ) {
    // there's at least a partial string     
    Serial.print ( "about to parse: " ); Serial.println ( strBuffer );
  }
    
  JsonDocument doc;
  DeserializationError error = deserializeJson ( doc, strBuffer );
  
  // Test if parsing succeeds.
  if (error) {
    Serial.println ( "Received string was not valid JSON" );
    Serial.println ( error.c_str() );
    return ( false );
  }
  
  // handle whatever it was
  
  if ( doc.containsKey ( "retain" ) ) {
    retain = doc [ "retain" ];
  } else {
    retain = false;
  }
  
  if ( doc.containsKey ( "command" ) ) {
      
    const char* cmd = doc [ "command" ];
    
    if (  ! strncmp ( cmd, "send", 5 ) ) {
    
      if ( strstr ( doc [ "topic" ], "seismo/piezo/" ) == NULL ) {
        // wrong topic
        Serial.print ( "JSON msg unhandled topic: ignored: " );
        Serial.println ( cmd );
        return false;
      }
    
      const int rtSize = 30;
      char revisedTopic [ rtSize ];
    
      if ( strstr ( doc [ "topic" ], "/energy" ) != NULL ) {
        energy = doc [ "value" ];
        EWMA_6m.record ( energy );
        EWMA_1h.record ( energy );

        snprintf ( revisedTopic, rtSize, "%s/energy", mqtt_baseTopic );
        sendValueToMQTT ( (const char *) revisedTopic,
                          energy,
                          (const char *) revisedTopic,
                         doc [ "retain" ] );
        strncpy ( lastMQTTmessage, doc [ "value" ], lastMQTTmessageLen );
      } else if ( strstr ( doc [ "topic" ], "/threshold" ) != NULL ) {
        threshold = doc [ "value" ];
        snprintf ( revisedTopic, rtSize, "%s/threshold", mqtt_baseTopic );
        sendValueToMQTT ( (const char *) revisedTopic,
                          threshold,
                          (const char *) revisedTopic,
                           doc [ "retain" ] );
        strncpy ( lastMQTTmessage, doc [ "value" ], lastMQTTmessageLen );
      } else if ( strstr ( doc [ "topic" ], "/triggered" ) != NULL ) {
        // expect a value of "0" or "1"
        triggered_P = ( (int) doc [ "value" ] ) != 0;
        snprintf ( revisedTopic, rtSize, "%s/triggered", mqtt_baseTopic );
        sendValueToMQTT ( (const char *) revisedTopic,
                          (const char *) triggered_P ? "1" : "0",
                          (const char *) revisedTopic,
                          doc [ "retain" ] );
        strncpy ( lastMQTTmessage, triggered_P ? "true" : "false", lastMQTTmessageLen );
      }
      strncpy ( lastMQTTtopic, revisedTopic, lastMQTTtopicLen );
      
    
      nSendings++;
      lastMQTTmessageAt_nts = now();
                      
    } else {
      Serial.print ( "JSON msg from Serial: bad command: " );
      Serial.println ( cmd );
    }
  } else {
    Serial.println ( "JSON string without command" );
  }
  
  yield ();
  return true;

}

// *****************************************************************************
// ********************************** HTML *************************************
// *****************************************************************************
  
void updateWebPage_root () {

#include "piezo_seismo_hub_web_page.cpp"

  yield ();

  htmlServer.send ( 200, "text/html", htmlMessage );
  
  if ( VERBOSE >= 25 ) Serial.println ( htmlMessage );

  #if defined TELEMETRY_ON
    snprintf ( topic, mqttTopicLen, "%s/%s/debug/html_message_length", PROGNAME, uniqueToken );
    sendValueToMQTT ( topic, strlen(htmlMessage), "html message length" );
  #endif

  if ( VERBOSE >= 20 ) {
    Serial.print ( F ( "done\n" ) );
  }
  
  forceWSUpdate = true;

}

void webSocketEvent ( uint8_t num, WStype_t type, uint8_t * payload, size_t length ) {

  // We got something from the web socket
  // Serial.printf ( "Event %d received\n", type );
  
	switch ( type ) {
		case WStype_ERROR:  // 0
			if ( VERBOSE >= 10 ) Serial.printf("[WSc] Error!\n");
			break;
		case WStype_DISCONNECTED:  // 1
			if ( VERBOSE >= 10 ) Serial.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED:  // 2
		  if ( length > 1 ) {
			  if ( VERBOSE >= 10 ) Serial.printf ( "[WSc] %s: Connected to url: %s\n", num, payload );
	    // bool sendTXT ( uint8_t num, uint8_t * payload, size_t length = 0, bool headerToPayload = false );
        // webSocket.sendTXT ( num, "Connected", 9 );
      } else {
        if ( VERBOSE >= 10 ) Serial.printf ( "[WSc] CONNECTED message length 1 (empty!)\n" );
      }
			break;
		case WStype_TEXT:  // 3
		  if ( length > 1 ) {
			  if ( VERBOSE >= 10 ) Serial.printf ( "[WSc] get text: %s\n", payload );
			  // send message to server
        // webSocket.sendTXT ( num, "ws text (non-JSON) message here", 32 );
        // if ( ! strncmp ( (const char *) payload, "OFF", 3 ) ) {
        //   // was off: turn on
        //   relayIsOn = 1;
        //   forceWSUpdate = true;
        // } else if ( ! strncmp ( (const char *) payload, "ON", 2 ) ) {
        //   relayIsOn = 0;
        //   forceWSUpdate = true;
        // } else {
        //   if ( VERBOSE >= 10 ) Serial.println ( F ( "Message not recognized" ) );
        // }
      } else {
        if ( VERBOSE >= 10 ) Serial.printf ( "[WSc] text message length 1 (empty!)\n" );
      }
			break;
		case WStype_BIN:  // 4
			if ( VERBOSE >= 10 ) Serial.printf("[WSc] get binary length: %u\n", length);
//      hexdump(payload, length);
			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
    case WStype_PING:  // 9?
      // pong will be send automatically
      if ( VERBOSE >= 10 ) Serial.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:  // 10
      // answer to a ping we send
      if ( VERBOSE >= 10 ) Serial.printf("[WSc] get pong\n");
      break;
    default:
      Serial.printf ( "[WSc] got unexpected event type %d\n", type );
      break;
  }
  
  // Serial.println ( F ( "Switch exited" ) );

}

void update_WebSocket () {

  // ArduinoJSON v.7.x
  // https://arduinojson.org/v7/how-to/upgrade-from-v6/

  JsonDocument doc;

  doc["PROGNAME"] = PROGNAME;
  doc["VERSION"] = VERSION;
  doc["VERDATE"] = VERDATE;  
  doc["UNIQUE_TOKEN"] = uniqueToken;

  char ipString[25];
  snprintf ( ipString, 25, "%u.%u.%u.%u", Network.ip[0], Network.ip[1], Network.ip[2], Network.ip[3] );
  doc["IP_STRING"] = ipString;
  doc["NETWORK_SSID"] = Network.ssid;
  doc["OFFERED_SSID"] = AP_SSID;
  doc["MDNS_ID"] = mdnsOtaId;

  if ( timeStatus() == timeSet ) {
    formatTimeString ( timeString, timeStringLen, now() );
  } else {
    snprintf ( timeString, timeStringLen, "&lt;time not set&gt;" );
  }

  doc["TIME"] = timeString;
  
  doc["ENERGY"] = energy;
	doc["EWMA_6m"] = EWMA_6m.value();
	doc["EWMA_1h"] = EWMA_1h.value();

  doc["THRESHOLD"] = threshold;
  doc["TRIGGERED"] = triggered_P ? "1" : "0";
    
  doc["MQTT_TOPIC"] = mqtt_clientID;
  doc["LAST_BOOT_AT"] = bootTimeString;

  // JsonArray dataArray = doc.createNestedArray("data");
  // const size_t pBufLen = 32;
  // char pBuf [ pBufLen ];
  
  serializeJson ( doc, jsonString );
    
  #if 0 && defined ( TELEMETRY_ON )
    snprintf ( pBuf, pBufLen, "%s/%s/debug/update_Websocket_JSON_string_len", PROGNAME, uniqueToken );
    sendValueToMQTT ( pBuf, strlen ( jsonString ), "update_Websocket JSON string len", MQTT_RETAIN );
  #endif

  if ( VERBOSE >= 20 )
    Serial.printf ( "doc size %d and JSON string length %d\n", doc.size(), measureJson ( doc ) );
          
  if ( VERBOSE >= 15 ) Serial.println ( jsonString );

  // webSocket.sendTXT ( 0, jsonString, strlen ( jsonString ) );  // both work
  webSocket.broadcastTXT ( jsonString, strlen ( jsonString ) );  // both work

  
  if ( VERBOSE >= 15 ) Serial.print ( F ("  ... sent\n" ) );
  
  if ( ( VERBOSE >= 15 ) && forceWSUpdate ) Serial.println ( F ( "forced WS update completing" ) );

  forceWSUpdate = false;

}

// *****************************************************************************
// ********************************** util *************************************
// *****************************************************************************

void indicateConnecting ( int value ) {
  #if defined ( pdConnecting )
    // 1 turns on; 0 turns off; -1 inverts
    // inverse true iff pdConnecting is inverse logic
    bool inverse = true;
    if ( value == -1 ) {
      value = 1 - digitalRead ( pdConnecting );
    } else {
      value != inverse;
    }
    digitalWrite ( pdConnecting, value != inverse );
  #endif
}

void reportSystemStatus() {
  snprintf ( topic, mqttTopicLen, "%s/telemetry/debug/heap_free", mqtt_baseTopic );
  sendValueToMQTT ( topic, ESP.getFreeHeap(), "Free heap space", true );
}

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
