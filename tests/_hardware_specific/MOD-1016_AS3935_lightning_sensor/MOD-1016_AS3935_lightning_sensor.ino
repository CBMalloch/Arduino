#define PROGNAME "MOD-1016_AS3935_lightning_sensor.ino"
#define VERSION "0.2.8"
#define VERDATE "2017-09-19"

/*
  Doesn't work as is. I'm trying the AS3935_demo program, and it appears to read registers OK
  So maybe I should use the as3935 library.
    Side notes: that program uses SoftWire, which initializes pin 14 (A0) for SDA
    and pin 17 (A3) for SCL.
  Waiting to see if AS3935_demo sees lightning...
  
  Now merging with the AS3935 demo program
  
  Many "Something else happened!" events
  
  Two kinds of calls - process and getTriggered. What events lead to each?
    getTriggered returns the value of _triggered
    process turns _triggered off if state is WaitR
  Does calling getInterruptFlags and getState clear these? No.
    When should they be called?
  Add a real-time clock to this setup, then hard-wire it; move it to an ESP so can 
    query a timeserver?
*/

#define VERBOSE 4

#include <AsyncDelay.h>
#include <SoftWire.h>
#include <AS3935.h>

#ifdef JTD
#include <DisableJTAG.h>
#endif

const unsigned long BAUDRATE = 115200;

const uint8_t i2cAddr = 0x03;
const int piSensor = 2;
const int intSensor = 0;  // Arduino pin 2 is INT0; pin 3 (not used here) is INT1
const int pdLED   = 13;

const byte antennaTuningValue = 0x02;

int distanceEstimate = 0;

uint8_t count = 0;

/* 
	connections:
		A0  SDA for sensor (yellow)
		A3  SCL for sensor (white)
		
		D2  IRQ line from sensor (blue)
		D13 LED

*/

AS3935 as3935;
bool ledState = true;
AsyncDelay d;
char states [ ] [ 6 ] = { "Off", "PwrUp", "Init1", "Init2", "Listn", "WaitR", "Cal" };


void ISR_lightning_sensor (void) {
  as3935.interruptHandler();
}


void displayRegs () {
  Serial.print ( "\nAS3936 Registers as of " ); 
  Serial.print ( millis() );
  Serial.println ( " ms" );
  /*
    AS3935 registers
      0x00 5-1 AFE_GB 0 PWD
        AFE Gain Boost (norm 0x12 = 18)
        Power-down (norm 0)
      0x01 6-4 NF_LEV 3-0 WDTH
        Noise Floor Level (norm 0)
        Watchdog threshold (norm 2)
      0x02 6 CL_STAT 5-4 MIN_NUM_LIGH 3-0 SREJ
        Reg is usually 0xc2 (using a reserved bit!)
        Clear statistics (norm 1)
        Minimum number of lightning (norm 0)
        Spike rejection (norm 2)
      0x03 7-6 LCO_FDIV 5 MASK_DIST 3-0 INT
        Frequency division ratio for antenna tuning (norm 0)
        Mask Disturber (norm 0)
        Interrupt (norm 0)
        
      0x04 S_LIG_L
        Energy of the single lightning LSByte
        
      0x05 S_LIG_M
        ... MSByte
      0x06 4-0 S_LIG_MM
        ... MMSByte
      0x07 5-0 DISTANCE
        Distance estimate (kilometers; norm 0x3f)
      0x08 7 DISP_LCO 6 DISP_SRCO 5 DISP_TRCO 3-0 TUN_CAP
        Display LCO on IRQ pin (norm 0)
        Display SRCO on IRQ pin (norm 0)
        Display TRCO on IRQ pin (norm 0)
        Internal tuning capacitors 0-120pF steps of 8pF (2)
  */
  
  uint8_t val;
  
  as3935.readRegister ( 0 , val );
  Serial.print ( "  AFE gain boost ( 0x12 ): " ); Serial.println ( ( val >> 1 ) & 0x1f, HEX );
  Serial.print ( "  Power-down ( 0 ): " ); Serial.println ( ( val >> 0 ) & 0x01, HEX );
  
  as3935.readRegister ( 1 , val );
  Serial.print ( "  Noise Floor Level ( 0 ): " ); Serial.println ( ( val >> 4 ) & 0x03, HEX );
  Serial.print ( "  Watchdog threshold ( 2 ): " ); Serial.println ( ( val >> 0 ) & 0x0f, HEX );

  as3935.readRegister ( 2 , val );
  Serial.print ( "  Clear statistics ( 1 ): " ); Serial.println ( ( val >> 6 ) & 0x01, HEX );
  Serial.print ( "  Minimum number of lightning ( 0 ): " ); Serial.println ( ( val >> 4 ) & 0x03, HEX );
  Serial.print ( "  Spike rejection ( 2 ): " ); Serial.println ( ( val >> 0 ) & 0x0f, HEX );

  as3935.readRegister ( 3 , val );
  Serial.print ( "  Frequency division ratio for antenna tuning ( 0 ): " ); 
    Serial.println ( ( val >> 6 ) & 0x02, HEX );
  Serial.print ( "  Mask Disturber ( 0 ): " ); Serial.println ( ( val >> 5 ) & 0x01, HEX );
  Serial.print ( "  Interrupt ( 0 ): " ); Serial.println ( ( val >> 0 ) & 0x0f, HEX );

  unsigned long energy = 0;
  as3935.readRegister ( 6 , val );
  energy |= ( val & 0x0f );
  energy <<= 4;
  as3935.readRegister ( 5 , val );
  energy |= ( val & 0xff );
  energy <<= 8;
  as3935.readRegister ( 4 , val );
  energy |= ( val & 0xff );
  if ( energy > 0 ) for ( int i = 0; i < 30; i++ ) Serial.print ( " " );
  Serial.print ( "  Energy of the single lightning: " ); Serial.println ( energy );
  
  if ( energy > 0 ) {
    as3935.readRegister ( 7 , val );
    val = ( val >> 0 ) & 0x3f;
    if ( val < 0x3f ) for ( int i = 0; i < 30; i++ ) Serial.print ( " " );
    Serial.print ( "  Distance estimate, km ( 63 ): " );
    Serial.print ( val );
  }
  Serial.println ();

  /*
    AS3935 software state
      stateOff = 0,
      statePoweringUp = 1, // first 2ms wait after power applied
      stateInitialising1 = 2,
      stateInitialising2 = 3,
      stateListening = 4,
      stateWaitingForResult = 5,
      stateCalibrate = 6,
  */

  Serial.print ( "  State: " );
  Serial.println ( states [ as3935.getState() ] );

  /*
    // no referent!
    Serial.print("EIFR: ");
    Serial.print ( EIFR < 0x10 ? "0x0" : "0x" );
    Serial.println(EIFR, HEX);
  */
  Serial.flush();
}

