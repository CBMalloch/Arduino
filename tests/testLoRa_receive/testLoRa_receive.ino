#define PROGNAME  "testLoRa_receive"
#define VERSION   "2.0.3"
#define VERDATE   "2018-03-29"

/*
  OLED display size is 128x64
  
  <https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide>
  <https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/mac.md>
  
  Next steps:
  	
  	√ set up JSON parser to receive JSON packets
  	o send the contents of the JSON packets to MQTT

*/

#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>
#include <U8x8lib.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <TimeLib.h>

#include <cbmNetworkInfo.h>

// ***************************************
// ***************************************

#define VERBOSE 12

#define BAUDRATE 115200
#define LORA_FREQ_BAND 433E6
#define NHISTOGRAMBINS 32

// ***************************************
// ***************************************

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

const int intervalBinSizes [ 3 ] = { 5, 5, 1 };
int intervalCounts [ 3 ] [ NHISTOGRAMBINS ];
#define LEAVE_SPACE "\n\n\n\n\n\n"

// LoRa packet size is one byte, so packet size limit is 255
const int bufLen = 256;
char strBuf [ bufLen ];

const int lineLen = 80;  // for line breaks

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
//const int timeZone = -5;  // Eastern Standard Time (USA)
const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)

/*
	home/temperature/solar_panel ==> home/temperature/air/outside/solar_panel
	home/system_status/solar_panel/azimuth_deg
	home/system_status/solar_panel/elevation_deg
	home/system_status/solar_panel_monitor/supply_v
*/

cbmNetworkInfo network;
WiFiClient conn_TCP;
PubSubClient conn_MQTT ( conn_TCP );
WiFiUDP conn_UDP;

const int timeStringBufLen = 30;
char timeStringBuffer [ timeStringBufLen ];

void connect( void );
int sendIt ( const char * topic, char * value, const char * name );
// void broadcastString ( char buffer[] );
// void handle_UDP_conversation ();
// int readAndInterpretCommandString ( char * theString );
// int interpretNewCommandString ( char * theTopic, char * thePayload );

void setup () {

  #ifdef BAUDRATE
    Serial.begin ( BAUDRATE );
    while ( !Serial && millis () < 2000 );
  #endif
  
  Serial.print ( "    \n" );  // sidestep Guru Meditation Error !!!????!!!
  
  pinMode ( pdBlink, OUTPUT );
  digitalWrite ( pdBlink, 0 );
  
  SPI.begin ( SX1278_SCK, SX1278_MISO, SX1278_MOSI, pdLoRa_SS );
  
  u8x8.begin();
  u8x8.setFont ( u8x8_font_chroma48medium8_r );
  u8x8.drawString( 0, 1, "LoRa Receiver" );
  
  LoRa_init();

  // ************************* set up networking ******************************
  
  // for security reasons, the network settings are stored in a private library
  network.init ( WIFI_LOCALE );
  
  if ( ! strncmp ( network.chipName, "unknown", CBM_NI_CHIPNAME_STRLEN ) ) {
    network.describeESP ( network.chipName );
  }

  // Connect to WiFi access point.
  Serial.print ("\nConnecting to "); Serial.print ( network.ssid );
  yield ();
  
  // WiFi.disconnect ();
  // DON'T FORGET THE LAST ARGUMENT: DNS!!!
  // ESP does not follow same order as arduino, dns is at the end
  // see https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiSTA.h#L42
  WiFi.config ( network.ip, network.gw, network.mask, network.dns );
  WiFi.begin ( network.ssid, network.password );
  
  if ( Serial ) {
		int nDots = 0;
		while ( WiFi.status() != WL_CONNECTED ) {
			Serial.print ( "." );
			if ( nDots++ >= lineLen ) {
				Serial.println ();
				nDots = 0;
			}
			delay ( 500 );  // implicitly yields but may not pet the nice doggy
		}
	
		if ( VERBOSE >= 4 ) {
			Serial.print ( "WiFi connected as " );
			network.printIP ( network.ip );
			Serial.println ();
		}
	}
  delay ( 50 );
  
  conn_UDP.begin ( port_UDP );
  
  if ( Serial && VERBOSE >= 4 ) Serial.println ( "UDP connected" );
  delay ( 50 );
  
  setSyncProvider ( getNtpTime );
  setSyncInterval ( 60 * 60 );  // seconds
  
  // conn_MQTT.setCallback ( handleReceivedMQTTMessage );
  conn_MQTT.setServer ( CBM_MQTT_SERVER, CBM_MQTT_SERVERPORT );
  connect ();
  
  yield ();

  // clear all the interval bins
  
  for ( int i = 0; i < 3; i++ ) {
    for  ( int j = 0; j < NHISTOGRAMBINS; j++ ) {
      intervalCounts [ i ] [ j ] = 0;
    }
  }
    
  timeStamp ( timeStringBuffer );
  if ( Serial && VERBOSE >= 4 ) {
    Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
    // leave some blank lines for packet display
    Serial.printf ( "Started at %s\n", timeStringBuffer );
    Serial.print ( LEAVE_SPACE );

  }
  u8x8.drawString( 0, 1, PROGNAME );
  u8x8.drawString( 0, 2, "v" VERSION " cbm" );
  u8x8.drawString( 0, 3, VERDATE );
  delay ( 100 );

}


