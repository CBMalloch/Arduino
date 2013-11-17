#define VERSION "0.0.1"
#define VERDATE "2013-11-13"
#define PROGMONIKER "MM"



DO NOT USE!!! BRICKS THE MINION!!!

Suspect that playing with the timers or something that we do kills the Minion.
The symptom is that the bootloader is OK and recognized by the PC, but the 
non-bootloader is NOT.
The way to bring it back from the dead is 
  1) connect a good Leonardo/Minion to a PC
  2) start up Arduino IDE
  3) select Leonardo/Minion and the appropriate serial port
  4) unplug the good one and plug in the brick
  5) select some other sketch (such as Blink)
  6) do upload; try a zillion times, screwing with the reset button, 
      plugging and unplugging, restarting the Arduino software
  7) until it works.
  
  
Probably the problem is that the radio uses D3 and D10 !!!
Once I fixed this in Minion_receiver, that worked fine. I expect this one would, too!


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


 Note: from C:\Program Files\arduino-1.0.1\hardware\arduino\variants\standard\pins_arduino.h
    static const uint8_t SS   = 10;
    static const uint8_t MOSI = 11;
    static const uint8_t MISO = 12;
    static const uint8_t SCK  = 13;
 
*/

#define MOTOR_CONTROL_SLAVE_ADDRESS 0x01

#define test_duration_ms 60000LU

#define VERBOSE 2

#define BAUDRATE 115200
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600

// pin definitions

#define pdSTATUSLED       3

#define pdESTOP           4
#define pdINTERRUPT       5
#define pdRS485RX         6
#define pdRS485TX         7
#define pdRS485_TX_ENABLE 8

#define pdINDICATORLED    9

#define LOOP_DELAY_ms 100
// #define TIME_TO_WAIT_FOR_REPLY_us 20000LU

// wah... indirectly used, but have to include for compiler
#include <RS485.h>
#include <CRC16.h>

// for Minion communications
#include <JeeLib.h>

//node ID of Rx (range 0-30)
#define idNodeMaster 20
const int Rx_NodeID=30; //emonTx node ID
//network group (can be in the range 1-250).
#define network 210
//Freq of RF12B can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. Match freq to module
#define freq RF12_915MHZ

#if 0
  #include <SoftwareSerial.h>
  SoftwareSerial MAX485 ( pdRS485RX, pdRS485TX );
#else
  #define MAX485 Serial1
#endif

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
                                       { 0,  pdSTATUSLED      , -1 },
                                       { 0,  pdINDICATORLED   , -1 }
                                      };
                                      
#define bufLen 40
char strBuf [ bufLen + 1 ];
int bufPtr;

#define MINSPEED 200

short testNumber = -1;

short wheelSpeedCommands [ 2 ] = { 0, 0 };

float MLXZero = 0.0;

