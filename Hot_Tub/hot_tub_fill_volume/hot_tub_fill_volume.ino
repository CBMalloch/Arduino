#define VERSION "2.4.0"
#define VERDATE "2018-03-18"
#define PROGMONIKER "HTFV"

/*

  Measure hot tub volume and temperature and control shutoff
  
  2014-02-17 2.0.0 cbm adding in MODBUS-RTU capability
      want LED to indicate valve status, one to indicate flowmeter rotation - neither required
  2014-02-21 2.1.0 cbm fixed pin assignment for RS-485
    -- current problems - screen doesn't initialize properly; blinky light stutters
                                        ONLY WHEN RS485 IS PLUGGED IN!
  2014-02-22 2.1.1 cbm added timing for RS485 (mb.Execute)
  2014-05-10 2.2.0 cbm added some diagnostics to see if counts is hammering gpm in the registers,
                       since at some point the gpm goes to zero...
  2014-05-11 2.2.1 cbm still working on it...
  2014-05-11 2.2.2 cbm replaced lastPulseAt_us with lastPulseAt_ms, preventing rollover problem
                       at 4.2e6 ms ( max unsigned long is 4294967296-1 )
  2016-11-20 2.3.0 cbm changed fill volume for 220-gallon new tub
  2016-11-23 2.3.1 cbm tuning for complete fill
  2017-04-07 2.3.2 cbm more of the same - set for 250 gals ( 175+50 too little; 175+90 too much )
  2018-03-18 2.4.0 cbm adding trimpot on A1 to set gallons requested
      
    box size 4x4x2"
    green LED lit when solenoid is open
    yellow LED indicates flow meter activity
  
    Arduino connections (digital pins):
      0 RX - reserved for serial comm - left unconnected
      1 TX - reserved for serial comm - left unconnected
      2 pulse input from flow meter
      4 control output for solenoid valve - 1 opens valve, 0 closes
      6 RX - RS485 connected to MAX485 pin 1
      7 TX - RS485 connected to MAX485 pin 4
      8 MAX485 driver enable connected to POE card pin 4 (purple)
     11 flowmeter pulse indicator LED
     12 solenoid state indicator LED
     13 blinky LED
 
    Arduino connections (analog pins):
      0 BC2301 NTC thermistor bridge
      4 SDA - I2C LCD216s display
      5 SCL - I2C LCD216s display

*/

#include <Wire.h>
#include <LCD216s.h>
#include <FormatFloat.h>

#define LCD_I2C_address 0

// const long BAUDRATE = 115200;
//  using #define makes BAUDRATE available to other compile units
#define BAUDRATE 115200
// the SoftwareSerial port talks to the bus
// it appears strongly that SoftwareSerial can't go 115200, but maybe will do 57600.
const long BAUDRATE485 = 57600;

// 0 serial port silent
// 1 some reporting
// 2 disables watchdog timer
// 4 timing for MODBUS
// 5 MODBUS becomes verbose
// 10 artificial time setting
#define HTFV_VERBOSE 1

// waterbed is slave 0x02; hot tub monitor is slave 0x01
const int SLAVE_ADDRESS = 0x03;

// alarm "Hot tub is full" is in Python code!

const int blinkPeriod_ms    = 1000;
const int tempPeriod_ms     = 2000;
const int flowRatePeriod_ms =  250;


const int pdPulse           =  2;
const int pdSolenoid        =  4;
const int pdRS485RX         =  6;
const int pdRS485_TX_ENABLE =  7;
const int pdRS485TX         =  8;
const int pdFlowmeterLED    = 11;
const int pdSolenoidLED     = 12;
const int pdBlinkyLED       = 13;

const int paBC2301          = A0;  // PTC thermistor
const int paPot             = A1;  // volume-setting pot

byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                      { 1,  pdPulse,           -1 },
                                      // { 0,  pdRS485RX        , -1 },    // handled by SoftwareSerial
                                      // { 0,  pdRS485TX        , -1 },    // handled by SoftwareSerial
                                      { 0,  pdRS485_TX_ENABLE, -1 },
                                      { 0,  pdSolenoid,        -1 },
                                      { 0,  pdFlowmeterLED,    -1 },
                                      { 0,  pdSolenoidLED,     -1 },
                                      { 0,  pdBlinkyLED,       -1 }
                                     };


LCD216s LCD ( LCD_I2C_address );

const float Rfixed = 9999.5;  // ohms, fixed resistor (lower half of voltage divider)
const float Rref = 10000.0;  // ohms
const float zeroC = 273.15;  // degR
const float Tref = 25.0 + zeroC;  // degR

// a1, b1, c1, d1 consts for NTC thermistor
const float abcd[] = { 3.354016E-03, 2.569850E-04, 2.620131E-06, 6.383091E-08 };  

const int lenPulsesList = 5;

volatile unsigned long pulseCount;
volatile unsigned long lastPulseAt_ms;

// " Frequency (Hz) = 4.5 * Q +/- 2%  where Q = flow rate (L/min) " from documentation
// so p/s = 4.5 * L/min
//            = 4.5 * L/m * 1m/60s * 1Gal/3.785L
//            = 4.5 Gal / s / 60 / 3.785
//    then Gal = 60 * 3.875 / 4.5 * p
// so Gal = 60 * 3.785 / 4.5 p = 50.46666667 p
// const float galsPerPulse = 4.5 / 60.0 / 3.875;

// " L/h = p/s * 60 / 5.5 " from Seeed
// so 1 pulse = s / 60 * 5.5 L / h * 1h/3600s * 1Gal/3.785L
//                  = 1 Gal
//    then Gal = 60 * 3.875 / 4.5 * p
// so Gal = 60 * 3.785 / 4.5 p = 50.46666667 p
// const float galsPerPulse = 4.5 / 60.0 / 3.875;

// " L/h = p/s * 60 / 7.5 " from Seeed #2
// THIS APPEARS TO BE THE CORRECT ONE
// so v L/h = p/s * 60s/m * 60m/h / 7.5,
//      v L * G/3.785L = p * 3600 / 7.5,
//      v G = p * 3600 * 3.785 / 7.5
const float galsPerPulse = 7.5 / 3600.0 / 3.875;

// if max flow rate is 1 Gal/s (unlikely), then fastest we should expect interrupts is 
//   1 / galsPerPulse / gal * pulse * 1 gal / s = 1 / galsPerPulse pulse / s
// so max simulation is 60 Gal/min
const unsigned long minMicrosPerPulse = 1e6 * galsPerPulse;
volatile int roguePulses;

const int bufLen = 60;
static char strBuf [ bufLen ];
static short bufPtr = 0;

const int fbufLen = 12;
char strFBuf[fbufLen];

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
   0: solenoid open
 Register assignment:
   0-1: flow temp (float)
   2-3: pulse count (unsigned long)
   4-5: gallons (float)
   6-7: flow rate gpm (float)
*/

#define COILSOLENOID     0
#define REGFLOWTEMP      0
#define REGPULSECOUNT    2
#define REGGALLONS       4
#define REGFLOWRATEGPM   6

// note that short is 16 bits

const short nCoils = 1;
unsigned short coils [ ( nCoils + 15 ) >> 4 ];
                            
const short nRegs = 8;
short regs[nRegs];

float stopAt_gals = 250.0;

// magic juju to return array size
// see http://forum.arduino.cc/index.php?topic=157398.0
template< typename T, size_t N > size_t ArraySize (T (&) [N]){ return N; }


// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void setup () {

  Serial.begin ( BAUDRATE );
  MAX485.begin ( BAUDRATE485 );
  
  Wire.begin ();
  delay ( 2000 );
  LCD.init ();
  delay ( 100 );
  
  // determine gallons requested by reading pot
  
  int potCounts;
  {
    int zone;
    int previousZone = -100;
    int inchesMenu[] = { 1, 2, 4, 6, 9, 12, 18, 20, 22 };
    unsigned int lastChangeAt_ms = 0UL;
    while ( ( lastChangeAt_ms == 0UL ) || ( ( millis() - lastChangeAt_ms ) < 10000UL ) ) {
      // divide pot into zones
      potCounts = analogRead ( paPot );
      zone = map ( potCounts, 0, 1023, 0, ArraySize ( inchesMenu ) - 1 );
      Serial.print ( "zone: " ); Serial.print ( zone ); Serial.print ( "  " );
      
      stopAt_gals = inchesMenu [ zone ] * 12.65;
      
      snprintf ( strBuf, bufLen, "%d inches    ", inchesMenu [ zone ] );
      LCD.setLoc ( 0, 0 );
      LCD.send ( (unsigned char *) &strBuf[0], strlen ( strBuf ) );
      Serial.print ( strBuf );
      
      formatFloat ( strFBuf, fbufLen, stopAt_gals, 1 );
      snprintf ( strBuf, bufLen, "%s gallons     ", strFBuf );
      LCD.setLoc ( 1,0 );
      LCD.send ( (unsigned char *) &strBuf[0], strlen ( strBuf ) );
      Serial.println ( strBuf );

      // Serial.print ( zone ); Serial.print ( " <?> " ); 
      // Serial.print ( previousZone ); Serial.print ( "      " );
      // Serial.print ( millis() ); Serial.print ( " < 10K + " );
      // Serial.println ( lastChangeAt_ms );
        
      if ( zone != previousZone ) {
        previousZone = zone;
        lastChangeAt_ms = millis();
      }
      delay ( 100 );
    }
    // zone is now good
  
    // 1 150 lb person is equivalent to 18.75 gal
    // tub diameter is 61"; call it 60"
    // tub surface area is 2827 in2
    // 12.65 gal / inch of depth => 13.09
    // 220 gal capacity - 2 people-volumes is about 175 gal
    // 175 gal doesn't reach top jets... trying for 200
    // first fill took 175 - 5 + 50 = 220 gallons to cover top nozzle + 2 in.
    
    // was 8" below top; chose 4"; it's now 3" below top
    // 4" => 50 gals; I suspect the flow meter is a little slow - it's somewhat positional
    
  
  }
  #if HTFV_VERBOSE > 0
  
    while ( !Serial && millis() < 10 );  // wait for serial port to be connected
    Serial.print ( F ( "\n\n\n\r" ) );
    Serial.print ( PROGMONIKER );
    Serial.print ( F ( ": Hot Tub Fill Volume and Temperature Monitor v" ) );
    Serial.print ( VERSION );
    Serial.print ( F ( " (" ) );
    Serial.print ( VERDATE );
    Serial.print ( F ( ")\r\n  at address 0x" ) ); 
    Serial.print (SLAVE_ADDRESS, HEX); 
    Serial.print ( F (" ready with verbosity ") );
    Serial.println ( HTFV_VERBOSE );
    
    formatFloat ( strFBuf, fbufLen, stopAt_gals, 1 );
    Serial.print ( F ("Scheduled stop at " ) ); 
    Serial.print ( strFBuf );
    Serial.println ( F (" gallons." ) ); 
    
    Serial.print ( F ( "Gallons per pulse " ) ); Serial.print ( galsPerPulse, 6 ); Serial.println ();
    Serial.print ( F ( "Min usec per pulse " ) ); Serial.print ( minMicrosPerPulse ); Serial.println ();
    
  #endif
  
 // initialize digital IO
  
  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( int i = 0; i < nPinDefs; i++ ) {
    if ( pinDefs [i] [0] == 1 ) {
      // input pin
      pinMode ( pinDefs [i] [1], INPUT );
      digitalWrite ( pinDefs [i] [1], 1 );  // enable internal 50K pull-up
    } else {
      // output pin
      pinMode ( pinDefs [i] [1], OUTPUT );
      digitalWrite ( pinDefs [i] [1], 0 );
    }
  }
      
  // initialize MODBUS

  mb.Init ( SLAVE_ADDRESS, nCoils, coils, nRegs, regs, MODBUS_port );
  if ( HTFV_VERBOSE >= 5 ) {
    mb.Set_Verbose ( ( Stream * ) &Serial, 10 );
  }
  
  snprintf ( strBuf, bufLen, "0.0gpm   " );
  LCD.setLoc ( 0, 8 );
  LCD.send ( (unsigned char *) &strBuf[0], strlen ( strBuf ) );
  snprintf ( strBuf, bufLen, " 0.0gal" );
  LCD.setLoc ( 1, 0 );
  LCD.send ( (unsigned char *) &strBuf[0], strlen ( strBuf ) );
  
  attachInterrupt ( 0, isr_pulse, CHANGE );
  pulseCount = 0UL;
  
  
  #if HTFV_VERBOSE > 0
  
    Serial.println ( "LCD and RS485 connections seem OK" );
    Serial.print ( F ("Mem: ") ); Serial.println ( availableMemory() );
    delay ( 500 );

  #endif
  
  // initialize program-specific stuff
  
  digitalWrite ( pdSolenoid, 1 );  // open the valve to allow filling!
  
  setRegValue ( regs, nRegs, REGFLOWTEMP,    (float) -273.15 );
  setRegValue ( regs, nRegs, REGPULSECOUNT,              0UL );
  setRegValue ( regs, nRegs, REGGALLONS,     (float)    0.0  );
  setRegValue ( regs, nRegs, REGFLOWRATEGPM, (float) -100.0  );
  
  reportCoils();

}

