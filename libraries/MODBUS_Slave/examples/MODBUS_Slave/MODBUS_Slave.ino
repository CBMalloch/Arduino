#define VERSION "0.2.1"
#define VERDATE "2013-02-15"
#define PROGMONIKER "TMS"

/*
  create and test a MODBUS slave on RS-485
  
  MAX485 connection:
    1 RO receiver output - data received by the chip over 485 is output on this pin
    2 ~RE receive enable (active low)
    3 DE driver enable
    4 DI driver input - input on this pin is driven onto RS485 output
    5 GND
    6 A - noninverting receiver input and noninverting driver output
    7 B - inverting receiver input and inverting driver output
    8 Vcc - 4.75V-5.25V
 
*/

/*

  Arduino connections (digital pins):
     0 RX - reserved for serial comm - left unconnected
     1 TX - reserved for serial comm - left unconnected
     2 RX - RS485 connected to MAX485 pin 1
     3 TX - RS485 connected to MAX485 pin 4
     4 MAX485 driver enable
     5 pulled-up input - COIL 0
     6 pulled-up input - COIL 1
     7 RS485 address high bit
     8 RS485 address low bit
     9 pulled-up input - COIL 2
    10 -
    11  } coils 3-5 outputs to LEDs
    12 -
    13 status LED

*/

// TESTMODE will use regular serial port and enable more reporting
// #undef TESTMODE
#define TESTMODE 1

#define BAUDRATE 115200

#include <SoftwareSerial.h>
#include <MODBUS_Slave.h>

// RS485_TX_ENABLE = 1 -> transmit enable
#define pdRS485_TX_ENABLE 4
#define RS485RX 2
#define RS485TX 3

#define SLAVE_ADDRESS_HIGH 7
#define SLAVE_ADDRESS_LOW  8

// note that short is 16 bits

byte mySlaveAddress;
#define nCoils 6
unsigned short coils [ ( nCoils + 15 ) >> 4 ];
static byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                        { 1,  4, -1 },
                                        { 1,  5,  0 },
                                        { 1,  6,  1 },
                                        { 1,  7, -1 },
                                        { 1,  8, -1 },
                                        { 1,  9,  2 },
                                        { 0, 10,  3 },
                                        { 0, 11,  4 },
                                        { 0, 12,  5 },
                                        { 0, 13, -1 }
                                      };
                                      
#define nRegs 8
short regs[nRegs];

#ifdef TESTMODE
  #define MAX485 Serial
#else
  SoftwareSerial MAX485 (RS485RX, RS485TX);
#endif

MODBUS_Slave mb;

#define bufLen 80
byte strBuf [ bufLen + 1 ];
int bufPtr;

void setup() {

  int i;
  
  MAX485.begin(BAUDRATE);  
  bufPtr = 0;
  
  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( i = 0; i < nPinDefs; i++ ) {
    #if TESTMODE
      MAX485.print ( i ); MAX485.print ( ": " );
      MAX485.print ( pinDefs [i] [0] ); MAX485.print ( ", " );
      MAX485.print ( pinDefs [i] [1] ); MAX485.print ( ", " );
      MAX485.print ( pinDefs [i] [2] ); MAX485.println ( );
    #endif
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
  
  mySlaveAddress = ( digitalRead ( SLAVE_ADDRESS_HIGH ) << 1 ) | digitalRead ( SLAVE_ADDRESS_LOW );
  mb.Init ( mySlaveAddress, nCoils, coils, nRegs, regs, (Stream*) &MAX485, pdRS485_TX_ENABLE );
  
  #if TESTMODE
    MAX485.print ( PROGMONIKER );
    MAX485.print ( ": Test MODBUS Slave v");
    MAX485.print ( VERSION );
    MAX485.print ( " (" );
    MAX485.print ( VERDATE );
    MAX485.print ( ")\n" );
    MAX485.print ("  at address 0x"); MAX485.print (mySlaveAddress, HEX); MAX485.println (" ready in TEST mode");
  #endif

  for (i = 0; i < ( nCoils + 15 ) >> 4; i++) {
    coils[i] = (i % 2) ? 0xdcba : 0x8765;
  }
  for (i = 0; i < nRegs; i++) {
    regs[i] = i * 3;
  }
  
  #if TESTMODE
    // reportCoils();
    // reportRegs();
  #endif
}

void loop() {

  unsigned char somethingChanged;
  static unsigned long lastBlinkAt_ms = 0;

  somethingChanged = mb.Execute ();
  #ifdef TESTMODE
    if ( somethingChanged ) {
      reportCoils();
      reportRegs();
    }
  #endif
  
  refreshCoils();
  
  if ( millis() - lastBlinkAt_ms > 500 ) {
    digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
  }
  
}

void refreshCoils () {
  
  unsigned char somethingChanged = 0;
  int i;
  unsigned short mask, coilWord;
  for ( i = 0; i < nPinDefs; i++ ) {
  
    if ( pinDefs [i] [2] != -1 ) {
    
      // it's associated with a coil
      
      coilWord = pinDefs [i] [2] >> 4;
      mask = 0x0001 << pinDefs [i] [2] ;
      
      if ( pinDefs [i] [0] == 1 ) {
        // input pin
        if ( digitalRead ( pinDefs [i] [1] ) ) {
          // high
          somethingChanged |= ! ( coils[coilWord] & mask );
          coils[coilWord]  |=   mask;
        } else {
          // low
          somethingChanged |=   ( coils[coilWord] & mask );
          coils[coilWord]  &= ~ mask;
        }
      } else {
        // output pin
        digitalWrite ( pinDefs [i] [1], coils[coilWord] & mask );
      }
    }
  }
     
  #ifdef TESTMODE
    if ( somethingChanged ) {
      reportCoils();
    }
  #endif
}


#ifdef TESTMODE
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