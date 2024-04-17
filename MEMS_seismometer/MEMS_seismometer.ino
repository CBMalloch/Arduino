#define PROGNAME "MEMS_seismometer"
#define VERSION "0.4.0" 
#define VERDATE "2024-04-16"
#define PROGMONIKER "SEISMO"

/* from LSM303DLH Example Code and /tests/_hardware_specific/accelerometer_magnetometer_LSM303DLH
   
   license: Creative commons share-alike v3.0
  
   Firmware:
   You can set the accelerometer's full-scale range by setting
   the ACCELERATION_FULL_SCALE_g constant to either 2, 4, or 8. This value is used
   in the init() function. For the most part, all other
   registers in the LSM303DHLC will be at their default value.
   
   Use the write() and read() functions to write
   to and read from the LSM303DHLC's internal registers.
   
   Use getAccel() to get the acceleration from the LSM303DHLC. 
   Pass each of those functions an array of floats, 
   where the data will be stored upon return from the void.
   
   Hardware:
   Only power and the two I2C lines are connected:
        Breakout -------------- Arduino
         1 Vdd                   5V
         14 Vdd                  5V
         7 GND                   GND
         2 SCL                   A5
         3 SDA                   A4
         
  New approach, since I question the validity of the data, being sampled at 
  intervals of 14ms (75Hz or so). Let's try sampling at 1000Hz for 250ms every second.
  Looks like the maximum iteration speed is 390Hz
                  
  Updates:
    2017-03-04 cbm for use with NodeMCU ESP8266 device
    2024-04-01 cbm 0.1.0 converting to work as seismometer
                         adding calculations for normal dist / sigma values
    2024-04-02 cbm 0.2.0 merging in ESP8266_basic
    2024-04-12 cbm 0.3.0 sample at 1000Hz for 250ms every second
    2024-04-16 cbm 0.3.6 charting
    
*/



// Note: also including <Ethernet.h> broke WiFi.connect!
#include <ESP8266WiFi.h>                  //https://github.com/esp8266/Arduino
// #include <DNSServer.h>
#include <ArduinoJson.h>                  //https://github.com/bblanchon/ArduinoJson

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

// #include <LITTLEFS.h>

#if ARDUINO_ESP8266_GENERIC
  // if it's an ESP-01, we need to define the pins for I2C
  #include <Wire.h>
#endif

#include <math.h>

#include <cbmThrobber.h>
#include <cbmNTP.h>
#include <cbmNetworkInfo.h>
#include <cbmCircularBuffer.h>  

#include <LSM303DLH.h>

#include <EWMA.h>

#include <Stats.h>

#define SECOND_ms  ( 1000UL )
#define MINUTE_ms  ( 60UL * SECOND_ms )
#define HOUR_ms    ( 60UL * MINUTE_ms )

#define DEVICE_ESP01

#define ACCELERATION_FULL_SCALE_g 2
#define SA0_BRIDGE_VALUE 0

// ***************************************
// ***************************************
#pragma mark -> vars MAIN PARAMETERS

#define BAUDRATE 115200
#define VERBOSE 2
#define TESTING false

// either define ( to enable ) or undef
#define USE_GENERIC_MDNS_NAME
#define ALLOW_PRINTING_OF_PASSWORD false

#define USE_WEBSOCKETS
#undef DO_BROADCAST
#undef DO_CONVERSATION

#undef WEBPAGE_FROM_LITTLEFS

const unsigned int port_UDP = 9250;

#define mqtt_basebaseTopic "seismo"

const float low_cutoff = 0.01;
const float high_cutoff = 40.0;


// ***************************************
// ***************************************

/********************************** GPIO **************************************/
#pragma mark -> vars GPIO


/*
  I2C cbm standard colors: yellow for SDA, blue for SCL
*/

#define INVERSE_LOGIC_OFF 1
#define INVERSE_LOGIC_ON  0
  
#if defined ( DEVICE_ESP01 )
  // Red LED is power; no control over it
  // GPIO0 for SDA
  // GPIO1 ( TX ) is blue, inverse logic
  // GPIO2 (next to GND) for SCL; 1K pullup on my boards
  // GPIO3 ( RX )
  #undef pdConnecting
  #undef pdThrobber      
  // if it's an ESP-01, we need to define the pins for I2C
  // AND PASS THEM TO seismometer.init
  #define USE_GPIO_02_INSTEAD_OF_TX1RX3
  #ifdef USE_GPIO_02_INSTEAD_OF_TX1RX3
    const int pdSDA                =  0;
    const int pdSCL                =  2;
  #else
    const int pdSDA                =  1;  // TX
    const int pdSCL                =  3;  // RX
  #endif
  // //   Wire.begin ( pdSDA, pdSCL, I2C_MASTER );
  // Wire.begin ( pdSDA, pdSCL );
  const bool throbberIsInverted_P  = false;
  const uint8_t pdThrobber         =  -1;
#elif defined ( DEVICE_ESP01S )
  // No red power LED
  // GPIO0 ( SDA )
  // GPIO1 ( TX )
  // GPIO2 ( SCL ) (next to GND) is blue, inverse logic; 1K pullup on my boards
  // GPIO3 ( RX )
  const int pdConnecting           = 1;
      
  // if it's an ESP-01, we need to define the pins for I2C
  #define USE_GPIO_02_INSTEAD_OF_TX1RX3
  #ifdef USE_GPIO_02_INSTEAD_OF_TX1RX3
    const int pdSDA                =  0;
    const int pdSCL                =  2;
  #else
    const int pdSDA                =  1;  // TX
    const int pdSCL                =  3;  // RX
  #endif
  // //   Wire.begin ( pdSDA, pdSCL, I2C_MASTER );
  // Wire.begin ( pdSDA, pdSCL );
  #undef pdThrobber      
#elif defined ( DEVICE_HUZZAH )
   // GPIO0 is red
   // GPIO2 is blue, inverse logic
   // GPIO4 is SDA; GPIO5 is SCL
  const bool throbberIsInverted_P  = false;
  const uint8_t pdThrobber         =  0;
  const int pdConnecting           =  2;
