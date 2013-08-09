#define VERSION "1.1.1"
#define VERDATE "2013-04-19"
#define PROGMONIKER "MT1"

/*

  Serves as master on EMMA bus, communicating speed commands for each wheel to slave

     
  Arduino connections (digital pins):
     0 RX - reserved for serial comm - left unconnected
     1 TX - reserved for serial comm - left unconnected
     4 ESTOP (yellow)
     5 INTERRUPT (RFU) (blue)
     6 RX - RS485 connected to MAX485 pin 1 (green)
     7 TX - RS485 connected to MAX485 pin 4 (orange)
     8 MAX485 driver enable (purple)
     9 status LED
 
*/

#define MOTOR_CONTROL_SLAVE_ADDRESS 0x01

#define test_duration_ms 15000LU

#define VERBOSE 1

#define BAUDRATE 115200
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600

// pin definitions

#define pdESTOP           4
#define pdINTERRUPT       5
#define pdRS485RX         6
#define pdRS485TX         7
#define pdRS485_TX_ENABLE 8

#define pdSTATUSLED       9

#define LOOP_DELAY_ms 100
// #define TIME_TO_WAIT_FOR_REPLY_us 20000LU

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

/* 
  NOTE: pdESTOP and pdINTERRUPT are to be *open drain* on the bus
  so than any processor can pull them down. Thus the specification of each
  as INPUT. In this mode, setting the pin to HIGH or 1 makes it high-Z
  and enables an internal 50K pullup resistor; setting the pin to LOW or 0
  pulls it strongly to ground, thus asserting the active-low signals.
*/

short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                       { 1,  pdESTOP         ,  -1 },
                                       { 1,  pdINTERRUPT     ,  -1 },
                                   //    { 0,  pdRS485RX  ,  3 },    // handled by SoftwareSerial
                                   //    { 0,  pdRS485TX  ,  4 },    // handled by SoftwareSerial
                                       { 0,  pdRS485_TX_ENABLE, -1 },
                                       { 0,  pdSTATUSLED      , -1 }
                                      };
                                      
#define bufLen 80
char strBuf[bufLen+1];
int bufPtr;

#define MINSPEED 200

short wheelSpeedCommands [ 2 ] = { 0, 0 };

void setup () {

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

  bufPtr = 0;  
  snprintf ( strBuf, bufLen, "%s: EMMA Master v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
  Serial.print ( strBuf );
    
  // deactivate lines that are ACTIVE LOW
  digitalWrite (pdESTOP, 1);
  digitalWrite (pdINTERRUPT, 1);
  
  // master.Set_Verbose ( ( Stream * ) &Serial, 6 );
  
  // reset motor control slave in case it was in timeout or estop
  master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
  delay ( 5 );
  master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );
  delay ( 5 );

}

void loop () {
  
  static unsigned long loopBeganAt_ms;
  float testPct, profiler;
  int maxSpeedAboveMin = 100;
  static unsigned long test_began_at_ms = millis();
  
  /*
  
    It may be that the channels used in the !g GO command go to the mixing mode,
    with channel 1 being speed and channel 2 being steering.
    
    Set mixing mode to 1 using RoboRun or ^MXMD 1
                    or 0 (separate)
    
    Need to swap the leads of one motor.
    
  */

  short send_command_interval_ms = 400;
  short print_interval_ms = 1000;
  static unsigned long lastLoopAt_us, lastPrintAt_ms = 0, lastSendAt_ms = 0, lastMsgReceivedAt_ms = 0;
  short direction;
  
  // receive_status = 0;
  
  #if VERBOSE >= 2
  if ( ( millis() - lastPrintAt_ms ) > print_interval_ms ) {
    Serial.print ( "Send interval (ms): " ); Serial.println ( send_command_interval_ms );
    if ( lastLoopAt_us != 0LU ) {
      Serial.print( "  Loop time (us): "); Serial.println ( micros() - lastLoopAt_us );
    }
    lastPrintAt_ms = millis();
  }
  lastLoopAt_us = micros();
  #endif
  
  testPct = float ( millis() - test_began_at_ms ) / float ( test_duration_ms );
  
  if ( testPct <= 1.0 ) {
  
    // test is still running
    
    if ( ( millis() - lastSendAt_ms ) > send_command_interval_ms ) {
      // construct packet to send new values to the registers
      
      // test scheduler - profiler should go from 0 to 1 and back
      
      if ( testPct < 0.5 ) {
        // subtest 0 - forward
        if ( testPct < 0.25 ) {
          profiler = testPct * 4.0;
        } else {
          profiler = ( 0.5 - testPct ) * 4.0;
        }
        direction = 1;
      } else {
        // subtest 1 - reverse
        if ( testPct < 0.75 ) {
          profiler = ( testPct - 0.5 ) * 4.0;
        } else {
          profiler = ( 1.0 - testPct ) * 4.0;
        }
        direction = -1;
      }
     
      wheelSpeedCommands [ 0 ] = direction * ( short ( profiler * maxSpeedAboveMin ) + MINSPEED );
      // wheelSpeedCommands [ 1 ] = direction * ( maxSpeedAboveMin - ( wheelSpeedCommands [ 0 ] - MINSPEED ) + MINSPEED );
      wheelSpeedCommands [ 1 ] = wheelSpeedCommands [ 0 ]; // to go straight

      master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
      delay ( 5 );
      master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );
      delay ( 5 );
      digitalWrite ( pdSTATUSLED, ! digitalRead ( pdSTATUSLED ) );
      
      #if VERBOSE >= 1
        Serial.print   ( "Speeds: " ); 
        Serial.print   ( wheelSpeedCommands [ 0 ] );
        Serial.print   ( ", " ); 
        Serial.println ( wheelSpeedCommands [ 1 ] );
      #endif
           
      lastSendAt_ms = millis();
      
    }
    
  } else {
  
    // done testing
    for ( ;; ) {
      delay (1000);
    }
  }

}
