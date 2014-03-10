#define VERSION "2.2.0"
#define VERDATE "2013-12-14"
#define PROGMONIKER "HTM"

/*

  NOTE v2.1.2, with correct firstloop temperature reading, not yet deployed!
  
  Functions:
    1) display time and hot-tub temperature
    2) serve as MODBUS/RTU slave to receive time from, provide the hot tub temperature to a master downstairs
    3) hot tub cover sensor? time-in-tub?
    4) receive XBee signals from R2D2
  
    combines systems: 
      Wire ( for I2C comm to thermometer )
      SoftwareSerial, MODBUS and MODBUS_SLAVE
      SPI ( for communications with numeric display )
 
    Arduino connections (digital pins):
      0 RX - reserved for serial comm - left unconnected
      1 TX - reserved for serial comm - left unconnected
      6 RX - RS485 connected to MAX485 pin 1  / POE card pin 3 (green)
      7 TX - RS485 connected to MAX485 pin 4 / POE card pin 5 (orange)
      8 MAX485 driver enable connected to POE card pin 4 (purple)
      9 status LED
     10 ~SS (slave select) - chip select for MAX7221 SPI LED display driver chip (orange, pin 3 of display)
     11 MOSI for SPI (green, pin 4 of display)
      NC MISO(brown, pin 5 of display)
     13 SCLK for SPI (brown, pin 6 of display)
 
    Arduino connections (analog pins):
      4 SDA - I2C thermometers (blue) DS75 pin 1
      5 SCL - I2C thermometers (brown) DS75 pin 2
      
   Digital display  -- note my display cards up to v1.1 are mislabeled, swapping MOSI and MISO labels!
    1 +5VDC (red)
    2 GND (black)
    3 SS (orange)
    4 MOSI(green)
    5 MISO(purple)
    6 SCK(brown)
    
   POE card
    1 GND (black)
    2 +5VDC (red)
    3 RX to MAX485 (green)
    4 EN to MAX485 (purple)
    5 TX to MAX485 (orange)
    
*/

// 0 serial port silent
// 1 some reportng
// 2 disables watchdog timer
// 5 Modbus becomes verbose
// 10 artificial time setting
#define VERBOSE 1

// regular serial port talks to motor controller
#define BAUDRATE 115200
// the SoftwareSerial port talks to the bus
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
#define BAUDRATE485 57600

// waterbed is slave 0x02
#define SLAVE_ADDRESS 0x01

#define TEMPERATURE_READING_INTERVAL_ms      10000L
#define TEMP_READING_TIMEOUT_ms               1000L
#define LIGHT_READING_INTERVAL_ms               20L
#define LIGHT_READING_SMOOTHING_HALF_LIFE_s    5.0

// alpha is the weight of the new reading;
// is then 1 - exp(log(0.5) / nPeriodsOfHalfLife)
// if we want half life of LIGHT_READING_SMOOTHING_HALF_LIFE_s sec, 
// then that's LIGHT_READING_SMOOTHING_HALF_LIFE_s * 1000.0 / LIGHT_READING_INTERVAL_ms periods
float light_EWMA_alpha = 1.0 - exp ( log ( 0.5 ) 
                         * float ( LIGHT_READING_INTERVAL_ms )
                         / ( LIGHT_READING_SMOOTHING_HALF_LIFE_s * 1000.0 ) );


#define HOT_TUB_THERMOMETER_ADDRESS        0x04
#define LOCAL_THERMOMETER_ADDRESS          0x05
  
#define DISPLAY_DURATION_ms 2000

// pin definitions

#define pdRS485RX         6
#define pdRS485TX         7
#define pdRS485_TX_ENABLE 8
#define pdSTATUSLED       9
#define pdSS             10
// MOSI. MISO (pin 12) not used for this display driver.
#define pdSDA            11
#define pdSCK            13

#define paLIGHT           0
// note that Wire uses analog 4 for SDA, analog 5 SCL

#include <avr/interrupt.h>
#include <avr/wdt.h>  
// see C:\Program Files\arduino-0022\hardware\tools\avr\avr\include\avr\wdt.h

#include <Wire.h>
#include <SPI.h>