#elif defined  ( DEVICE_AMICA )
   // GPIO4 (D2) is SDA; GPIO5 (D1) is SCL
   // GPIO16 (D0) is red but this is usually used to drive reset 
   //   LOW to pin RST (CH_PD / chip_enable)
   // blue is GPIO1 TX conflicts with the use of Serial
   // see https://www.make-it.ca/nodemcu-details-specifications/
   // specify the GPIO number 
   // GPIOs 4, 12, 14, 15 are PWM
  const bool throbberIsInverted_P  = false;
  const uint8_t pdThrobber         =  12;  // GPIO12 is D6
  const bool throbberPWM_P         = true;
  #undef pdConnecting
#elif defined  ( DEVICE_SONOFF_PAGODA )
  // GPIO4 is red (after board modification) (inverse logic, hardware PWM)
  // GPIO13 is green (inverse logic, no PWM)
  // GPIO0 is the pushbutton (has pullup resistor)
  // GPIO12 is the relay output
  // GPIO14 is J1 Pin 5
  const bool throbberIsInverted_P  = false;
  const uint8_t pdThrobber         =  -1;
  #undef pdConnecting
#elif defined  ( DEVICE_SONOFF_BLOCK )
  // GPIO0 is the pushbutton (has pullup resistor)
  // GPIO12 is the relay output
  //   red LED associated with relay
  // GPIO13 is blue (inverse logic, PWM)
  const bool throbberIsInverted_P  = false;
  const uint8_t pdThrobber         =  -1;
  #undef pdConnecting
#elif defined  ( DEVICE_SONOFF_BULB )
  const bool throbberIsInverted_P  = false;
  const uint8_t pdThrobber         =  -1;
  #undef pdConnecting
#elif defined ( DEVICE_INKPLATE )
  const int pdThrobber             =  0;
  const int pdConnecting           =  2;
#endif

#ifdef pdThrobber
  //                         pin            inverted_P         PWM_P
  cbmThrobber throbber ( pdThrobber, throbberIsInverted_P, throbberPWM_P );
#endif

/********************************* Network ************************************/
#pragma mark -> vars Network

// locales include M5, CBMIoT, CBMDATACOL, CBMDDWRT, CBMDDWRT3, CBMDDWRT3GUEST,
//   CBMBELKIN, CBM_RASPI_MOSQUITTO
// need to use CBMDDWRT3 for my own network access
// can use CBMIoT or CBMDDWRT3GUEST for Sparkfun etc.

// need to use CBMDDWRT4 for my own network access
// maybe not CBMDATACOL
// can use CBMM5 or CBMDDWRT3GUEST for Sparkfun etc.
#define WIFI_LOCALE CBMDDWRT4

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

const size_t mqttClientIDLen                  = 50;
char mqtt_clientID [ mqttClientIDLen ]        = "<unset>";

const int mqttTopicLen                        = 50;
char mqtt_baseTopic [ mqttTopicLen ];
char mqttCmdTopic [ mqttTopicLen ];
char mqttStatusTopic [ mqttTopicLen ];
char mqttTimeoutTopic [ mqttTopicLen ];
char mqttEnergyTopic [ mqttTopicLen ];

const bool MQTT_RETAIN = true;

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

#ifdef WEBPAGE_FROM_LITTLEFS
  // otherwise will be defined by the subroutine-local HERE definition
  const size_t htmlMessageLen = 8192; 
  char htmlMessage [htmlMessageLen];    // must be global to avoid stack overflow!!!
#endif

/******************************* Global Vars **********************************/
#pragma mark -> vars global


char uniqueToken [ 9 ];
unsigned long lastRebootAt_nts                 = 0UL;

const size_t pBufLen = 128;
char pBuf [ pBufLen ];
const size_t fBufLen = 8;
char fBuf [ fBufLen ];

char topic [ mqttTopicLen ];

const int htmlMessageLen = 3072;
char htmlMessage [htmlMessageLen];
const int timeStringLen = 32;
char timeString [ timeStringLen ] = "<unset>";
char bootTimeString [ timeStringLen ] = "<unset>";

const size_t jsonStrSize = 1000; 
char jsonString [ jsonStrSize ];  // needs to be global to be on heap!

// clearLittleFS_P reflects the pulldown state of DIO pin pdForceNewConnection
bool clearLittleFS_P = false;

bool forceWSUpdate = false;

/**************************** App-Specific Vars *******************************/
#pragma mark -> vars app-specific

LSM303DLH accelerometer;
BPF filtered_energy;
EWMA peak_EWMA;
// Stats energyStats;
Stats loop_time_stats;

#pragma mark MM energy - num datapoints

const size_t chart_num_datapoints = 20;
CircularBuffer<float> energies ( chart_num_datapoints );

float calibrated_mean = 0.0;
float calibrated_stDev = 0.0;

float realEnergy[3], realMag[3];  // calculated acceleration values here

static float peakDuringInterval              =   0.0;
    
const float F_sampling = 62;  // high-speed: 389.7;
const float T_sampling = 1.0 / F_sampling;
// NOTE: needs checking!!!
// int loopDelay_ms = 5;  // at 0, get 1000 iterations in 6 seconds => 177Hz
const unsigned long loopDelay_ms = (unsigned long) 1000.0 * T_sampling;

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
void initializeLittleFS ();
void initializeWebServer ();

bool connect_WiFi ();

bool connect_MQTT ();
void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length );
int sendValueToMQTT ( const char * topic, long value, const char * name, bool retainP = false );
int sendValueToMQTT ( const char * topic, const char * value, const char * name, bool retainP = false );

int interpretNewCommandString ( char * theTopic, char * thePayload );
int readAndInterpretCommandString ( char * theString );

void updateWebPage_root ();
void webSocketEvent ( uint8_t num, WStype_t type, uint8_t * payload, size_t length );
void update_WebSocket ();
void update_WebSocketArray ();

