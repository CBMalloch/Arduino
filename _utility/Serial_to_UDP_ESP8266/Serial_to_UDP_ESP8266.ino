/* 
  Serial to UDP via ESP8266
  Charles B. Malloch, PhD
  2016-02-12
  
  Program to replace chunks of my home control system, specifically 
  the XBee-to-Toshiba-Windows2K-to-UDP bits.
  
  Broadcast any serial input received ( with a linend ) on the UDP broadcast address and given port
  Respond to (side-channel) conversations at this particular IP address

  Load the software and open the serial monitor
  A command window running nc -4u 172.16.5.11 12345 will send and receive side-channel 
    exchanges
  Another command window running "nc -4ul 172.16.255.255 12345 will receive anything that
    is presented to the serial port of the ESP-01.
    
  When powered from the right-hand USB port of cbmPro2 via a BUB set to 3.3V logic, 
  and installed on a breadboard with a 3.3V regulator, it seems to work fine.
  
  With a *nix system, one can listen to the results with netcat, e.g.:
    nc -4u 172.16.5.11 12345
  for two-way conversations, and 
    nc -4lu 172.16.255.255 12345
  to get the broadcasts.
  
  Plan: make it more easily configurable with a broadcast object and a conversation object
  
  Also refactor cbmNetworkInfo to separate the ESP identification from the network address assignment
  
  2016-02-16  2.0.0 Put some local message echoing into VERBOSE > 2 guards. Now ready for production.
    
*/

#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <cbmNetworkInfo.h>

// ***************************************
// ***************************************

#define BAUDRATE    115200
#define WIFI_LOCALE CBMDDWRT3
const unsigned int port_UDP = 9216;
// either define ( to enable ) or undef
#define DO_BROADCAST
#define DO_CONVERSATION

// ***************************************
// ***************************************

#define VERBOSE          1
#define pdThrobber      13

cbmNetworkInfo Network;

#define WLAN_PASS       password
#define WLAN_SSID       ssid

WiFiUDP conn_UDP;

void handleSerialInput ();
void broadcastString ( char buffer[] );
void handle_UDP_conversation ();

void setup () {
  
  Serial.begin ( BAUDRATE );
  for ( int i = 0; i < 5; i++ ) {
    delay ( 1000 );
    yield ();
  }
  
  while ( !Serial && millis() < 10000 ) {
    delay ( 200 );  // wait up to 5 more seconds for serial to come up
    yield ();
  }
  
  pinMode ( pdThrobber, OUTPUT );
  digitalWrite ( pdThrobber, 1 );
  
  // for security reasons, the network settings are stored in a private library
  Network.init ( WIFI_LOCALE );
  
  if ( ! strncmp ( Network.chipName, "unknown", 12 ) ) {
    Network.describeESP ( Network.chipName );
  }

  // Connect to WiFi access point.
  Serial.print ("\nConnecting to "); Serial.println ( Network.ssid );
  yield ();
  
  WiFi.config ( Network.ip, Network.gw, Network.mask );
  WiFi.begin ( Network.ssid, Network.password );
  
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.print ( "." );
    delay ( 500 );  // implicitly yields but may not pet the nice doggy
  }
  
  conn_UDP.begin ( port_UDP );
  
  Serial.println();

  Serial.print ( "WiFi connected as " ); Serial.print ( WiFi.localIP() );
  Serial.print ( "; will use USB port " ); Serial.println ( port_UDP );
  yield ();
   
  Serial.println ( F ( "[Serial_to_UDP_ESP8266 v2.1.0 2016-02-18]" ) );
  
  yield ();
  
}

void loop() {
  
  static unsigned long lastBlinkAt_ms       =    0;
  static unsigned long lastCommoCheckAt_ms  =    0;
  const unsigned long commoCheckInterval_ms =    2;
  const unsigned long blinkHalfCycleTime_ms =  500;

  
  if ( ( millis() - lastCommoCheckAt_ms ) > commoCheckInterval_ms ) {
    #ifdef DO_BROADCAST
      handleSerialInput();
    #endif
    #ifdef DO_CONVERSATION
      handle_UDP_conversation ();
    #endif

    lastCommoCheckAt_ms = millis();
  }

  if ( ( millis() - lastBlinkAt_ms ) > blinkHalfCycleTime_ms ) {
    digitalWrite ( pdThrobber, 1 - digitalRead ( pdThrobber ) );
    lastBlinkAt_ms = millis();
  }
  
  yield ();


}

