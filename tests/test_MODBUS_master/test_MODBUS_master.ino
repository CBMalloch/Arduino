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
#define pdRS485_TX_ENABLE 12

#define pdLED 13

#define paPOT 0

#define MSGDELAY_MS 250

#include <CRC16.h>

CRC CheckSum;	// From Checksum Library, CRC16.h, CRC16.cpp

#include <SoftwareSerial.h>
SoftwareSerial MAX485(RS485RX, RS485TX);

byte nPinDefs;
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
int receive_status = 0;

unsigned char slaves [ ] = { 0x00, 0x06, 0x07 };
byte nSlaves;

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

  nSlaves = sizeof ( slaves ) / sizeof ( byte );

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

#define MESSAGE_TIMEOUT_ms 5
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
  
  if ( ( ( millis() - lastCharReceivedAt_ms ) > MESSAGE_TIMEOUT_ms ) && bufPtr485 != 0 ) {
    // kill old messages to keep from blocking up with a bad message piece
    #ifdef TESTMODE
      _port->println ( "timeout" );
    #endif
    receive_status = -81;  // timeout
    bufPtr = 0;
  }

  return;
}

void loop() {

  short send_command_interval_ms;
  static unsigned long lastSendAt_ms = 0;
  
  static char slave_k = 0;
  static char coil = -1;
  static unsigned char coil_value = 0x00;
  
  send_command_interval_ms = analogRead ( paPOT );
  
  if ( ( millis() - lastSendAt_ms ) > send_command_interval_ms ) {
    memcpy ( strBuf485, "\x07\x05\x00\x04\x00\x00\x00\x00", 8 );
    
    if ( coil < 5 ) {
      coil++;
    } else {
      coil = 3;
      coil_value ^= 0x01;
      if (coil_value == 0x00) slave_k = ( slave_k + 1 ) % nSlaves;
    }
        
    strBuf485 [ 0 ] = slaves [ slave_k ];
    strBuf485 [ 3 ] = coil;
    strBuf485 [ 5 ] = coil_value;
    digitalWrite ( pdLED, ( ( coil & 0x01 ) ^ coil_value ) );
    
    snprintf ( strBuf, bufLen, "%d: coil %d set %d\n", 
      slaves [ slave_k ], coil, coil_value );
    Serial.print ( strBuf );
    
    MODBUS_Send ( strBuf485, 6 );
    
    // just to make sure we don't mistakenly think the slave replied when it didn't
    memcpy ( strBuf485, "\x77\x77\x77\x77\x77\x77\x77\x77", 8 );
    
    bufPtr485 = 0;  // clear buffer...
    lastSendAt_ms = millis();
  }
  
  MODBUS_Receive ( );
  
  if ( bufPtr485 > 0 ) {
  
    if ( ( receive_status == 0 ) 
      && ( bufPtr485 > 1 )
      && ( strBuf485[1] & 0x80 ) ) {
      receive_status = 99;
    }
    
    Serial.print ("        Received: ");
    for ( int i = 0; i < bufPtr485; i++ ) {
      Serial.print ( " 0x" ); Serial.print ( strBuf485 [ i ], HEX );
    }
    
    snprintf ( strBuf, bufLen, "; status %d\n", receive_status );
    Serial.print ( strBuf );
    
    bufPtr485 = 0;

  } else {
    Serial.println ();
  }
    
}