void loop () {

  static int done = 0;

  static unsigned long lastBlinkAt_ms = 0;
  static int blinkOnDuration_ms = 500;
  
  static unsigned long lastTempAt_ms = 0;
  static float T_K, T_F;
  
  static unsigned long pulsesAt_ms [lenPulsesList];
  static unsigned long lastPulseCounts [lenPulsesList];
  static int maxPulseNo = lenPulsesList - 1;
  static unsigned long lastPulseCount = 0;
  static int nPulsesInList = 0;
  static int solenoidState = 2;

  #if 1 && ( HTFV_VERBOSE >= 2 )
    static unsigned long lastDiagnosticReportAt_ms = 0;
    const unsigned long diagnosticReportInterval_ms = 1000;
    int diff;
  #endif
  
  static unsigned long lastFlowRateAt_ms = 0;
  static float gpm;
  
  int mb_status;
  
  // * * * * * * * * * * * * * *
  // * * * * * * * * * * * * * *

  #if 1 && ( HTFV_VERBOSE >= 4 )
    unsigned long mbExecuteBegan_us = micros();
  #endif
  mb_status = mb.Execute ();
  #if 1 && ( HTFV_VERBOSE >= 4 )
  unsigned long mbExecuteTook_us = micros() - mbExecuteBegan_us;
  // Serial.print ( F ( "mb.Execute took " ) ); Serial.print ( mbExecuteTook_us ); Serial.println ( F ( "us" ) );
  static int nOK = 0;
  if ( mbExecuteTook_us > 2500 ) {
    // took long
    Serial.print ( nOK );
    for ( int i = 0; i < ( mbExecuteTook_us / 100 ); i++ ) {
      Serial.print ( F ( " " ) );
    }
    Serial.print ( F ( "M = " ) ); Serial.println ( mbExecuteTook_us );
    nOK = 0;
  } else {
    nOK++;
  }
  #endif

  // * * * * * * * * * * * * * *
  // * * * * * * * * * * * * * *
 
  // Serial.print ( "a ct: " ); Serial.println ( pulseCount );
  // Serial.print ( "minMicrosPerPulse: " ); Serial.println ( minMicrosPerPulse );
  
  if ( !done ) {



    
    /* *********************************
       calculate and display temperature
       ********************************* */
    
    if ( ( millis() - lastTempAt_ms ) > tempPeriod_ms ) {
    /*

    The formula for the temperature measured by the BC2301 10K at 25degC NTC thermistor is:
      lnR = ln ( R / Rref ), where Rref is the reference temp of 25degC
      1 / T = a1 + b1 * lnR + c1 * lnR ^ 2 + d1 * lnR ^ 3
        
    */
      int counts = analogRead ( paBC2301 );
      float v = counts * 5.0 / 1023.0;
      float Rmeas = ( Rfixed / v ) * 5.0 - Rfixed;  // ( Rmeas + Rfixed ) / 5.0 = Rfixed / v
      float lnR = log ( Rmeas / Rref );  // where Rref is the reference temp of 25degC; in c, log is natural log
      float invT = abcd[0] + abcd[1] * lnR + abcd[2] * lnR * lnR + abcd[3] * lnR * lnR * lnR;
      T_K = 1 / invT;
      T_F = ( T_K - zeroC ) * 1.8 + 32.0;
      
      /*
      Serial.print ( "counts: " ); Serial.println ( counts );
      Serial.print ( "Rmeas: " ); Serial.println ( Rmeas );
      Serial.print ( "lnR: " ); Serial.println ( lnR );
      Serial.print ( "invT: " ); Serial.println ( invT );
      Serial.print ( "T_K: " ); Serial.println ( T_K );
      Serial.print ( "T_F: " ); Serial.println ( T_F );
      */
      formatFloat(strFBuf, fbufLen, T_F, 1);
      // \xdf is a degrees mark
      // the double double quote is needed to terminate the hex literal
      snprintf ( strBuf, bufLen, "%5s\xdf""F ", strFBuf );
      // Serial.print ( T_F, 1 ); Serial.print ( "degF   " );
      
      LCD.setLoc ( 0, 0 );
      LCD.send ( (unsigned char *) &strBuf[0], strlen ( strBuf ) );  // why not just strBuf (which doesn't work!)?
      
      setRegValue ( regs, nRegs, REGFLOWTEMP, T_F );

      lastTempAt_ms = millis ();
    }



    
    /* *******************************
       calculate and display volume
       ******************************* */
    
    if ( pulseCount > lastPulseCount ) {
      if ( nPulsesInList >= lenPulsesList ) { 
        // pulses list is full
        // nPulsesInList is equal to lenPulsesList
        // the last one is at index maxPulseNo, which is lenPulsesList - 1
        // shift left
        // Serial.print ( "Shifting L: " );
        for ( int i = 0; i < maxPulseNo; i++ ) {                // changed from i <= maxPulseNo  v2.3.0
          pulsesAt_ms [ i ] = pulsesAt_ms [ i + 1 ];
          lastPulseCounts [ i ] = lastPulseCounts [ i + 1 ];
          // Serial.print ( i ); Serial.print ( " " );
        }
        nPulsesInList = maxPulseNo;
        // Serial.println ();
        // Serial.print ( "1ct: " ); Serial.println ( pulseCount );
      }
      // there is room in pulses list
      // Serial.print ( "2ct: " ); Serial.println ( pulseCount );
      // next pulse will go at index nPulsesInList, which will never exceed maxPulseNo here
      pulsesAt_ms [ nPulsesInList ] = lastPulseAt_ms;
      lastPulseCounts [ nPulsesInList ] = pulseCount;
      nPulsesInList++;
      
      #if 1 && ( HTFV_VERBOSE >= 2 )
        // seemed to stop last time at about 4295221 ms
        Serial.println ( F ( "pulses list: ") );
        for ( int i = 0; i < nPulsesInList; i++ ) {
          Serial.print ( F ( "  " ) ); Serial.print ( i );
          Serial.print ( F ( ": " ) ); Serial.print ( lastPulseCounts [i] );
          Serial.print ( F ( "; at: " ) ); Serial.println ( pulsesAt_ms [i] );
        } 
      #endif

      setRegValue   ( regs, nRegs, REGPULSECOUNT, pulseCount );

      lastPulseCount = pulseCount;
      
      // Serial.print ( "ct: " ); Serial.print ( pulseCount );
      // Serial.print ( "; ms: " ); Serial.print ( lastPulseAt_ms ); Serial.print ( "; " ); 
      float gals = float ( pulseCount ) * galsPerPulse;
      // Serial.print ( gals ); 
      // Serial.print ( " gallons");
      // Serial.println ();
      
      setRegValue ( regs, nRegs, REGGALLONS, gals );

      if ( gals < 99.95 ) {
        // if it rounds to 100.0, "gal" gets pushed over
        formatFloat ( strFBuf, fbufLen, gals, 1 );
        snprintf ( strBuf, bufLen, "%4sgal ", strFBuf );
      } else {
        snprintf ( strBuf, bufLen, "%4dgal ", int ( gals ) );
      }
      
      LCD.setLoc ( 1, 0 );
      LCD.send ( (unsigned char *) &strBuf[0], strlen ( strBuf ) );
     
      // if ( int ( gals ) % 2 == 0 ) {
      if ( gals < stopAt_gals ) {
        // turn solenoid on
        digitalWrite ( pdSolenoid, 1 );
        blinkOnDuration_ms = 750;
      } else {
        // turn solenoid off
        digitalWrite ( pdSolenoid, 0 );
        blinkOnDuration_ms = 100;
        done = 1;
      }
      
    }  // pulseCount increased
    

      // Serial.print ( F ( "gpm 1 " ) ); Serial.println ( gpm ); 


    /* *******************************
       calculate and display flow rate
       ******************************* */ 
    
    if ( ( millis () - lastFlowRateAt_ms ) > flowRatePeriod_ms ) {
      if ( nPulsesInList > 0 ) { 
        gpm = float( lastPulseCounts [ nPulsesInList - 1 ] - lastPulseCounts [ 0 ] )
            / float( millis() - pulsesAt_ms [ 0 ] ) 
            * 60.0 * 1000.0 * galsPerPulse;
        formatFloat(strFBuf, fbufLen, gpm, 2);
        // Serial.println ( strFBuf );
        snprintf ( strBuf, bufLen, "%sgpm  ", strFBuf );
        
        #if 1 && ( HTFV_VERBOSE >= 2 )
        
          diff = lastPulseCounts [ nPulsesInList - 1 ] - lastPulseCounts [ 0 ];
          Serial.print ( F ( "  diff:" ) ); Serial.println ( diff );
          
          Serial.print ( F ( "Millis: " ) ); Serial.println ( millis() );
          Serial.print ( F ( "  first pulse at " ) ); Serial.println ( pulsesAt_ms [ 0 ] );
          Serial.print ( F ( "GPP: " ) ); Serial.println ( galsPerPulse );
          
        #endif
      } else {
        gpm = -1.0;
        snprintf ( strBuf, bufLen, "flow unk" );
      }
      LCD.setLoc ( 0, 8 );
      LCD.send ( (unsigned char *) &strBuf[0], strlen ( strBuf ) );
      
      setRegValue ( regs, nRegs, REGFLOWRATEGPM, gpm );

      lastFlowRateAt_ms = millis ();
    }
    

      // Serial.print ( F ( "gpm 2 " ) ); Serial.println ( gpm ); 


    /* *******************************
       display solenoid state
       ******************************* */ 

    if ( digitalRead ( pdSolenoid ) != solenoidState ) {
      // Serial.print ( "7a ct: " ); Serial.println ( pulseCount );
      if ( digitalRead ( pdSolenoid ) ) {
        snprintf ( strBuf, bufLen, "open  " );
        solenoidState = 1;
      } else {
        snprintf ( strBuf, bufLen, "closed" );
        solenoidState = 0;
      }
      LCD.setLoc ( 1, 8 );
      LCD.send ( (unsigned char *) &strBuf[0], strlen ( strBuf ) );
      
      digitalWrite ( pdSolenoidLED, solenoidState );
      setCoilValue ( coils, nCoils, COILSOLENOID, solenoidState );
    }
    
  }  // !done
  
  
  // Serial.print ( F ( "gpm 3 " ) ); Serial.println ( gpm ); 
  
  // blink - on with duration blinkOnDuration_ms, then off for total cycle of blinkPeriod_ms
  
  if ( digitalRead ( pdBlinkyLED ) ) {
    // LED is on. Turn it off?
    if ( ( millis() - lastBlinkAt_ms ) > blinkOnDuration_ms ) {
      digitalWrite ( pdBlinkyLED, 0 );
    }
  }
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkPeriod_ms ) {
    // turn LED on and restart the cycle
    digitalWrite ( pdBlinkyLED, 1 );
    lastBlinkAt_ms = millis ();
  }
  
  LCD.setLoc ( 1, 15 );
  LCD.send ( roguePulses ? ( unsigned char * ) "*" : ( unsigned char * ) " ", 1 );


  /*
     diagnostic prints if necessary
  */

  #if 1 && ( HTFV_VERBOSE >= 2 )
    if ( ( millis() - lastDiagnosticReportAt_ms ) > diagnosticReportInterval_ms ) {
      Serial.print ( F ( "gpm var " ) ); Serial.println ( gpm ); 
      Serial.print ( F ( "gpm diff " ) ); Serial.println ( gpm - getRegValueF ( regs, nRegs, REGFLOWRATEGPM ) );

      Serial.print ( F ( "pulse count var " ) ); Serial.println ( pulseCount );
      Serial.print ( F ( "pulse count diff " ) ); Serial.println ( pulseCount - getRegValueL ( regs, nRegs, REGPULSECOUNT ) );
 
      Serial.println ();

      lastDiagnosticReportAt_ms = millis();
    }
  #endif

  
  delay ( 10 );

}

