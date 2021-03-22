#define PROGNAME "MOD-1016_AS3935_lightning_sensor.ino"
#define VERSION "0.3.14"
#define VERDATE "2019-08-08"

/*

  NOT THIS ONE!!!
  
  
  Other program doesn't work as is. I'm trying the AS3935_demo program, 
    and it appears to read registers OK
  So this program uses the as3935 library.
    Side notes: that program uses SoftWire, which initializes pin 14 (A0) for SDA
    and pin 17 (A3) for SCL.
  Waiting to see if AS3935_demo sees lightning...
  
  It did until I added a second throbber, I think... 
  Backing out of that didn't help
  
  Now merging with the AS3935 demo program
  
  Many "Something else happened!" events
  
  Two kinds of calls - process and getTriggered. What events lead to each?
    getTriggered returns the value of _triggered
    process turns _triggered off if state is WaitR
  Does calling getInterruptFlags and getState clear these? No.
    When should they be called?
  Add a real-time clock to this setup, then hard-wire it; move it to an ESP so can 
    query a timeserver? -- done with perl serial catcher for now
    
  v0.3.0 removed dumping registers unless VERBOSE or actual lightning detected
  v0.3.5  2018-08-15 cbm relinked with updated AS3935 libraries; works once again.
  v0.3.13 2019-08-08 cbm seems never to trigger; refactoring to try to understand and
                         correct this
  v0.3.14 2019-08-08 cbm still tyring to fix; have moved many strings to flash memory F( "..." )
  
*/

#include <AsyncDelay.h>
#include <SoftWire.h>
#include <AS3935.h>

#ifdef JTD
  #include <DisableJTAG.h>
#endif

//************************
//************************

#define VERBOSE 4

const unsigned long BAUDRATE = 115200;
const int tabColumn = 40;

// 2 is default; bigger number rejects more spikes
// back to 2 when I found that debug wires were raising the noise...
// 2019-06-05 2 -> 1 -> 0
const int SPIKE_REJECTION_VALUE = 0;  // 0-15

// threshold number of strikes within 1 minute
// 0x00 -> 1; 0x01 -> 5; 0x02 -> 9; 0x03 -> 16 in bits 5-4 of register 2
// 2019-06-05 0x01 -> 0x00
const int STRIKE_THRESHOLD_VALUE = 0x00;

// mask events that are classed as man-made disturbances
// 2019-06-05 1 -> 0
// 2019-08-07 0 -> 1 -> 0
const int DISTURBER_MASK_VALUE = 0;

// 0 -> 1 -> 2 -> 3 -> 0 2018-08-17 cbm
// back to 0 when I found that debug wires were raising the noise...
const int NOISE_FLOOR_VALUE = 0;


//************************
//************************

/*************************** hardware incl GPIO *******************************/

/* 
	connections:
		A0  SDA for sensor (yellow)
		A3  SCL for sensor (white)
		
		D2  IRQ line from sensor (blue)
		D13 LED

*/

// const int      SDA               =   A0;   // AKA D14; predefined
// const int      SCL               =   A3;   // AKA D17; predefined

const bool     indoorsP          = true;

const int      piSensor          =    2;  // hardware interrupt pin from sensor
// const int      intSensor         =    0;  // Arduino pin 2 is INT0; pin 3 (not used here) is INT1
const int      pdThrobber        =   13;

const uint8_t i2cAddr            = 0x03;
const byte    antennaTuningValue = 0x02;

/******************************* Global Vars **********************************/

int distanceEstimate = 0;
uint8_t count = 0;
bool ledState = true;

AS3935 as3935;

AsyncDelay throbberTimeout, textThrobberTimeout;

bool callbackEnabled = false;  // prevent callback activation until initialization complete

char states [ ] [ 6 ] = { "Off", "PwrUp", "Init1", "Init2", "Listn", "WaitR", "Cal" };

/************************** Function Prototypes *******************************/

void ISR_lightning_sensor (void);
void displayRegs ();

/******************************************************************************/

