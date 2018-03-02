#define PROGNAME  "user_of_ESP_text_to_MQTT"
#define VERSION   "0.1.0"
#define VERDATE   "2018-02-27"

// ***************************************
// ***************************************

// SoftwareSerial is limited to 57600 for RX
// #include <SoftwareSerial.h>
// <https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html>
// #include <AltSoftSerial.h>

#include <cbmNetworkInfo.h>

#define BAUDRATE       115200
// Note: on older machines, SoftwareSerial is limited to 57600 baud
#define espBAUDRATE     115200

// ---------------------------------------
// ---------------------------------------

// need to use CBMDDWRT3 for my own network access
// CBMDATACOL for other use
// can use CBMM5 or CBMDDWRT3GUEST for Sparkfun etc.

// Note: we will detect if at M5

#define WIFI_LOCALE CBMDATACOL

// ---------------------------------------
// ---------------------------------------

#define VERBOSE            10
#define atM5            false

// the canonical Serial is connected to the programmer / computer
// so we use SoftwareSerial to connect the ESP

const int pdEspRX = 10;
const int pdEspTX = 11;
const int pdLED  = 13;

/************************* MQTT Setup *********************************/

// values hidden for security reasons
char MQTT_SERVER []   = "mosquitto";
int MQTT_SERVERPORT   = 1883;
char MQTT_USERNAME [] = CBM_MQTT_USERNAME;
char MQTT_KEY []      = CBM_MQTT_KEY;

// set up a new serial port
// SoftwareSerial esp =  SoftwareSerial( pdEspRX, pdEspTX );
#define esp Serial2

unsigned long lastIncomingCharacterAt_ms = 0UL;
const unsigned long lengthOfWaitForNewIncomingMessage_ms = 50;

const int bufLen = 200;
char strBuf [ bufLen ];
int strBufPtr = 0;

void setup ( void ) {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 10000 ) );
  
  esp.begin ( espBAUDRATE );
  
  pinMode ( pdLED, OUTPUT ); digitalWrite ( pdLED, 0 );
  
  Serial.print ( "\n\n" );
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  
  lastIncomingCharacterAt_ms = millis();
  while ( ( millis() - lastIncomingCharacterAt_ms ) <= lengthOfWaitForNewIncomingMessage_ms ) {
    while ( int len = esp.available() ) {
      for ( int i = 0; i < len; i++ ) {
      
        if ( strBufPtr >= bufLen - 1 ) {
          // -1 to leave room for null ( '\0' ) appendix
          Serial.println ( "uetm: BUFFER OVERRUN" );
          strBuf [ 0 ] = '\0';
          strBufPtr = 0;
          // flush Serial input buffer
          while ( esp.available() ) esp.read();
          break;
        }
      
        strBuf [ strBufPtr ] = esp.read();
        // LEDToggle();
        // Serial.println(buffer[strBufPtr], HEX);
        if ( strBufPtr > 5 && strBuf [ strBufPtr ] == '\r' ) {  // 0x0d
          // append terminating null
          strBuf [ ++strBufPtr ] = '\0';
          Serial.print ( "\n<< " ); Serial.println ( strBuf );
          // clear the buffer
          strBufPtr = 0;
        } else if ( strBuf [ strBufPtr ] == '\n' ) {  // 0x0a
          // ignore LF
        } else {
          strBufPtr++;
        }  // handling linends
      }  // counting to len
      lastIncomingCharacterAt_ms = millis();
    }  // esp.available
  }  // wait a while


  snprintf ( strBuf, bufLen, 
    "{ \"command\": \"connect\", \"networkSSID\": \"%s\", \"networkPW\": \"%s\", "
    "\"MQTThost\":\"192.168.5.1\", \"MQTTuserID\": \"cbmalloch\", \"MQTTuserPW\": \"\" }\n", 
    atM5 ? "M5_IoT_MQTT" : "cbm_IoT_MQTT", atM5 ? "m5launch" : "cbmLaunch" );
  Serial.print ( ">> " ); Serial.println ( strBuf );
  esp.println ( strBuf );
  
  lastIncomingCharacterAt_ms = millis();
  while ( ( millis() - lastIncomingCharacterAt_ms ) <= lengthOfWaitForNewIncomingMessage_ms ) {
    while ( int len = esp.available() ) {
      for ( int i = 0; i < len; i++ ) {
      
        if ( strBufPtr >= bufLen - 1 ) {
          // -1 to leave room for null ( '\0' ) appendix
          Serial.println ( "uetm: BUFFER OVERRUN" );
          strBuf [ 0 ] = '\0';
          strBufPtr = 0;
          // flush Serial input buffer
          while ( esp.available() ) esp.read();
          break;
        }
      
        strBuf [ strBufPtr ] = esp.read();
        // LEDToggle();
        // Serial.println(buffer[strBufPtr], HEX);
        if ( strBufPtr > 5 && strBuf [ strBufPtr ] == '\r' ) {  // 0x0d
          // append terminating null
          strBuf [ ++strBufPtr ] = '\0';
          Serial.print ( "\n<< " ); Serial.println ( strBuf );
          // clear the buffer
          strBuf [ 0 ] = '\0';
          strBufPtr = 0;
        } else if ( strBuf [ strBufPtr ] == '\n' ) {  // 0x0a
          // ignore LF
        } else {
          strBufPtr++;
        }  // handling linends
      }  // counting to len
      lastIncomingCharacterAt_ms = millis();
    }  // esp.available
  }  // wait a while
}

void loop () {
  static int lastBlinkAt_ms = 0UL;
  const int blinkInterval_ms = 100UL;
  
  static int status = 0;
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    // time to blink!
    status = 1 - status;
    digitalWrite ( pdLED, status );
    
    snprintf ( strBuf, bufLen, "{ \"command\": \"send\", \"topic\": \"cbmalloch/feeds/onoff\", \"value\": %d }\n", status );
    Serial.print ( ">> " ); Serial.println ( strBuf );
    esp.println ( strBuf );
    
    strBuf [ 0 ] = '\0';
    strBufPtr = 0;
    
    lastBlinkAt_ms = millis();
  }
    
  lastIncomingCharacterAt_ms = millis();
  while ( ( millis() - lastIncomingCharacterAt_ms ) <= lengthOfWaitForNewIncomingMessage_ms ) {
    while ( int len = esp.available() ) {
      for ( int i = 0; i < len; i++ ) {
    
        if ( strBufPtr >= bufLen - 1 ) {
          // -1 to leave room for null ( '\0' ) appendix
          Serial.println ( "uetm: BUFFER OVERRUN" );
          strBuf [ 0 ] = '\0';
          strBufPtr = 0;
          // flush Serial input buffer
          while ( esp.available() ) esp.read();
          break;
        }
    
        strBuf [ strBufPtr ] = esp.read();
        // Serial.print ( strBuf [ strBufPtr ] );
        // LEDToggle();
        // Serial.println(buffer[strBufPtr], HEX);
        if ( strBufPtr > 5 && strBuf [ strBufPtr ] == '\r' ) {  // 0x0d
          // append terminating null
          strBuf [ ++strBufPtr ] = '\0';
          Serial.print ( "\n<< " ); Serial.println ( strBuf );
          // clear the buffer
          strBuf [ 0 ] = '\0';
          strBufPtr = 0;
        } else if ( strBuf [ strBufPtr ] == '\n' ) {  // 0x0a
          // ignore LF
        } else {
          strBufPtr++;
        }  // handling linends
      }  // counting to len
      lastIncomingCharacterAt_ms = millis();
    }  // esp.available
  }  // waiting for messaging to complete
  
}
