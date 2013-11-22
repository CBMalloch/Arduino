#define VERSION "1.2.0"
#define VERDATE "2013-11-20"
#define PROGMONIKER "SSEM"

/*

  Serves as master on EMMA bus, communicating speed commands for each wheel to slave
  Interfaces a TTL serial string of speed (0-100%) and direction (-10 to 10) commands
  to the motor controller.

     
  Arduino connections (digital pins):
     0 RX - reserved for serial comm - TTL input from Minion_receiver
     1 TX - reserved for serial comm - TTL output - not used except for monitoring
     4 ESTOP (yellow)
     5 INTERRUPT (RFU) (blue)
     6 RX - RS485 connected to MAX485 pin 1 (green)
     7 TX - RS485 connected to MAX485 pin 4 (orange)
     8 MAX485 driver enable (purple)
     9 status LED
     
    10 MLX90609 CSB
    11 MLX90609 MOSI
    12 MLX90609 MISO
    13 MLX90609 SCK
    
    14 (A0) Selector pin 1 (1's)
    15 (A1) Selector pin 4 (2's)
    16 (A2) Selector pin 3 (4's)
    17 (A3) Selector pin 6 (8's)
    
    Selector pins 2 and 5: +5v


 Note: from C:\Program Files\arduino-1.0.1\hardware\arduino\variants\standard\pins_arduino.h
    static const uint8_t SS   = 10;
    static const uint8_t MOSI = 11;
    static const uint8_t MISO = 12;
    static const uint8_t SCK  = 13;
 
*/

#define MOTOR_CONTROL_SLAVE_ADDRESS 0x01

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

#include <SoftwareSerial.h>
SoftwareSerial MAX485 ( pdRS485RX, pdRS485TX );

#include <MODBUS.h>
MODBUS MODBUS_port ( ( Stream * ) &MAX485, pdRS485_TX_ENABLE );
#include <MODBUS_Master.h>
MODBUS_Master master ( MODBUS_port );

#define STEERINGGAIN 0.5
    
#define AGEOLDESTFRESHMESSAGE_ms 2000LU
#define INTERCHARTIMEOUT_ms 2000LU
#define PRINTINTERVAL_ms 500LU

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
                                      
#define bufLen 60
char strBuf [ bufLen + 1 ];
int bufPtr;

#define MINSPEED 200

short testNumber = -1;

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
  snprintf ( strBuf, bufLen, "%s: Serial Slave EMMA Master v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
  Serial.print ( strBuf );
    
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
    
  // Serial.println ("Halting"); for ( ;; ) { }
  
  
  // first commanded wheel speeds must be zero
  for ( int i = 0; i < 2; i++ ) { wheelSpeedCommands [ i ] = 0; }
  sendWheelSpeedCommandsToSlave ( wheelSpeedCommands );

}

#define maxSpeedAboveMin 100
  