float getTotalEnergy ( LSM303DLH accelerometer = accelerometer );
// void initializeStatistics ( unsigned long initializationPeriod_ms = 10 * SECOND_ms );

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
  initializeLittleFS ();
  initializeWebServer ();
  yield();
  
  accelerometer.init ( ACCELERATION_FULL_SCALE_g, SA0_BRIDGE_VALUE, pdSDA, pdSCL );
  
  filtered_energy.init ( low_cutoff, high_cutoff, F_sampling );

  peak_EWMA.setAlpha ( peak_EWMA.alpha ( 1000 ) );  // n periods of half-life

  #if VERBOSE > 100
  
    /************************** Report successful init **************************/

    Serial.println();
    snprintf ( pBuf, pBufLen, "%s v%s %s", PROGNAME, VERSION, VERDATE );
    Serial.printf ( "\n%s cbm\n", pBuf );
    snprintf ( topic, mqttTopicLen, "firmware/%s", uniqueToken );
    sendValueToMQTT ( topic, pBuf, "firmware" );
  
    #ifdef TESTING
      Serial.printf ( " TESTING level %d\n", TESTING );
    #else
      Serial.println ();
    #endif
    Serial.printf ( "Compiled at %s on %s\n\n\n", __TIME__, __DATE__ );
    
  #endif
  
  // titles for graph
  Serial.println ( "filtered_energy.value()  peakDuringInterval  peak_EWMA" );

}

void loop() {

  const unsigned long wsSendInterval_ms        =   5UL * SECOND_ms;
  static unsigned long lastWSSendAt_ms         =   0UL;

  const unsigned long statusDelay_ms           =   5UL * MINUTE_ms;
  static unsigned long lastStatusAt_ms         =   0UL;
        
  const unsigned long timeCheckDelay_ms        =   5UL * MINUTE_ms;
  static unsigned long timeCheckStatusAt_ms    =   0UL;

  const unsigned long peakHoldInterval_ms      =   1UL * MINUTE_ms;
  static unsigned long newPeakAt_ms            =   0UL;
  
  static unsigned long lastLoopAt_ms           =   millis();
  
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
  
  #ifdef USE_WEBSOCKETS
  
    webSocket.loop();
    yield();
      
    /***********/
  
    if ( ( forceWSUpdate ) || ( lastWSSendAt_ms == 0UL ) 
        || ( ( millis() - lastWSSendAt_ms ) > wsSendInterval_ms ) ) {
      
      if ( ( VERBOSE >= 15 ) && forceWSUpdate ) Serial.println ( F ( "forced WS update" ) );

      update_WebSocket ();
      update_WebSocketArray ();
      lastWSSendAt_ms = millis();

    }
  
    yield ();
  #endif

  /****************************************************************************/  
   
  if ( ( millis() - lastStatusAt_ms ) > statusDelay_ms ) {
    reportSystemStatus();
    lastStatusAt_ms = millis();
  }

  /****************************************************************************/

  #undef HIGH_SPEED_SAMPLING

  #ifdef HIGH_SPEED_SAMPLING

    // measure for the first quarter of every second
  
    if ( ( millis() % 1000 ) < 250UL ) {
      // we're within the first quarter of a second...
      // WAH max speed seems to be only 99 iters per quarter-second
    
      const unsigned long highSpeedDuration_ms = 250UL;
      unsigned long highSpeedBeganAt_ms = millis();
      unsigned long highSpeedIterationCount = 0UL;
    
      while ( ( millis() - highSpeedBeganAt_ms ) < highSpeedDuration_ms ) {
      
        float totalEnergy;
        // total energy is the *square* resultant of 3 axes, including gravity
        if ( TESTING ) {
          totalEnergy = 5.0 * cos ( (float) highSpeedIterationCount * 0.02 );
        } else {
          totalEnergy = getTotalEnergy ( accelerometer );
        }

        // we *could* turn it into m/s acceleration, but maybe we want energy...
        // totalEnergy = sqrt ( totalEnergy ) * float ( ACCELERATION_FULL_SCALE_g ) * 9.8;

        // Serial.print ( "  ==> " ); Serial.println ( totalEnergy );

        // remove gravitational energy to get net acceleration of the ground
        // filtered_energy MAY be negative!!

        // multiply for better MQTT scaling
        filtered_energy.record ( totalEnergy * 1e2 );

        // energyStats.record ( filtered_energy.value() );  // of a wave, probably not appropriate...
      
        highSpeedIterationCount++;
      }
    
      snprintf ( topic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "telemetry/millis" );
      sendValueToMQTT (  topic, millis(), "millis" );
      snprintf ( topic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "telemetry/high_speed_iteration_count" );
      sendValueToMQTT (  topic, highSpeedIterationCount, "iterations" );
      snprintf ( topic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "telemetry/high_speed_frequency" );
      snprintf ( fBuf, fBufLen, "%6.1f", 1000.0 * highSpeedIterationCount / ( millis() - highSpeedBeganAt_ms ) );
      sendValueToMQTT ( topic, fBuf , "frequency" );
      delay ( 2 );
    }

  #else

    float totalEnergy;
    // total energy is the *square* resultant of 3 axes, including gravity
    totalEnergy = getTotalEnergy ( accelerometer );
    
    // we *could* turn it into m/s acceleration, but maybe we want energy...
    // totalEnergy = sqrt ( totalEnergy ) * float ( ACCELERATION_FULL_SCALE_g ) * 9.8;
    
    // Serial.print ( "  ==> " ); Serial.println ( totalEnergy );
    
    // remove gravitational energy to get net acceleration of the ground
    // filtered_energy MAY be negative!!
    
    // multiply for better MQTT scaling
    filtered_energy.record ( totalEnergy * 1e2 );
      
    // energyStats.record ( filtered_energy.value() );  // of a wave, probably not appropriate...
    
  #endif
  
  // DISPLAY: update smooth peak value using EWMA
  float sqEnergy = filtered_energy.value() * filtered_energy.value();
  if ( sqEnergy > peak_EWMA.value() ) {
    peak_EWMA.reset();  // to latch the high value
  }
  peak_EWMA.record ( sqEnergy );
  
  const unsigned long sendInterval_ms = 2UL * SECOND_ms;
  static unsigned long lastSendAt_ms = millis();
  if ( ( millis() - lastSendAt_ms ) > sendInterval_ms ) {
    snprintf ( pBuf, pBufLen, "%6.4f", peak_EWMA.value() );
    sendValueToMQTT ( mqttEnergyTopic, pBuf, "EWMA_PeakEnergy" );
    lastSendAt_ms = millis();
  }
  
  #pragma mark MM energy - load
  const unsigned long recordInterval_ms = 1UL * SECOND_ms;
  static unsigned long lastRecordAt_ms = millis();
  if ( ( millis() - lastRecordAt_ms ) > recordInterval_ms ) {
    energies.store ( peak_EWMA.value() );
    lastRecordAt_ms = millis();
  }
  
  // if ( totalEnergy < 0
  //   || filtered_energy.value() < 0
  //   || peak_EWMA.value() < 0 ) {
  //   snprintf ( pBuf, pBufLen, "%6.4f", totalEnergy );
  //   sendValueToMQTT ( mqtt_baseTopic, "telemetry/debug/totalEnergy", pBuf, "totalEnergy", 1 );
  //   snprintf ( pBuf, pBufLen, "%6.4f", filtered_energy.value() );
  //   sendValueToMQTT ( mqtt_baseTopic, "telemetry/debug/filteredEnergy", pBuf, "filteredEnergy", 1 );
  //   snprintf ( pBuf, pBufLen, "%6.4f", peak_EWMA.value() );
  //   sendValueToMQTT ( mqtt_baseTopic, "telemetry/debug/peakEWMA", pBuf, "peakEWMA", 1 );
  //   Serial.println ( "********************************************************" );
  // }
  
  
  // REPORTING: monitor peak during protracted interval = reporting period
  // REPORTING means for web page display
  
  if ( filtered_energy.value() > peakDuringInterval ) {
    peakDuringInterval = filtered_energy.value();
  }
  // if ( ( millis() - newPeakAt_ms ) > peakHoldInterval_ms ) {
  //   // Serial.println ( peakDuringInterval );
  //   peakDuringInterval = 0UL;
  //   newPeakAt_ms = millis();
  // }
  
  if ( VERBOSE >= 4 ) {
    // main printing for Serial monitor
    Serial.print ( filtered_energy.value() ); Serial.print ( "   " ); 
    Serial.print ( peakDuringInterval ); Serial.print ( "   " ); 
    Serial.print ( peak_EWMA.value() );
    Serial.println ();
  }
  
  // Serial.print ( "Acceleration " ); printAltAz ( realAccel );
  // Serial.print ( "Magnetic field " ); printAltAz ( realMag );

  // float uncorrectedHeading = accelerometer.getHeading( realMag );
  // float correctedHeading   = accelerometer.getTiltHeading ( realMag, realAccel );
  /* print both the level, and tilt-compensated headings below to compare */
  // Serial.print ( getHeading( realMag ), 3 );  // this only works if the sensor is level
  // Serial.print ( "\t\t" );  // print some tabs
  // Serial.println ( getTiltHeading ( realMag, realAccel ), 3 );  // see how awesome tilt compensation is?!
  
  // Serial.print ( "\n\n\n++++++++++++++++++++++++++++++++++++++++\n\n" );
  
  // unsigned long debounceTimer_ms;
  // debounceTimer_ms = millis();
  // while ( ( millis() - debounceTimer_ms ) < 250 ) {
  //   if ( ! digitalRead ( pdPB ) ) {
  //     debounceTimer_ms = millis();
  //   }
  // }
  
  unsigned long now = millis();
  unsigned long lastLoopTook_ms = now - lastLoopAt_ms;
  lastLoopAt_ms = now;
  
  static int loopInitializationCount = 20;
  
  if ( loopInitializationCount == 0 ) {
    loop_time_stats.record ( lastLoopTook_ms );
  } else {
    // skip the first few loops until things stabilize
    loopInitializationCount--;
  }
  
  const unsigned long loopReportInterval_ms = 2UL * SECOND_ms;
  static unsigned long lastLoopReportAt_ms = millis();
  if ( ( millis() - lastLoopReportAt_ms ) > loopReportInterval_ms ) {
    snprintf ( pBuf, pBufLen, "%s/%s", mqtt_baseTopic, "telemetry/loop_time_ms" );
    sendValueToMQTT ( pBuf, lastLoopTook_ms, "loop time", false );
    lastLoopReportAt_ms = millis();
  }

  if ( loopDelay_ms > lastLoopTook_ms ) {
    delay ( loopDelay_ms - lastLoopTook_ms );
  } else {
    const unsigned long printInterval_ms = 5UL * SECOND_ms;
    static unsigned long printedAt_ms = millis();
    if ( ( millis() - printedAt_ms ) > printInterval_ms ) {
      Serial.printf ( "Loop took excessive time: %lu > %lu\n", lastLoopTook_ms, loopDelay_ms );
      printedAt_ms = millis();
    }
    // and don't delay any more...
  }
  
}

