#define PROGNAME "Backup_indicator"
#define VERSION "4.0.4"
#define VERDATE "2018-07-08"

/*
  Program backup_indicator
  Author  Charles B. Malloch, PhD
  Date    2015-06-10
  
  Program actuates a tri-color simple LED strip (no WS2012) with colors 
  to illuminate the backup target and indicate proximity.
  
  Range sensor measures range to back of car
  
  If we were using ultrasonic,
    trigger pulse 10uS min logic 1
    echo output duration is 58 * distance (cm) (us)
    
  But ultrasonic beam width is too wide to use effectively, so
  measure distance with Sharp infrared triangulation rangefinder

  As of version 3.0.0, I've added two things: a sensor switch to tell if the garage door 
  is closed, and an ESP8266 to send car-presence and door-closed data to MQTT on my RasPi.
  
  Do we send directly to Mosquitto using MQTT or should we interpret ASCII with Node-Red?
  Mosquitto, I think - why add an unnecessary layer of indirection? Make the Arduino send 
  value strings to the ESP, which will tack on the destination location and send it off.
  
  As of v4.x, we will be running ESP_text_to_MQTT.ino in the attached ESP-01, so we need to 
  manage it. Data will be sent to the ESP in JSON format. Examples:
  
  { "command": "setAssignment", "assignment": <assignment>, "retain": true }
  
  { "command": "connect", 
    "networkSSID": "cbmDataCollection", 
    "networkPW": "super_secret", 
    "MQTThost": "Mosquitto", 
    "MQTTuserID": "cbmalloch",
    "MQTTuserPW": "different_super_secret" }
    
  { "command": "send", 
    "topic": "cbmalloch/test", 
    "value": 12 }
    
  BEFORE WE MAKE THIS CHANGE, WE NEED TO LOOK-SEE IF THE ATmega328 has sufficient space 
  to accommodate the additional text data and the ArduinoJSON library.
  ... looks OK  ... *is* OK
  ... but these changes pushed beyond the capabilities of the ATmega168 I *had* been using,
      even programming directly without the Arduino bootloader presentß

  1.4 added blinking LED on pin 13;
  1.5 increased tolerance for "no motion"; decreased timeout
  2.0 slowed blinking during distance reporting; refactored
  2.0.1 decreased parkingStickyTime_ms from 10 min to 10 sec, set also in guiding range
  2.1.0 added reporting for Bluetooth Graphics app; time out lights when not moving
  3.0.0 added garage-door Hall-effect sensor, ESP-01 output
  3.0.3 moved zone transitions to tune and to apply to new car
  3.1.0 recognized that there were 5 zones, requiring only 4 boundaries
  3.1.1 tuning for new forward position of CR-V
  2017-02-16 3.2.0 cbm keep alive for 2 minutes after garage door opening
  2018-07-04 4.0.0 cbm change to accommodate ESP_text_to_MQTT.ino in attached ESP-01 device
             4.0.1 cbm add "assignment" command with retain ON
             4.0.2 cbm send at doubling intervals up to a max interval

*/



#include <ArduinoJson.h>

#include <Stats.h>
#include <cbmNetworkInfo.h>


// ***************************************
// ***************************************

const unsigned long BAUDRATE              =                 115200;
const int           VERBOSE               =                      2;
#undef              BLUETOOTH_GRAPH

/*
 distance thresholds:
   0: close enough to not be the garage door
   1: close enough to be in guide zone
   2: close enough to be in distance indicating zone
   3: close enough to be in parked zone
   4: too close
   closer: people walking by
*/

//                                                     gone | guiding | distanceIndicating | parked | tooClose
const float distanceThresholds [ ]                    = {  4.4,      2.7,                 1.7,     1.5 };
// const float distanceThresholds [ ]                    = { 1.0, 0.6, 0.3, 0.2 };      // testing
       
const float moveDistanceThreshold_m                   =                      0.080;
                
