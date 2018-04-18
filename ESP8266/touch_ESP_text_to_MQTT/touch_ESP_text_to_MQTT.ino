#define PROGNAME  "touch_ESP_text_to_MQTT"
#define VERSION   "0.2.1"
#define VERDATE   "2018-03-02"

// ***************************************
// ***************************************

// SoftwareSerial is limited to 57600 for RX
// #include <SoftwareSerial.h>
// <https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html>
// #include <AltSoftSerial.h>

#include <cbmNetworkInfo.h>

#define BAUDRATE       115200
#define VERBOSE             4
#define atM5             true

// ---------------------------------------
// ---------------------------------------

// need to use CBMDDWRT3 for my own network access
// CBMDATACOL for other use
// can use CBMM5 or CBMDDWRT3GUEST for Sparkfun etc.

// Note: we will detect if at M5

#define WIFI_LOCALE cbmIoT

// ---------------------------------------
// ---------------------------------------

const int nTouchPins = 6;
const int pTouches [ nTouchPins ] = { 23, 22, 18, 17, 16, 15 };


// the canonical Serial is connected to the programmer / computer
// so we use SoftwareSerial to connect the ESP

const int pdEspRX = 10;
const int pdEspTX = 11;
const int pdLED  = 13;

/************************* MQTT Setup *********************************/

cbmNetworkInfo Network;

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
  
  // for security reasons, the network settings are stored in a private library
  Network.init ( WIFI_LOCALE );
  
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
          Serial.print ( "<< " ); Serial.println ( strBuf );
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
    "\"MQTThost\": \"%s\", \"MQTTuserID\": \"cbmalloch\", \"MQTTuserPW\": \"\" }", 
    atM5 ? "M5_IoT_MQTT" : Network.ssid, atM5 ? "m5launch" : Network.password, 
    atM5 ? "192.168.5.1" : "192.168.5.1" );
  Serial.println ( ">> " ); Serial.println ( strBuf );
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
          Serial.print ( "<< " ); Serial.println ( strBuf );
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
  
  float colorComponents [ 3 ] = { 0.0, 0.0, 0.0 };
  float alpha = 0.9;
    
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
        int val = touchRead ( pTouches [ i ] ) - touchIdleValues [ i ];
        if ( val < 0 ) val = 0;
        avg += val;
      }
      avg /= iterLimit;
      if ( avg < 0 ) avg = 0;
      touchValues [ i ] = avg;
      if ( VERBOSE >= 5 ) {
        if ( i != 0 ) Serial.print ( " : " );
        Serial.print ( touchValues [ i ] ); 
      }
    }
    if ( VERBOSE >= 5 ) Serial.println ();
    
    // at this point, 0 <= avg < 1024
    // actually will occupy less than half that range, due to high idle values
  
    if ( readStatus == 0 ) {
      static unsigned long oldLedColor = 0UL;
      int color [ 3 ];
      // EWMA smoothing of each color component
      for ( int j = 0; j < 3; j++ ) {
        colorComponents [ j ] = alpha * colorComponents [ j ] + ( 1.0 - alpha ) * touchValues [ j ];
        color [ j ] = constrain ( map ( int ( colorComponents [ j ] ), 0, 512, 0, 256 ), 0, 256 );
      }
        
      unsigned long ledColor = ( color [ 0 ] << 16 ) + ( color [ 1 ] << 8 ) + color [ 2 ];
      if ( ledColor != oldLedColor ) {
        snprintf ( strBuf, bufLen, "{ \"command\": \"send\", \"topic\": \"cbmalloch/feeds/onoff\", \"value\": \"0x%06lX\" }", ledColor );
        Serial.print ( ">> " ); Serial.println ( strBuf );
        esp.println ( strBuf );
        oldLedColor = ledColor;
        lastSendAt_ms = millis();
      }
    } else {
      bool ledState = ( touchValues [ 3 ] > 64 ) xor ( touchValues [ 4 ] > 64 ) xor ( touchValues [ 5 ] > 64 );
      static int oldLedState = 0;
      if ( ledState != oldLedState ) {
        snprintf ( strBuf, bufLen, "{ \"command\": \"send\", \"topic\": \"cbmalloch/feeds/onoff\", \"value\": \"%d\" }", ledState ? 1 : 0 );
        Serial.println ( ">> " ); Serial.println ( strBuf );
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
            Serial.print ( "<< " ); Serial.println ( strBuf );
          
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
