#define VERSION "1.1.0"
#define VERDATE "2013-04-18"
#define PROGMONIKER "MCW"

/*
  Authors:
    Adam Siegel
    Charles B. Malloch, PhD
    
  Arduino program for the EMMA slave that will directly communicate with the
  motor controller. Its task is to relay velocity commands to each wheel and to
  shut everything down if either:
    1) ESTOP is asserted (active LOW)
    2) updates from the command computer cease for watchdog_timeout_value_ms milliseconds.
     
  Arduino connections (digital pins):
     0 RX - reserved for serial comm - left unconnected
     1 TX - reserved for serial comm - left unconnected
     4 ESTOP (yellow)
     5 INTERRUPT (RFU) (blue)
     6 RX - RS485 connected to MAX485 pin 1 (green)
     7 TX - RS485 connected to MAX485 pin 4 (orange)
     8 MAX485 driver enable (purple)
     9 status LED
   
 
Plans:
 
*/

#define watchdog_timeout_value_ms 500

// TESTMODE will use regular serial port and enable more reporting
#define TESTMODE 1

// regular serial port talks to motor controller
#define BAUDRATE 115200
// the SoftwareSerial port talks to the EMMA bus
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600

// pin definitions

#define pdESTOP           4
#define pdINTERRUPT       5
#define pdRS485RX         6
#define pdRS485TX         7
#define pdRS485_TX_ENABLE 8

#define pdSTATUSLED       9

static byte nPinDefs;
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

#define SLAVE_ADDRESS 0x01


#include <Arduino.h>
#include <SoftwareSerial.h>
SoftwareSerial MAX485 (pdRS485RX, pdRS485TX);
// wah... indirectly used, but have to include for compiler
#include <RS485.h>
#include <CRC16.h>
#include <MODBUS.h>
MODBUS MODBUS_port ( ( Stream * ) &MAX485, pdRS485_TX_ENABLE );
#include <MODBUS_Slave.h>
MODBUS_Slave mb;


// note that short is 16 bits
#define nCoils 1
unsigned short coils [ ( nCoils + 15 ) >> 4 ];
                                     
/*
 Register assignment:
   0: left motor speed command
   1: right motor speed command
   2: time (ms) since last command was received from master
*/

#define nRegs 3
short regs[nRegs];

#define bufLen 80
char strBuf [ bufLen + 1 ];
int bufPtr;

int coilValue ( unsigned short * coils, int num_coils, int coilNo );
int setCoilValue ( unsigned short * coils, int num_coils, int coilNo, int newValue );