#include <FormatFloat.h>

byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                      // { 0,  pdRS485RX        , -1 },    // handled by SoftwareSerial
                                      // { 0,  pdRS485TX        , -1 },    // handled by SoftwareSerial
                                      // { 0,  pdSS             , -1 },    // handled by SPI
                                      // { 0,  pdSDA            , -1 },    // handled by SPI
                                      // { 0,  pdSCK            , -1 },    // handled by SPI
                                      { 0,  pdRS485_TX_ENABLE, -1 },
                                      { 0,  pdSTATUSLED      , -1 }
                                     };


#define LINELEN 60
#define BUFLEN ( LINELEN + 3 )
char strBuf [ BUFLEN ];
#define FBUFLEN 8
char fbuf [ FBUFLEN ];


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

#include <MODBUS_var_access.h>

/*
 Coil assignment:
   0: tub occupied
   1: R2D2 Ozone
   2: R2D2 pump pre-run
   3: R2D2 pump full run
 Register assignment:
   0-1: hot tub temp (float)
   2-3: local temp (float)
   4-5: time - hhmm  - set by master
   6-7: ambient light measurement (float)
   8: LED brightness ( 0 - 15 )
*/

#define REGHOTTUBTEMP    0
#define REGAMBIENTTEMP   2
#define REGTIME          4
#define REGAMBIENTLIGHT  6
#define REGLEDBRIGHTNESS 8

// note that short is 16 bits
#define nCoils 4
unsigned short coils [ ( nCoils + 15 ) >> 4 ];
                              
#define nRegs 10
short regs[nRegs];

int device;
// Wire will shift L 1; last bit (R/~W) will be added 
#define deviceBaseAddress 0x09
#define deviceAddress(device) ((deviceBaseAddress << 3) + device)

// resolution setting for each digital thermometer
int CResolutions[] = {0, 0, 0, 0, 2, 2, 0, 0}; 
/*
     value   bits res    degC res    degF res    conversion time ms
      0          9          1/2        0.9             150
      1         10          1/4        0.45            300
      2         11          1/8        0.225           600
      3         12          1/16       0.1125         1200
*/

int conversionTimes_ms[8];

void displayNumber ( int num, int decimalOnDigit = 0 );


