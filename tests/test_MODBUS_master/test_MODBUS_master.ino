#define VERSION "0.1.0"
#define VERDATE "2013-02-17"
#define PROGMONIKER "MBM"

/*
  test a master MODBUS RTU node on a MAX485 network
  
  MAX485 connection:
    1 RO receiver output - data received by the chip over 485 is output on this pin
    2 ~RE receive enable (active low)
    3 DE driver enable
    4 DI driver input - input on this pin is driven onto RS485 output
    5 GND
    6 A - noninverting receiver input and noninverting driver output
    7 B - inverting receiver input and inverting driver output
    8 Vcc - 4.75V-5.25V

    Not all pins on the Leonardo support change interrupts, so only the following can be used for RX: 
      8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI). 
 
 
  Arduino connections (digital pins):
     0 RX - reserved for serial comm - left unconnected
     1 TX - reserved for serial comm - left unconnected
     2 N/C
     3N/C
     4 N/C
     5 N/C
     6 N/C
     7 N/C
     8 N/C
     9 N/C
    10 RX - RS485 connected to MAX485 pin 1 - handled by SoftwareSerial
    11 TX - RS485 connected to MAX485 pin 4 - handled by SoftwareSerial
    12 MAX485 driver enable
    13 status LED

*/

#define BAUDRATE 115200
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600

#define RS485RX 10
#define RS485TX 11
// RS485_TX_ENABLE = 1 -> transmit enable
#define pdRS485_TX_ENABLE 12

#define pdLED 13

#define MSGDELAY_MS 250

#include <CRC16.h>

CRC CheckSum;	// From Checksum Library, CRC15.h, CRC16.cpp

#include <SoftwareSerial.h>
SoftwareSerial MAX485(RS485RX, RS485TX);

static byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                        { 0, 12, -1 },
                                        { 0, 13, -1 }
                                      };
                                      
#define bufLen 80
char strBuf[bufLen+1];
int bufPtr;

#define bufLen485 80
unsigned char strBuf485[bufLen485+1];
int bufPtr485;

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(BAUDRATE);
  MAX485.begin(BAUDRATE485);
  
  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( int i = 0; i < nPinDefs; i++ ) {
    if ( pinDefs [i] [0] == 1 ) {
      // input pin
      pinMode ( pinDefs [i] [1], INPUT );
      digitalWrite ( pinDefs [i] [1], 1 );  // enable internal pullups
    } else {
      // output pin
      pinMode ( pinDefs [i] [1], OUTPUT );
      digitalWrite ( pinDefs [i] [1], 0 );
    }
  }

  bufPtr = 0;
  bufPtr485 = 0;
  
  snprintf ( strBuf, bufLen, "%s: Test MODBUS Master v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
  Serial.print ( strBuf );

}

void MODBUS_Send ( unsigned char *buf, short nChars ) {
  if ( nChars > 0 )	{
    // set the error flag if an exception occurred
    unsigned short CRC = CheckSum.CRC16 ( buf, nChars );
    buf[nChars++] = CRC & 0x00ff;				// lower byte
    buf[nChars++] = CRC >> 8;						// upper byte
    digitalWrite ( pdRS485_TX_ENABLE, HIGH );
    for ( int i = 0; i < nChars; i++ ) {
      MAX485.write ( buf[i] );
    }
    digitalWrite ( pdRS485_TX_ENABLE, LOW );
  }
  return;
}

#define MODBUS_RECEIVE_TIMEOUT_ms 2

void MODBUS_Receive ( ) {

  static unsigned long lastCharReceivedAt_ms;
  
  // capture characters from stream
  lastCharReceivedAt_ms = millis();
  while ( ( millis() - lastCharReceivedAt_ms ) < MODBUS_RECEIVE_TIMEOUT_ms ) {
    
    if ( MAX485.available() ) {
    
      if ( bufPtr485 < bufLen485 ) {
        // grab the available character
        unsigned char c;
        c = MAX485.read();
        // if ( c != (char) '0xff' ) {
          
        strBuf485 [ bufPtr485++ ] = c;
        lastCharReceivedAt_ms = millis();
      } else {
        // error - buffer overrun -- whine
        for (;;) {
          digitalWrite (pdLED, 1);
          delay (100);
          digitalWrite (pdLED, 0);
          delay (100);
        }
      }
      digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
    }  // character available
    
  }
  
  return;
}

void loop() {

  #define SEND_COMMAND_INTERVAL_ms 1000
  static unsigned long lastSendAt_ms = 0;
  
  static unsigned char s7c4s = 0x01;
  static unsigned char coil = 3;
  
  if ( ( millis() - lastSendAt_ms ) > SEND_COMMAND_INTERVAL_ms ) {
    // slave 7's coil 4
    memcpy ( strBuf485, "\x07\x05\x00\x04\x00\x00\x00\x00", 8 );
    
    if ( coil < 5 ) {
      coil++;
    } else {
      coil = 3;
      s7c4s ^= 0x01;
    }
        
    strBuf485 [ 3 ] = coil;
    strBuf485 [ 5 ] = s7c4s;
    digitalWrite (pdLED, s7c4s);
    
    MODBUS_Send ( strBuf485, 6 );
    memcpy ( strBuf485, "\x77\x77\x77\x77\x77\x77\x77\x77", 8 );
    bufPtr485 = 0;  // clear buffer...
    Serial.print ( "." );
    lastSendAt_ms = millis();
  }
  
  MODBUS_Receive ( );
  
  if ( bufPtr485 > 0 ) {
    Serial.print ("Received: ");
    for ( int i = 0; i < bufPtr485; i++ ) {
      Serial.print ( " 0x" ); Serial.print ( strBuf485 [ i ], HEX );
    }
    Serial.println ();
    bufPtr485 = 0;
  }
    
}