void loop () {
  
  static unsigned long loopBeganAt_ms;
  
  /*
  
    It may be that the channels used in the !g GO command go to the mixing mode,
    with channel 1 being speed and channel 2 being steering.
    
    Set mixing mode to 1 using RoboRun or ^MXMD 1
                    or 0 (separate)
    
    Need to swap the leads of one motor.
    
  */
  #define LEDFLASHINTERVAL_ms 500LU

  short send_command_interval_ms = 400;
  static unsigned long  lastLoopAt_us, 
                        lastPrintAt_ms = 0LU, 
                        lastSendAt_ms = 0LU, 
                        lastCharReceivedAt_ms = 0LU,
                        lastMsgReceivedAt_ms = 0LU,
                        lastFlashAt_ms = 0LU;
  #define EMMAFORWARDDIR  1
  #define EMMAREVERSEDIR -1
  
  static int commandedDirection = 1;
  static float commandedSpeed = 0.0;
  static float commandedSteering = 0.0;
  static int waitForZeroSpeed = 1, oldCommandedDirection = 1;
  static unsigned long lastCommandAt_ms = 0LU;
  static short stopped = 0;

  #define inBufLen 16
  static char inBuf [ inBufLen ];
  static short inBufPtr = 0;

  // receive_status = 0;
  
  #if VERBOSE >= 2
  if ( ( millis() - lastPrintAt_ms ) > PRINTINTERVAL_ms ) {
    // Serial.print ( "Send interval (ms): " ); Serial.println ( send_command_interval_ms );
    // if ( lastLoopAt_us != 0LU ) {
      // Serial.print( "  Loop time (us): "); Serial.println ( micros() - lastLoopAt_us );
    // }
    // lastPrintAt_ms = millis();
  }
  lastLoopAt_us = micros();
  #endif
  
  /* message format:   
           4 digits, right justified: speed ( -100 - 100 )
           \0 (0x00)
           4 digits, right justified:  steering ( -10 - 10 ) -10 is L, 10 is R
           \0 (0x00)
           \n (0x0a)
        */
         
  if ( ( inBufPtr != 0 ) && ( ( millis () - lastCharReceivedAt_ms ) > INTERCHARTIMEOUT_ms ) ) {
    // dump current buffer, which is stale
    Serial.println ( "___flush___" );
    inBufPtr = 0;
  }
  
  while ( Serial.available() ) {
    static short dumping = 0;   // to resynchronize, we dump characters until we see a cr 
    
    char theChar = Serial.read();
    
    if ( theChar == 0x0a ) {
      // newline; handle the buffer and clear it
      if ( inBufPtr == 15 ) {
        inBuf[4] = '\0';
        inBuf[9] = '\0';
        inBuf[14] = '\0';
        commandedDirection = atoi ( & inBuf [ 0 ] );
        commandedSpeed = float ( atoi ( & inBuf [ 5 ] ) );
        commandedSteering = float ( atoi ( & inBuf [ 10 ] ) );
        lastCommandAt_ms = millis();
        #if VERBOSE >= 4
          Serial.print ( "!!! " );
        #endif
      } else {
        Serial.print ( "inBufptr: " ); Serial.println ( inBufPtr );
      }
      
      #if VERBOSE > 4
      for ( int i = 0; i < inBufPtr; i++ ) {
        Serial.print ( " 0x" ); Serial.print (inBuf [ i ], HEX );
      }
      Serial.println ( "  v" );
      #endif

      dumping = 0;
      inBufPtr = 0;
      
      if ( commandedSpeed == 0 ) {
        waitForZeroSpeed = 0;
        oldCommandedDirection = commandedDirection;
      }
      
      if ( commandedDirection != oldCommandedDirection ) {
        // wait for speed command to go to zero before proceeding!
        waitForZeroSpeed = 1;
      }
      
    } else if (    ( ! (   ( theChar == ' ' )
                        || ( theChar == '-' ) 
                        || ( isDigit ( theChar ) ) ) ) 
                || ( inBufPtr >= inBufLen ) ) {
      // we need to resynchronize; start dumping characters
      // causes: either a buffer overflow or a char that's neither a digit nor a space
      dumping = 0;
      inBufPtr = 0;
    } else if ( ! dumping ) {
      // buffer the character and press on
      inBuf [ inBufPtr++ ] = theChar;
    } else {
      // we're dumping. Dump on!
    }
    #if VERBOSE >= 10
      for ( int i = 0; i < inBufPtr; i++ ) {
        Serial.print ( " 0x" ); Serial.print (inBuf [ i ], HEX );
      }
      Serial.println ();
    #endif
    lastCharReceivedAt_ms = millis();
  }
  
  
  if ( ( millis() - lastCommandAt_ms ) < AGEOLDESTFRESHMESSAGE_ms ) {
    // calculate wheel speeds

    if ( waitForZeroSpeed == 0 ) {
      if ( commandedSpeed == 0 ) {
        // zero-turn mode
        wheelSpeedCommands [ 0 ] = commandedDirection * pct2cmd (   commandedSteering );
        wheelSpeedCommands [ 1 ] = commandedDirection * pct2cmd ( - commandedSteering );
      } else {
        // turn while moving
        float steeringAmount = STEERINGGAIN * commandedSteering;
        wheelSpeedCommands [ 0 ] = commandedDirection * pct2cmd ( max (  1, commandedSpeed - steeringAmount ) );
        wheelSpeedCommands [ 1 ] = commandedDirection * pct2cmd ( max (  1, commandedSpeed + steeringAmount ) );
      }
    } else {
      // direction was changed; full stop until zero speed is commanded
      wheelSpeedCommands [ 0 ] = 0;
      wheelSpeedCommands [ 1 ] = 0;
    }
    if ( 1 && ( ( millis() - lastPrintAt_ms ) > PRINTINTERVAL_ms ) ) {
      #if VERBOSE >= 2
        Serial.print ( "Direction: " ); Serial.print ( commandedDirection );
        Serial.print ( " Speed: " ); Serial.print ( commandedSpeed );
        Serial.print ( " Steering: " ); Serial.print ( commandedSteering );
        Serial.print ( "   " ); 
        Serial.print ( " : ( " ); Serial.print ( wheelSpeedCommands [ 0 ] );
        Serial.print ( ", " ); Serial.print ( wheelSpeedCommands [ 1 ] );
        Serial.print ( "; waitForZeroSpeed: " ); Serial.print ( waitForZeroSpeed );
        Serial.println ( ") " ); 
      #endif
      
      lastPrintAt_ms = millis();
    }
    
    sendWheelSpeedCommandsToSlave ( wheelSpeedCommands );
    stopped = 0;
    
  } else {
    if ( ! stopped ) {
      wheelSpeedCommands [ 0 ] = 0;
      wheelSpeedCommands [ 1 ] = 0;
      sendWheelSpeedCommandsToSlave ( wheelSpeedCommands );
      Serial.println ( "STOP 1" );
      stopped = 1;
    }
  }
   
  // lastCommandAt_ms = 0;
  lastSendAt_ms = millis();
  
  if ( ( millis() - lastFlashAt_ms ) > LEDFLASHINTERVAL_ms ) {
    digitalWrite ( pdSTATUSLED, 1 - digitalRead ( pdSTATUSLED ) );
    lastFlashAt_ms = millis();
  }
     
}

short pct2cmd ( float pct ) {
  if ( abs ( pct ) < 0.1 ) {
    return ( 0 );
  } else if ( pct > 0 ) {
    return ( pct * 0.01 * maxSpeedAboveMin + MINSPEED );
  } else {
    return ( pct * 0.01 * maxSpeedAboveMin - MINSPEED );
  }
}


void sendWheelSpeedCommandsToSlave ( short wheelSpeedCommands [ 2 ] ) {

  master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
  delay ( 5 );
  master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );
  delay ( 5 );
  digitalWrite ( pdINDICATORLED, ! digitalRead ( pdINDICATORLED ) );
  
  #if VERBOSE >= 8
    Serial.print   ( "Speeds: " ); 
    Serial.print   ( wheelSpeedCommands [ 0 ] );
    Serial.print   ( ", " ); 
    Serial.println ( wheelSpeedCommands [ 1 ] );
  #endif
}