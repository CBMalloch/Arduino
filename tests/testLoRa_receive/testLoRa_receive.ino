#define PROGNAME  "testLoRa_receive"
#define VERSION   "0.1.8"
#define VERDATE   "2017-11-30"

/*
  display size is 128x64
  
  <https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide>
  <https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/mac.md>
*/

#include <SPI.h>
#include <LoRa.h>
#include <U8x8lib.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <cbmNetworkInfo.h>
#include <TimeLib.h>

#define BAUDRATE 115200
#define VERBOSE 12
#define LORA_FREQ_BAND 433E6

const int pdLoRa_SS  = 18;    // CS chip select
const int pdLoRa_RST = 14;    // reset
const int pdLoRa_DI0 = 26;    // IRQ interrupt request

const int SX1278_SCK  =  5;   // clock
const int SX1278_MISO = 19;   // master in slave out
const int SX1278_MOSI = 27;   // master out slave in

const int U8X8_SCL = 15;   // I2C clock
const int U8X8_SDA =  4;   // I2C data
const int U8X8_RST = 16;   // I2C reset

const int pdBlink = 25;    // LED - not 13!

String packetText;
String packetRSSI;

const int bufLen = 255;
char strBuf [ bufLen ];

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8( U8X8_SCL, U8X8_SDA, U8X8_RST );

#define WIFI_LOCALE CBMDATACOL
// CBMDATACOL CBMDDWRT3
const unsigned int port_UDP = 9251;

// NTP Servers:
IPAddress timeServer ( 17, 253, 14, 125 ); // time.apple.com
// IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov


//const int timeZone = 1;     // Central European Time
const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)

cbmNetworkInfo network;
WiFiUDP conn_UDP;

const int timeStringBufLen = 25;
char timeStringBuffer [ timeStringBufLen ];

void setup () {

  #ifdef BAUDRATE
    Serial.begin ( BAUDRATE );
    while ( !Serial && millis () < 2000 );
  #endif
  
  pinMode ( pdBlink, OUTPUT );
  digitalWrite ( pdBlink, 0 );
  
  SPI.begin ( SX1278_SCK, SX1278_MISO, SX1278_MOSI, pdLoRa_SS );
  
  u8x8.begin();
  u8x8.setFont ( u8x8_font_chroma48medium8_r );
  u8x8.drawString( 0, 1, "LoRa Receiver" );
  
  LoRa.setPins ( pdLoRa_SS, pdLoRa_RST, pdLoRa_DI0 );
  if ( ! LoRa.begin ( LORA_FREQ_BAND ) ) {
    Serial.println ( "LoRa init failed!" );
    u8x8.drawString( 0, 1, "LoRa init failed!" );
    while ( 1 );
  }
  
  /*
    void setSpreadingFactor(int sf);
    void setSignalBandwidth(long sbw);
    void setCodingRate4(int denominator);
    void setPreambleLength(long length);
    void setSyncWord(int sw);
    void enableCrc();
    void disableCrc();
  */
  
  // LoRa.enableCrc();
  
    // for security reasons, the network settings are stored in a private library
  network.init ( WIFI_LOCALE );
  
  if ( ! strncmp ( network.chipName, "unknown", 12 ) ) {
    network.describeESP ( network.chipName );
  }

  // Connect to WiFi access point.
  Serial.print ("\nConnecting to "); Serial.print ( network.ssid );
  yield ();
  
  // WiFi.disconnect ();
  WiFi.config ( network.ip, network.gw, network.mask, network.dns );
  WiFi.begin ( network.ssid, network.password );
  
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.print ( "." );
    delay ( 500 );  // implicitly yields but may not pet the nice doggy
  }
  
  if ( Serial && VERBOSE >= 4 ) {
    Serial.print ( "WiFi connected as " );
    network.printIP ( network.ip );
    Serial.println ();
  }
  delay ( 50 );
  
  conn_UDP.begin ( port_UDP );
  
  if ( Serial && VERBOSE >= 4 ) Serial.println ( "UDP connected" );
  delay ( 50 );
  
  setSyncProvider ( getNtpTime );
  setSyncInterval ( 60 * 60 );  // seconds

  
  timeStamp ( timeStringBuffer );
  if ( Serial && VERBOSE >= 4 ) {
    Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
    Serial.printf ( "Started at %s\n", timeStringBuffer );
  }
  u8x8.drawString( 0, 1, PROGNAME );
  u8x8.drawString( 0, 2, "v" VERSION " cbm" );
  u8x8.drawString( 0, 3, VERDATE );
  delay ( 100 );

}