float getTotalEnergy ( LSM303DLH accelerometer ) {
  float realEnergy [ 3 ];
  float totalEnergy = 0.0;
  
  // the three values are calibrated and in m/s2 units
  accelerometer.getAccel ( realEnergy );
  
  for ( int i = 0; i < 3; i++ ) {
    totalEnergy += realEnergy[i] * realEnergy[i];
  }
  
  // totalEnergy is proportional to energy
  return totalEnergy;  
}

// void initializeStatistics ( unsigned long initializationPeriod_ms ) {
//   unsigned long initializationBeganAt_ms = millis();
//   energyStats.reset();
//   
//   while ( ( millis() - initializationBeganAt_ms ) < initializationPeriod_ms ) {
//     energyStats.record ( static_cast<double> ( getTotalEnergy ( accelerometer ) ) );
//   }
//   
//   calibrated_mean = energyStats.mean();
//   calibrated_stDev = energyStats.stDev();
// }


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

  Serial.begin ( BAUDRATE ); 
  while ( !Serial && ( millis() < 5000 ) ) {
    indicateConnecting ( -1 );
    delay ( 200 );  // wait a little more, if necessary, for Serial to come up
    yield ();
  }
  Serial.print ( F ( "\n\nGPIO initialized\n" ) );

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
  
  WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
  WiFi.begin ( Network.ssid, Network.password );
  
  if ( ( VERBOSE >= 4 ) ) {
    Serial.printf ( "WiFi parameters: %u.%u.%u.%u gw %u.%u.%u.%u mask %u.%u.%u.%u dns %u.%u.%u.%u\n",
       Network.ip   [ 0 ], Network.ip   [ 1 ], Network.ip   [ 2 ], Network.ip   [ 3 ],
       Network.gw   [ 0 ], Network.gw   [ 1 ], Network.gw   [ 2 ], Network.gw   [ 3 ],
       Network.mask [ 0 ], Network.mask [ 1 ], Network.mask [ 2 ], Network.mask [ 3 ],
       Network.dns  [ 0 ], Network.dns  [ 1 ], Network.dns  [ 2 ], Network.dns  [ 3 ]  );
  }

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
    Serial.print ( F ( "t" ) );
    delay ( 100 );
    // yield();
  }
  Serial.println ();
  
}