const unsigned long checkInterval_ms                  =                      200UL;
const unsigned long printInterval_ms                  =                      500UL;
const unsigned long graphInterval_ms                  =                      200UL;
      unsigned long blinkInterval_ms                  =                     1000UL;
const unsigned long moveSendInterval_ms               =                     1000UL;
const unsigned long watchdogInterval_ms               =              30UL * 1000UL;
                
// 9375ms, 18750ms, 37500ms, 75sec, 150sec, 5min, 10min, 20min, 40min, 80min -> 120min
const unsigned long sendDataToCloudInitialInterval_ms =                     9375UL;
const unsigned long sendDataToCloudMaxInterval_ms     = 2UL * 60UL * 60UL * 1000UL;

// keep awake after garage door opening                
const unsigned long keepAwakeInterval_ms              =        2UL * 60UL * 1000UL;
// time out lights after parked                
const unsigned long parkingTimeout_ms                 =              30UL * 1000UL;
const unsigned long parkingStickyTime_ms              =              10UL * 1000UL;
    
// ***************************************
// ***************************************

/********************************** GPIO **************************************/

const int ppRed       = 5;
const int ppGreen     = 9;
const int ppBlue      = 3;
       
const int paRange     = 0;
const int pdHallPwr   = 10;
const int pdHallGnd   = 11;
const int pdHallSense = 12;
const int pdThrobber  = 13;

/********************************* Network ************************************/

// The cbmNetworkInfo object has (at least) the following instance variables:
// .ip
// .gateway
// .mask
// .ssid
// .password
// .chipID    ( GUID assigned by manufacturer )
// .chipName  ( globally-unique name assigned by cbm and marked on PCB

cbmNetworkInfo Network;

// locales include M5, CBMIoT, CBMDATACOL, CBMDDWRT, CBMDDWRT3, CBMDDWRT3GUEST,
//   CBMBELKIN, CBM_RASPI_MOSQUITTO
#define WIFI_LOCALE CBMDATACOL

/********************************* MQTT ***************************************/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char * MQTT_FEED = "home/garage/+";

/******************************* Global Vars **********************************/

Stats counts_stats;

double distance_m;

enum { 
       dz_gone = 0,
       dz_guiding = 1,
       dz_distanceIndicating = 2,
       dz_parked = 3,
       dz_tooClose = 4
     };

/*
  0 - green / green
  1 - green / white
  2 - red / white
  3 - red / red
*/
const byte colorRed    [3] = { 0xff, 0x00, 0x00 };
const byte colorGreen  [3] = { 0x00, 0xff, 0x00 };
const byte colorBlue   [3] = { 0x00, 0x00, 0xff };
const byte colorYellow [3] = { 0x60, 0x50, 0x00 };
const byte colorCyan   [3] = { 0x00, 0x60, 0x60 };
const byte colorWhite  [3] = { 0x80, 0x80, 0x80 };
const byte colorBlack  [3] = { 0x00, 0x00, 0x00 };
float flashDC;
int flashPair, flashState; 
unsigned long lastFlashStartedAt_ms, flashPeriod_ms;

/************************** Function Prototypes *******************************/

void identifySelfToESP ();
void sendNetworkCredentialsToESP ();
void dispatchMQTTvalue ( const char * topic, float value, const char * name );
void dispatchMQTTvalue ( const char * topic, int value, const char * name );
void dispatchMQTTvalue ( const char * topic, char * value, const char * name );

/******************************************************************************/
/******************************************************************************/