void setup () {

  const int dly = 5;
  
  #ifdef JTD
    disableJTAG();
  #endif

  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 2000 );
  
  Serial.println ( "Startup" );

  // sda, scl, address, tunCap, indoor, cbTriggered (<callback when triggered> or NULL)
  // NOTE use of constants SDA and SCL makes startup fail!
  as3935.initialise ( 14, 17, i2cAddr, antennaTuningValue, indoorsP, cbTriggered );
  // no delays necessary - internal to as3935 code
  // delay ( dly );
  
  // Serial.println ( "Initialized" );

  // as3935.reset();
  // delay ( dly );
  // as3935.clearStats();
  // delay ( dly );
  
  // as3935.calibrate();
  // start implicitly calls calibrate
  as3935.start();
  // delay ( dly );
    
  // Serial.println ( "Started" );

  // process steps through initilization states, until it's in Listen state	
  while ( as3935.getState() != AS3935::stateListening ) as3935.process();  // 
  
  // delay ( dly );
  
  if ( ! as3935.setNoiseFloor ( NOISE_FLOOR_VALUE ) ) {
    Serial.println ( F( "Failed to set noise floor!" ) );
    while ( 1 );
  }
  delay ( dly );
  
  if ( 1 ) {
    // check to see if communications is working...
    uint8_t val = 0xff;
    if ( ! as3935.readRegister ( AS3935::regNoiseFloor , val ) ) {
      Serial.println ( F( "Failed to read noise floor register!" ) );
      while ( 1 );
    }
    val = ( val >> 4 ) & 0x07;
    if ( val != NOISE_FLOOR_VALUE ) {
      Serial.print ( F( "Noise floor was set to" ) );
      Serial.print ( NOISE_FLOOR_VALUE );
      Serial.print ( F( "but remained ==> " ) );
      Serial.println ( val );
      while ( 1 );
    }
  }
  
  
  // as3935.calibrate();
  // delay ( 20 );
  // bit 5 of interrupt mask register can be set to ignore disturbance events
  as3935.setRegisterBit ( AS3935::regInt, 5, DISTURBER_MASK_VALUE );
  delay ( 20 );
  
  uint8_t mask, r, newValue;
  
  // set spike rejection
  newValue = SPIKE_REJECTION_VALUE;
  mask = B00001111;
  as3935.readRegister ( AS3935::regSpikeRej, r );
  Serial.print ( F( "Setting spike rejection; register 0x" ) ); 
  Serial.print ( r, HEX );
  r &= ~ mask;
  r |= newValue << 0;  
  Serial.print ( F( " => 0x" ) ); 
  Serial.print ( r, HEX );
  Serial.println ();
  as3935.writeRegister ( AS3935::regSpikeRej, r );
  delay ( 20 );
  
  // set threshold number of strikes within 1 minute 
  // could use as3935.setMinimumLightnings ( 1 ), but instead setting it myself
  // 0x00 -> 1; 0x01 -> 5; 0x02 -> 9; 0x03 -> 16 in bits 5-4 of register 2
  mask = B00110000;
  newValue = STRIKE_THRESHOLD_VALUE;
  as3935.readRegister ( AS3935::regSpikeRej, r );
  Serial.print ( F( "Setting threshold number of spikes; register 0x" ) ); 
  Serial.print ( r, HEX );
  r &= ~ mask;
  r |= newValue << 4;
  Serial.print ( F( " => 0x" ) ); 
  Serial.print ( r, HEX );
  Serial.println ();
  as3935.writeRegister ( AS3935::regSpikeRej, r );
  delay ( 20 );
    
  pinMode ( pdThrobber, OUTPUT );
  digitalWrite ( pdThrobber, ledState );
  
  // 2019-07-07 INPUT->INPUT_PULLUP
  pinMode ( piSensor, INPUT_PULLUP );
  // 2019-07-07 intSensor -> digitalPinToInterrupt(piSensor)
  attachInterrupt ( digitalPinToInterrupt(piSensor), ISR_lightning_sensor, RISING );
  
  // Serial.println ( F( "Interrupt attached" ) );
  
  // 2019-08-08 added just for yuks
  // as3935.calibrate();
  // delay ( 20 );
  
  
  throbberTimeout.start ( 1000, AsyncDelay::MILLIS );
  
  if ( VERBOSE >= 2 ) {
    textThrobberTimeout.start ( 10UL * 60UL * 1000UL, AsyncDelay::MILLIS );
  }
  
  callbackEnabled = true;
  
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

  int distanceEstimate = -1;
  static int oldState = -1;
  
  /*
    AS3935 software state
      stateOff = 0,
      statePoweringUp = 1,        // first 2ms wait after power applied
      stateInitialising1 = 2,
      stateInitialising2 = 3,
      stateListening = 4,
      stateWaitingForResult = 5,  // after interrupt recognized and serviced
      stateCalibrate = 6,
      
    4 routines affect the state:
      initialize
      start
      process
      calibrate
  */
  
  if ( as3935.process() ) {
  
    int state = as3935.getState();
    if ( state != oldState ) {
      for ( int i = 0; i < tabColumn; i++ ) Serial.print ( " " );
      Serial.print ( "===> new state: " );
      Serial.println ( state );
      oldState = state;
    }
  
    int interruptFlags = as3935.getInterruptFlags();
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
  
  if (as3935.getBusError()) {
    Serial.println ( "\nBus error!" );
    as3935.clearBusError();
  }

  // if ( as3935.getTriggered() ) {
  //   
  //   Serial.print ( "\n\n---------- triggered ----------\n" );
  // 
  //   interruptFlags = as3935.getInterruptFlags ();
  //   
  //   bool handled = false;
  //   if ( interruptFlags & AS3935::intNoiseLevelTooHigh ) {
  //     if ( VERBOSE >= 10 ) displayRegs ( );
  //     Serial.print ( "Noise" );
  //     handled = true;
  //   }
  //   if ( interruptFlags & AS3935::intDisturberDetected ) {
  //     if ( VERBOSE >= 10 ) displayRegs ( );
  //     Serial.print ( "Disturber" );
  //     handled = true;
  //   }
  //   if ( interruptFlags & AS3935::intLightningDetected ) {
  //     displayRegs ( );
  //     distanceEstimate = as3935.getDistance();
  //     Serial.print ( "Lightning at distance " );
  //     Serial.print ( distanceEstimate );
  //     Serial.print ( " km" );
  //     handled = true;
  //   }
  //   if ( handled ) {
  //     Serial.println ( " happened!" );
  //     Serial.println("----------------------\n\n");
  //   } else {
  //     if ( VERBOSE >= 20 ) displayRegs ( );
  //     Serial.print ( "Funny flags: 0x0" );
  //     Serial.println ( interruptFlags, HEX );
  //   }
  // 
  // }

  if (throbberTimeout.isExpired()) {
    ledState = ! ledState;
    digitalWrite ( pdThrobber, ledState );
    
    if (++count > 5) {
      count = 0;
      // displayRegs ();
    }
    throbberTimeout.start(1000, AsyncDelay::MILLIS);
  }
  
  if ( VERBOSE >= 2 ) {
    if (textThrobberTimeout.isExpired()) {
      Serial.println ( "Still alive..." );
      textThrobberTimeout.start ( 10UL * 60UL * 1000UL, AsyncDelay::MILLIS );
    }
  }
  
}