void setup () {
  #ifdef JTD
    disableJTAG();
  #endif

  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 2000 );
  
  // sda, scl, address, tunCap, indoor, timestamp
  as3935.initialise ( 14, 17, i2cAddr, antennaTuningValue, true, NULL );
  delay ( 20 );
  as3935.start();
  
  // delay ( 20 );
  d.start(1000, AsyncDelay::MILLIS);
  while ( ! d.isExpired() ) as3935.process();
  
  as3935.setNoiseFloor(0);
  delay ( 20 );
  // as3935.calibrate();
  // delay ( 20 );
  
  pinMode ( pdLED, OUTPUT );
  digitalWrite ( pdLED, ledState );
  
  pinMode ( piSensor, INPUT );
  attachInterrupt ( intSensor, ISR_lightning_sensor, RISING );
  d.start (1000, AsyncDelay::MILLIS);
  
  #if VERBOSE >= 2
  
    Serial.print ( "\n\nLightning Sensor [" );
    Serial.print ( PROGNAME );
    Serial.print ( "] v");
    Serial.print ( VERSION );
    Serial.print ( " (" );
    Serial.print ( VERDATE );
    Serial.print ( ")\n\n" );
    
    displayRegs ();
  
  #endif

}

void loop () {

  int interruptFlags = -1;
  int distanceEstimate = -1;
  static int oldState = -1;
  
  int state = as3935.getState();
  if ( state != oldState ) {
    Serial.print ( "  State: " );
    Serial.println ( states [ as3935.getState() ] );
    oldState = state;
  }
  
  if ( as3935.process() ) {
    interruptFlags = as3935.getInterruptFlags();
    distanceEstimate = as3935.getDistance();

    Serial.println ( "\n------ process -------" );
    Serial.print ( "Interrupt flags: 0x" );
    if ( interruptFlags < 0x10 ) Serial.print ( "0" );
    Serial.print ( interruptFlags, HEX );
    if ( distanceEstimate != 63 ) {
      Serial.print ( "; ( Distance: " );
      Serial.print ( distanceEstimate );
      Serial.print ( " km )" );
    }
    Serial.println ();
  }
  
  if (as3935.getBusError()) Serial.println ( "\nBus error!" );

  if ( as3935.getTriggered() ) {
    
    Serial.print ( "\n\n---------- triggered ----------\n" );

    displayRegs ( );
    
    interruptFlags = as3935.getInterruptFlags ();
    switch ( interruptFlags ) {
      case 0x01:
        Serial.print ( "Noise" );
        break;
      case 0x04:
        Serial.print ( "Disturber" );
        break;
      case 0x08:
        distanceEstimate = as3935.getDistance();
        Serial.print ( "Lightning at distance " );
        Serial.print ( distanceEstimate );
        Serial.print ( " km" );
        break;
      case -1:
        break;
      default:
        Serial.print ( "Something else" );
        break;
    }
    if ( interruptFlags >= 0 ) {
      Serial.println ( " happened!" );
      Serial.println("----------------------\n\n");
    }

  }


  if (d.isExpired()) {
    ledState = ! ledState;
    digitalWrite ( pdLED, ledState );
    
    if (++count > 5) {
      count = 0;
      // displayRegs ();
    }
    d.start(1000, AsyncDelay::MILLIS);
  }
  
}