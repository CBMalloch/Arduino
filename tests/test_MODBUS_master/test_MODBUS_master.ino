#define VERSION "0.2.0"
#define VERDATE "2013-02-21"
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

    Not all pins on the Leonardo supMODBUS_port change interrupts, so only the following can be used for RX: 
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
    
    
    Statistics:
       In 2556 message pairs, the mean time between the completion of the sending and the 
       completion of the receipt of the reply was 6.84ms, with a standard deviation of 2.31ms.
       With a 2 ms inter-character timeout
       With a 1 ms timeout, mean = 7.28, std = 2.70
       With a 3 ms timeout, mean = 7.48, std = 1.90
       With a 10 ms timeout, mean = 12.99, std = 0.71
      With a 2 ms timeout, the average number of iterations to receive a message was 2.79 with a std of 0.98.
*/

#define VERBOSE 2
#define BAUDRATE 115200
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600

#define RS485RX 10
#define RS485TX 11
#define pdRS485_TX_ENABLE 12

#define pdLED 13

#define paPOT 0

// wah... indirectly used, but have to include for compiler
#include <RS485.h>
#include <CRC16.h>

#include <SoftwareSerial.h>
SoftwareSerial MAX485 ( RS485RX, RS485TX );

#include <MODBUS.h>
MODBUS MODBUS_port ( ( Stream * ) &MAX485, pdRS485_TX_ENABLE );

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
int receive_status;

#define MESSAGE_TTL_ms 5

unsigned char slaves [ ] = { 0x00, 0x06, 0x07 };
byte nSlaves;

void setup() {
 // Open serial communications and wait for MODBUS_port to open:
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

void loop() {

  short send_command_interval_ms, print_interval_ms = 1000;
  static unsigned long lastLoopAt_us, lastPrintAt_ms = 0, lastSendAt_ms = 0, lastMsgReceivedAt_ms = 0;
  
  static char slave_k = 0;
  static char coil = -1;
  static unsigned char coil_value = 0x00;
  unsigned char commandSentThisLoop = 0;
  
  receive_status = 0;
  
  send_command_interval_ms = analogRead ( paPOT );
  
  #if VERBOSE > 1
  if ( ( millis() - lastPrintAt_ms ) > print_interval_ms ) {
    Serial.print ( "Send interval (ms): " ); Serial.println ( send_command_interval_ms );
    if ( lastLoopAt_us != 0LU ) {
      Serial.print( "  Loop time (us): "); Serial.println ( micros() - lastLoopAt_us );
    }
    lastPrintAt_ms = millis();
  }
  lastLoopAt_us = micros();
  #endif
  
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
    
    #if VERBOSE >= 4
      snprintf ( strBuf, bufLen, "%d: coil %d set %d\n", 
        slaves [ slave_k ], coil, coil_value );
      Serial.print ( strBuf );
    #endif
    
    MODBUS_port.Send ( strBuf485, 6 );
    
    // just to make sure we don't mistakenly think the slave replied when it didn't
    memcpy ( strBuf485, "\x77\x77\x77\x77\x77\x77\x77\x77", 8 );
    
    bufPtr485 = 0;  // clear buffer...
    lastSendAt_ms = millis();
    commandSentThisLoop = 1;
  }
  
  
  // **************** receive reply ****************
  
  if ( commandSentThisLoop && ( slaves [ slave_k ] != 0x00 ) ) {
  
    #define timeToWaitForReply_us 20000LU
    unsigned long beganWaitingForReply_us = micros();
    int nIterationsRequired = 0;
    while ( ( ! bufPtr485 ) && ( ( micros() - beganWaitingForReply_us ) < timeToWaitForReply_us ) ) {
      bufPtr485 = MODBUS_port.Receive ( strBuf485, bufLen485, 2 );
      Serial.print ( "." );
      nIterationsRequired++;
    }
    unsigned long itTook_us = micros() - beganWaitingForReply_us;
    
    if ( bufPtr485 ) {
      lastMsgReceivedAt_ms = millis();
      Serial.print ( "Reply receipt turnaround took " );
      Serial.print ( itTook_us );
      Serial.print ( "us in " );
      Serial.print ( nIterationsRequired );
      Serial.println ( " iterations" );
    } else {
      Serial.print ( "Message receive timed out of testing (" );
      Serial.print ( timeToWaitForReply_us );
      Serial.println ( "us)" );
    }
  }
  
  if ( ( ( millis() - lastMsgReceivedAt_ms ) > MESSAGE_TTL_ms ) && ( bufPtr485 != 0 ) ) {
    // kill old messages to keep from blocking up with a bad message piece
    Serial.println ( "timeout" );
    receive_status = -81;  // timeout
    bufPtr485 = 0;
  }

  
  if ( bufPtr485 > 0 ) {
  
    if ( ( receive_status == 0 ) 
      && ( bufPtr485 > 1 )
      && ( strBuf485[1] & 0x80 ) ) {
      // high order bit of command code in reply indicates error condition exists
      receive_status = 99;
    }
    
    #if VERBOSE >= 5      Serial.print ("        Received: ");
      for ( int i = 0; i < bufPtr485; i++ ) {
        Serial.print ( " 0x" ); Serial.print ( strBuf485 [ i ], HEX );
      }
      
      snprintf ( strBuf, bufLen, "; status %d\n", receive_status );
      Serial.print ( strBuf );
    #endif
    
    bufPtr485 = 0;

  } else {
    #if VERBOSE >= 5
      Serial.println ();
    #endif
  }
    
  commandSentThisLoop = 0;
  
}