void setup() {

  Serial.begin ( BAUDRATE );
  MAX485.begin ( BAUDRATE485 );
  
  
  // initialize SPI and numeric display
  
  SPI.begin();
  
  SPI.setBitOrder ( MSBFIRST );
  SPI.setDataMode ( SPI_MODE0 );
  SPI.setClockDivider ( SPI_CLOCK_DIV4 );
  
  displayWrite ( 0x0c, 0x00 );  // normal operation -> shutdown mode
  delay ( 20 );
  displayWrite ( 0x0c, 0x01 );  // shutdown mode -> normal operation
  delay ( 20 );
  
  // displayWrite ( 0x0f, 0x01 );  // display test
  // delay ( 1000 );
  // displayWrite ( 0x0f, 0x00 );  // turn off display test
  
  displayWrite ( 0x09, 0x1f );  // decode(code B) first 4 characters
  displayWrite ( 0x0a, 0x08 );  // display brightness
  displayWrite ( 0x0b, 0x07 );  // display all characters (digit 7 will be colon etc)
  
  // initialize MODBUS

  mb.Init ( SLAVE_ADDRESS, nCoils, coils, nRegs, regs, MODBUS_port );
  if ( VERBOSE >= 5 ) {
    mb.Set_Verbose ( ( Stream * ) &Serial, 10 );
  }
  
  
  // initialize I2C and thermometer
    
  Wire.begin();        // join i2c bus (address optional for master)

  initDS75 ( HOT_TUB_THERMOMETER_ADDRESS );
  initDS75 ( LOCAL_THERMOMETER_ADDRESS );
  
  for ( int i = 0; i < 8; i++ ) {
    conversionTimes_ms [ i ] = 150 << CResolutions [ i ];
  }

  // initialize digital IO
  
  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( int i = 0; i < nPinDefs; i++ ) {
    if ( pinDefs [i] [0] == 1 ) {
      // input pin
      pinMode ( pinDefs [i] [1], INPUT );
      digitalWrite ( pinDefs [i] [1], 1 );  // enable internal 50K pullup
    } else {
      // output pin
      pinMode ( pinDefs [i] [1], OUTPUT );
      digitalWrite ( pinDefs [i] [1], 0 );
    }
  }
  
  
  // report

  #if VERBOSE > 0
    Serial.print ( F ( "\n\n\n" ) );
    Serial.print ( PROGMONIKER );
    Serial.print ( F ( ": Hot Tub Monitor v" ) );
    Serial.print ( VERSION );
    Serial.print ( F ( " (" ) );
    Serial.print ( VERDATE );
    Serial.print ( F ( ")\n  at address 0x" ) ); 
    Serial.print (SLAVE_ADDRESS, HEX); 
    Serial.print ( F (" ready with verbosity ") );
    Serial.println ( VERBOSE );
    
    // Serial.print ( "\n  Hot tub DS75." ); Serial.println ( HOT_TUB_THERMOMETER_ADDRESS );
    // Serial.print ( "  Ambient DS75." ); Serial.println ( LOCAL_THERMOMETER_ADDRESS );
    
    // Serial.print ( "\n  EWMA time " ); 
    // Serial.print ( LIGHT_READING_SMOOTHING_HALF_LIFE_s );
    // Serial.print ( " sec; at " );
    // Serial.print ( LIGHT_READING_INTERVAL_ms );
    // Serial.print ( " ms per loop, alpha is " );
    // Serial.println ( light_EWMA_alpha );
  #endif
  
  #if VERBOSE <= 1
    setAndStartWatchdogTimer ( 9 );
  #endif
  
  setFloatRegValue ( regs, nRegs, REGHOTTUBTEMP,  -273.15 );
  setFloatRegValue ( regs, nRegs, REGAMBIENTTEMP, -273.15 );
  
}
 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

ISR(WDT_vect) { 
  // heh, heh
  asm("jmp 0");
};

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
  
