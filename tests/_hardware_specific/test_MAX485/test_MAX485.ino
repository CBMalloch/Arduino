/*
  test  a MAX485 network
    
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

#define BAUDRATE 115200
#define RS485RX 10
#define RS485TX 11
// RS485_TX_ENABLE = 1 -> transmit enable
#define pdRS485_TX_ENABLE 12

#define pdLED 13

#include <SoftwareSerial.h>
SoftwareSerial MAX485(RS485RX, RS485TX);

#define bufLen 80
char strBuf [ bufLen + 1 ];
short bufPtr;

#define bufLen485 80
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600
unsigned char strBuf485 [ bufLen485 + 1 ];
short bufPtr485;
unsigned long received485At;

void setup() {
  
  for (int i = 12; i <= 13; i++) {
    pinMode (i, OUTPUT);
    digitalWrite (i, LOW);
  }
  pinMode (RS485TX, OUTPUT);

  Serial.begin(BAUDRATE);
 
  MAX485.begin(BAUDRATE485);
  
  bufPtr = 0;
  bufPtr485 = 0;
}

void loop() {

  Serial_Receive ();
  
  if ( bufPtr > 0 ) {
    Serial.print ("ECHO ("); Serial.print (bufPtr); Serial.print ("): ");
    for (int i = 0; i < bufPtr; i++) {
      Serial.print ( strBuf [ i ] );
    }
    Serial.println ();

    MODBUS_Send ( ( unsigned char * ) strBuf, bufPtr );
    bufPtr = 0;
  }
  
  MODBUS_Receive ();
  if ( bufPtr485 > 0 ) {
    Serial.print (" >> ");
    for (int i = 0; i < bufPtr485; i++) {
      Serial.write ( strBuf485 [ i ] );
    }
    Serial.println ();
    bufPtr485 = 0;
  }

}

void MODBUS_Send ( unsigned char *buf, short nChars ) {
  if ( nChars > 0 )	{
    // set the error flag if an exception occurred
    // unsigned short CRC = CheckSum.CRC16 ( buf, nChars );
    // buf[nChars++] = CRC & 0x00ff;				// lower byte
    // buf[nChars++] = CRC >> 8;						// upper byte
    digitalWrite ( pdRS485_TX_ENABLE, HIGH );
    for ( int i = 0; i < nChars; i++ ) {
      MAX485.write ( buf[i] );
      // 1 bit time at 115200 is 9 ms
      // delayMicroseconds (100);  // doesn't help
    }
    digitalWrite ( pdRS485_TX_ENABLE, LOW );
  }
  return;
}

// a character time at 57600 baud = 5760 cps is 1/5.76 ms = 0.17 ms
// at 9600 baud = 960 cps it's 1/.96 = 1 ms
#define MODBUS_RECEIVE_TIMEOUT_ms 2

void MODBUS_Receive ( ) {

  static unsigned long lastCharReceivedAt_ms;
  
  // capture characters from stream
  lastCharReceivedAt_ms = millis();
  while ( ( millis() - lastCharReceivedAt_ms ) < MODBUS_RECEIVE_TIMEOUT_ms ) {
    
    if ( MAX485.available() ) {
    
      if ( bufPtr485 < bufLen485 ) {
        // grab the available character
        unsigned char c;
        c = MAX485.read();
        if ( c != 0xff ) {
          Serial.print ( c ); Serial.print ( "  0x" ); Serial.println ( c, HEX );
          strBuf485 [ bufPtr485++ ] = c;
          lastCharReceivedAt_ms = millis();
        }
      } else {
        // error - buffer overrun -- whine
        Serial.println (" -- buffer overrun -- ");
        for (;;) {
          digitalWrite (pdLED, 1);
          delay (100);
          digitalWrite (pdLED, 0);
          delay (100);
        }
      }
      // digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
    }  // character available
    
  }
  
  return;
}

#define SERIAL_RECEIVE_TIMEOUT_ms 1000

void Serial_Receive ( ) {

  static unsigned long lastCharReceivedAt_ms;
  
  digitalWrite (pdLED, HIGH);
  
  // capture characters from stream
  lastCharReceivedAt_ms = millis();
  while ( ( millis() - lastCharReceivedAt_ms ) < SERIAL_RECEIVE_TIMEOUT_ms ) {
    
    if ( Serial.available() ) {
    
      if ( bufPtr < bufLen ) {
        // grab the available character
        unsigned char c;
        c = Serial.read();
        strBuf [ bufPtr++ ] = c;
        lastCharReceivedAt_ms = millis();
        
      } else {
        // error - buffer overrun -- whine
        Serial.print ("Serial buffer overrun!");
        for (;;) {
          digitalWrite (pdLED, 1);
          delay (100);
          digitalWrite (pdLED, 0);
          delay (100);
        }
      }
      // digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
    }  // character available
    
  }
   
  digitalWrite (pdLED, LOW);

  
  return;
}