float getRegValueF ( short * regs, int num_regs, int startingRegNo ) {
  /*
    Given the reg array, its size, and a starting reg number, this subroutine gets the value
      contained in the 2 regs ( 2 x 16 bits = 32 bits )
    Returns -1 if the specified reg span is invalid
  */
  float val;
  if ( ( startingRegNo + 1 ) < num_regs) {
    memcpy ( &val, &regs [ startingRegNo ], 4 );
    // Serial.print ( F (  " set int: "  ) ); Serial.println ( val );
    return ( val );
  } else {
    return ( -1.0 );
  }
}

long getRegValueL ( short * regs, int num_regs, int startingRegNo ) {
  /*
    Given the reg array, its size, and a starting reg number, this subroutine gets the value
      contained in the 2 regs ( 2 x 16 bits = 32 bits )
    Returns -1 if the specified reg span is invalid
  */
  unsigned long val;
  if ( ( startingRegNo + 1 ) < num_regs) {
    memcpy ( &val, &regs [ startingRegNo ], 4 );
    // Serial.print ( F (  " set int: "  ) ); Serial.println ( val );
    return ( val );
  } else {
    return ( -1L );
  }
}


// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 


// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>   interrupt service routines for  flow meter    <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

volatile unsigned long lastFallingEdgeAt_us = micros();

void isr_pulse () {
  int flowPinState;
  flowPinState = digitalRead ( pdPulse );
  digitalWrite ( pdFlowmeterLED, flowPinState );
  if ( flowPinState ) {
    // just went high
    if ( ( micros() - lastFallingEdgeAt_us ) > minMicrosPerPulse ) {
      pulseCount++;
      // lastPulseAt_us = micros();
      // v2.1.1 provide millis instead of micros here
      lastPulseAt_ms = millis();   //store current time in pulseTime
      roguePulses = 0;
    } else {
      roguePulses = 1;
    }
  } else {
    // is low - must be a falling edge
    lastFallingEdgeAt_us = micros();
  }
}

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
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>               general-purpose subroutines               <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void sendMessage (char msg[]) {
  char smBuf[42];
  snprintf (smBuf, 40, "[%s %7lu: %s]\n", PROGMONIKER, millis(), msg);
  Serial.print (smBuf);
  // Serial.print ( msg );
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

int availableMemory () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

  
/*

2014-02-16  1.001.000  cbm tested and verified calibration using water and bucket
2014-02-17 2.000.000 cbm adding MODBUS-RTU
                                               
*/