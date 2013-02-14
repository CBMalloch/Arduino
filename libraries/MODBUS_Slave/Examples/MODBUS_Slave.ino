#define VERSION "0.1.0"
#define VERDATE "2013-02-14"
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

// TESTMODE will use regular serial port and enable more reporting
// #undef TESTMODE
#define TESTMODE 1

#define BAUDRATE 115200
// how much time we allow for a message to be under construction before we
// decide it's time to discard it as an orphan or as a malformed frame
// #define MESSAGE_TIME_TO_LIVE_ms ( 2 * ( bufLen * 10 ) / ( BAUDRATE / 1000 ) )
// 80 * 10 / 11.5 is about 70 plus some fudge is 100
#define MESSAGE_TIME_TO_LIVE_ms 100

#include <SoftwareSerial.h>
#include <MODBUS_Slave.h>

// pdTALK = 1 -> transmit enable, receive disable
#define pdTALK 4
#define RS485RX 7
#define RS485TX 8

#define pdLED 13

#define MSGDELAY_MS 250

// note that short is 16 bits

byte mySlaveAddress;
#define nCoils 12
unsigned short coils[ ( nCoils + 15 ) >> 4 ];
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
  
  for ( i = 2; i <= 8; i++ ) {
    pinMode ( i, INPUT );
    digitalWrite ( i, 1 );  // enable internal pullups
  }
  for ( i = 9; i <= 12; i++ ) {
    pinMode ( i, OUTPUT );
    digitalWrite ( i, 0 );
  }
  
  mySlaveAddress = 0x0c;
  mb.Init ( mySlaveAddress, nCoils, coils, nRegs, regs, (Stream*) &MAX485 );
  
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

  static unsigned long messageReceiptBeganAt_ms = 0;

  if ( ( ( millis() - messageReceiptBeganAt_ms ) > MESSAGE_TIME_TO_LIVE_ms ) && bufPtr != 0 ) {
    // kill old messages to keep from blocking up with a bad message piece
    #ifdef TESTMODE
      MAX485.println ( "timeout" );
    #endif
    bufPtr = 0;
    messageReceiptBeganAt_ms = 0;
  }
  
  // capture characters from stream
  while ( MAX485.available() ) {

    if ( bufPtr == 0 ) {
      // we are starting the receipt of a message
      messageReceiptBeganAt_ms = millis();
    }
    
    if ( bufPtr < bufLen ) {
      // grab the available character
      strBuf[bufPtr++] = MAX485.read();
    } else {
      // error - buffer overrun -- whine
      for (;;) {
        digitalWrite (pdLED, 1);
        delay (100);
        digitalWrite (pdLED, 0);
        delay (100);
      }
    }
  }

  // bufPtr points to (vacant) char *after* the character string in strBuf,
  // and so it is the number of chars currently in the buffer
  if ( mb.Check_Data_Frame ( strBuf, bufPtr ) == 1 ) {
    // process it
    digitalWrite (pdLED, 1);
    mb.Process_Data ( strBuf, bufPtr );
    #ifdef TESTMODE
      reportCoils();
      reportRegs();
    #endif
    digitalWrite (pdLED, 0);
    bufPtr = 0;
  }
  
  refreshCoils();
  
}

void refreshCoils () {
  // coils 0 through 6 will drive LEDs on d2-d8
  // d9-d12 will be read into coils 7-10
  
  unsigned short somethingChanged = 0;
  int i;
  unsigned short mask;
  for ( i = 0; i <= 6; i++ ) {
    mask = 0x0001 << i ;
    if ( digitalRead ( i + 2 ) ) {
      // digital input is high
      somethingChanged |= ! ( coils[0] & mask );
      coils[0] |= mask;
    } else {
      // digital input is low
      somethingChanged |=   ( coils[0] & mask );
      coils[0] &= ~ mask;
    }
  }
  
  for ( i = 7; i <= 10; i++ ) {
    mask = 0x0001 << i;
    digitalWrite ( i + 2, coils[0] & mask );
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