void loop () {

  static unsigned long lastDisplayChangedAt_ms = 0;
  static unsigned long lastBlinkToggleAt_ms = 0;
  static unsigned long lastTempRequestAt_ms = 0, 
                       temp_should_be_good_at_ms = 0;
  static unsigned long lastLightReadingAt_ms = 0;
  static int displayMode = -1;  // 0 displaying temperature; 1 displaying time
  int mb_status;
  static float tHotTub_degF, tAmbient_degF;
  static int temperatureMode = 0;  // changed from -1, which prevented firstloop reading
  
  static unsigned char firstLoopP = 1;
  
  #if VERBOSE <= 1
    wdt_reset();
  #endif
  
  // for testing
  if ( VERBOSE >= 10 ) {
    unsigned long mins = millis() / 60000;
    int hours = mins / 60;
    mins = mins - 60 * hours;
    hours %= 24;
    setIntRegValue ( regs, nRegs, REGTIME, hours * 100 + mins );
  }
  
  mb_status = mb.Execute ();
  
  if ( ( ( millis() - lastLightReadingAt_ms ) > LIGHT_READING_INTERVAL_ms )
      || firstLoopP ) {
    // lightReading values may range from 0 to 1023
    int lightReading = analogRead ( paLIGHT );
    static float light = -1;
    if ( light < 0.0 ) {
      light = lightReading;
    } else {
      light = light_EWMA_alpha * lightReading + ( 1 - light_EWMA_alpha ) * light;
    }
    setFloatRegValue ( regs, nRegs, REGAMBIENTLIGHT,  light );
    // Serial.print ( "Amb: " ); Serial.print ( lightReading );
    // Serial.print ( "  EWMA: " ); Serial.println ( light );

    // brightness values may range from 0 to 15
    byte display_brightness;
    // display_brightness = int ( ( light * light ) / 500.0 + 0.5 ) & 0x0f;  0 -> 0; 19 -> 0; 1024 -> 51 -> 15; 300 -> 15
    // display_brightness = int ( light / 20.0 + 0.5 );
    // if ( display_brightness > 15 ) display_brightness = 15;
    display_brightness = constrain ( map ( light, 20, 300, 0, 15 ), 0, 15 );
    setShortRegValue ( regs, nRegs, REGLEDBRIGHTNESS,  display_brightness );
    
    displayWrite ( 0x0a, display_brightness );  // display brightness can be 0x00 to 0x0f
    
    // Serial.print ( "Light reading: " ); Serial.print ( lightReading );
    // Serial.print ( "; EWMA: " ); Serial.print ( light );
    // Serial.print ( "; brightness: " ); Serial.println ( display_brightness );
    
    lastLightReadingAt_ms = millis();
  }

  // the temperature measurement process requires several times through the loop. To 
  // follow it to completion, we should always enter if temperatureMode is nonzero.
  if ( ( ( millis() - lastTempRequestAt_ms ) > TEMPERATURE_READING_INTERVAL_ms )
      || firstLoopP || temperatureMode != 0 ) {
    float t_degF;
    digitalWrite ( pdSTATUSLED, 1 );
  
    switch ( temperatureMode ) {
      case 0:
        // call for hot tub temperature
        Serial.print ( "Mem: " ); Serial.println ( availableMemory () );
        Serial.println ( "Call HT temp" );
        init_DS75_read ( HOT_TUB_THERMOMETER_ADDRESS );
        temp_should_be_good_at_ms = millis() + conversionTimes_ms [ HOT_TUB_THERMOMETER_ADDRESS ];
        temperatureMode++;
        break;
        
      case 1:
        // get hot tub temperature if it should be ready
        if ( millis() <= temp_should_be_good_at_ms ) {
          // don't expect it to be ready yet
          break;
        }
        if ( ( millis() - temp_should_be_good_at_ms ) < TEMP_READING_TIMEOUT_ms ) {
          // keep trying to read
          Serial.println ( "Read HT temp" );
          t_degF = c2f ( read_DS75_degC ( ) );
          if ( t_degF > -400.0 ) {
            // got an OK reading
            tHotTub_degF = t_degF;
            formatFloat ( fbuf, FBUFLEN, t_degF, 4 );
            snprintf (strBuf, BUFLEN, "DS75.%d %s degF", 
                     HOT_TUB_THERMOMETER_ADDRESS,
                     fbuf);
            sendMessage (strBuf);
            
            setFloatRegValue ( regs, nRegs, REGHOTTUBTEMP,  tHotTub_degF );
            
            temperatureMode++;
            break;
          } else {
            // keep waiting for a reading
          }
        } else {
          // done trying to read. Wah!
          Serial.print ( "Hot tub temp timeout!\n" );
          temperatureMode++;
        }
        break;
        
      case 2:
        // call for ambient temperature
        // Serial.println ( "Call ambient temp" );
        init_DS75_read ( LOCAL_THERMOMETER_ADDRESS );
        temp_should_be_good_at_ms = millis() + conversionTimes_ms [ LOCAL_THERMOMETER_ADDRESS ];
        temperatureMode++;
        break;
        
      case 3:
        // get ambient temperature if it should be ready
        if ( millis() <= temp_should_be_good_at_ms ) {
          // don't expect it to be ready yet
          break;
        }
        if ( ( millis() - temp_should_be_good_at_ms ) < TEMP_READING_TIMEOUT_ms ) {
          // keep trying to read
          // Serial.println ( "Read ambient temp" );
          t_degF = c2f ( read_DS75_degC ( ) );
          if ( t_degF > -400.0 ) {
            // got an OK reading
            tAmbient_degF = t_degF;
            formatFloat ( fbuf, FBUFLEN, t_degF, 4 );
            snprintf (strBuf, BUFLEN, "DS75.%d %s degF", 
                     LOCAL_THERMOMETER_ADDRESS,
                     fbuf);
            sendMessage (strBuf);
            
            setFloatRegValue ( regs, nRegs, REGAMBIENTTEMP,  tAmbient_degF );
            
            temperatureMode++;
            break;
          } else {
            // keep waiting for a reading
          }
        } else {
          // done trying to read. Wah!
          Serial.print ( "Ambient temp timeout!\n" );
          temperatureMode++;
        }
        break;
        
      default:
        // finished readings
        Serial.println ( "Temps done" );
        lastTempRequestAt_ms = millis();
        temperatureMode = 0;
        digitalWrite ( pdSTATUSLED, 0 );
        break;
  
    }
  }

  // display the time and the temperature
  
  #define NDISPLAYMODES 3
  
  if ( ( ( millis () - lastDisplayChangedAt_ms ) > DISPLAY_DURATION_ms  )
      || firstLoopP ) {
    displayMode++;
    if ( displayMode >= NDISPLAYMODES ) displayMode = 0;
    
    if ( displayMode == 0 ) {
      // currently displaying temperature - change to time
      displayTime ( intRegValue ( regs, nRegs, REGTIME ) );
    } else if ( displayMode == 1 ) {
      // currently displaying time - change to hot tub temperature
      
      displayTemperature ( floatRegValue ( regs, nRegs, REGHOTTUBTEMP ) );
      // displayTemperature ( -39.7 );
    } else {
      // currently displaying time - change to ambient temperature
      displayTemperature ( floatRegValue ( regs, nRegs, REGAMBIENTTEMP ) );
    }
    
    lastDisplayChangedAt_ms = millis ();
    
  }
  
  firstLoopP = 0;
  
  delay ( 10 );
  
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 


// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>             subroutines for MODBUS  comm          <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void reportCoils ( ) {
  reportCoils ( coils, nCoils );
}