void loop() {

  static unsigned long lastBlinkAt_ms = 0UL;
  static unsigned long lastPacketAt_ms = 0UL;
  const unsigned long blinkOnTime_ms = 5UL;
  const unsigned long blinkOffTime_ms = 45UL;
  const unsigned long packetBlinkDelay_ms = 1000UL;
          
  // try to parse packet
  int packetSize = LoRa.parsePacket ();
  if ( packetSize ) {
  
    // received a packet
    timeStamp ( timeStringBuffer );
    
    strBuf [ 0 ] = '\0';
    int cp = 0;
    while ( LoRa.available() ) {
      if ( cp < bufLen - 1 ) {
        strBuf [ cp++ ] = LoRa.read ();
        strBuf [ cp ] = '\0';
      } else {
        cp++;
        LoRa.read ();  // to dump characters that won't fit into the buffer
      }
      // char chr [ 2 ];
      // chr [ 0 ] = LoRa.read ();
      // chr [ 1 ] = '\0';
      // strncat ( strBuf, chr, bufLen - strlen ( strBuf ) - 1 );
    }
    
    char csi [ 3 ] = "\x1b[";
    static bool lastPacketWasOK = false;
    if ( strlen ( strBuf ) < 100 && cp == packetSize ) {
      if ( Serial && VERBOSE >= 8 ) {
        if ( lastPacketWasOK ) {
          // erase previous line before putting new one
          Serial.printf ( "%s1A%s?2K", csi, csi );
        }
        Serial.printf ( "%s: received: (%d) '%s'", timeStringBuffer, cp, strBuf );
      }
      packetText = strBuf;
      lastPacketWasOK = true;
    } else {
      if ( Serial && VERBOSE >= 1 ) {
        Serial.printf ( "%s \aERROR! BAD PACKET: (cp=%d; packetSize=%d) '%s'\n", 
          timeStringBuffer, cp, packetSize, strBuf );
      }
      u8x8.drawString ( 0, 4, "XXXXXXXXXXXX" );
      u8x8.drawString ( 0, 5, "XXXXXXXXXXXX" );
      u8x8.drawString ( 0, 6, "XXXXXXXXXXXX" );
      u8x8.drawString ( 0, 7, "XXXXXXXXXXXX" );
      lastPacketWasOK = false;
      return;
    }
    
    /*
      LoRa.print ( bootCount );
      LoRa.print ("; bat: " );
      LoRa.print ( counts );
    */
    
    u8x8.clearLine ( 4 );
    u8x8.drawString ( 0, 4, timeStringBuffer+5 );
    
    int bc = packetText.toInt();  // converts as much of string as is numeric
    char currentid [10];
    snprintf ( currentid, 10, "%6d", bc );
    u8x8.clearLine ( 5 );
    u8x8.drawString ( 0, 5, "R: " );
    u8x8.drawString ( 3, 5, currentid );
    
    int bat = packetText.substring(packetText.indexOf("bat: ")+5).toInt();
//    int bat = packetText.indexOf("bat: ");
    char currentbat [10];
    snprintf ( currentbat, 10, "%6d", bat );
    u8x8.clearLine ( 6 );
    u8x8.drawString ( 0, 6, "bat: " );
    u8x8.drawString ( 5, 6, currentbat );
    

    // print RSSI & SNR of packet
    if ( Serial && VERBOSE >= 8 ) {
      Serial.printf ( "; RSSI %4d; SNR %5.2f\n", LoRa.packetRssi(), LoRa.packetSnr() );
    }
    char RSSI_SNR [16];
    snprintf ( RSSI_SNR, 16, "SS/SN:%4d/%4.1f", LoRa.packetRssi(), LoRa.packetSnr() );
    u8x8.clearLine ( 7 );
    u8x8.drawString ( 0, 7, RSSI_SNR );
        
    lastPacketAt_ms = millis();
  }
  
  
  if ( ( millis() - lastPacketAt_ms ) > 1 * 60 * 1000 ) {
    // been over a minute since last receive
    Serial.println ( "Ack! Lost periodic receipt of messages!" );
    LoRa.dumpRegisters ( Serial );
    while ( 1 );
  }
  
  int nextLEDState = -1;
  if ( digitalRead ( pdBlink ) ) {
    // is on
    if ( ( millis() - lastBlinkAt_ms ) > blinkOnTime_ms ) {
      nextLEDState = 0;
      lastBlinkAt_ms = millis();
    }
  } else {
    if ( ( millis() - lastBlinkAt_ms ) > blinkOffTime_ms ) {
      nextLEDState = 1;
      lastBlinkAt_ms = millis();
    }
  }
  if ( ( millis() - lastPacketAt_ms ) < packetBlinkDelay_ms ) {
    nextLEDState = 1;
    lastBlinkAt_ms = millis();
  }
  
  if ( nextLEDState != -1 ) digitalWrite ( pdBlink, nextLEDState );
  
}

void timeStamp ( char ts [] ) {
  if ( year() > 1970 ) {
    snprintf ( ts, timeStringBufLen, "%04d-%02d-%02d %02d:%02d:%02d",
       year(), month(), day(), hour(), minute(), second() );
    if ( Serial && VERBOSE >= 18 ) Serial.printf ( "Time: %s\n", ts );
  } else {
    snprintf ( ts, timeStringBufLen, "Millis: %d", millis() );
    if ( Serial && VERBOSE >= 18 ) Serial.printf ( "NoTime: %s\n", ts );
  }
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  while ( conn_UDP.parsePacket() > 0 ); // discard any previously received packets
  if ( VERBOSE >= 18 ) Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = conn_UDP.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      if ( VERBOSE >= 18 ) Serial.println ("Receive NTP Response");
      conn_UDP.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  if ( VERBOSE >= 4 ) Serial.println ("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  conn_UDP.beginPacket(address, 123); //NTP requests are to port 123
  conn_UDP.write(packetBuffer, NTP_PACKET_SIZE);
  conn_UDP.endPacket();
}