// **********************************************
// **********************************************
//
// FELN FELN FELN FELN FELN FELN FELN FELN FELN
//   Don't forget to set value for UDP port
// FELN FELN FELN FELN FELN FELN FELN FELN FELN
//
// **********************************************
// **********************************************


/* 
  Serial to UDP via ESP8266
  Charles B. Malloch, PhD
  2016-02-12
  
  Program to replace chunks of my home control system, specifically 
  the XBee-to-Toshiba-Windows2K-to-UDP bits.
  
  NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
  
      port_UDP is configured in this software - verify it's correct before flashing!!!

  NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
  
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
  
  2016-02-16  2.0.0 Put some local message echoing into VERBOSE > 2 guards. Now ready for production.
  2019-02-16  2.2.1 updated to remove access point (AP) mode from WiFi
  2020-12-31  2.2.2 removed call to flush which only calls endPacket again,
                    thus broadcasting a zero-length message...
    
*/

#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <cbmNetworkInfo.h>

// ***************************************
// ***************************************

#define BAUDRATE    115200
#define VERBOSE          2
// need to use CBMDDWRT3 or CBMDATACOL for my own network access
#define WIFI_LOCALE CBMDATACOL
#define ALLOW_PRINTING_OF_PASSWORD false





//                ***** DANGER - need to check / change as needed! *****


// HSM: 9202; WFM 9203; HTMM 9204; testing 9208
const unsigned int port_UDP = 9203;







// either define ( to enable ) or undef
#define DO_BROADCAST
#define DO_CONVERSATION

// ***************************************
// ***************************************

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
  // for ( int i = 0; i < 5; i++ ) {
  //   delay ( 1000 );
  //   yield ();
  // }
  
  while ( !Serial && millis() < 10000 ) {
    delay ( 200 );  // wait up to 5 more seconds for serial to come up
    yield ();
  }
  
  pinMode ( pdThrobber, OUTPUT );
  digitalWrite ( pdThrobber, 1 );
  
  // for security reasons, the network settings are stored in a private library
  Network.init ( WIFI_LOCALE );
  // Network.describeESP ( Network.chipName );

  // Connect to WiFi access point.
  Serial.print ("\nConnecting to '"); Serial.print ( Network.ssid );
  if ( ( VERBOSE >= 10 ) && ALLOW_PRINTING_OF_PASSWORD ) {
    Serial.print ( "' with password '" );
    Serial.print ( Network.password );
  }
  Serial.println ( "'" );
  if ( VERBOSE >= 10 ) {
    Serial.printf ( "  ip: %s\n", Network.ip.toString().c_str() );
    Serial.printf ( "  gateway: %s\n", Network.gw.toString().c_str() );
    Serial.printf ( "  mask: %s\n", Network.mask.toString().c_str() );
    Serial.printf ( "  dns: %s\n", Network.dns.toString().c_str() );
  }
  yield ();
  
  WiFi.config ( Network.ip, Network.gw, Network.mask, Network.dns );
  WiFi.mode ( WIFI_STA );  // 2019-02-16 to remove AP from default mode WIFI_AP_STA
  WiFi.begin ( Network.ssid, Network.password );
  
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.print ( "." );
    delay ( 1000 );  // implicitly yields but may not pet the nice doggy
  }
  
  conn_UDP.begin ( port_UDP );
  
  Serial.println();

  Serial.print ( "WiFi connected as " ); Serial.print ( WiFi.localIP() );
  Serial.print ( "; will use USB port " ); Serial.println ( port_UDP );
  yield ();
   
  Serial.println ( F ( "[Serial_to_UDP_ESP8266 v2.2.2 2020-12-31]" ) );
  
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
          strBuf [ ++strBufPtr ] = '\0';  // leave the \r on the msg
          // strBuf [ strBufPtr ] = '\0';  // overwrite the \r instead
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
      
    Serial.print ( "About to broadcast '" ); 
    Serial.print ( strBuffer ); Serial.print ( "' to " );
    Serial.print ( bcast ); Serial.print ( ":" ); Serial.println ( port_UDP );
    // conn_UDP.beginPacket ( Network.ip, port_UDP );
    conn_UDP.beginPacket ( bcast, port_UDP );
    conn_UDP.print ( strBuffer );
    // conn_UDP.print ( '\r' );
    conn_UDP.endPacket ();
    // conn_UDP.flush ();  // flush just calls endPacket
    
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
          Serial.print ( " chars) received from " );
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
          
          // for ( int i = 0; i < 4; i++ ) {
            // Serial.print ( i ); Serial.print (": " ); 
            // Serial.print ( conn_UDP.remoteIP() [ i ] ); Serial.print ( " :: " );
            // Serial.println ( Network.ip [ i ] );
          // }
          

        }
        yield ();
        
        // we don't want to catch broadcasts that we just sent
        // originator IP seems to be our IP - 1 -- don't know if this is a standard!
        
        if (    ( conn_UDP.remoteIP() [ 0 ] ==  Network.ip [ 0 ] )
             && ( conn_UDP.remoteIP() [ 1 ] ==  Network.ip [ 1 ] )
             && ( conn_UDP.remoteIP() [ 2 ] ==  Network.ip [ 2 ] )
             && ( conn_UDP.remoteIP() [ 3 ] ==  Network.ip [ 3 ] - 1 )   // !!!!!!!!!!! ????????????
             && ( conn_UDP.remotePort() == port_UDP ) 
           ) {
          // it was from ourself
          if ( VERBOSE > 2 ) Serial.println ( "Packet from self: dropped" );
        } else {

          // Reply
          
          reply_to_message ( conn_UDP, packetBuf );
        
          Serial.println ( packetBuf );
        }
        
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
    
    conn_UDP.flush();
    
  }
  
#endif
// endif DO_CONVERSATION