/******************************************************************************/
/***************************** lightning sensor *******************************/
/******************************************************************************/

void ISR_lightning_sensor (void) {
  as3935.interruptHandler();
}

void displayRegs () {

  static bool firstTimeP = true;
  
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
  Serial.print ( F( "  AFE gain boost ( 0x12 ): " ) ); Serial.println ( ( val >> 1 ) & 0x1f, HEX );
  Serial.print ( F( "  Power-down ( 0 ): " ) ); Serial.println ( ( val >> 0 ) & 0x01, HEX );
  
  as3935.readRegister ( 1 , val );
  Serial.print ( F( "  Noise Floor Level ( 0 ): " ) ); Serial.println ( ( val >> 4 ) & 0x03, HEX );
  Serial.print ( F( "  Watchdog threshold ( 2 ): " ) ); Serial.println ( ( val >> 0 ) & 0x0f, HEX );

  as3935.readRegister ( 2 , val );
  Serial.print ( F( "  Clear statistics ( 1 ): " ) ); Serial.println ( ( val >> 6 ) & 0x01, HEX );
  int num = ( ( val >> 4 ) & 0x03 );
  int nums [] = { 1, 5, 9, 16 };
  Serial.print ( F( "  Minimum reportable strikes/min ( 0 ): " ) ); Serial.print ( num ); 
    Serial.print ( F( " => " ) ); Serial.println ( nums [ num ] );
  Serial.print ( F( "  Spike rejection ( 2 ): " ) ); Serial.println ( ( val >> 0 ) & 0x0f, HEX );

  as3935.readRegister ( 3 , val );
  Serial.print ( F( "  Frequency division ratio for antenna tuning ( 0 ): " ) ); 
    Serial.println ( ( val >> 6 ) & 0x02, HEX );
  Serial.print ( F( "  Mask Disturber ( 0 ): " ) ); Serial.println ( ( val >> 5 ) & 0x01, HEX );
  Serial.print ( F( "  Interrupt ( 0 ): " ) ); Serial.println ( ( val >> 0 ) & 0x0f, HEX );
  
  if ( ! firstTimeP ) {
  
    // avoid printing residual values during startup 

    unsigned long energy = 0;
    as3935.readRegister ( 6 , val );
    energy |= ( val & 0x0f );
    energy <<= 4;
    as3935.readRegister ( 5 , val );
    energy |= ( val & 0xff );
    energy <<= 8;
    as3935.readRegister ( 4 , val );
    energy |= ( val & 0xff );
    if ( energy > 0 ) for ( int i = 0; i < tabColumn; i++ ) Serial.print ( " " );
    Serial.print ( F( "  Energy of the single lightning: " ) ); Serial.println ( energy );
  
    if ( energy > 0 ) {
      as3935.readRegister ( 7 , val );
      val = ( val >> 0 ) & 0x3f;
      if ( val < 0x3f ) for ( int i = 0; i < tabColumn; i++ ) Serial.print ( " " );
      Serial.print ( F( "  Distance estimate, km ( 63 ): " ) );
      Serial.print ( val );
    }
    Serial.println ();
    
  }
  

  /*
    // no referent!
    Serial.print("EIFR: ");
    Serial.print ( EIFR < 0x10 ? "0x0" : "0x" );
    Serial.println(EIFR, HEX);
  */
  Serial.flush();
  
  firstTimeP = false;
    
}

