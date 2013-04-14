#define VERSION "0.3.0"
#define VERDATE "2013-04-14"
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

#define pdRS485RX 10
#define pdRS485TX 11
#define pdRS485_TX_ENABLE 12

#define pdLED 13

#define paPOT 0

// wah... indirectly used, but have to include for compiler
#include <RS485.h>
#include <CRC16.h>

#include <SoftwareSerial.h>
SoftwareSerial MAX485 ( pdRS485RX, pdRS485TX );

#include <MODBUS.h>
MODBUS MODBUS_port ( ( Stream * ) &MAX485, pdRS485_TX_ENABLE );
#include <MODBUS_Master.h>
MODBUS_Master master ( MODBUS_port );

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
  
  snprintf ( strBuf, bufLen, "%s: Test MODBUS Master v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
  Serial.print ( strBuf );

}

void loop() {

  short send_command_interval_ms, print_interval_ms = 1000;
  static unsigned long lastLoopAt_us, lastPrintAt_ms = 0, lastSendAt_ms = 0, lastMsgReceivedAt_ms = 0;
  
  static char slave_k = 0;
  static char coil = -1;
  static unsigned char coil_value = 0x00;
  
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
    
    if ( coil < 5 ) {
      coil++;
    } else {
      coil = 3;
      coil_value ^= 0x01;
      if (coil_value == 0x00) slave_k = ( slave_k + 1 ) % nSlaves;
    }
        
    digitalWrite ( pdLED, ( ( coil & 0x01 ) ^ coil_value ) );
    
    #if VERBOSE >= 4
      snprintf ( strBuf, bufLen, "%d: coil %d set %d\n", 
        slaves [ slave_k ], coil, coil_value );
      Serial.print ( strBuf );
    #endif
    
    master.Write_Single_Coil ( slaves [ slave_k ], coil, coil_value );
    
    lastSendAt_ms = millis();
  }
    
}

