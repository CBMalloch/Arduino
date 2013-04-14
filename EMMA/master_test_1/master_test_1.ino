#define VERSION "1.0.0"
#define VERDATE "2013-04-10"
#define PROGMONIKER "MT1"

/*

  Serves as master on EMMA bus, communicating speed commands for each wheel to slave

*/

#define MOTOR_CONTROL_SLAVE_ADDRESS 0x01

#define test_duration_ms 30000LU

#define VERBOSE 2

#define BAUDRATE 115200
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600

#define pdESTOP 4
#define pdINTERRUPT 5
#define pdRS485RX 10
#define pdRS485TX 11
#define pdRS485_TX_ENABLE 12

#define pdLED 13

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
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                       { 0,  4,  -1 },
                                       { 0,  5,  -1 },
                                   //    { 0, 10,  3 },    // handled by SoftwareSerial
                                   //    { 0, 11,  4 },    // handled by SoftwareSerial
                                        { 0, 12, -1 },
                                        { 0, 13, -1 }
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
  
  snprintf ( strBuf, bufLen, "%s: Test MODBUS Master v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
  Serial.print ( strBuf );
    
  // deactivate lines that are ACTIVE LOW
  digitalWrite (pdESTOP, 1);
  digitalWrite (pdINTERRUPT, 1);
  
  // master.Set_Verbose ( ( Stream * ) &Serial, 6 );
  
  // reset motor control slave in case it was in timeout or estop
  master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
  master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );

  delay ( 20 );

}

void loop () {
  
  static unsigned long loopBeganAt_ms;
  float profiler;
  int maxSpeedAboveMin = 600;
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
  
  // receive_status = 0;
  
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
  
  if ( ( millis() - test_began_at_ms ) <= test_duration_ms ) {
  
    // test is still running
    
    if ( ( millis() - lastSendAt_ms ) > send_command_interval_ms ) {
      // construct packet to send new values to the registers
      
      
      profiler = ( millis() - test_began_at_ms ) * 2.0 / test_duration_ms;
      if ( profiler > 1 ) profiler = 2 - profiler;
      
      // now profiler goes from 0 to 1 and back to 0 over full duration of test
      
      wheelSpeedCommands [ 0 ] = short ( profiler * maxSpeedAboveMin ) + MINSPEED;
      wheelSpeedCommands [ 1 ] = maxSpeedAboveMin - ( wheelSpeedCommands [ 0 ] - MINSPEED ) + MINSPEED;

      master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
      master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );
           
      lastSendAt_ms = millis();
      
    }
    
  } else {
  
    // done testing
    for ( ;; ) {
      delay (1000);
    }
  }

}