#ifdef DO_BROADCAST
  void handleSerialInput() {

    int len;
    
    // collect serial input and send it out when necessary
    const int strBufSize = 120;
    static char strBuf [ strBufSize ];
    static int strBufPtr = 0;
    
    if ( len = Serial.available() ) {
      for ( int i = 0; i < len; i++ ) {
        
        if ( strBufPtr >= strBufSize - 1 ) {
          // -1 to leave room for null ( '\0' ) appendix
          Serial.println ( "BUFFER OVERRUN" );
          strBufPtr = 0;
        }
        
        strBuf [ strBufPtr ] = Serial.read();
        // LEDToggle();
        // Serial.println(buffer[strBufPtr], HEX);
        if ( strBufPtr > 5 && strBuf [ strBufPtr ] == '\r' ) {  // 0x0d
          // append terminating null
          strBuf [ ++strBufPtr ] = '\0';
          broadcastString ( strBuf );
          strBufPtr = 0;
        } else if ( strBuf [ strBufPtr ] == '\n' ) {  // 0x0a
          // ignore LF
        } else {
          strBufPtr++;
        }  // handling linends
        
        yield ();
        
      }  // reading the input buffer
    }  // Serial.available
        
  }

  void broadcastString ( char strBuffer[] ) {
    
    // calculate the broadcast address using the mask
    IPAddress bcast = Network.ip;
    for ( int i = 0; i < 4; i++ ) {
      // set the bits whose corresponding mask bits are 0
      unsigned char m = Network.mask [ i ];
      bcast [ i ] |= ~m;
    }
      
    Serial.print ( "About to send '" ); Serial.print ( strBuffer ); Serial.println ( "'" );
    // conn_UDP.beginPacket ( Network.ip, port_UDP );
    conn_UDP.beginPacket ( bcast, port_UDP );
    conn_UDP.println ( strBuffer );
    // conn_UDP.print ( '\r' );
    conn_UDP.endPacket ();
    yield ();
  }
#endif
// endif DO_BROADCAST

#ifdef DO_CONVERSATION
  void reply_to_message ( WiFiUDP conn_UDP, char* incoming_message );

  void handle_UDP_conversation () {
    
    const int packetBufSize = 512;
    static char packetBuf [ packetBufSize ];
    static int packetBufPtr = 0;
    
    int nChars = conn_UDP.parsePacket();
   
    if ( nChars ) {
      if ( nChars < packetBufSize ) {
      
        conn_UDP.read ( packetBuf, nChars );
        packetBuf [ nChars ] = '\0';
        
        if ( VERBOSE > 2 ) {
          Serial.print ( millis() / 1000 );
          Serial.print ( ": Packet (" );
          Serial.print ( nChars );
          Serial.print ( "chars) received from " );
          Serial.print ( conn_UDP.remoteIP() );
          Serial.print ( ":");
          Serial.println ( conn_UDP.remotePort() );
        
          // display the packet contents in hex
          for ( int i = 1; i <= nChars; i++ ) {
            Serial.print ( packetBuf [ i - 1 ], HEX );
            if ( i % 32 == 0 ) {
              Serial.println ();
            }
            else Serial.print ( ' ' );
          }
          Serial.println();
        }
        yield ();
        
        // Reply
        
        reply_to_message ( conn_UDP, packetBuf );
      
        Serial.println ( packetBuf );
        Serial.println ();
        
      } else {
        // message too long
        conn_UDP.flush ();
        Serial.print ( "Message length (" ); Serial.print ( nChars );
        Serial.print ( ") exceeds packet buffer size (" ); Serial.print ( packetBufSize );
        Serial.println ( "). Message dropped." );
      }
    }  // nChars is nonzero
    yield ();
  }
 
  void reply_to_message ( WiFiUDP conn_UDP, char* incoming_message ) {
    
    // send zero or more packets in reply to an incoming message
    
    conn_UDP.beginPacket( conn_UDP.remoteIP(), conn_UDP.remotePort() );
    conn_UDP.print   ( "--> from ESP8266 chip id " );
    conn_UDP.print   ( ESP.getChipId () );
    conn_UDP.print   ( " at IP " );
    conn_UDP.print   ( WiFi.localIP() );
    conn_UDP.print   ( ":" );
    conn_UDP.print   ( port_UDP );
    conn_UDP.endPacket();

    conn_UDP.beginPacket( conn_UDP.remoteIP(), conn_UDP.remotePort() );
    conn_UDP.print   ( ":\r\n--> " );
    conn_UDP.print   ( incoming_message );
    conn_UDP.print   ( "--> from " );
    conn_UDP.print   ( conn_UDP.remoteIP() );
    conn_UDP.print   ( ":" );
    conn_UDP.print   ( conn_UDP.remotePort() );
    conn_UDP.print   ( "\r\n\n" );
    conn_UDP.endPacket();
    
  }
  
#endif
// endif DO_CONVERSATION

