#define PROGNAME  "touch_ESP_text_to_MQTT"
#define VERSION   "0.2.0"
#define VERDATE   "2018-03-02"

// ***************************************
// ***************************************

// SoftwareSerial is limited to 57600 for RX
// #include <SoftwareSerial.h>
// <https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html>
// #include <AltSoftSerial.h>

#include <cbmNetworkInfo.h>

#define BAUDRATE       115200

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

const int nTouchPins = 6;
const int pTouches [ nTouchPins ] = { 23, 22, 18, 17, 16, 15 };


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

unsigned long touchIdleValues [ nTouchPins ] = { 0, 0, 0, 0, 0, 0 };


void setup ( void ) {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 10000 ) );
  
  const int iterLimit = 100;
  for ( int j = 0; j < iterLimit; j++ ) {
    for ( int i = 0; i < nTouchPins; i++ ) {
      int v = touchRead ( pTouches [ i ] );
      touchIdleValues [ i ] += v;
      if ( i != 0 ) Serial.print ( " : " );
      Serial.print ( v ); 
    }
    Serial.println ();
    delay ( 10 );
  }
  
  for ( int i = 0; i < nTouchPins; i++ ) touchIdleValues [ i ] /= iterLimit;
    
  esp.begin ( BAUDRATE );
  
  pinMode ( pdLED, OUTPUT ); digitalWrite ( pdLED, 0 );
  
  Serial.print ( "\n\n" );
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  
  lastIncomingCharacterAt_ms = millis();
  while ( ( millis() - lastIncomingCharacterAt_ms ) <= lengthOfWaitForNewIncomingMessage_ms ) {
    while ( int len = esp.available() ) {
      for ( int i = 0; i < len; i++ ) {
      
        if ( strBufPtr >= bufLen - 1 ) {
          // -1 to leave room for null ( '\0' ) appendix
          Serial.println ( "tetm: BUFFER OVERRUN" );
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
    "\"MQTThost\": \"%s\", \"MQTTuserID\": \"cbmalloch\", \"MQTTuserPW\": \"\" }\n", 
    atM5 ? "M5_IoT_MQTT" : "cbm_IoT_MQTT", atM5 ? "m5launch" : "cbmLaunch", atM5 ? "192.168.5.1" : "192.168.5.1" );
  Serial.print ( ">> " ); Serial.println ( strBuf );
  esp.println ( strBuf );
  
  lastIncomingCharacterAt_ms = millis();
  while ( ( millis() - lastIncomingCharacterAt_ms ) <= lengthOfWaitForNewIncomingMessage_ms ) {
    while ( int len = esp.available() ) {
      for ( int i = 0; i < len; i++ ) {
      
        if ( strBufPtr >= bufLen - 1 ) {
          // -1 to leave room for null ( '\0' ) appendix
          Serial.println ( "tetm: BUFFER OVERRUN" );
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
  static unsigned long lastBlinkAt_ms = 0UL;
  const unsigned long blinkInterval_ms = 500UL;
  
  static unsigned long lastReadAt_ms = 0UL;
  const unsigned long readInterval_ms = 20UL;
  
  static unsigned long lastSendAt_ms = 0UL;
  const unsigned long refractoryPeriod_ms = 200UL; 
    
  static int readStatus = 0;
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    static int ledStatus = false;
    // time to blink!
    ledStatus = ! ledStatus;
    digitalWrite ( pdLED, ledStatus );
    lastBlinkAt_ms = millis();
  }
  
  if ( ( ( millis() - lastReadAt_ms ) > readInterval_ms ) &&  ( ( millis() - lastSendAt_ms ) > refractoryPeriod_ms ) ) {
  
    unsigned long touchValues [ nTouchPins ];
    for ( int i = 0; i < nTouchPins; i++ ) {
      unsigned long avg = 0;
      const int iterLimit = 20;
      for ( int j = 0; j < iterLimit; j++ ) {
        int val = touchRead ( pTouches [ i ] ) - touchIdleValues [ i ] - 1;
        if ( val < 0 ) val = 0;
        avg += val;
      }
      avg /= iterLimit;
      if ( avg > 255 ) avg = 255;
      touchValues [ i ] = avg;
      if ( i != 0 ) Serial.print ( " : " );
      Serial.print ( touchValues [ i ] ); 
    }
    Serial.println ();
  
    if ( readStatus == 0 ) {
      static int oldLedColor = 0;
      unsigned long color = ( ( ( ( touchValues [ 0 ] & 0xff ) << 8 ) + ( touchValues [ 1 ] & 0xff ) ) << 8 ) + ( touchValues [ 2 ] & 0xff );
      if ( color != oldLedColor ) {
        snprintf ( strBuf, bufLen, "{ \"command\": \"send\", \"topic\": \"cbmalloch/feeds/onoff\", \"value\": \"0x%06lX\" }\n", color );
        Serial.print ( ">> " ); Serial.println ( strBuf );
        esp.println ( strBuf );
        oldLedColor = color;
        lastSendAt_ms = millis();
      }
    } else {
      bool x = ( touchValues [ 3 ] > 64 ) xor ( touchValues [ 5 ] > 64 );
      bool tog = ( touchValues [ 4 ] > 64 );
      static int oldLedState = 0;
      int ledState = ( ! ( x == tog ) ) ? 1 : 0 ;
      if ( ledState != oldLedState ) {
        snprintf ( strBuf, bufLen, "{ \"command\": \"send\", \"topic\": \"cbmalloch/feeds/onoff\", \"value\": \"%d\" }\n", ledState ? 1 : 0 );
        Serial.print ( ">> " ); Serial.println ( strBuf );
        esp.println ( strBuf );
        oldLedState = ledState;
        lastSendAt_ms = millis();
      }
    }
    readStatus = 1 - readStatus;
    
    strBuf [ 0 ] = '\0';
    strBufPtr = 0;
  
    lastIncomingCharacterAt_ms = millis();
    while ( ( millis() - lastIncomingCharacterAt_ms ) <= lengthOfWaitForNewIncomingMessage_ms ) {
      while ( int len = esp.available() ) {
        for ( int i = 0; i < len; i++ ) {
    
          if ( strBufPtr >= bufLen - 1 ) {
            // -1 to leave room for null ( '\0' ) appendix
            Serial.println ( "tetm: BUFFER OVERRUN" );
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
          
            // char *strstr(const char *haystack, const char *needle);
            if ( strstr ( strBuf, (const char *) "0 ... Failed" ) ) {
              Serial.println ( "Resetting ESP chip" );
              setup();
            }
                    
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
    
    lastReadAt_ms = millis();
  }
  
}