void loop() {

  static int nGoodDumps = 0;
  const unsigned long stallTimeout_ms = 10UL * 60UL * 1000UL;
  static unsigned long lastStallResolvedAt_ms = 0UL;
  static bool goodDumpRequested = true;
  static int nBadDumps = 0;

  static unsigned long lastBlinkAt_ms = 0UL;
  static unsigned long lastPacketAt_ms = 0UL;
  const unsigned long blinkOnTime_ms = 5UL;
  const unsigned long blinkOffTime_ms = 45UL;
  const unsigned long packetBlinkDelay_ms = 3 * 60 * 1000UL;
  
  static unsigned long lastReceiptAt_ms [ 3 ] = { 0UL, 0UL, 0UL };
          
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
      
  conn_MQTT.loop();
  delay ( 10 );  // fix some issues with WiFi stability

  if ( !conn_MQTT.connected () ) connect ();

  yield ();
  
  timeStamp ( timeStringBuffer );

  // try to parse packet
  int packetSize = LoRa.parsePacket ();
  if ( packetSize ) {
  
    // received a packet
    
    strBuf [ 0 ] = '\0';
    int cp = 0;
    while ( LoRa.available() ) {
      if ( cp < bufLen - 1 ) {
        strBuf [ cp++ ] = LoRa.read ();
        strBuf [ cp ] = '\0';
      } else {
        cp++;
        LoRa.read ();  // to dump a character that won't fit into the buffer
      }
      // char chr [ 2 ];
      // chr [ 0 ] = LoRa.read ();
      // chr [ 1 ] = '\0';
      // strncat ( strBuf, chr, bufLen - strlen ( strBuf ) - 1 );
    }
    
    if ( ( cp == packetSize ) && ( checkPacketIsValid ( strBuf ) ) ) {
      packetText = strBuf;
    } else {
    
      /*
                       Bad packet
      */
        
      if ( Serial && VERBOSE >= 15 ) {
        /*
          For better debugging, we're going to eliminate most lines sent by the receiver
          to the laptop. We choose to display only the most recent line received from each 
          of two senders. The way we do this is to send the Serial output to a program that 
          can emulate some of the old displays; VT-100 is the most well-known of these. 
          However, XTerm is a more recent one which handles a superset of the commands, and 
          we've chosen that one, although we're not using anything foreign to VT-100.
          How it works is we send some control characters to the screen that move the cursor 
          (where the next text will appear) and erase the text on the current line.
          The details: each command is signalled by the Control Sequence Introducer, which
          is the ASCII character "escape" (ESC) followed by a left square bracket. The next 
          characters identify the command itself and sometimes also provide a repeat count.
          Details are found on, among others, the web sites
          <https://www.xfree86.org/4.8.0/ctlseqs.html> and
          <https://vt100.net/docs/vt100-ug/chapter3.html>
          The ones I use are:
            Control Sequence Introducer (CSI): \x1b[
            cursor down s lines (CUD): CSI s B
              Note: CUD stops at the bottom margin
            erase current line (EL): CSI P2K
            cursor up s lines (CUU): CSI s A
            Cursor Next Line s Times (default = 1) (CNL): CSI s E
              Note: CNL scrolls if necessary
            Cursor Preceding Line s Times (default = 1) (CPL): CSI s F
            Set foreground color to red: CSI 31 m
            Set foreground color to default: CSI 39 m
            Set background color to yellow: CSI 43 m
            Set background color to default: CSI 49 m
            Colors, from 0=black red green yellow blue magenta cyan 7=white 9=default
          Also used:
            Carriage return (not including line feed) (CR): \r
            Newline (CRLF): \n (which does better than \x1b[nE
            Alarm (BEL): \a
        */
        // go down 2 lines before printing
        Serial.printf ( "\n\%s \aERROR! BAD PACKET: (cp=%d; packetSize=%d) '", 
          timeStringBuffer, cp, packetSize );
        // sanitize each character before printing
        for ( int i = 0; i < packetSize; i++ ) {
          char c = strBuf [ i ];
          // if ( c < 0x20 ) {
          //   // I thought we needed semicolon (e.g. \x1b[3;1m) but no.
          //   // control character; print red
          //   Serial.printf ( "\x1b[31m%c\x1b[39m", c + 0x60 );
          // } else if ( c & 0x80 ) {
          //   // high character; print against yellow background
          //   Serial.printf ( "\x1b[43m%c\x1b[49m", c & 0x7f );
          // } 
          if ( ( c < 0x20 ) || ( c & 0x80 ) ) {
            // bad character; print in green
            Serial.print ( "\x1b[32m" );
            if ( c < 0x10 ) Serial.print ( "0" );
            Serial.print ( c, HEX );
            Serial.print ( "\x1b[39m" );
          } else {
            Serial.print ( c );
          }
        }
        // leave some blank lines for packet display
        Serial.print ( LEAVE_SPACE );
        // and stay there
      }
      u8x8.drawString ( 0, 4, "XXXXXXXXXXXX" );
      u8x8.drawString ( 0, 5, "XXXXXXXXXXXX" );
      u8x8.drawString ( 0, 6, "XXXXXXXXXXXX" );
      u8x8.drawString ( 0, 7, "XXXXXXXXXXXX" );
      return;
    }  // bad packet
    
    /*
      LoRa.print ( bootCount );
      LoRa.print ("; bat: " );
      LoRa.print ( counts );
    */
    
    u8x8.clearLine ( 4 );
    u8x8.drawString ( 0, 4, timeStringBuffer+5 );
    
    
    // concept proven.
    
    
    char cJSONindicator;
    int bc;
		float bat;
		int programmedSleepDuration_ms = 0;
    	
		const size_t jsonBufferSize = 
				JSON_ARRAY_SIZE(15) 
			+ JSON_OBJECT_SIZE(2) 
			+ JSON_OBJECT_SIZE(3) 
			+ JSON_OBJECT_SIZE(8);
			
		{  // scope for JSON object
			DynamicJsonBuffer jsonBuffer(jsonBufferSize);
			// note: <buf>.parse... whacks the string if it doesn't have to copy it
			const char * strBuf2 = jsonBuffer.strdup(strBuf);
			JsonObject& root = jsonBuffer.parseObject ( strBuf2 );
		
			// since we copied it and didn't give the orig to the parser, strBuf is still good 
		
			if ( root.success () ) {
			
				const int tbufLen = 60;
				char strTBuf [ tbufLen ];
				const int nbufLen = 14;
				char strNBuf [ nbufLen ];

				// object was OK JSON
				cJSONindicator = '+';
				bc = root["packetSN"];
				bat = root["vSupp"];
				
				programmedSleepDuration_ms = root [ "sleepsTotal_ms" ];  // returns 0 or NULL if it's not present
				
				// send data to Mosquitto
				
				/*
				
				+ 2018-02-04 18:11:07: received: (252) '{
					"devID":		"LoRa32u4 1",
					"packetSN":	13723,
					"progID":		{"name":"solar_tracker_LoRa_tracker",					
					"ver":			"2.2.18",					
					"verDate":	"2018-01-13"},					
					"vSupp":		3.896774,					
					"tChip":		6.320634,					
					"accel":		[0.167908,0.146179,1.028931],					
					"mag":			[-87.71423,-495.2222,-166.4675],					
					"heading":	186.0881}';
				 RSSI  -79; SNR 14.75
				
				*/
				
				snprintf ( strTBuf, tbufLen, "home/temperature/air/outside/solar_panel" );
  			snprintf ( strNBuf, nbufLen, "%6.2f", (double) root["tChip"] * 1.8 + 32.0 );
  			sendIt ( strTBuf, strNBuf, (char *) "temperature of MPU9250 chip" );

				snprintf ( strTBuf, tbufLen, "home/system_status/solar_panel/azimuth_deg" );
  			snprintf ( strNBuf, nbufLen, "%4.2f", (double) root["heading"] );
				sendIt ( strTBuf, strNBuf, (char *) "heading degrees" );
				
				/*
					data not present in current string...
				snprintf ( strTBuf, tbufLen, "home/system_status/solar_panel/pitch_deg" );
  			snprintf ( strNBuf, nbufLen, "%4.2f", (double) root["tilt"]["pitch"] );
				sendIt ( strTBuf, strNBuf, (char *) "pitch degrees" );
				snprintf ( strTBuf, tbufLen, "home/system_status/solar_panel/roll_deg" );
  			snprintf ( strNBuf, nbufLen, "%4.2f", (double) root["tilt"]["roll"] );
				sendIt ( strTBuf, strNBuf, (char *) "roll degrees" );
				*/
	
				snprintf ( strTBuf, tbufLen, "home/system_status/solar_panel_monitor/supply_v" );
				snprintf ( strNBuf, nbufLen, "%4.3f", (double) root["vSupp"] );
				sendIt ( strTBuf, strNBuf, (char *) "supply voltage" );
				
				snprintf ( strTBuf, tbufLen, "home/system_status/solar_panel_monitor/packet_SN" );
				snprintf ( strNBuf, nbufLen, "%u", (unsigned int) root["packetSN"] );
				sendIt ( strTBuf, strNBuf, (char *) "packet ID" );

			} else {
				
				cJSONindicator = '-';
				bc = packetText.toInt();  // converts as much of string as is numeric
				int batIndex = packetText.indexOf("bat: ") + 5;
				bat = packetText.substring ( batIndex ).toFloat();
				// int bat = packetText.indexOf("bat: ");
			}
    }
    
    char currentID [10];
    snprintf ( currentID, 10, "%6d", bc );
    u8x8.clearLine ( 5 );
    u8x8.drawString ( 0, 5, "R: " );
    u8x8.drawString ( 3, 5, currentID );
    
    char currentbat [10];
    snprintf ( currentbat, 10, "%4.2f", bat );
    u8x8.clearLine ( 6 );
    u8x8.drawString ( 0, 6, "bat: " );
    u8x8.drawString ( 5, 6, currentbat );
    
    /*
                      Report about a good packet
    */
    
    if ( Serial && VERBOSE >= 8 ) {
      // char *strstr(const char *haystack, const char *needle);
      int deviceIndex = 2;
      char * pLoRa = strstr ( strBuf, "LoRa32u4" );
      if ( pLoRa != NULL ) {
        
        char cNum = pLoRa [ 9 ];
        // Serial.print ( "LoRa number: " ); Serial.println ( cNum );
        
        // LoRa 0 up 2 lines; LoRa 1 up 1 line
        deviceIndex = cNum == '0' ? 1 : 0;
      }
      
      // for ( int i = 0; i < 2 * deviceIndex + 1; i++ ) Serial.print ( "\x1b[1F" );   // go up

      // Serial.print ( "\x1b[2K" );   // erase this line
      Serial.printf ( "%c %s: received: (%d) '%s'", 
      	cJSONindicator, timeStringBuffer, packetSize, strBuf );
      Serial.printf ( "; RSSI %4d; SNR %5.2f", LoRa.packetRssi(), LoRa.packetSnr() );
      
      unsigned long timeSinceLastMessage_ms = millis() - lastReceiptAt_ms [ deviceIndex ];
      int histoIndex = timeSinceLastMessage_ms / 1000 / intervalBinSizes [ deviceIndex ];
      if ( histoIndex >= NHISTOGRAMBINS ) histoIndex = NHISTOGRAMBINS - 1;
      intervalCounts [ deviceIndex ] [ histoIndex ]++;
      lastReceiptAt_ms [ deviceIndex ] = millis();
      
      Serial.print ( "\n" );
      if ( programmedSleepDuration_ms ) {
      	Serial.print ( "    Sleeps: " );
      	while ( programmedSleepDuration_ms > 0 ) {
					int intervalIndex = -1;
					int oneSleep_ms = 0;

					if ( programmedSleepDuration_ms >= 8000 ) {
						oneSleep_ms = 8000;
					} else if ( programmedSleepDuration_ms >= 4000 ) {
						oneSleep_ms = 4000;
					} else if ( programmedSleepDuration_ms >= 2000 ) {
						oneSleep_ms = 2000;
					} else if ( programmedSleepDuration_ms >= 1000 ) {
						oneSleep_ms = 1000;
					} else if ( programmedSleepDuration_ms >= 500 ) {
						oneSleep_ms = 500;
					} else if ( programmedSleepDuration_ms >= 250 ) {
						oneSleep_ms = 250;
					} else if ( programmedSleepDuration_ms >= 128 ) {
						oneSleep_ms = 128;
					} else if ( programmedSleepDuration_ms >= 64 ) {
						oneSleep_ms = 64;
					} else if ( programmedSleepDuration_ms >= 32 ) {
						oneSleep_ms = 32;
					} else if ( programmedSleepDuration_ms >= 16 ) {
						oneSleep_ms = 16;
					} 
					Serial.printf ( "%d", oneSleep_ms );
					programmedSleepDuration_ms -= oneSleep_ms;
					if ( programmedSleepDuration_ms ) Serial.print ( ", " );
      	}
      } else {
				for ( int i = 0; i < NHISTOGRAMBINS; i++ ) {
					Serial.printf ( "%6d", intervalCounts [ deviceIndex ] [ i ] );
				}
      }
      // Serial.print ( "\r" );
      
      // for ( int i = 0; i < 2 * deviceIndex; i++ ) Serial.print ( "\n" );   // go back down
      Serial.print ( "\n" );
      
    }
    
    char RSSI_SNR [16];
    snprintf ( RSSI_SNR, 16, "SS/SN:%4d/%4.1f", LoRa.packetRssi(), LoRa.packetSnr() );
    u8x8.clearLine ( 7 );
    u8x8.drawString ( 0, 7, RSSI_SNR );
    
    lastPacketAt_ms = millis();
    
		if ( 0 && ( ( millis() - lastStallResolvedAt_ms ) > 15000UL ) && goodDumpRequested ) {
	
			/*
												Dump regs while all good
			*/
	
			nGoodDumps++;
			Serial.printf ( "\nDump %d of registers while all good:\n", nGoodDumps );
			LoRa.dumpRegisters ( Serial );
			Serial.printf ( "End of dump %d of registers while all good\n", nGoodDumps );
			Serial.print ( LEAVE_SPACE );
			goodDumpRequested = false;
		}
  }  // received a packet
  
  if ( Serial 
      && ( ( millis() - lastPacketAt_ms ) > stallTimeout_ms )  // we're stalled
      && ( lastStallResolvedAt_ms < lastPacketAt_ms )        // but we haven't yet resolved it
     ) {
    
    /*
                      Report a stall
    */
    
    // been too long, too long since last receipt
    // go down several lines before printing
    nBadDumps++;
    Serial.printf ( "\n%s: Ack! Lost periodic receipt of messages!\n", timeStringBuffer );
    Serial.printf ( "Post-stall dump %d of registers:\n", nBadDumps );
    LoRa.dumpRegisters ( Serial );
    Serial.printf ( "End of post-stall dump %d of registers\n", nBadDumps );
    Serial.print ( LEAVE_SPACE );
    lastStallResolvedAt_ms = millis();
    goodDumpRequested = true;
    LoRa.idle();  // try this rather than re-initing
    // LoRa_init ();
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

bool checkPacketIsValid ( char *strBuf ) {
  return ( 
                (		 ( strstr ( strBuf, "bat:"     )   != NULL )
          				|| ( strstr ( strBuf, "LoRa32u4" )   != NULL ) )
         );
}

void LoRa_init () {
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
  
  #if true
    LoRa.enableCrc();
    if ( Serial ) Serial.println ( "CRC enabled" );
  #endif
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

/*----------- MQTT code ----------*/

void connect () {
  
  unsigned long startTime_ms = millis();
  static bool newConnection = true;
  int nDots;
  bool printed;
  
  printed = false;
  nDots = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    if ( VERBOSE > 4 && ! printed ) { 
      Serial.print ( "\nChecking wifi...\n" );
      printed = true;
      newConnection = true;
    }
    Serial.print ( "." );
    if ( nDots++ >= lineLen ) {
      Serial.println ();
      nDots = 0;
    }
    delay ( 1000 );
  }
  if ( newConnection && VERBOSE > 4 ) {
    Serial.print ( "\n  WiFi connected as " ); 
    Serial.print ( WiFi.localIP() );
    Serial.print (" in ");
    Serial.print ( millis() - startTime_ms );
    Serial.println ( " ms" );    
  }
  
  startTime_ms = millis();
  printed = false;
  nDots = 0;
  while ( ! conn_MQTT.connected ( ) ) {
    conn_MQTT.connect ( "cbm_solar-panel_client" );  // wants client ID
    if ( VERBOSE > 4 && ! printed ) { 
      Serial.print ( "  Checking MQTT ( status: " );
      Serial.print ( conn_MQTT.state () );
      Serial.print ( " )...\n" );
      /*
        -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
        -3 : MQTT_CONNECTION_LOST - the network connection was broken
        -2 : MQTT_CONNECT_FAILED - the network connection failed
        -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
         0 : MQTT_CONNECTED - the cient is connected
         1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
         2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
         3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
         4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
         5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
      */
      printed = true;
      newConnection = true;
    }
    Serial.print( "." );
    if ( nDots++ >= lineLen ) {
      Serial.println ();
      nDots = 0;
    }
    delay( 1000 );
  }
  
  if ( newConnection ) {
    if ( VERBOSE > 4 ) {
      Serial.print ("\n  MQTT connected in ");
      Serial.print ( millis() - startTime_ms );
      Serial.println ( " ms" );
    }
    /*
    conn_MQTT.subscribe ( MQTT_FEED );  // topic[, QoS]
    if ( VERBOSE > 4 ) {
      Serial.print ( "  Subscribed to MQTT feed '" );
      Serial.print ( MQTT_FEED );
      Serial.println ( "'" );
    }
    */
  }
  // client.unsubscribe("/example");
  newConnection = false;
}

int sendIt ( const char * topic, char * value, const char * name ) {
  
  int length = strlen ( value );
  if ( length < 1 ) {
    if ( VERBOSE >= 10 ) {
      Serial.print ( "\nNot sending null payload " );
      Serial.println ( name );
    }
    return 0;
  }
  
  if ( VERBOSE >= 10 ) {
    Serial.print( "\nSending " );
    Serial.print ( topic );
    Serial.print ( " ( name = " );
    Serial.print ( name );
    Serial.print ( " ): " );
    Serial.print ( value );
    Serial.print ( " ... " );
  }
  
  bool success = conn_MQTT.publish ( topic, value );
  if ( VERBOSE >= 10 ) Serial.println ( success ? "OK!" : "Failed" );
  
  return success ? length : -1;
};