bool initializeMQTT () {

  if ( ! strncmp ( uniqueToken, "<unset>", 9 ) ) {
    Serial.println ( F ( "ERROR: uniqueToken not yet set!" ) );
    while ( 1 );
  }
  snprintf ( mqtt_clientID, mqttClientIDLen, "%s_%s", PROGNAME, uniqueToken );
  
  snprintf ( mqtt_baseTopic, mqttTopicLen, "%s/%s", mqtt_basebaseTopic, uniqueToken );
  snprintf ( mqttCmdTopic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "command" );
  snprintf ( mqttStatusTopic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "status" );
  snprintf ( mqttTimeoutTopic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "timeout_ms" );
  snprintf ( mqttEnergyTopic, mqttTopicLen, "%s/%s", mqtt_baseTopic, "EWMA_peak_energy" );
  
  conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  
  
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
    Serial.printf ( "Startup recorded at %s\n", bootTimeString );
  } else {
    Serial.println ( F ( "WARNING: No NTP response within 30 seconds..." ) );
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
      Serial.println ( F ("OTA Start") );
    });
  
    ArduinoOTA.onEnd([]() {
      Serial.println ( F ("\nOTA End") );
    });
  
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
           if (error == OTA_AUTH_ERROR)    Serial.println ( F ("OTA Auth Failed") );
      else if (error == OTA_BEGIN_ERROR)   Serial.println ( F ("OTA Begin Failed") );
      else if (error == OTA_CONNECT_ERROR) Serial.println ( F ("OTA Connect Failed") );
      else if (error == OTA_RECEIVE_ERROR) Serial.println ( F ("OTA Receive Failed") );
      else if (error == OTA_END_ERROR)     Serial.println ( F ("OTA End Failed") );
    });
  
    ArduinoOTA.begin();
    Serial.printf ( "ArduinoOTA running with host name %s\n", mdnsOtaId );
    
    yield();
    
  #endif
}

void initializeLittleFS () {
#ifdef WEBPAGE_FROM_LITTLEFS

  if ( clearLittleFS_P ) {
    Serial.print ( F ("clearing FS...") );
    if ( LITTLEFS.format() ) {
      Serial.println ( F (" ok") );
    } else {
      Serial.println ( F (" FAILED") );
    }
    yield();
  }
  
  LITTLEFS.begin();
  
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
  
  #ifdef USE_WEBSOCKETS
    webSocket.begin();
    webSocket.onEvent ( webSocketEvent );
    // the below are applicable to a WebSocket CLIENT...
    // webSocket.setReconnectInterval ( 15000 );  // ms to reconnect after failure
    // ms; <ping interval> <response timeout> <disconnect if n failures>
    // webSocket.enableHeartbeat ( 30000, 3000, 2 );
  #endif
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
  
  Serial.print ( F ( "\nconnect_WiFi: connecting to network '" ) ); Serial.print ( Network.ssid );
  Serial.printf ( "' at IP address " );
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
      Serial.print ( WiFi.status() );
      if ( ++nDots % 80 == 0 ) Serial.print ( F ( "\n" ) );
      lastDotAt_ms = millis();
    }
    
    if ( ( millis() - lastBlinkAt_ms ) > blinkPeriod_ms ) {
      indicateConnecting ( -1 );
      lastBlinkAt_ms = millis();
    }
    
    if ( ( millis() - startTime_ms ) > connectionTimeoutPeriod_ms ) {
      Serial.println ( F ( "connect_WiFi: connection timeout!" ) );
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
  conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  
  yield ();

  newConnection = false;
  indicateConnecting ( 0 );
  
  return true;
}