void cbTriggered ( void ) {

  /*
    This callback is called by AS3935::interruptHandler in AS3935.h, *before* 
    _triggered is made true, and *before* _state is changed to stateWaitingForResult
    
    That may affect what goes on herein...
    
    The call stack is this:
      ISR_lightning_sensor services the hardware interrupt attached to 
        the pin piSensor
      ISR_lightning_sensor immediately calls as3935.interruptHandler()
      which immediately calls *this* routine. Nothing has yet changed any internal flags  
        or anything, including _interruptFlags (which is what is dumbly returned by 
        as3935.getInterruptFlags()
        
    *BUT*
    
    This routine unconditionally prints "---------- triggered;", and that never happens.
    Therefore, we can conclude that the interrupt is never occurring. 
    
    We can test this by manually raising the voltage on the interrupt wire and seeing if 
    the rougine gets triggered.
    
  */

  const int tBufLen = 80;
  char tBuf [ tBufLen ];
  bool handled = false;
  
  if ( ! callbackEnabled ) {
    return;
  }

  int interruptFlags = as3935.getInterruptFlags ();
  
  // Serial.print ( "" );
  snprintf ( tBuf, tBufLen, "\n\n---------- triggered; flags: %#06x ----------\n", interruptFlags );
  Serial.print ( tBuf );

  
  if ( interruptFlags & AS3935::intNoiseLevelTooHigh ) {
    if ( VERBOSE >= 10 ) displayRegs();
    Serial.print ( F( "Noise " ) );
    handled = true;
  }
  if ( interruptFlags & AS3935::intDisturberDetected ) {
    if ( VERBOSE >= 10 ) displayRegs();
    Serial.print ( F( "Disturber " ) );
    handled = true;
  }
  if ( interruptFlags & AS3935::intLightningDetected ) {    
    distanceEstimate = as3935.getDistance();
    snprintf ( tBuf, tBufLen, "Lightning at distance %d km ", distanceEstimate );
    Serial.print ( tBuf );
    handled = true;
  }
  if ( handled ) {
    Serial.print ( F( "happened!\n----------------------\n\n" ) );
  } else {
    if ( VERBOSE >= 20 ) displayRegs();
    Serial.println ( F( "                                                    Funny flags!" ) );
  }

}