void setup () {
  
  // note that the HC-06 (respond "linvorV1.8" to AT+VERSION\n) bluetooth modules 
  // come configured at some other baud rate; some 9600, some 57600, some 115200
  Serial.begin ( BAUDRATE );
    
  pinMode ( pdHallPwr,   OUTPUT       ); digitalWrite ( pdHallPwr, 1 );
  pinMode ( pdHallGnd,   OUTPUT       ); digitalWrite ( pdHallGnd, 0 );
  pinMode ( pdHallSense, INPUT_PULLUP );
  pinMode ( pdThrobber,  OUTPUT       );
  flashPeriod_ms = 1000UL;
  flashDC = 1.0;
  
  counts_stats.reset();
  
  identifySelfToESP ();

  // connect the ESP to the network
  sendNetworkCredentialsToESP ();
    
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  if ( distanceThresholds [ 0 ] < 2.0 ) Serial.println ( " TEST MODE " );
  
  for ( int i = 0; i < 3; i++ ) {
    setColor ( colorRed   );
    delay ( 500 );
    setColor ( colorGreen );
    delay ( 500 );
    setColor ( colorBlue );
    delay ( 500 );
  }
  setColor ( colorBlack );
  
}

void loop () {
  
  static unsigned long lastCheckAt_ms         = 0UL;
  static unsigned long lastPrintAt_ms         = 0UL;
  static unsigned long lastGraphAt_ms         = 0UL;
  static unsigned long lastFlashChangeAt_ms   = 0UL;
  static unsigned long lastBlinkAt_ms         = 0UL;
  static unsigned long lastStatusSentAt_ms    = 0UL;
  static unsigned long lastMoveSentAt_ms      = 0UL;
  static unsigned long doorOpenedAt_ms        = 0UL;
  static unsigned long lastWatchdogAt_ms      = 0UL;
  
  static unsigned long sendingInterval_ms     = sendDataToCloudInitialInterval_ms;
  
  unsigned long        flashDuration_ms     = 0UL;
                                            
  static float         staticDistance_m     = -100.0;
  static unsigned long lastMovedAt_ms       = millis();
  
  static int oldParkedP = 0;
  static int oldClosedP = 0;
  
  static float rangefinder_average_counts;
  static float rangefinder_v, rangefinder_pct;

  static int parkedP = 0;
  static int distanceZone = dz_gone;
  
  static int closedP = 0;
  
  // check / average every time through loop
  counts_stats.record ( double ( analogRead ( paRange ) ) );
  
  // update distance calculations
    
  if ( ( millis() - lastCheckAt_ms ) > checkInterval_ms ) {
  
    // voltage returned is inversely related to range within 100cm - 550cm
    distance_m = counts2distance_m ( counts_stats.mean() );
    
    // we are firmly parked if we are at parking distance (<distanceThresholds[2]) and lights have timed out (parkingTimeout_ms)
    // we are unparked if we've been beyond the distance indicating zone (> distanceThresholds[1]) for long enough (parkingStickyTime_ms)
  
    if ( distance_m > distanceThresholds [ 0 ] ) {                
      // cyan flashing e.g. > 4.4 m
      distanceZone = dz_gone;
      flashPair = 0;
      flashPeriod_ms = 1000UL;
      flashDC = 0.9;
      if ( ( millis() - lastMovedAt_ms ) > parkingStickyTime_ms ) {
        // gone for long enough to time out
        parkedP = 0;
      } 
    } else if ( distance_m > distanceThresholds [ 1 ] ) {         
      // blue e.g. 2.6-4.4 m
      distanceZone = dz_guiding;
      flashPair = 1;
      flashPeriod_ms = 400UL;
      flashDC = 0.75;
      if ( ( millis() - lastMovedAt_ms ) > parkingStickyTime_ms ) {
        // gone for long enough to time out
        parkedP = 0;
      } 
    } else if ( distance_m > distanceThresholds [ 2 ] ) {       
      // yellow / blue e.g. 1.6-2.6 m
      distanceZone = dz_distanceIndicating;
      flashPair = 2;
      // p will increase from 0.0 to 1.0 as distance_m decreased from distanceThresholds [ 1 ] to distanceThresholds [ 2 ]
      // so p indicates "urgency"
      float p = 1.0 - ( distance_m - distanceThresholds [ 2 ] ) / ( distanceThresholds [ 1 ] - distanceThresholds [ 2 ] );
      flashPeriod_ms = ( unsigned long ) ( linterp ( 2000.0, 200.0, p ) + 0.5 );
      flashDC = linterp ( 0.5, 0.8, p );
    } else if ( distance_m > distanceThresholds [ 3 ] ) {         
      // green e.g. 1.4-1.6 m
      distanceZone = dz_parked;
      flashPair = 3;
      flashPeriod_ms = 1000UL;
      flashDC = 1.0;
      if ( ( millis() - lastMovedAt_ms ) > parkingTimeout_ms ) {
        // parked for long enough to time out
        parkedP = 1;
      } 

    } else {                                                      // red / red = too close! e.g. < 1.5 m
      distanceZone = dz_tooClose;
      flashPair = 4;
      flashPeriod_ms = 1000UL;
      flashDC = 1.0;
    }
    
    // check whether we're moving
    
    if ( fabs ( staticDistance_m - distance_m ) > moveDistanceThreshold_m ) {
      staticDistance_m = distance_m;
      lastMovedAt_ms = millis();
    }
    
    // is garage door closed? The pin will be pulled down to 0 if the door is closed.
    closedP = 1 - digitalRead ( pdHallSense );


    // printing
    
    if ( ( ( millis() - lastPrintAt_ms ) > printInterval_ms )
         && ( VERBOSE >= 4 ) ) { 
        Serial.print ( millis() ); Serial.print ( ": " );
        // Serial.print ( pulseWidth_us_EWMA.value() ); Serial.print ( "us; " );
        Serial.print ( counts_stats.mean() ); Serial.print ( " counts, average of " );
        Serial.print ( counts_stats.num() ); Serial.print ( "; " );
        #if use_EWMA
          Serial.print ( rangefinder_v_EWMA.value() ); Serial.print ( " volts; " );
        #else
          Serial.print ( rangefinder_v ); Serial.print ( " volts; " );
        #endif
        Serial.print ( int ( rangefinder_pct * 100.0 ) ); Serial.print ( " percent; " );
        Serial.println ();
        
        Serial.print ( distance_m ); Serial.print ( "m; " );
        Serial.print ( flashPair );  Serial.print ( "; " );
        // Serial.print ( justForPrinting_remainingTime_s ); Serial.print ( " sec left" );
        Serial.println ();
        
        Serial.print ( parkedP ? "" : "Not " ); Serial.print ( "parked" );
        Serial.println ();
        Serial.print ( closedP ? "" : "Not " ); Serial.print ( "closed" );
        Serial.println ();
      
        Serial.println ();
      
      lastPrintAt_ms = millis();
    }
    
    
    #ifdef BLUETOOTH_GRAPH
    
      // graphing on Android via bluetooth dongle
      if ( ( millis() - lastGraphAt_ms ) > graphInterval_ms ) {
        Serial.print ( "E" );
        Serial.print ( distance_m ); Serial.print ( "," );
        Serial.print ( distanceZone ); Serial.print ( "," );
        Serial.print ( parkedP ? "1" : "0" ); Serial.print ( "," );
        Serial.print ( counts_stats.num()   ); Serial.print ( "," );
        Serial.print ( counts_stats.mean()  ); Serial.print ( "," );
        Serial.print ( counts_stats.stDev() ); Serial.print ( "," );
        Serial.println ();
        
        lastGraphAt_ms = millis();
      }
    #endif

    counts_stats.reset();

    lastCheckAt_ms = millis();
  
    // parkedP says we're firmly parked; closedP says the door's closed

  }  // update
  
  
  // update status
  
  if ( oldClosedP && ! closedP ) {
    // door just opened
    doorOpenedAt_ms = millis();
  }
  
  if (   ( parkedP != oldParkedP )
      || ( closedP != oldClosedP )
      || (
             ( ( millis() - lastMovedAt_ms ) < moveSendInterval_ms )
          && ( ( millis() - lastMoveSentAt_ms ) > moveSendInterval_ms ) )
     ) {
    sendingInterval_ms = 0UL;
  }
  
  if ( ( ( millis() - lastStatusSentAt_ms ) > sendingInterval_ms ) ) {
    
    
    
    
    // the ESP-01 will pick this data out and send it off to MQTT
    
    dispatchMQTTvalue ( "home/garage/car_parked", parkedP, "parkedP" );
    dispatchMQTTvalue ( "home/garage/door_closed", closedP, "closedP" );
    dispatchMQTTvalue ( "home/garage/parking_distance_m", staticDistance_m, "staticDistance_m" );
    
    
    
    
    oldParkedP = parkedP;
    oldClosedP = closedP;
    lastMoveSentAt_ms = millis();
    lastStatusSentAt_ms = millis();
    
    sendingInterval_ms = constrain ( sendingInterval_ms * 2, 
                                     sendDataToCloudInitialInterval_ms, 
                                     sendDataToCloudMaxInterval_ms );    
    
  }   

  // watchdogInterval_ms
  
  if ( ( millis() - lastWatchdogAt_ms ) > watchdogInterval_ms ) {
    dispatchMQTTvalue ( "home/garage/watchdog", PROGNAME " v" VERSION " " VERDATE " cbm", "watchdog" );
    lastWatchdogAt_ms = millis();
  }

  
  // lights
    
  if ( ( ( millis() - lastMovedAt_ms ) > parkingStickyTime_ms ) 
    && ( ( millis() - doorOpenedAt_ms ) > keepAwakeInterval_ms ) ) {
    // not moving
    setColor ( colorBlack );
    flashPair = -1;
    // justForPrinting_remainingTime_s = 0;
  } else if ( ( millis() - lastFlashChangeAt_ms ) > flashPeriod_ms ) {
    // change flash state
    flashState = flashState ? 0 : 1;
    // if flashState is 0 (for color 0) then the stateDurationFraction will be flashDC
    //   so the time spent on color 0 increases with increasing flashDC 
    float stateDurationFraction = ( flashState ? 1.0 - flashDC : flashDC );
    flashDuration_ms = int ( stateDurationFraction * flashPeriod_ms );
    setFlashState ( flashPair, flashState );
    lastFlashChangeAt_ms = millis();
    
  }
  
  // blinky LED
  
  if ( parkedP ) {
    blinkInterval_ms = 1000UL;
  } else {
    blinkInterval_ms = 500UL;
  }
  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    digitalWrite ( pdThrobber, 1 - digitalRead ( pdThrobber ) );
    lastBlinkAt_ms = millis();
  }
  
}