void reportRegs ( ) {
  reportRegs ( regs, nRegs );
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>     subroutines for numeric display device       <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void displayTime ( int timehhmm ) {
  displayNumber ( timehhmm );
  displayWrite ( 7 + 1, 0x60 );  // display colon (digit 7, segs A and B)
}

void displayTemperature ( float temperature ) {
  // assume temperature < 1000 degF
  int t = int ( temperature * 10 + ( ( temperature > 0 ) ? 0.5 : -0.5 ));
  displayNumber ( t, 0x0001 << 1 );
}
 
void displayNumber ( int num, int decimalOnDigit ) {
  // decimalOnDigit is mask where a 1 bit puts a decimal on the corresponding digit
  // note that LSB (the 1 bit) corresponds to digit 0 (the rightmost)
  // Serial.println ( decimalOnDigit );
  
  #define NUM_DIGITS_IN_DISPLAY 4
  
  displayWrite ( 7 + 1, 0x00 );  // remove colon
  
  // oops - my built boards have digit 0 on the L, where my hand-built board has it on the R!!!
  // this sub is defined with digit 0 on the R - the digitPos function is to reverse the digit order
  #define displayBoardIsHandBuilt 0
  #if displayBoardIsHandBuilt
    #define digitPos( d ) ( d + 1 )
  #else
    #define digitPos( d ) ( 3 - d + 1 )
  #endif
    
  int absNum, d9 = NUM_DIGITS_IN_DISPLAY;
  absNum = abs ( num );
  
  
  char rightmostDecimalDigit = -1;  // there can be more than one in some anticipated cases
  byte isDecimalNumber = 0;
  byte negativeSignRequired = num < 0; 
  
  if ( decimalOnDigit ) {
    for ( int k = 0; ( k < NUM_DIGITS_IN_DISPLAY ) && ( rightmostDecimalDigit < 0 ); k++ ) {
      if ( decimalOnDigit & ( 0x0001 << k ) ) {
        // this one is the rightmost digit with a D.P.
        rightmostDecimalDigit = k;
        // if there are any other digits with a decimal point specified, it's not considered a decimal number
        isDecimalNumber = ! ( decimalOnDigit ^ ( 0x0001 << k ) );
      }
    }
  }
  
  
  for ( int d = 0; d < d9; d++ ) {
    // digit 0 is the rightmost
    int remaining_digits;
    byte rightmost_digit;
    remaining_digits = absNum / 10;
    rightmost_digit = absNum - ( remaining_digits * 10 );
    /*
      Serial.print ( d ); 
      if ( decimalOnDigit & ( 0x00000001 << d ) ) Serial.print ( "*" );
      Serial.print ( ": " );
      Serial.print ( absNum ); Serial.print ( "  " ); 
      Serial.print ( remaining_digits ); Serial.print ( " -> " ); 
      Serial.println ( rightmost_digit );
    */
    // add the decimal point if it belongs to this digit
    if ( decimalOnDigit & ( 0x00000001 << d ) ) rightmost_digit |= 0x80;
    
    // consider blanking leading zeros
    if (
               ! isDecimalNumber          // it's not a decimal number
            || remaining_digits != 0      // there's something left
            || rightmost_digit  != 0      // this is not a zero
            || d < rightmostDecimalDigit  // may not blank leading zeros to the right of the tens column
         ) {
      // normal digit display
      displayWrite ( digitPos ( d ), rightmost_digit );  // register for digit 0 is 1
      
      // byte w = 
                  // ( ( isDecimalNumber == 0 )      << 3 )
                // | ( ( remaining_digits != 0 )     << 2 )
                // | ( ( rightmost_digit  != 0 )     << 1 )
                // | ( ( d < rightmostDecimalDigit ) << 0 );
                
      // displayWrite ( digitPos ( d ), 0x80 + w );
      // displayWrite ( digitPos ( d ), isDecimalNumber );
      
    } else {
      // digit is a leading zero, in a number that has a decimal point
      if ( negativeSignRequired ) {
        displayWrite ( digitPos ( d ), 0x0a );  // 0x0a is a dash
        negativeSignRequired = false;
      } else {
        displayWrite ( digitPos ( d ), 0x0f );  // 0x0f is a blank
      }
    }
    absNum = remaining_digits;
  }

  if (    absNum                 // there's still something left!
       || negativeSignRequired   // we didn't have room for the negative sign!
     ) {
    // too big - couldn't display
    displayWrite ( digitPos ( 0 ), 0x0a );
    displayWrite ( digitPos ( 1 ), 0x0a );
    displayWrite ( digitPos ( 2 ), 0x0a );
    displayWrite ( digitPos ( 3 ), 0x0a );
  }
}

void displayWrite ( byte address, byte value) {
  /*
      16-bits sent with each sending
      D15 is MSB, D0 LSB
      D0-D7 data; D7 is the decimal point
      D8-D11 register address
      D12-D15 ignored
      
  */
  
  // take the SS pin low to select the chip:
  digitalWrite ( pdSS, LOW );
  //  send in the address and value via SPI:
  SPI.transfer ( address & 0x0f );
  SPI.transfer ( value & 0xff );
  // take the SS pin high to de-select the chip:
  digitalWrite ( pdSS, HIGH ); 
}



// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>        subroutines for temperature sensor          <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void initDS75 ( int device_pin_address ) {
  // set register 0 (temperature) for reading
  int TRegPtr = 0x00;
  int CRegPtr = 0x01;
  int CFaultTolerance = 0;
  int CPolarity = 0;
  int CThermostatMode = 0;
  int CShutdown = 0;
  int dA;
  int CResolution, ConfigValue;
  
  dA = deviceAddress ( device_pin_address );
  CResolution = CResolutions [ device_pin_address ];
  ConfigValue = (CResolution      & 0x03) << 5 
              | (CPolarity        & 0x01) << 2
              | (CThermostatMode  & 0x01) << 1
              | (CShutdown        & 0x01) << 0;
                  
  Wire.beginTransmission(dA); Wire.write(CRegPtr); Wire.write(ConfigValue); Wire.endTransmission();
  Wire.beginTransmission(dA); Wire.write(TRegPtr); Wire.endTransmission();
}
  
