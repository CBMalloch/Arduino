#define VERSION "1.0.0"
#define VERDATE "2013-12-29"
#define PROGMONIKER "TRTC"

/*

  Working. Works with TinyRTC modules. Note that these modules combine a DS1307 real-time clock,
  AT24C32 4K serial EEPROM, and a backup battery that preserves both. A cool thing.
  The backup battery is a CR2032, which is a RECHARGEABLE coin cell battery.
  
  All such boards I've seen have a 3-pin spot for a (TO-92?) DS18S20 one-wire temperature
  sensor, but fail to include this component
  
  Connect up the board thus:
    BAT - N/C - can read out the battery voltage from here
    GND
    Vcc - 5V will do
    SDA (to pin A4)
    SCL (to pin A5)
    DS - N/C would be for an installed DS18S20 temperature sensor
    SQ - N/C square wave output to drive interrupts if desired

*/

#include <Wire.h>                       // For some strange reasons, Wire.h must be included here
#include <DS1307new.h>
#include <OneWire.h>

#define VERBOSE 2

#define pdTEMP 2
#define pdSQ   3

#define BAUDRATE 115200

#define INTERCHARTIMEOUT_ms 5000LU

#define bufLen 80
static char strBuf [ bufLen ];
static short bufPtr = 0;

// DS18S20 Temperature chip i/o
// Note that the DS18S20 chip is a 3-pin TO-92-like unit that is NOT PRESENT on the cheap boards I bought!
OneWire ds (pdTEMP);