float linterp ( float x0, float y0, float p ) {
  if ( p < 0.0 ) p = 0.0;
  if ( p > 1.0 ) p = 1.0;
  return ( x0 + ( y0 - x0 ) * p );
}

double counts2distance_m ( double counts ) {
  double rangefinder_v = counts / 1023.0 * 5.0;
  double rangefinder_pct = ( 2.50 - rangefinder_v ) / ( 2.50 - 1.40 );
  // voltage returned is inversely related to range within 100cm - 550cm
  double distance_m = 0.01 / linterp ( 1.0 / 100.0, 1.0 / 550.0, rangefinder_pct );
  // double distance_ft = 39.37 * distance_m / 12.0;
  return ( distance_m );
}
    
void setFlashState ( int pair, int choice ) {
  #if 0
  Serial.print ( "               flash state             " );
  Serial.print ( pair ); Serial.print ( "; " );
  Serial.print ( choice );
  Serial.println ();
  #endif
  switch ( pair * 2 + choice ) {
    case 0:
      // pair 0, choice 0
      setColor ( colorCyan );
      break;
    case 1:
      // pair 0, choice 1
      setColor ( colorBlack );
      break;
    case 2:
      // pair 1, choice 0
      setColor ( colorBlue );
      break;
    case 3:
      // pair 1, choice 1
      setColor ( colorBlue );
      break;
    case 4:
      // pair 2, choice 0
      setColor ( colorYellow );
      break;
    case 5:
      // pair 2, choice 1
      setColor ( colorBlue );
      break;
    case 6:
      // pair 3, choice 0
      setColor ( colorGreen );
      break;
    case 7:
      // pair 3, choice 1
      setColor ( colorGreen );
      break;
    case 8:
      // pair 4, choice 0
      setColor ( colorRed );
      break;
    case 9:
      // pair 4, choice 1
      setColor ( colorRed );
      break;
  }
}