void setup() {
  int i;
  
  Serial.begin( BAUDRATE ); 
  bufPtr = 0;
  MAX485.begin ( BAUDRATE485 );

  mb.Init ( SLAVE_ADDRESS, nCoils, coils, nRegs, regs, MODBUS_port );
  mb.Set_Verbose ( ( Stream * ) &Serial, -1 );
  // mb.Set_Verbose ( ( Stream * ) &Serial, 10 );

  #if TESTMODE > 0
    Serial.print ( PROGMONIKER );
    Serial.print ( ": Test EMMA Motor Control Watchdog Slave v");
    Serial.print ( VERSION );
    Serial.print ( " (" );
    Serial.print ( VERDATE );
    Serial.print ( ")\n" );
    Serial.print ("  at address 0x"); 
    Serial.print (SLAVE_ADDRESS, HEX); 
    Serial.print (" ready in TEST mode ");
    Serial.println ( TESTMODE );
  #endif
 
  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( i = 0; i < nPinDefs; i++ ) {
    Serial.print ( i ); Serial.print ( ": " );
    Serial.print ( pinDefs [i] [0] ); Serial.print ( ", " );
    Serial.print ( pinDefs [i] [1] ); Serial.print ( ", " );
    Serial.print ( pinDefs [i] [2] ); Serial.println ( );
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
      
  // initialize the motor controller
  // set timeout value (ms)
  //   robot will stop automatically if no commands come for this interval
  Serial.println ( "^rwd 1000" );
  // mixing mode 0 (separate) or 1 (ch 1 speed, ch2 steering)
  Serial.println ( "^mxmd 0" );
  Serial.println ( "%eesav" );

}

void loop () {
  static unsigned long last_command_at_ms;
  int status;
  static unsigned long lastBlinkToggleAt_ms = 0;
  static bool halted = false;
  static int LEDstatus = 0;
  
  status = mb.Execute ();
  
  #if TESTMODE > 2
    if ( status == 1 ) {
      reportCoils();
      reportRegs();
    }
    if ( status < 0 ) {
      // error
      Serial.print ( "Error: " ); Serial.println ( status );
    }
  #endif

  /*
    Every time through the loop, we will check to see if there’s a new command.
    If so, we handle the command and record the time we noticed the new command.
    We tell if there was a new command by checking coil 0; the master will turn it
    on when it sends a new command. We turn it off so we can tell when the master
    turns it on again.
  */

  if ( coilValue ( coils, nCoils, 0 ) == 1 ) {
  
    // we got new values from the master
    last_command_at_ms = millis ();
    
    if ( halted && ( regs[0] == 0 ) && ( regs[1] == 0 ) ) {
      // reset if master has sent zeroes to all motors
      halted = false;
      Serial.println (" ********** RESET RECEIVED **********" );
    }
    
    // don't use else here, since we want to test again
    if ( ! halted ) {
      // send new speed commands to motors
      Serial.print ( "!g 1 "); Serial.println( - regs[0] );
      Serial.print ( "!g 2 "); Serial.println( - regs[1] );
    }
    // note that we’ve processed this command
    setCoilValue ( coils, nCoils, 0, 0 );
    digitalWrite ( pdSTATUSLED, ! digitalRead ( pdSTATUSLED ) );
    // Serial.print (" LED "); Serial.print (pdSTATUSLED); Serial.print (":  "); Serial.println (LEDstatus);
    
  }
  
  // we’ve handled any pending update from the master
  // now we need to check to make sure it’s not time to stop everything (as in emergency or timeout)

  int ESTOP = ! digitalRead ( pdESTOP );
  unsigned long time_since_last_command_ms = millis () - last_command_at_ms;
  if ( ( time_since_last_command_ms > watchdog_timeout_value_ms ) || ESTOP ) {
    // oops! we’ve had a timeout or an ESTOP. Stop everything
    
    if ( ! halted ) {
    
      // set both wheel speeds to 0
      Serial.println ( "!g 1 0");
      Serial.println ( "!g 2 0");
    
      if ( ESTOP ) {
        Serial.println ("Awk! ESTOP");
      } else {
        // must have been timeout
        snprintf ( strBuf, bufLen, "Awk! Timeout of %d ms\n", time_since_last_command_ms );
        Serial.print ( strBuf );
      }
      
      halted = true;
      
      digitalWrite ( pdSTATUSLED, 1 );
    }

  }
}

// defined automatically for Arduino
// _BV creates a mask for bit n
// #define _BV(n) (1 << n) 

#define coilArrayElementSize_log2bits 4
#define coilArrayElementSizeMask ( ~ ( 0xff << coilArrayElementSize_log2bits ) )
// coilNo >> 4 is a quicker way to divide coilNo by 16, giving us the coils array element number
#define coilArrayElement(coilNo) ( coilNo >> coilArrayElementSize_log2bits )

int coilValue ( unsigned short * coils, int num_coils, int coilNo ) {
  /*
    Given the coils array, its size, and a coil number, this subroutine returns the value of that coil
    Returns -1 if the specified coil is invalid
    in which coil coilNo is located
    coilNo & 0x000f is a way to identify the bit within the appropriate word for that particular coil
  */
  if (coilNo < num_coils) {
    short coilArrayEltNo = coilArrayElement ( coilNo );
    short coilBitMask = _BV ( coilNo & coilArrayElementSizeMask );
    return ( ( coils [ coilArrayEltNo ] & coilBitMask ) != 0 );
  } else {
    return ( -1 );
  }
}

int setCoilValue ( unsigned short * coils, int num_coils, int coilNo, int newValue ) {
  /*
    Given the coils array, its size, and a coil number, this subroutine sets the value of that coil
    Returns the new value if the specified coil is valid
    Returns -1 if the specified coil is invalid
  */
  if (coilNo < num_coils) {
    short coilArrayEltNo = coilArrayElement ( coilNo );
    short coilBitMask = _BV ( coilNo & coilArrayElementSizeMask );
    if ( newValue ) {
      // set the bit
      coils [ coilArrayEltNo ] |= coilBitMask;
    } else {
      // clear the bit
      coils [ coilArrayEltNo ] &= ~ coilBitMask;
    }
    return ( ( coils [ coilArrayEltNo ] & coilBitMask ) != 0 );
  } else {
    return ( -1 );
  }
}


#ifdef BAUDRATE
void reportCoils () {
  int i;
  unsigned short bitval;
  Serial.println ("Coils:");
  for (i = 0; i < nCoils; i++) {
    bitval = ( coils [ i >> 4 ] >> (  i & 0x000f ) ) & 0x0001;
    Serial.print ("  "); Serial.print (i); Serial.print (": "); Serial.print (bitval); 
    Serial.println ();
  }
}

void reportRegs () {
  int i;
  Serial.println ("Registers:");
  for (i = 0; i < nRegs; i++) {
    Serial.print ("  "); Serial.print (i); Serial.print (": "); Serial.print (regs[i]); 
    Serial.println ();
  }
}
#endif