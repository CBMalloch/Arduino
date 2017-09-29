#include <cbmNetworkInfo.h>

// ***************************************
// ***************************************

const int BAUDRATE = 115200;
#define WIFI_LOCALE CBMDDWRT3GUEST
const int VERBOSE = 4;

// ***************************************
// ***************************************

cbmNetworkInfo Network;

#define WLAN_PASS       password
#define WLAN_SSID       ssid

void setup() {
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 ) {
    // wait for serial port to open
    delay ( 20 );
  }
  
  delay ( 200 );
  Serial.println ( "\n\nESP8266 Hardware Test" );

  // for security reasons, the network settings are stored in a private library
  Network.init ( WIFI_LOCALE );

  if ( ! strncmp ( Network.chipName, "unknown", 12 ) ) {
    Network.describeESP ( Network.chipName, &Serial1 );
  }
  
  // Connect to WiFi access point.
  Serial1.print ("\nConnecting to "); Serial1.println ( Network.ssid );
  yield ();
  
  WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
  WiFi.begin ( Network.ssid, Network.password );

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);  // implicitly yields but may not pet the nice doggy
  }
  Serial.println();

  Serial.print ( "WiFi connected as " ); Serial.print ( WiFi.localIP() );
  Serial.println ( );
  yield ();

}

void loop() {

  const int bufLen = 80;
  char strBuf [ bufLen ];
  static int bufPtr = 0;
  int i, len;
  
  if ( len = Serial.available() ) {
    for ( i = 0; i < len; i++ ) {
      if ( bufPtr >= bufLen ) {
        Serial.println ( "BUFFER OVERRUN" );
        bufPtr = 0;
      }
      strBuf [ bufPtr ] = Serial.read();
      if ( bufPtr > 0 && strBuf [ bufPtr ] == '\n' ) {
        strBuf [ bufPtr ] = '\0';                          // overwrite the '\n'
        Serial.print ( millis() );
        Serial.print (": got '");
        Serial.print ( strBuf );
        Serial.println ( "'" );
        // would handle string if we weren't just echoing it
        bufPtr = 0;
        break;
      } else if ( strBuf [ bufPtr ] == 0x0a ) {
        // ignore LF
      } else {
        bufPtr++;
      }
    }
  }
  yield();
}