void setColor ( const byte color [ 3 ] ) {
  analogWrite ( ppRed,   color [ 0 ] );
  analogWrite ( ppGreen, color [ 1 ] );
  analogWrite ( ppBlue,  color [ 2 ] );
}

void identifySelfToESP () {

  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject ();

  // { "command": "setAssignment", "assignment": <assignment>, "retain": true }
  
  root["command"]    = "setAssignment";
  root["assignment"] = PROGNAME " v" VERSION " " VERDATE " cbm";
  root["retain"]     = true;
  
  root.printTo ( Serial );
  Serial.println ();
}

void sendNetworkCredentialsToESP () {
  
  Network.setCredentials ( WIFI_LOCALE );
  
  /*
    CBM_MQTT_SERVER, CBM_MQTT_SERVERPORT, CBM_MQTT_USERNAME, CBM_MQTT_KEY are 
    globals set by cbmNetworkInfo.h
    now we also have .ssid and .password in the network object
  */
  
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject ();

  /*
    { "command": "connect", 
      "networkSSID": "cbmDataCollection", 
      "networkPW": "super_secret", 
      "MQTThost": "Mosquitto", 
      "MQTTuserID": "cbmalloch",
      "MQTTuserPW": "different_super_secret" }
  */
  
  root["command"]     = "connect";
  root["networkSSID"] = Network.ssid;
  root["networkPW"]   = Network.password;
  root["MQTThost"]    = CBM_MQTT_SERVER;
  root["MQTTuserID"]  = CBM_MQTT_USERNAME;
  root["MQTTuserPW"]  = CBM_MQTT_KEY;
  
  root.printTo ( Serial );
  Serial.println ();

}

