#define PROGNAME  "user_of_ESP_text_to_MQTT"
#define VERSION   "0.0.1"
#define VERDATE   "2018-02-27"

// ***************************************
// ***************************************

#include <SoftwareSerial.h>

#define BAUDRATE       115200
#define VERBOSE            10

const int pdssRX = 10;
const int pdssTX = 11;
const int pdLED  = 13;

const size_t strBufLen = 128;
char strBuf [ strBufLen ];
int strBufPtr = 0;

// set up a new serial port
SoftwareSerial esp =  SoftwareSerial( pdssRX, pdssTX );


void setup ( void ) {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 10000 ) );
  
  esp.begin ( BAUDRATE );
  
  pinMode ( pdLED, OUTPUT ); digitalWrite ( pdLED, 0 );
  
  Serial.print ( "\n\n" );
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );

  char strBuf [ 200 ];
  snprintf ( strBuf, 200, "{ \"command\": \"connect\", \"networkSSID\": \"cbm_IoT_MQTT\", \"networkPW\": \"SECRET\", \"MQTTuserID\": \"cbmalloch\", \"MQTTuserPW\": \"\" }\n" );
  Serial.print ( "sent to ESP: " ); Serial.println ( strBuf );
  esp.println ( strBuf );
  
  while ( millis() < 10000 ) {
    while ( int len = esp.available() ) {
      for ( int i = 0; i < len; i++ ) {
      
        if ( strBufPtr >= strBufLen - 1 ) {
          // -1 to leave room for null ( '\0' ) appendix
          Serial.println ( "INPUT BUFFER OVERRUN" );
          strBufPtr = 0;
          // flush Serial input buffer
          while ( esp.available() ) esp.read();
          return ( false );
        }
      
        strBuf [ strBufPtr ] = esp.read();
        // LEDToggle();
        // Serial.println(buffer[strBufPtr], HEX);
        if ( strBufPtr > 5 && strBuf [ strBufPtr ] == '\r' ) {  // 0x0d
          // append terminating null
          strBuf [ ++strBufPtr ] = '\0';
          Serial.println ( strBuf );
          // clear the buffer
          strBufPtr = 0;
        } else if ( strBuf [ strBufPtr ] == '\n' ) {  // 0x0a
          // ignore LF
        } else {
          strBufPtr++;
        }  // handling linends
      }  // counting to len
    }  // esp.available
  }  // wait a while
}

void loop () {
  static int lastBlinkAt_ms = 0UL;
  const int blinkInterval_ms = 500UL;
  
  static int status = 0;
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    // time to blink!
    status = 1 - status;
    digitalWrite ( pdLED, status );
    
    snprintf ( strBuf, 200, "{ \"command\": \"send\", \"topic\": \"cbmalloch/feeds/onoff\", \"value\": %d }\n", status );
    Serial.print ( "sent to ESP: " ); Serial.println ( strBuf );
    esp.println ( strBuf );
    
    lastBlinkAt_ms = millis();
  }
    
  if ( int len = esp.available() ) {
    for ( int i = 0; i < len; i++ ) {
    
      if ( strBufPtr >= strBufLen - 1 ) {
        // -1 to leave room for null ( '\0' ) appendix
        Serial.println ( "INPUT BUFFER OVERRUN" );
        strBufPtr = 0;
        // flush Serial input buffer
        while ( esp.available() ) esp.read();
        return ( false );
      }
    
      strBuf [ strBufPtr ] = esp.read();
      // LEDToggle();
      // Serial.println(buffer[strBufPtr], HEX);
      if ( strBufPtr > 5 && strBuf [ strBufPtr ] == '\r' ) {  // 0x0d
        // append terminating null
        strBuf [ ++strBufPtr ] = '\0';
        Serial.println ( strBuf );
        // clear the buffer
        strBufPtr = 0;
      } else if ( strBuf [ strBufPtr ] == '\n' ) {  // 0x0a
        // ignore LF
      } else {
        strBufPtr++;
      }  // handling linends
    }  // counting to len
  }  // esp.available
  
}
