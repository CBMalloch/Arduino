#define VERSION "0.2.0"
#define VERDATE "2013-11-04"
#define PROGMONIKER "MT3"

/*

  Serves as master on EMMA bus, communicating speed commands for each wheel to slave
  
  This test expects a MLX90609 angular rate sensor to be attached, which should help steer straight.
  In actual use, a rate sensor drifts, and vibration might make it drift more. Direction must be 
  calculated by integrating instantaneous rates; anything that happens between rate readings is lost.
  In addition, any inaccuracy in a reading is integrated and kept forever. I have measured a drift rate 
  *directly related* to the reading frequency, and have seen up to 30deg/hour drift in a fixed, non-vibrating 
  setting.
  
  This test is intended as a proof-of-concept. The plan is to have EMMA:
    1) turn L 90deg,
    2) turn R 90deg to straight,
    3) turn R 90deg,
    4) turn L 90deg to straight again.
 
  It now works. Next step will be making this program's activities a subroutine, and 
  setting up a hex or octal encoder switch to select that or another subroutine.

     
  Arduino connections (digital pins):
     0 RX - reserved for serial comm - left unconnected
     1 TX - reserved for serial comm - left unconnected
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

#define pdMLXCS          10
#define pdMLXMOSI        11
#define pdMLXMISO        12
#define pdMLXSCK         13

#define pdSELECT_BIT_0   14
#define pdSELECT_BIT_1   15
#define pdSELECT_BIT_2   16
#define pdSELECT_BIT_3   17

// MLX90609 function codes
#define MLX_STATR 0x88
#define MLX_ADCC  0x90
#define MLX_CHAN  0x08
#define MLX_ADEN  0x04
#define MLX_ADCR  0x80

// end-of-conversion (ADC complete)
#define MLX_EOC ( strBuf [ 0 ] & 0x40 )
// not command accepted
#define MLX_nCA ( strBuf [ 0 ] & 0x80 )
// still busy
#define MLX_BUSY ( ( strBuf [ 0 ] & 0x88 ) == 0x88 )


#define LOOP_DELAY_ms 100
// #define TIME_TO_WAIT_FOR_REPLY_us 20000LU

// wah... indirectly used, but have to include for compiler
#include <RS485.h>
#include <CRC16.h>
#include <SPI.h>

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
                                       { 0,  pdSTATUSLED      , -1 },
                                       { 0,  pdINDICATORLED   , -1 },
                                       { 0,  pdMLXCS          , -1 },
                                       { 0,  pdSELECT_BIT_0  ,  -1 },
                                       { 0,  pdSELECT_BIT_1  ,  -1 },
                                       { 0,  pdSELECT_BIT_2  ,  -1 },
                                       { 0,  pdSELECT_BIT_3  ,  -1 }
                                      };
                                      
#define bufLen 80
char strBuf [ bufLen + 1 ];
int bufPtr;

#define MINSPEED 200

short testNumber = -1;

short wheelSpeedCommands [ 2 ] = { 0, 0 };

float MLXZero = 0.0;


void setup () {

 // Open serial communications and wait for MODBUS_port to open:
  Serial.begin(BAUDRATE);
  MAX485.begin(BAUDRATE485);
  SPI.begin();

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
  
  digitalWrite ( pdMLXCS, 1 );  // deselect the MLX90609
  
  // master.Set_Verbose ( ( Stream * ) &Serial, 6 );
  
  // reset motor control slave in case it was in timeout or estop
  // master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
  // delay ( 5 );
  // master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );
  // delay ( 5 );
  
  // delay ( 200 );  // just for yucks
    
  // initialize MLX90609
  bufPtr = transact  ( MLX_ADCC | MLX_ADEN, 2, (byte *) strBuf );
  
  if ( VERBOSE > 1 ) {
    Serial.print ( "activate ADC: reply (length " );
    Serial.print ( bufPtr );
    Serial.println ( ")" );
    for ( int i = 0; i < bufPtr; i++ ) {
      Serial.print ( " 0x" );
      Serial.print ( strBuf [ i ], HEX );
    }
    Serial.println ();
  }
  
 if ( MLX_nCA ) {
    Serial.println ("Halting because initialization refused"); for ( ;; ) { }
  }

  // Serial.println ("Halting"); for ( ;; ) { }
  
  Serial.println ( "Calibrating MLX90609..." );
  
  // bufPtr = transact  ( 0x94, 2, (byte *) strBuf );
  // Serial.println ( MLX_ADCC, HEX );
  // Serial.println ( MLX_ADEN, HEX );
  // Serial.println ( MLX_ADCC | MLX_ADEN, HEX );
  // bufPtr = transact  ( MLX_STATR, 2, (byte *) strBuf );
    
  // calibrate MLX90609 for zero angular velocity reading
  
  if ( VERBOSE <= 4 ) {
    int MLX_nCAlibrationIterations = 100;
    for (int i = 0; i < MLX_nCAlibrationIterations; i++) {
      int x;
      float rate_mv;
      bufPtr = transact  ( MLX_ADCC | MLX_ADEN, 2, (byte *) strBuf );  // channel 0 - angular rate
 
      if ( ! ( ( ( ( byte ) strBuf [ 0 ] ) & 0xa0 ) == 0x20 
            && ( ( ( byte ) strBuf [ 1 ] ) & 0x06 ) == 0x04 ) ) {
        Serial.print ("Halting because of bad reply to request for ADC: "); 
        Serial.print ( "0x" ); Serial.print ( strBuf [ 0 ], HEX ); 
        Serial.print ( " 0x" ); Serial.print ( strBuf [ 1 ], HEX ); 
        for ( ;; ) { }
      } 
      delay ( 1 );
      bufPtr = transact  ( MLX_ADCR, 2, (byte *) strBuf );
      
      if ( 1 ) {
        Serial.print ( "Reply: " );
        for ( int k = 0; k < 8; k++ ) {
          Serial.print ( ( strBuf [ 0 ]  >> ( 7 - k )) & 0x01 );
        }
        Serial.print ( " " );
        for ( int k = 0; k < 8; k++ ) {
          Serial.print ( ( strBuf [ 1 ]  >> ( 7 - k )) & 0x01 );
        }
        Serial.print ( "B: " );
      }
      
      x = ( ( int ( ( byte ) strBuf [ 0 ] & 0x0f ) << 8 ) + ( byte ) strBuf [ 1 ] ) >> 1;
      // rate_mv = 25.0 / 12.0 * x + 400.0;
      // Serial.print ( " 0x" ); Serial.print ( (byte) strBuf [ 0 ], HEX ); 
      // Serial.print ( " 0x" ); Serial.print ( (byte) strBuf [ 1 ], HEX ); 
      Serial.print ( " 0x" ); Serial.print ( x, HEX ); 
      Serial.print (";  x: "); Serial.println (x);
      
      // the following fails to work because of the endian-ness issue
      // x = ( ( ( (int *) strBuf ) [ 0 ] ) & 0x0ffe ) >> 1;
      // Serial.print ("x: "); Serial.println (x);
      
      MLXZero += x;
      delay ( 20 );
    }
    MLXZero = MLXZero / float ( MLX_nCAlibrationIterations );
    Serial.print ("Bias difference: "); Serial.println ( MLXZero - 1008 );
    
    if ( abs ( MLXZero - 1008 ) > 50 ) {
      Serial.println ("Halting from excessive bias difference"); for ( ;; ) { }
    }
    
    
    // MLXZero = 0.0;
  }
  
  
  testNumber =  ( digitalRead ( pdSELECT_BIT_3 ) << 3 )
              | ( digitalRead ( pdSELECT_BIT_2 ) << 2 )
              | ( digitalRead ( pdSELECT_BIT_1 ) << 1 )
              | ( digitalRead ( pdSELECT_BIT_0 ) << 0 );

  testNumber = 2;

}

#define maxSpeedAboveMin 100
  
void loop () {
  
  static unsigned long loopBeganAt_ms;
  float testPct, profiler;
  static unsigned long testBeganAt_ms = millis();
  static short testState = -1;
  
  // variables for the angular rate sensor
  int x;
  float rate_mv, rate;
  static float delta_heading, heading = 0;
  static unsigned long heading_read_at_us = 0UL, last_print_at_ms = 0;
  
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
                        lastFlashAt_ms = 0,
                        flashStateBeganAt_ms = 0;
  static short nFlashes = 0, flashState = 0;
  short direction;
  
  // receive_status = 0;
  
  #if VERBOSE >= 2
  if ( ( millis() - lastPrintAt_ms ) > print_interval_ms ) {
    // Serial.print ( "Send interval (ms): " ); Serial.println ( send_command_interval_ms );
    // if ( lastLoopAt_us != 0LU ) {
      // Serial.print( "  Loop time (us): "); Serial.println ( micros() - lastLoopAt_us );
    // }
    Serial.print ( "Test selected: " ); Serial.println ( testNumber );
    lastPrintAt_ms = millis();
  }
  lastLoopAt_us = micros();
  #endif
  
  testPct = float ( millis() - testBeganAt_ms ) / float ( test_duration_ms );
  
  bufPtr = transact  ( MLX_ADCC | MLX_ADEN, 2, (byte *) strBuf );  // channel 0 - angular rate
  delay ( 1 );
  bufPtr = transact  ( MLX_ADCR, 2, (byte *) strBuf );
  x = ( ( ( ( (byte) strBuf [ 0 ] & 0x0f ) ) << 8 ) + (byte) strBuf [ 1 ] ) >> 1;
  rate_mv = 25.0 / 12.0 * x + 400.0;
  rate = ( x - MLXZero ) / 12.8;
  
  if ( heading_read_at_us != 0 ) {
    // this is not the first rate reading, so we can now calculate delta heading and then heading
    unsigned long delta_t_us;
    delta_t_us = micros() - heading_read_at_us;
    heading_read_at_us = micros();
    delta_heading = rate * delta_t_us / 1.0e6;
    heading += delta_heading;
  } else {
    heading_read_at_us = micros();
  }

  if ( 1 && ( millis() - last_print_at_ms ) > print_interval_ms ) {

    Serial.print ( "Raw rate: " ); Serial.print ( x );
    Serial.print ( "   " ); 
    // Serial.print ( rate_mv ); Serial.print ( " mv" );
    // Serial.print ( "   " ); 
    // Serial.print ( rate ); Serial.print ( " deg/S" );
    // Serial.print ( "" );
    
    Serial.print ( "        Heading: " );
    Serial.print ( heading ); Serial.println ( " deg" );
    
    Serial.print ( "Test state: " ); Serial.println ( testState );
    
    last_print_at_ms = millis();
  }
   
  if ( testPct <= 1.0 ) {
  
    // test is still running
    
    if ( ( millis() - lastSendAt_ms ) > send_command_interval_ms ) {
      // construct packet to send new values to the registers
      
      switch ( testNumber ) {
        case 0:
          break;
        case 1:
          rotate90deg4x ( testState, heading, wheelSpeedCommands );
          break;
        case 2:
          goToWall ( testState, heading, wheelSpeedCommands );
          break;
        default:
          break;
      }

      master.Write_Regs ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 2, wheelSpeedCommands );
      delay ( 5 );
      master.Write_Single_Coil ( MOTOR_CONTROL_SLAVE_ADDRESS, 0, 1 );
      delay ( 5 );
      digitalWrite ( pdINDICATORLED, ! digitalRead ( pdINDICATORLED ) );
      
      #if VERBOSE >= 1
        Serial.print   ( "Speeds: " ); 
        Serial.print   ( wheelSpeedCommands [ 0 ] );
        Serial.print   ( ", " ); 
        Serial.println ( wheelSpeedCommands [ 1 ] );
      #endif
           
      lastSendAt_ms = millis();
      
    }
    
    #define flashRestDur 1000UL
    #define flashDur      100UL
    #define flashInterval  50UL
    
    switch ( flashState ) {
      case 0:
        // flash waiting between bursts
        if ( ( millis() - flashStateBeganAt_ms ) > flashRestDur ) {
          // begin new burst
          nFlashes = 0;
          digitalWrite ( pdSTATUSLED, 1 );
          flashState = 1;
          flashStateBeganAt_ms = millis();
        }
        break;
      case 1:
        // flash on within burst
        if ( ( millis() - flashStateBeganAt_ms ) > flashDur ) {
          // turn off
          nFlashes++;
          digitalWrite ( pdSTATUSLED, 0 );
          flashState = 2;
          flashStateBeganAt_ms = millis();
        }
        break;
      case 2:
        // flash off within burst
        if ( ( millis() - flashStateBeganAt_ms ) > flashInterval ) {
          if ( nFlashes < ( testState + 1 ) ) {
            // turn (back) on
            digitalWrite ( pdSTATUSLED, 1 );
            flashState = 1;
            flashStateBeganAt_ms = millis();
          } else {
            flashState = 0;
            flashStateBeganAt_ms = millis();
            digitalWrite ( pdSTATUSLED, 0 );
          }
        }
        break;
      default:
        break;
    }
    
  } else {
  
    // done testing
    for ( ;; ) {
      delay (1000);
    }
  }

}

short pct2cmd ( float pct ) {
  return ( pct * maxSpeedAboveMin ) + MINSPEED;
}



// maybe this is what's causing bad receipts from rate sensor
// it works with test program, whose strBuf is unsigned
// if MLX_nCA is bad, then bad receipts????


int transact ( byte command, int bytesToRead, byte * result ) {

  int i = 0, n = 0;
  
  // Serial.print ( "\nbytesToRead: " ); Serial.println ( bytesToRead );
  result [ 0 ] = 0x80;
  while ( MLX_nCA ) {
    // Serial.print ( "cmd: 0x" ); Serial.println ( command, HEX );
    digitalWrite(pdMLXCS, LOW);
    SPI.transfer ( command );
    for ( i = 0; i < bytesToRead; i++ ) {
      // result [ i ] = 0xff;
      result [ i ] = SPI.transfer ( 0x00 );
      // Serial.print ( "-- 0x" ); Serial.println ( result [ i ], HEX ); 
    }
    digitalWrite(pdMLXCS, HIGH);
    // Serial.print ("                          -> ");
    // for ( i = 0; i < bytesToRead; i++ ) {
      // Serial.print ( " 0x" );
      // Serial.print ( result [ i ], HEX );
    // }
    // Serial.println ();
    n++;
    // delay ( 1 );
  }
  
  if ( MLX_BUSY ) {
    Serial.println ( "Reply MLX_BUSY" );
  }

  // Serial.print ( "n: " ); Serial.println ( n );
  return ( i );
  
}


void rotate90deg4x (short &testState, float heading, short wheelSpeedCommands [ 2 ] ) {
  switch ( testState ) {
    case -1: 
      // startup
      testState = 0;
      Serial.print ( "Entering test state " ); Serial.println ( testState );
      break;
    case 0:
      // turning L 90deg
      if ( heading > -90.0 ) {
        wheelSpeedCommands [ 0 ] = pct2cmd ( 0.2 );
        wheelSpeedCommands [ 1 ] = - pct2cmd ( 0.2 );
      } else {
        wheelSpeedCommands [ 0 ] = 0.0;
        wheelSpeedCommands [ 1 ] = 0.0;
        // on to next test
        testState++;
        Serial.print ( "Entering test state " ); Serial.println ( testState );
      }
      break;
    case 1:
      // turning R 90deg
      if ( heading < 0.0 ) {
        wheelSpeedCommands [ 0 ] = - pct2cmd ( 0.2 );
        wheelSpeedCommands [ 1 ] = pct2cmd ( 0.2 );
      } else {
        wheelSpeedCommands [ 0 ] = 0.0;
        wheelSpeedCommands [ 1 ] = 0.0;
        // on to next test
        testState++;
        Serial.print ( "Entering test state " ); Serial.println ( testState );
      }
      break;
    case 2:
      // turning R 90deg (again)
      if ( heading < 90.0 ) {
        wheelSpeedCommands [ 0 ] = - pct2cmd ( 0.2 );
        wheelSpeedCommands [ 1 ] = pct2cmd ( 0.2 );
      } else {
        wheelSpeedCommands [ 0 ] = 0.0;
        wheelSpeedCommands [ 1 ] = 0.0;
        // on to next test
        testState++;
        Serial.print ( "Entering test state " ); Serial.println ( testState );
      }
      break;
    case 3:
      // turning L 90deg
      if ( heading > 0.0 ) {
        wheelSpeedCommands [ 0 ] = pct2cmd ( 0.2 );
        wheelSpeedCommands [ 1 ] = - pct2cmd ( 0.2 );
      } else {
        wheelSpeedCommands [ 0 ] = 0.0;
        wheelSpeedCommands [ 1 ] = 0.0;
        // on to next test
        testState++;
        Serial.print ( "Entering test state " ); Serial.println ( testState );
      }
      break;
    default:
      break;
  }
}

void goToWall (short &testState, float heading, short wheelSpeedCommands [ 2 ] ) {
  if ( testState < 0 ) {
    testState = 0;
    Serial.print ( "Entering test state " ); Serial.println ( testState );
  }
  wheelSpeedCommands [ 0 ] = pct2cmd ( 0.2 );
  wheelSpeedCommands [ 1 ] = pct2cmd ( 0.2 );
}