void dispatchMQTTvalue ( const char * topic, float value, const char * name ) {
  char pBuf [ 12 ];
  dtostrf ( value, 3, 2, pBuf );
  dispatchMQTTvalue ( topic, (char *) &pBuf, name );
}

void dispatchMQTTvalue ( const char * topic, int value, const char * name ) {
  char pBuf [ 12 ];
  snprintf ( pBuf, 12, "%d", value );
  dispatchMQTTvalue ( topic, (char *) &pBuf, name );
}

void dispatchMQTTvalue ( const char * topic, char * value, const char * name ) {

  // Memory pool for JSON object tree.
  //
  // Inside the brackets, 200 is the size of the pool in bytes.
  // Don't forget to change this value to match your JSON document.
  // Use arduinojson.org/assistant to compute the capacity.

  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject ();

  /*
  
    Send data via ESP-01 via JSON via MQTT to Mosquitto
    
    { "command": "send", 
      "topic": "cbmalloch/test", 
      "value": 12 }
    
    
  */
  
  int tLen = strlen ( topic );
  int vLen = strlen ( value );
  
  if ( VERBOSE >= 10 ) {
    Serial.print( "Sending " );
    Serial.print ( topic );
    Serial.print ( " ( name = " );
    Serial.print ( name );
    Serial.print ( ", len = " );
    Serial.print ( tLen + vLen );
    Serial.print ( " ): " );
    Serial.print ( value );
    Serial.print ( " ... " );
  }
  
  root["command"] = "send";
  root["topic"] = topic;
  root["value"] = value;
  
  root.printTo ( Serial );
  Serial.println ();
  
};