void handleReceivedMQTTMessage ( char * topic, byte * payload, unsigned int length ) {

  const size_t pBufLen = 128;
  char pBuf [ pBufLen ];
  memcpy ( pBuf, payload, length );
  pBuf [ length ] = '\0';
  
  if ( VERBOSE >= 8 ) {
    Serial.print ( F ( "Got: " ) );
    Serial.print ( topic );
    Serial.print ( F ( ": " ) );
    Serial.print ( pBuf );
    Serial.println ();
  }
        
  int retval = interpretNewCommandString ( topic, pBuf );
  if ( retval < 0 ) {
    Serial.print ( F ( "  --> Bad return from iNCS: " ) ); Serial.println ( retval );
    return;
  }
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

int interpretNewCommandString ( char * theTopic, char * thePayload ) {

  // returns a negative value on error
  
  int theIndex = -1000;
  
  char buf [ 20 ];
  size_t lenPrefix = strlen ( mqttCmdTopic );
  
  if ( strncmp ( theTopic, mqttCmdTopic, lenPrefix ) ) return ( -1 );
  char * theRest = theTopic + lenPrefix;
  
  Serial.print ( F ( "theRest: '" ) ); Serial.print ( theRest ); Serial.println ( F ( "'" ) );
  Serial.print ( F ( "thePayload: '" ) ); Serial.print ( thePayload ); Serial.println ( F ( "'" ) );
  
  // if ( ! strncmp ( theRest, "/update_interval_s", strlen ( theTopic ) - lenPrefix ) ) {
  //   // what remains should now be an integer
  //   theIndex = atoi ( thePayload );
  //   timedScreenUpdateInterval_ms = theIndex * 1000;  // value was in seconds
  // }
  
  theIndex = 0;

  // setRelay ( theIndex );
  // don't tell MQTT - *it* told *us*
    
  return ( theIndex );
}

#ifdef DO_CONVERSATION
int readAndInterpretCommandString ( char * theString ) {
  
  const int LOCALVERBOSE = 1;
  const int MAXPOSSIBLELINELENGTH = 2 + 12 * 7;
  const int MAXPOSSIBLESTRINGLENGTH = ( MAXPOSSIBLELINELENGTH + 2 ) * nPixels;
  int len = strnlen ( theString, MAXPOSSIBLESTRINGLENGTH );
  int nLinesRead = 0;
  
  if ( len <= 0 || len >= MAXPOSSIBLESTRINGLENGTH ) {
    if ( VERBOSE >= 8 || LOCALVERBOSE >= 6 ) {
      Serial.printf ( "message length %d >= max %d\n", len, MAXPOSSIBLESTRINGLENGTH );
    }
    return ( -1 );
  }
  
  if ( VERBOSE >= 12 || LOCALVERBOSE >= 6 ) {
    Serial.printf ( "full msg: \n%s\n", theString );
  }
    
  char line_separator[] = "\n";
  char * p_lineEnd;
  char * one_line = strtok_r ( theString, line_separator, &p_lineEnd );
  
  while ( one_line != NULL ) {
    int theIndex = 0;
    int nPieces = 0;
    uint32_t temp [ maxPhases ];
    
    if ( VERBOSE >= 12 || LOCALVERBOSE >= 6 ) {
      Serial.printf ( "line: %s\n", one_line );
    }

  
    char word_separator[] = " ";
    char * p_wordEnd;
    char * one_word = strtok_r ( one_line, word_separator, &p_wordEnd );
    
    while ( one_word != NULL ) {
      if ( strnlen ( one_word, MAXPOSSIBLELINELENGTH ) == 8 ) {
        // old-style iiRRGGBB format, one piece
        char ti [ 3 ], col [ 7 ];
        strncpy ( ti, one_word, 2 ); ti [ 2 ] = '\0';
        theIndex = strtol ( ti, NULL, 16 );
        strncpy ( col, one_word + 2, 6 ); col [ 6 ] = '\0';
        temp [ 0 ] = strtoul ( col, NULL, 16 );
        if ( VERBOSE >= 12 || LOCALVERBOSE >= 6 ) {
          Serial.printf ( "Old-style message: %s\n  index: %d; remaining string: %s\n", 
            one_word, theIndex, col );
        }
        nPieces++;
        
      } else {
        // modern style. parse the pieces
        // the first word is the pixel index
        theIndex = strtol ( one_word, NULL, 10 );
        if ( VERBOSE >= 12 || LOCALVERBOSE >= 6 ) {
          Serial.printf ( "pixel index: %s; index: %d\n", one_word, theIndex );
        }
        one_word = strtok_r ( NULL, word_separator, &p_wordEnd );        // grab the next piece
        
        while ( one_word != NULL ) {
          // the remaining words are the colors
          if ( VERBOSE >= 12 || LOCALVERBOSE >= 6 ) Serial.printf ( "color: %s\n", one_word );
          temp [ nPieces ] = strtoul ( one_word, NULL, 16 );
          nPieces++;
          one_word = strtok_r ( NULL, word_separator, &p_wordEnd );
        }
      }  // modern style line = space delimited list
        
      if ( theIndex < 0 || theIndex > nPixels ) return ( -2 );
      // now we know the number of pieces, we can set up the color vector
      
      int theQuotient = maxPhases / nPieces;
      if ( theQuotient * nPieces == maxPhases ) {
        // even divisor - valid!
        // each value should be replicated theQuotient times
        int m = 0;
        for ( int j = 0; j < nPieces; j++ ) {
          for ( int k = 0; k < theQuotient; k++ ) {
            pixelColorSettings [ theIndex ] [ m ] = temp [ j ];
            m++;
          }
        }
      } else {
        // error: the number of pieces doesn't divide maxPhases
        Serial.print ( F ( "  --> ERROR: the number of pieces (" ) ); Serial.print ( nPieces );
        Serial.print ( F ( ") doesn't divide evenly into the number of phases (" ) ); Serial.print ( maxPhases );
        Serial.println ( F ( ")!" ) );
        for ( int j = 0; j < maxPhases; j++ ) pixelColorSettings [ theIndex ] [ j ] = 0x000000;
        return ( -3 );
      }
      
      one_word = strtok_r ( NULL, word_separator, &p_wordEnd );

    }  // looping through each word
    
    nLinesRead++;
    one_line = strtok_r ( NULL, line_separator, &p_lineEnd );

  }  // looping through each line
  
  return ( nLinesRead );
}
#endif
// endif DO_CONVERSATION

// *****************************************************************************
// ********************************** HTML *************************************
// *****************************************************************************
  
void updateWebPage_root () {

  #ifdef WEBPAGE_FROM_LITTLEFS
  
    File f = LITTLEFS.open ( "/ESP_web_socket_page.html", "r" );
    if ( VERBOSE >= 10 ) Serial.printf ( "LITTLEFS file%s opened\n", f ? "" : " not" );
    if ( f ) {
      f.readString().toCharArray( htmlMessage, htmlMessageLen );
      if ( VERBOSE >= 10 ) Serial.printf ( "LITTLEFS file ESP_web_socket_page.html read\n" );
    } else {
      Serial.printf ( "LITTLEFS file ESP_web_socket_page.html not available" );
    }
    
  #else
  
    if ( VERBOSE >= 18 ) {
      Serial.print ( F ( "\nSetting htmlMessage from HERE\n" ) );
    }

    const char * htmlMessage = R"HTML(

      <!DOCTYPE html>
      <html>
        <head>
          <meta charset="UTF-8">
          <link referrerpolicy="no-referrer" rel="icon" href="http://miniTunes.cbmHome/favicon.ico">
          <style> 
            p{} .data { font-size: 24px; font-weight: bold; }
            .right { text-align: right; }
            table thead th { 
              font-weight: bold; 
              padding: 5px; 
              border-spacing: 5px; }
            thead { padding: 5px;
              background: #CFCFCF; 
              border-bottom: 3px solid #000000; }
            table { border: 2px solid black; }
            td {
                border: 1px solid gray; 
                padding: 5px; 
                border-spacing: 5px; }
            .bigbutton {
                font-size:18px;
                width:200px;
                height:60px;
                background: #c080c0; }
          </style>
          
          <script src="//d3js.org/d3.v4.js"></script>
          <script src="https://www.gstatic.com/charts/loader.js"></script>

          <script type="text/javascript">
      
            var ws;
            var wsUri = "ws:";
            var loc = window.location;
            if (loc.protocol === "https:") { wsUri = "wss:"; }
            // wsUri += "//" + loc.host + loc.pathname.replace("simple","ws/simple");
            wsUri += "//" + loc.host + ":81/";
            ws = new WebSocket ( wsUri );
  
            function wsConnect () {
    
              ws.onerror = function (error) {
                document.getElementById ( 'STATUS' ).innerHTML = "ERROR!!";
                document.getElementById ( 'STATUS' ).style = "background:pink; color:red";
                document.getElementById ("overlay").style.display = "block";
              };
              ws.onclose = function ( event ) {
                // event 1000 is clean; 1006 is "no close packet"
                if (event.wasClean) {
                  document.getElementById ( 'STATUS' ).innerHTML = "Connection cleanly closed";
                  // alert('[close] Connection closed cleanly, code=${event.code} reason=${event.reason}');
                } else {
                  // e.g. server process killed or network down
                  // event.code is usually 1006 in this case
                  document.getElementById ( 'STATUS' ).innerHTML = "Connection broken";
                  // alert('[close] Connection died');
                }
                document.getElementById ( 'STATUS' ).style = "background:none; color:red";
                document.getElementById ("overlay").style.display = "block";
                // in case of lost connection tries to reconnect every 3 secs
                setTimeout ( wsConnect, 3000 );
              }
              ws.onopen = function () {
                document.getElementById ( 'STATUS' ).innerHTML = "connected";
                document.getElementById ( 'STATUS' ).style = "background:none; color:black";
                ws.send ( "Open for data" );
              }
              ws.onmessage = function ( msg ) {

                document.getElementById ("overlay").style.display = "none";

                var data = msg.data;
                
                try {             
                  var jsonObj = JSON.parse(data);
                }
                catch(err) {
                  // alert ( err.message + '\n\n' + data );
                  return;
                }
                
                if ( jsonObj.hasOwnProperty ( 'PROGNAME' ) ) {
                  document.getElementById ( 'TITLE' ).innerHTML = ( jsonObj.UNIQUE_TOKEN ).concat ( " Seismometer" );
                  document.getElementById ( 'PROGNAME' ).innerHTML = jsonObj.PROGNAME;
                  document.getElementById ( 'VERSION' ).innerHTML = jsonObj.VERSION;
                  document.getElementById ( 'VERDATE' ).innerHTML = jsonObj.VERDATE;
                  document.getElementById ( 'UNIQUE_TOKEN' ).innerHTML = jsonObj.UNIQUE_TOKEN;
                  document.getElementById ( 'IP_STRING' ).innerHTML = jsonObj.IP_STRING;
                  document.getElementById ( 'NETWORK_SSID' ).innerHTML = jsonObj.NETWORK_SSID;
                  var temp = jsonObj.MDNS_ID;
                  document.getElementById ( 'MDNS_ID' ).innerHTML = 
                    ( ( temp == "" ) || ( temp == "<failure>" ) ) ? "<em>mDns failed</em>" : temp.concat ( ".local" );
                  document.getElementById ( 'TIME' ).innerHTML = jsonObj.TIME;
                  document.getElementById ( 'MQTT_TOPIC' ).innerHTML = jsonObj.MQTT_TOPIC;
                
                  document.getElementById ( 'LAST_BOOT_AT' ).innerHTML = jsonObj.LAST_BOOT_AT;
                
                  document.getElementById ( 'CUR_ENERGY' ).innerHTML = jsonObj.CUR_ENERGY;
                  document.getElementById ( 'PEAK_ENERGY' ).innerHTML = jsonObj.PEAK_ENERGY;

                }
                
                if ( jsonObj.hasOwnProperty ( 'PEAK_ENERGY_LIST' ) ) {
                  // now we expect an array of energy values
                  // ENERGY_FIRST_INDEX is timestamp of first value in table
                  // ENERGY_LIST_N is how many energies are populated in the list
                  energyChart ( jsonObj.ENERGY_FIRST_INDEX, jsonObj.PEAK_ENERGY_LIST, jsonObj.ENERGY_LIST_N );
                }
                
                                
              }  // ws.onmessage
              

            }  // wsConnect
  
            function pageUnload () {
              document.getElementById ( 'STATUS' ).innerHTML = "page unloaded";
              document.getElementById ( 'STATUS' ).style = "background:none; color:red";
              document.getElementById ( 'overlay' ).style.display = "block";
            }
            
            google.charts.load('current',{packages:['corechart']}).then(function () {
              // google.charts.setOnLoadCallback(doChart);
              data = new google.visualization.DataTable ({
                cols: [{id: 'Index', type: 'number'}, {id: 'Vibration', type: 'number'}]
              });
            });
            maxArrLen = 120;  // cannot use "const"
            

            function energyChart ( firstEnergy_sec, energyList, nValidEnergies ) {

/*
              arr = [ ['Index', 'Vibration'] ];
              for ( var i = 0; i < energyList.length; i++ ) {
                arr.push ( [ index, energyList [ i ] ] );
                index += 1;
              }
              
#pragma mark MM energy - HTML5

              document.getElementById ( 'ENERGY_LIST_LENGTH' ).innerHTML = energyList.length;
              document.getElementById ( 'ENERGY_ARRAY_LENGTH' ).innerHTML = arr.length;
              
              data = google.visualization.arrayToDataTable(arr);
*/

              // firstEnergy_sec is timestamp of first value in table
              // nValidEnergies is how many energies are populated in the list

/*


                  limit  
      data          │    
 ───────────────    │    
                    │    


            energyList     
           ────────────  

*/


              // firstIndex might be within the data table, or it might not
              // we might need to skip a part of the energyList
              // and prune the data table to make room for only the new data
              
              // but the energyList is zero-padded, so need to look at nValidEnergies
              // its indices run from firstIndex to firstIndex + nValidEnergies - 1
              
              if ( data.getNumberOfRows() == 0 ) {
                // just starting out...
                firstNewEnergyIndex = 0;
                newEnergyCount = nValidEnergies;
              } else {
              
                firstTable_sec = data.getValue ( 1, 0 );  // getValue(rowIndex, columnIndex)
                lastTable_sec = data.getValue ( data.getNumberOfRows() - 1, 0 );
                            
                // firstNewEnergyIndex is how many energies must we skip?
                // 75 + 1 - 76 or so    WRONG
                firstNewEnergyIndex = lastTable_sec + 1 - firstEnergy_sec;
                if ( firstNewEnergyIndex < 0 ) firstNewEnergyIndex = 0;
                // firstNewEnergyIndex will never be too big
                
              }
              
              //  80 - 0 or so
              newEnergyCount = nValidEnergies - firstNewEnergyIndex;
            
              // 76 + 0 or so
              newData_sec = firstEnergy_sec + firstNewEnergyIndex;
            
              supernumeraryRowCount = ( data.getNumberOfRows() + newEnergyCount ) - maxArrLen;
              if ( supernumeraryRowCount > 0) {
                data.removeRows ( 0, supernumeraryRowCount );
              }
              

              
              for ( var i = firstNewEnergyIndex; i < nValidEnergies; i++ ) {
                data.addRow ([ Number ( newData_sec ), Number ( energyList [ i ] ) ]);
                newData_sec += 1;
              }

              document.getElementById ( 'ENERGY_TABLE_LENGTH' ).innerHTML = data.getNumberOfRows();


            
              const options = {
                title: 'Vibration',
                // hAxis: {title: 'Square Meters'},
                // vAxis: {title: 'Price in Millions'},
                backgroundColor: '#fff8f8',
                colors:['red'],
                lineWidth: 4,
                legend: 'none'
              };

              const chart = new google.visualization.LineChart(document.getElementById('PLOT_DIV'));
              chart.draw(data, options);


            }

          </script>
    
          <title id="TITLE">Seismometer</title>

        </head>
      
      
        <body onload="wsConnect()" onunload="pageUnload()">

          <div id="overlay" style="
              position: fixed; /* Sit on top of the page content */
              display: block; /* shown by default */
              width: 100%; /* Full width (cover the whole page) */
              height: 100%; /* Full height (cover the whole page) */
              top: 0;
              left: 0;
              right: 0;
              bottom: 0;
              background-color: rgba(0,0,0,0.25); /* background color, with opacity */
              z-index: 2; /* Specify a stack order in case you're using a different order for other elements */
              cursor: pointer; /* Add a pointer on hover */
          ">  
            <span style="
              position: absolute;
              top: 50%;
              left: 50%;
              font-size: 50px;
              background: none;
              color: red;
              transform: translate(-50%,-50%);
              -ms-transform: translate(-50%,-50%);
            ">
              <span id="STATUS">initializing...</span>
            </span>
          </div> 

          <h1>Status: MEMS seismometer</h1>
            <h2><p>    Assignment: Test frequency range of MEMS seismometer</p></h2>
            <h3><span id="PROGNAME">..progname..
              </span>   v<span id="VERSION">..version..
              </span> <span id="VERDATE">..verdate..</span> cbm
            </h3>
            
        
            <h4>    Running on 
                      <span id="UNIQUE_TOKEN">unique_token</span> at 
                      <span id="IP_STRING">..ipstring..</span> / 
                      <span id="MDNS_ID">..mdns..</span> via access point 
                      <span id="NETWORK_SSID">..network_id..</span>
                    at
                      <span id="TIME">..time..</span>
            </h4>

          <br><br>MQTT topic: <em><span id="MQTT_TOPIC">..mqtt_topic..</span></em><br>
          
          <br>
          Current energy: <bold><span id="CUR_ENERGY">..current_energy..</span></bold><br>
          Peak energy: <bold><span id="PEAK_ENERGY">..peak_energy..</span></bold><br>
          Energy table length: <span id="ENERGY_TABLE_LENGTH">..energy_table_length..</span><br>
          
           <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js">
           </script> 
          
          
          
          <div id="PLOT_DIV">
          <canvas id="SEISMO_CHART" style="width:100%;max-width:700px"></canvas> 
          Here we would put a plot...
          </div>
              
          <br><br>Last boot at <span id="LAST_BOOT_AT">..last_boot..</span><br>

        </body>
      </html>
    )HTML";

  #endif

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

#ifdef USE_WEBSOCKETS

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
        webSocket.sendTXT ( num, "Connected", 9 );
      } else {
        if ( VERBOSE >= 10 ) Serial.printf ( "[WSc] CONNECTED message length 1 (empty!)\n" );
      }
			break;
		case WStype_TEXT:  // 3
		  if ( length > 1 ) {
			  if ( VERBOSE >= 10 ) Serial.printf ( "[WSc] get text: %s\n", payload );
			  // send message to server
        webSocket.sendTXT ( num, "ws text (non-JSON) message here", 32 );
        if ( ! strncmp ( (const char *) payload, "OFF", 3 ) ) {
          // was off: turn on
          forceWSUpdate = true;
        } else if ( ! strncmp ( (const char *) payload, "ON", 2 ) ) {
          forceWSUpdate = true;
        } else {
          if ( VERBOSE >= 10 ) Serial.println ( F ( "Message not recognized" ) );
        }
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
  doc["MDNS_ID"] = mdnsOtaId;

  if ( timeStatus() == timeSet ) {
    formatTimeString ( timeString, timeStringLen, now() );
  } else {
    snprintf ( timeString, timeStringLen, "&lt;time not set&gt;" );
  }
  
  snprintf ( pBuf, pBufLen, "%5.2f", filtered_energy.value() );
  doc["CUR_ENERGY"] = pBuf;
  snprintf ( pBuf, pBufLen, "%5.2f", peakDuringInterval );
  doc["PEAK_ENERGY"] = pBuf;
  peakDuringInterval = 0UL;

  doc["TIME"] = timeString;
  
  doc["MQTT_TOPIC"] = mqttCmdTopic;
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

void update_WebSocketArray () {

  // ArduinoJSON v.7.x
  // https://arduinojson.org/v7/how-to/upgrade-from-v6/
  
  JsonDocument doc;
  
  #pragma mark MM energy - WebSocket
  
  // timestamp of first value in table
  doc["ENERGY_FIRST_INDEX"] = energies.firstIndex();
  // how many energies are populated in the list
  doc["ENERGY_LIST_N"] = ( energies.count() < energies.bufSize() ) ? energies.count() : energies.bufSize();
  JsonArray energies_json = doc["PEAK_ENERGY_LIST"].to<JsonArray>();
  {
    // wah that we have to do it this way...
    float energies_linearArray [ chart_num_datapoints ];
    energies.entries ( energies_linearArray );


    for ( int i = 0; i < chart_num_datapoints; i++ ) {
      energies_json.add ( energies_linearArray [ i ] );
    }
  }
  
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
  
}

#endif

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