void setup()
{
  Serial.begin(BAUDRATE);
  
  pinMode (pdSQ, INPUT);
  digitalWrite (pdSQ, HIGH);                // Pullup on
  
  if ( ! RTC.isPresent () ) {
    Serial.println ( "DS1307 clock board is not visible!" );
    for ( ;; ) { } 
  }


  // given the original code, it looks like the clock registers are stored in the RAM at 0x00-0x07
  // RTC.setRAM ( 0, (uint8_t *) &startAddr, sizeof(uint16_t) );  // Store startAddr in NV-RAM address 0x08 

  RTC.getTime();
  // Control Register for SQW pin which can be used as an interrupt.
  // 0x00=disable SQW pin, 0x10=1Hz,
  // 0x11=4096Hz, 0x12=8192Hz, 0x13=32768Hz
  RTC.ctrl = 0x00;
  RTC.setCTRL();

  bufPtr = 0;  
  snprintf ( strBuf, bufLen, "%s: TinyRTC v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
  Serial.print ( strBuf );

}

void loop() {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  
  #define inBufLen 16
  static char inBuf [ inBufLen ];
  static short inBufPtr = 0;

  static unsigned long  // lastLoopAt_us, 
                        // lastPrintAt_ms = 0LU, 
                        // lastSendAt_ms = 0LU,
                        // lastMsgReceivedAt_ms = 0LU,
                        lastCharReceivedAt_ms = 0LU
                       ;

  if ( ( inBufPtr != 0 ) && ( ( millis () - lastCharReceivedAt_ms ) > INTERCHARTIMEOUT_ms ) ) {
    // dump current buffer, which is stale
    Serial.println ( "___flush___" );
    inBufPtr = 0;
  }
 
  // message formats:  T hhmmss\n (0x0a)  and D yymmdd\n (0x0a)

   while ( Serial.available() ) {
    static short dumping = 0;   // to resynchronize, we dump characters until we see a cr 
    
    char theChar = Serial.read();
    
    if ( theChar == 0x0a ) {
      // newline; handle the buffer and clear it
      unsigned long longval;
      if ( inBufPtr == 8) {
        inBuf [ 8 ] = '\0';
        longval = atol ( & inBuf [2] );
      } else {
        Serial.print ( "inBufPtr: " ); Serial.println ( inBufPtr );
      }
      
      #if VERBOSE > 4
      for ( int i = 0; i < inBufPtr; i++ ) {
        Serial.print ( " 0x" ); Serial.print (inBuf [ i ], HEX );
      }
      Serial.println ( "  v" );
      #endif

      dumping = 0;
      inBufPtr = 0;
      
      // set the clock
      
      if ( inBuf [ 0 ] == 'D' ) {
        int y, m, d;
        d = longval % 100;
        longval /= 100;
        m = longval % 100;
        longval /= 100;
        y = longval + 2000;

        Serial.print ( "Set date: " ); 
        Serial.print ( y ); Serial.print ( "-" );
        Serial.print ( m ); Serial.print ( "-" );
        Serial.print ( d ); Serial.print ( "\n" );

        RTC.stopClock ();
        RTC.fillByYMD ( y, m, d );
        RTC.setTime();
        RTC.startClock();
      } else if ( inBuf [ 0 ] == 'T' ) {
        int h, m, s;
        s = longval % 100;
        longval /= 100;
        m = longval % 100;
        longval /= 100;
        h = longval;
        
        Serial.print ( "Set time: " ); 
        Serial.print ( h ); Serial.print ( ":" );
        Serial.print ( m ); Serial.print ( ":" );
        Serial.print ( s ); Serial.print ( "\n" );

        RTC.stopClock ();
        RTC.fillByHMS ( h, m, s );
        RTC.setTime();
        RTC.startClock();
      } else {
        Serial.print ( "Bad input: " ); Serial.println ( inBuf );
      }
      
    } else if (    ( ! (   ( theChar == ' ' )
                        || ( theChar == 'T' ) 
                        || ( theChar == 'D' )
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
 
  RTC.getTime();
  
  // one-wire to talk to DS18B20 temperature chip
  
  if ( ds.reset () ) {
    ds.reset_search();

    while ( ds.search(addr) ) {
      
      Serial.print("R=");
      for( i = 0; i < 8; i++) {
        Serial.print(addr[i], HEX);
        Serial.print(" ");
      }

      if ( OneWire::crc8( addr, 7) != addr[7]) {
        Serial.print("CRC is not valid!\n");
        continue;
      }

      if ( addr[0] == 0x10) {
        Serial.print("Device is a DS18S20 family device.\n");
      }
      else if ( addr[0] == 0x28) {
        Serial.print("Device is a DS18B20 family device.\n");
      }
      else {
        Serial.print("Device family is not recognized: 0x");
        Serial.println(addr[0],HEX);
        continue;
      }

      ds.reset();
      ds.select(addr);
      ds.write(0x44,1);         // start conversion, with parasite power on at the end

      delay(1000);     // maybe 750ms is enough, maybe not
      // we might do a ds.depower() here, but the reset will take care of it.

      present = ds.reset();
      ds.select(addr);    
      ds.write(0xBE);         // Read Scratchpad

      Serial.print("P=");
      Serial.print(present,HEX);
      Serial.print(" ");
      for ( i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ds.read();
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
      Serial.print(" CRC=");
      Serial.print( OneWire::crc8( data, 8), HEX);
      Serial.println();
    
    }

    Serial.print("No more addresses.\n");  

  } else {
    // Serial.println ( "No one-wire device detected!" );
  }
  
  
  switch (RTC.dow)                      // Friendly printout the weekday
  {
    case 1:
      Serial.print("MON");
      break;
    case 2:
      Serial.print("TUE");
      break;
    case 3:
      Serial.print("WED");
      break;
    case 4:
      Serial.print("THU");
      break;
    case 5:
      Serial.print("FRI");
      break;
    case 6:
      Serial.print("SAT");
      break;
    case 7:
      Serial.print("SUN");
      break;
  }
  Serial.print ( " " );
  
  Serial.print(RTC.year, DEC);          // Year need not to be changed
  
  Serial.print("-");
  
  if (RTC.month < 10)                   // correct month if necessary
  {
    Serial.print("0");
    Serial.print(RTC.month, DEC);
  }
  else
  {
    Serial.print(RTC.month, DEC);
  }
  
  Serial.print("-");
  
  if (RTC.day < 10)                    // correct date if necessary
  {
    Serial.print("0");
    Serial.print(RTC.day, DEC);
  }
  else
  {
    Serial.print(RTC.day, DEC);
  }
  Serial.print(" ");

  Serial.print(" ");
  
  if (RTC.hour < 10)                    // correct hour if necessary
  {
    Serial.print("0");
    Serial.print(RTC.hour, DEC);
  } 
  else
  {
    Serial.print(RTC.hour, DEC);
  }
  Serial.print(":");
  if (RTC.minute < 10)                  // correct minute if necessary
  {
    Serial.print("0");
    Serial.print(RTC.minute, DEC);
  }
  else
  {
    Serial.print(RTC.minute, DEC);
  }
  Serial.print(":");
  if (RTC.second < 10)                  // correct second if necessary
  {
    Serial.print("0");
    Serial.print(RTC.second, DEC);
  }
  else
  {
    Serial.print(RTC.second, DEC);
  }
  
  uint8_t MESZ = RTC.isMEZSummerTime();
  if ( MESZ ) {
    Serial.print ( " (DST) " );
  } else {
    Serial.print ( " " );
  }
  
  Serial.print ( "\n" );
  
  /*
  RTC.getRAM (  0, (uint8_t *) &lastAddr,  sizeof(uint16_t) );
  lastAddr = lastAddr + 1;              // we want to use it as address counter for example
  RTC.setRAM (  0, (uint8_t *) &lastAddr,  sizeof(uint16_t) );
  Serial.print ( "Address counter ( at 0 ) : 0x" ); Serial.println ( lastAddr, HEX );
  RTC.getRAM ( 54, (uint8_t *) &TimeIsSet, sizeof(uint16_t) );
  */
   
  delay(1000);                          // wait a second
}