float init_DS75_read ( int device_pin_address ) {
  Wire.requestFrom ( deviceAddress ( device_pin_address ), 2, true);
}
  
float read_DS75_degC () {

  byte c1, c2;                    // the two words received from the DS75
  int c;                          // word comprising c1 and c2
  int	t1, t2; 										// int part and 1e3 times fract part degF
  float degC = -273.15;           // temp in degC
  
  if ( Wire.available() >= 2 ) {
    c1 = Wire.read();
    c2 = Wire.read();
    c = c1 << 8 | c2;
    if (c & 0x8000) {
      // negative number...
      degC = - ( (-c >> 8) + 0.0625 * ((-c >> 4) & 0x000f) );
    } else {
      degC = (c >> 8) + 0.0625 * ((c >> 4) & 0x000f);
    }
  }
  
  return ( degC );
}

float c2f ( float degC ) {
  return ( degC * 1.8 + 32.0 );
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>               general-purpose subroutines               <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void sendMessage (char msg[]) {
  char smBuf[42];
  snprintf (smBuf, 40, "[%s %7lu: %s]\n", PROGMONIKER, millis(), msg);
  Serial.print (smBuf);
  // Serial.print ( msg );
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void setAndStartWatchdogTimer ( byte scaler ) {
  /*
    set up the watchdog timer to automatically reboot 
    if a loop takes more than the set period
    
    Period is 2^(scaler-6) seconds, so 
      0 - 15.125 ms
      1 - 31.25 ms
      2 - 62.5 ms
      3 - 125 ms
      4 - 250 ms
      5 - 500 ms
      6 - 1 second
      7 - 2 seconds
      8 - 4 seconds
      9 - 8 seconds
      
      Valid values from 0 to 9
      
      also use 
      
          ISR(WDT_vect) { 
            // heh, heh - restarts program, but doesn't hard-reset processor
            asm("jmp 0");
          };

      and
          wdt_reset();  // pet the nice doggy

  */
  
  // Serial.print ( "Scaler: 0x" ); Serial.println ( scaler, HEX );
  
  byte WDTCSR_nextValue = ( 1 << WDIE )                             // bit 6
                        | ( 0 << WDE  )                             // bit 3
                        | ( ( ( scaler >> 3 ) & 0x01 ) << WDP3 )    // bit 5
                        | ( ( ( scaler >> 2 ) & 0x01 ) << WDP2 )    // bit 2
                        | ( ( ( scaler >> 1 ) & 0x01 ) << WDP1 )    // bit 1
                        | ( ( ( scaler >> 0 ) & 0x01 ) << WDP0 );   // bit 0
                                
  // Serial.print ( "Next value for WDTCSR: 0x" ); Serial.println ( WDTCSR_nextValue, HEX );
  
  cli();  // disable interrupts by clearing the global interrupt mask
  
  wdt_reset();
  // indicate to the MCU that no watchdog system reset has been requested
  MCUSR &= ~ ( 1 << WDRF );
  
  // WDIE / WDE
  //   1     0 -- interrupt mode
  //   1     1 -- interrupt and system reset
  //   0     1 -- system reset mode
  
  // use interrupt mode with a software reset,
  //   since system reset mode seems to just hang
  
  // the prescaler (time-out) value's bits are WDP3, WDP2, WDP1, and WDP0, 
  // with WDP3 being the high-order bit
  
  // set WDCE (watchdog change enable) and WDE (watchdog system reset enable)
  //   to enable writing of bits within 4 cycles  
  // Keep old prescaler setting to prevent unintentional time-out
  WDTCSR |= ( 1 << WDCE ) | ( 1 << WDE );
  
  // Set new prescaler(time-out) value

  WDTCSR = WDTCSR_nextValue;
   
  sei();  // enable interrupts by setting the global interrupt mask
  
}

int availableMemory () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


// float getAndRecordTemperature ( int device_pin_address, int modbus_register ) {
  // float t_degF;
  
  // t_degF = c2f ( read_DS75_degC ( device_pin_address ) );
  // formatFloat ( fbuf, FBUFLEN, t_degF, 4 );
  // snprintf (strBuf, BUFLEN, "DS75.%d %s degF", 
           // device_pin_address,
           // fbuf);
  // sendMessage (strBuf);
  
  // setFloatRegValue ( regs, nRegs, modbus_register,  t_degF );
  
  // return ( t_degF );
  
// }
  
/*

2013-07-28 2.001.003 cbm moved ambient light EWMA measurement outside metered loop
                                                for quicker reponse
2013-07-28 2.001.004 cbm fixed firstLoopP problem
                                                floated light_EWMA_alpha and corrected time units (0.5s = 500.0 ms)
 2013-07-29 2.001.005 cbm researching hang
                                               
*/