void setup () {

  rf12_initialize ( idNodeMaster, freq, network ); //Initialize RFM12 with settings defined above

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

  
  if ( Serial ) {
    bufPtr = 0;  
    snprintf ( strBuf, bufLen, "%s: EMMA Master v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
    Serial.print ( strBuf );


    Serial.print("Node: ");
    Serial.print(idNodeMaster);
    Serial.print(" Freq: ");
    if (freq == RF12_433MHZ) Serial.print("433Mhz");
    if (freq == RF12_868MHZ) Serial.print("868Mhz");
    if (freq == RF12_915MHZ) Serial.print("915Mhz");
    Serial.print(" Network: ");
    Serial.println (network);
    Serial.print ( "Listening for node: " );
    Serial.println ( Rx_NodeID );
  }
    
  // deactivate lines that are ACTIVE LOW
  digitalWrite (pdESTOP, 1);
  digitalWrite (pdINTERRUPT, 1);
    
  // master.Set_Verbose ( ( Stream * ) &Serial, 6 );
  
  // reset motor control slave in case it was in timeout or estop
  // master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
  // delay ( 5 );
  // master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );
  // delay ( 5 );
  
  // delay ( 200 );  // just for yucks
    
  // if ( Serial ) Serial.println ("Halting"); for ( ;; ) { }
  
  // first commanded wheel speeds must be zero
  for ( int i = 0; i < 2; i++ ) { wheelSpeedCommands [ i ] = 0; }
  sendWheelSpeedCommandsToSlave ( wheelSpeedCommands );

}

#define maxSpeedAboveMin 100
  
void loop () {
  
  static unsigned long loopBeganAt_ms;

  static unsigned long lastRecvAt_ms = 0LU;  
  /*
  
    It may be that the channels used in the !g GO command go to the mixing mode,
    with channel 1 being speed and channel 2 being steering.
    
    Set mixing mode to 1 using RoboRun or ^MXMD 1
                    or 0 (separate)
    
    Need to swap the leads of one motor.
    
  */

  short send_command_interval_ms = 400;
  short print_interval_ms = 1000;
  static unsigned long  lastLoopAt_us, 
                        lastPrintAt_ms = 0, 
                        lastSendAt_ms = 0, 
                        lastMsgReceivedAt_ms = 0,
                        lastFlashAt_ms = 0;
  short direction;
  
  // receive_status = 0;
  
  #if VERBOSE >= 2
  if ( Serial && ( ( millis() - lastPrintAt_ms ) > print_interval_ms ) ) {
    // Serial.print ( "Send interval (ms): " ); Serial.println ( send_command_interval_ms );
    // if ( lastLoopAt_us != 0LU ) {
      // Serial.print( "  Loop time (us): "); Serial.println ( micros() - lastLoopAt_us );
    // }
    lastPrintAt_ms = millis();
  }
  lastLoopAt_us = micros();
  #endif
  
  byte RFCommand;
  float RFSpeed;
  float RFSteering;
  
  if ( rf12_recvDone () ) {
    if ( rf12_crc == 0 && ( rf12_hdr & RF12_HDR_CTL ) == 0 ) {
      int node_id = (rf12_hdr & 0x1F); //extract nodeID from payload
      if (node_id == Rx_NodeID) { //check data is coming from node with the correct ID
      
        // the data is in rf12_data; its length is in byte zero
        
        int packetLength = rf12_data [ 0 ];
        
        /* packet definition:   
          byte 0 - length of remainder
          byte 1 - command
            0 - stop
            1 - run
          bytes 2-3 - speed ( -100 - 100% )
          bytes 4-5 - steering ( -10 - 10 ) -10 is L, 10 is R
        */ 
        
        switch ( packetLength ) {
          case 5:
            memcpy ( ( unsigned char * ) &RFSpeed,    ( unsigned char * ) rf12_data + 2, 2 );
            memcpy ( ( unsigned char * ) &RFSteering, ( unsigned char * ) rf12_data + 4, 2 );
            
            // no break; get command below
          case 1:
            RFCommand = rf12_data [ 1 ];
            if ( RFCommand == 0 ) {
              // stop everything!
              wheelSpeedCommands [ 0 ] = 0;
              wheelSpeedCommands [ 1 ] = 0;
            } else {
              // calculate wheel speeds
              wheelSpeedCommands [ 0 ] = pct2cmd ( RFSpeed - 0.5 * RFSteering );
              wheelSpeedCommands [ 1 ] = pct2cmd ( RFSpeed + 0.5 * RFSteering );
            }
            break;
          default:
            break;
        }
        
      } else if ( Serial ) {
        Serial.print ( "Discarding packet from " );
        Serial.println ( node_id );
      }
    } else {
      if ( Serial ) Serial.println ( "Nonzero CRC or header control is 1" );
    }
  } //End of receive code

  
  if ( ( millis() - lastSendAt_ms ) > send_command_interval_ms ) {
        
    sendWheelSpeedCommandsToSlave ( wheelSpeedCommands );        
    lastSendAt_ms = millis();
    
  }
  
  #define flashRestDur 1000UL
  #define flashDur      100UL
  #define flashInterval  50UL
  
  if ( ( millis() - lastFlashAt_ms ) > flashInterval ) {
    digitalWrite ( pdSTATUSLED, 1 - digitalRead ( pdSTATUSLED ) );
  }

}

short pct2cmd ( float pct ) {
  return ( pct * maxSpeedAboveMin ) + MINSPEED;
}

void sendWheelSpeedCommandsToSlave ( short wheelSpeedCommands [ 2 ] ) {

  master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
  delay ( 5 );
  master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );
  delay ( 5 );
  digitalWrite ( pdINDICATORLED, ! digitalRead ( pdINDICATORLED ) );
  
  #if VERBOSE >= 1
  if ( Serial ) {
    Serial.print   ( "Speeds: " ); 
    Serial.print   ( wheelSpeedCommands [ 0 ] );
    Serial.print   ( ", " ); 
    Serial.println ( wheelSpeedCommands [ 1 ] );
  }
  #endif
}
