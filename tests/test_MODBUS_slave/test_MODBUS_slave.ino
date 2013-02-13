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
#undef TESTMODE
// #define TESTMODE 1

#define BAUDRATE 115200

#include <SoftwareSerial.h>
#include <MODBUS_Slave.h>

// pdTALK = 1 -> transmit enable, receive disable
#define pdTALK 4
#define RS485RX 7
#define RS485TX 8

#define pdLED 13

#define MSGDELAY_MS 250

// note that short is 16 bits

#define slave 0x000c
#define nCoils 12
unsigned short coils[ ( nCoils + 15 ) >> 4 ];
#define nRegs 8
short regs[nRegs];

#ifdef TESTMODE
#define MAX485 Serial
MODBUS_Slave mb( slave, nCoils, coils, nRegs, regs, (Stream*) &Serial );
#else
SoftwareSerial MAX485 (RS485RX, RS485TX);
MODBUS_Slave mb( slave, nCoils, coils, nRegs, regs, (Stream*) &MAX485 );
#endif

#define bufLen 80
byte strBuf[bufLen+1];
int bufPtr;

void setup() {

  int i;
  
  MAX485.begin(BAUDRATE);  
  bufPtr = 0;
  
  for (i = 0; i < ( nCoils + 15 ) >> 4; i++) {
    coils[i] = (i % 2) ? 0xdcba : 0x8765;
  }
  for (i = 0; i < nRegs; i++) {
    regs[i] = i * 3;
  }
  
  #if TESTMODE
    Serial.print ("MODBUS_Slave at address 0x"); Serial.print (slave, HEX); Serial.println (" ready in TEST mode");
    // reportCoils();
    // reportRegs();
  #endif
}

void loop() {

  // capture characters from stream
  if ( MAX485.available() ) {
    if ( bufPtr < bufLen ) {
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
  
  #ifndef TESTMODE
    refreshCoils();
  #endif
  
}

void refreshCoils () {
  // coils 0 through 6 will drive LEDs on d2-d8
  // d9-d12 will be read into coils 7-10
  
  int i;
  unsigned short mask;
  for ( i = 0; i <= 6; i++ ) {
    mask = 0x0001 << i ;
    if ( digitalRead ( i + 2 ) ) {
      coils[0] |= mask;
    } else {
      coils[0] &= ~ mask;
    }
  }
  
  for ( i = 7; i <= 10; i++ ) {
    mask = 0x0001 << i;
    digitalWrite ( i + 2, coils[0] & mask );
  }
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