#define VERSION "0.3.0"
#define VERDATE "2013-02-22"
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

  ATtiny connections (digital pins):

     0 RX - RS485 connected to MAX485 pin 1
     1 TX - RS485 connected to MAX485 pin 4
     2 MAX485 driver enable
     3-
     4 } coils 0-2 outputs to LEDs
     5 -

    
  Plans:
    Either #define or inline functions for offset and mask values given position and size
    
    SO SAD... TOO BAD... TOO BIG TO FIT ON ATTINY85!
    
*/

// TESTMODE will use regular serial port and enable more reporting
#undef TESTMODE
// #define TESTMODE 1

#undef BAUDRATE
// #define BAUDRATE 115200
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600

#define RS485RX 0
#define RS485TX 1
#define pdRS485_TX_ENABLE 2

#include <SoftwareSerial.h>
SoftwareSerial MAX485 (RS485RX, RS485TX);

// wah... indirectly used, but have to include for compiler
#include <RS485.h>
#include <CRC16.h>

#include <MODBUS.h>
MODBUS MODBUS_port ( ( Stream * ) &MAX485, pdRS485_TX_ENABLE );

#include <MODBUS_Slave.h>
MODBUS_Slave mb;

// note that short is 16 bits

char mySlaveAddress;
#define nCoils 3
unsigned short coils [ ( nCoils + 15 ) >> 4 ];
static byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                        { 1,  2, -1 },
                                        { 0,  3,  0 },
                                        { 0,  4,  1 },
                                        { 0,  5,  2 }
                                      };
                                      
#define nRegs 8
short regs[nRegs];

#ifdef BAUDRATE
#define bufLen 80
unsigned char strBuf [ bufLen + 1 ];
int bufPtr;
#endif

void setup() {

  int i;
  
  #ifdef BAUDRATE
    Serial.begin ( BAUDRATE );
  #endif
  
  MAX485.begin ( BAUDRATE485 );
  
  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( i = 0; i < nPinDefs; i++ ) {
    #if 0 && BAUDRATE
      Serial.print ( i ); Serial.print ( ": " );
      Serial.print ( pinDefs [i] [0] ); Serial.print ( ", " );
      Serial.print ( pinDefs [i] [1] ); Serial.print ( ", " );
      Serial.print ( pinDefs [i] [2] ); Serial.println ( );
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
  
  mySlaveAddress = 0x03;
  mb.Init ( mySlaveAddress, nCoils, coils, nRegs, regs, MODBUS_port );
  
  #if BAUDRATE
    mb.Set_Verbose ( ( Stream * ) &Serial, -1 );
    
    Serial.print ( PROGMONIKER );
    Serial.print ( ": Test MODBUS Slave v");
    Serial.print ( VERSION );
    Serial.print ( " (" );
    Serial.print ( VERDATE );
    Serial.print ( ")\n" );
    Serial.print ("  at address 0x"); Serial.print (mySlaveAddress, HEX); Serial.println (" ready in TEST mode");
  #endif

  for (i = 0; i < ( nCoils + 15 ) >> 4; i++) {
    coils[i] = (i % 2) ? 0xdcba : 0x8765;
  }
  for (i = 0; i < nRegs; i++) {
    regs[i] = i * 3;
  }
  
}

void loop() {

  int status;
  static unsigned long lastBlinkToggleAt_ms = 0;

  status = mb.Execute ();
  #ifdef BAUDRATE
    if ( status == 1 ) {
      reportCoils();
      reportRegs();
    }
    if ( status < 0 ) {
      // error
      Serial.print ( "Error: " ); Serial.println ( status );
    }
  #endif
  
  refreshCoils();
  
  if ( ( millis() - lastBlinkToggleAt_ms ) > 1000 ) {
    digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
    lastBlinkToggleAt_ms = millis();
  }
  
}

void refreshCoils () {
  
  unsigned char coilsThatChanged = 0;
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
          coilsThatChanged |= ~coils[coilWord] & mask;
          coils[coilWord]  |=   mask;
        } else {
          // low
          coilsThatChanged |=  coils[coilWord] & mask;
          coils[coilWord]  &= ~ mask;
        }
      } else {
        // output pin
        digitalWrite ( pinDefs [i] [1], coils[coilWord] & mask );
      }
    }
  }
     
  #ifdef BAUDRATE
    if ( coilsThatChanged ) {
      // will fail if more than 16 coils, because each word will be mapped on this single word!
      Serial.print ( "Mask of coils that changed: 0x" ); Serial.print ( coilsThatChanged, HEX ); Serial.println ();
      reportCoils();
    }
  #endif
  
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