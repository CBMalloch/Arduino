#define PROGNAME  "solar_tracker_LoRa_tracker"
#define VERSION   "2.2.19"
#define VERDATE   "2018-02-15"

/*
  <https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test>
  <http://www.diymalls.com/lora32u4-lorawan?product_id=84>
  <https://github.com/hj91/nodemcu-esp8266/blob/master/lora_sender/lora_sender.ino>
  
  Chip is ATMEGA32U4
  choose board Arduino Mega ADK? No. Adafruit Feather 32u4, I think.
  
	Thanks to Kris Winer for the MPU-9250 parts of this code, via SparkFun:
		MPU9250 Basic Example Code
		by: Kris Winer
		date: April 1, 2014
		license: Beerware - Use this code however you'd like. If you
		find it useful you can buy me a beer some time.
		Modified by Brent Wilkins July 19, 2016
		
	Things left to do:
		√ build board to hold LoRa32u4 and MPU9250
			hacks: 
				1) mis-wired the MPU9250 socket - wrong header; can kludge
				2) no point in measuring vSupp, since it's developed and delivered by 
					Sparkfun Sunny Buddy. Can get vBat off that board, I think, though.
					Check into this. Nope.
		√ fix the (useless) vSupp calculation
			Verify that pin D10 is read with analogRead ( 13 ). 
			Nope. The pin marked 10 (Arduino pin numbering) is probably D10,
			but it's definitely the one read with analogRead ( 10 ) even though
			it's identified on the pinout as ADC13.
			Corrected in v2.1.3
		√ convert output to JSON
		o measure battery life
		√ increase the sleepy time
		o fix the cycle time - it's a little longer than requested
			when sleeping for 1 min, it's 7-8sec in excess
		
			
*/

#include <EEPROM.h>
#include <SPI.h>
#include <LoRa.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <cbmMPU9250.h>
#include "quaternionFilters.h"
#include <ArduinoJson.h>

#define BAUDRATE 115200
#define TESTING false

/*
	We're trying to track down a periodic sleep error where, instead of the 
	usual cycle times of about 60 seconds, including 51942 ms of sleep, we 
	have a cycle time of about 51 seconds, still including 51942 ms of sleep.
	The sleep times are calculated by 60 minus the measured awake time, and are 
	apparently *always* 51942ms, never varying!
	
	It's thus apparently no use tracking the sleep times.
	
	Maybe somehow we're occasionally skipping the processing time? If so, how might 
	we be still calculating the sleep time of 51942ms?
	
	
+ 2017-12-26 09:16:36: received: (247) '{"devID":"LoRa32u4 1","packetSN":19,"progID":{"progName":"solar_tracker_LoRa_tracker","version":"2.2.13","verDate":"2017-12-26"},"vSupp":3.870968,"tChip":23.97421,"tilt":{"roll":-1.79874,"pitch":0.185954},"heading":125.0074,"sleepsTotal_ms":51942}'; RSSI  -28; SNR 17.25
+ 2017-12-26 09:17:27: received: (249) '{"devID":"LoRa32u4 1","packetSN":20,"progID":{"progName":"solar_tracker_LoRa_tracker","version":"2.2.13","verDate":"2017-12-26"},"vSupp":3.870968,"tChip":23.99518,"tilt":{"roll":-2.150842,"pitch":-0.587687},"heading":125.2239,"sleepsTotal_ms":51942}'; RSSI  -28; SNR  7.25

	Nope. Apparently we're still doing the full processing - otherwise the numbers 
	would stay the same or, if no warmup took place, the pose measurements would be 
	wild.

*/
# define MAINTAIN_SLEEP_LOG false
#define ALLOW_HARDWARE_SLEEP true
const unsigned long warmupPeriodDuration = 800UL;

// compensate for sleep duration mean error
// 1 sec / day = 1 / 86400 = 1.157e-5
// 1.2; 1.1304; 1.132174; 1.1311; 1.13135; 1.132074
// it's apparent that it's temperature sensitive, with a value of 1.124 at -15degC
// and a value of 1.132 at 20degC
// We could calibrate it more, but it seems moot...
const float LoRa32u4_1_sleep_reduction_factor = 1 / 1.128;
  	

#if TESTING
	#define VERBOSE 12
	const unsigned long cycleOverallDuration = 5 * 1000UL;
	#define MAINTAIN_SLEEP_LOG true
#else
	#define VERBOSE 0
	const unsigned long cycleOverallDuration = 1 * 60 * 1000UL;
#endif

#if MAINTAIN_SLEEP_LOG
	// const int sleepVectorSize = 32;
	// int sleepVector_code [ sleepVectorSize ]; 
	// int sleepVectorIndex = 0;
	long programmedSleepsTotal_ms = 0;
#endif

#if ALLOW_HARDWARE_SLEEP
  #ifndef cbi
    #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
  #endif
  #ifndef sbi
    #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
  #endif
#endif

#define LORA_FREQ_BAND 433E6
// LoRa32u4 ports
const int pdLoRa_SS   =  8;       // CS chip select
const int pdLoRa_RST  =  4;       // reset
const int pdLoRa_DI0  =  7;       // IRQ interrupt request
   
const int pdLED       = 13;       // LED
const int pdSleep     = 12;       // last time I loaded, was 17, which isn't exposed
const int pdStatusLED =  5;       // for debugging
const int paBat       = 10;       // reads half the battery voltage
  // relative to 3V3; e.g. 512 counts = 1V15 implies vSupp = 3V3

int packetNum = 0;
float vSupp;
const int uniqueIDlen = 20;
char uniqueID [ uniqueIDlen ];

cbmMPU9250 myIMU;

void blinkStatus ( int n, unsigned long rate_ms = 250UL );

void setup () {

  pinMode ( pdSleep, INPUT_PULLUP );
  pinMode ( pdLED, OUTPUT ); digitalWrite ( pdLED, 0 );
	pinMode ( pdStatusLED, OUTPUT ); blinkStatus ( 1 );
  pinMode ( pdLoRa_RST, OUTPUT ); digitalWrite ( pdLoRa_RST, 1 ); 
	pinMode ( pdStatusLED, OUTPUT ); digitalWrite ( pdStatusLED, 0 );

  #if TESTING
		Serial.begin ( BAUDRATE );
		while ( !Serial && millis () < 10000 );
  #endif
  
  LoRa.setPins ( pdLoRa_SS, pdLoRa_RST, pdLoRa_DI0 );
  
  if ( ! LoRa.begin ( LORA_FREQ_BAND ) ) {
    Serial.println ( "LoRa init failed!" );
    while ( 1 );
  }
  
  /*
      void setSpreadingFactor(int sf);
      void setSignalBandwidth(long sbw);
      void setCodingRate4(int denominator);
      void setPreambleLength(long length);
      void setSyncWord(int sw);
      void enableCrc();
      void disableCrc();
  */
  
  LoRa.enableCrc();
  
  for ( int i = 0; i < uniqueIDlen; i++ ) {
    const int baseAddr = 0x00;
    uniqueID [ i ] = EEPROM.read ( baseAddr + i );
  }
  
  // set up IMU
  
  #if TESTING
		if ( VERBOSE >= 12 ) Serial.print ( "About to test MPU9250" );
  #endif
  // Read the WHO_AM_I register, this is a good test of communication
  byte c = myIMU.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  if ( c != 0x71 ) {
		Serial.print("WHO_AM_I_MPU9250 is "); Serial.print(c, HEX);
		Serial.print("; s.b. 0x71; could not connect");
		while ( 1 );
	}
	
	#if VERBOSE >= 15
		Serial.println("MPU9250 is online...");

		myIMU.MPU9250SelfTest(myIMU.SelfTest);
		Serial.print("x-axis self test: acceleration trim within : ");
		Serial.print(myIMU.SelfTest[0],1); Serial.println("% of factory value");
		Serial.print("y-axis self test: acceleration trim within : ");
		Serial.print(myIMU.SelfTest[1],1); Serial.println("% of factory value");
		Serial.print("z-axis self test: acceleration trim within : ");
		Serial.print(myIMU.SelfTest[2],1); Serial.println("% of factory value");
		Serial.print("x-axis self test: gyration trim within : ");
		Serial.print(myIMU.SelfTest[3],1); Serial.println("% of factory value");
		Serial.print("y-axis self test: gyration trim within : ");
		Serial.print(myIMU.SelfTest[4],1); Serial.println("% of factory value");
		Serial.print("z-axis self test: gyration trim within : ");
		Serial.print(myIMU.SelfTest[5],1); Serial.println("% of factory value");
	#endif

	// Calibrate gyro and accelerometers, load biases in bias registers
	myIMU.calibrateMPU9250(myIMU.gyroBias, myIMU.accelBias);
	
	myIMU.initMPU9250();
	// Initialize device for active mode read of acclerometer, gyroscope, and
	// temperature
	if ( VERBOSE >= 10 ) Serial.println("MPU9250 initialized for active data mode....");

	// Read the WHO_AM_I register of the magnetometer, this is a good test of
	// communication
	byte d = myIMU.readByte(AK8963_ADDRESS, WHO_AM_I_AK8963);
	if ( d != 0x48 ) {
		Serial.print("WHO_AM_I_AK8963 is "); Serial.print(d, HEX);
		Serial.print("; s.b. 0x48; could not connect");
		while ( 1 );
	}
	
	// Get magnetometer calibration from AK8963 ROM
	myIMU.initAK8963(myIMU.magCalibration);
	// Initialize device for active mode read of magnetometer
	if ( VERBOSE >= 10 ) {
		Serial.println("AK8963 initialized for active data mode....");
		//  Serial.println("Calibration values: ");
		Serial.print("X-Axis sensitivity adjustment value ");
		Serial.println(myIMU.magCalibration[0], 2);
		Serial.print("Y-Axis sensitivity adjustment value ");
		Serial.println(myIMU.magCalibration[1], 2);
		Serial.print("Z-Axis sensitivity adjustment value ");
		Serial.println(myIMU.magCalibration[2], 2);
	}

	

    
  if ( Serial ) {
    Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
    Serial.print ( "Unique ID: '" ); Serial.print ( uniqueID ); Serial.println ( "'" );
    Serial.print ( "Hardware sleep is" ); 
    	Serial.print ( ALLOW_HARDWARE_SLEEP ? "" : " not" ); 
    	Serial.print ( " enabled\n");
  }
  delay ( 1000 );
  
  #if ALLOW_HARDWARE_SLEEP
    // CPU Sleep Modes 
    // SM2 SM1 SM0 Sleep Mode
    // 0    0  0 Idle
    // 0    0  1 ADC Noise Reduction
    // 0    1  0 Power-down -- int-1_int-0_pinChg TWImatch WDT
    // 0    1  1 Power-save -- int-1_int-0_pinChg TWImatch timer2 WDT
    // 1    0  0 Reserved
    // 1    0  1 Reserved
    // 1    1  0 Standby(1) -- mainClk timerOscillator int-1_int-0_pinChg TWImatch WDT
    // 1    1  1 Extended Standby -- mainClk timerOscillator int-1_int-0_pinChg TWImatch timer2 WDT
  
    // SMCR is the sleep mode control register
    // 0  010 -> power-down mode
  
    cbi ( SMCR, SE  );     // clear sleep enable
    cbi ( SMCR, SM2 );     // power down mode
    sbi ( SMCR, SM1 );     // power down mode
    cbi ( SMCR, SM0 );     // power down mode

  #endif

}

void loop() {

	/*
		Here's the plan. 
		The cycle of activities consists of
			1) We enable and start reading the sensors.
			2) We reach a point where we consider these readings to be stable and 
				accurate. We power up the transmitter and send the readings over LoRa.
			3) We power everything down to prepare for resting a while and rest.
				This resting might be sleeping the processor, but by connecting a wire 
				we can signal the processor to not sleep but just wait. This can help 
				when we want to reprogram the device, or just for debugging. If we aren't 
				sleeping, we'll be flashing the LED during this period
		
		We begin a cycle, either by waking up or by completing a previous cycle, with 
		the zeroing of the time-marker thisCycleBeganAt_ms.
	*/
	
  static unsigned long thisCycleBeganAt_ms = 0UL;
  static bool sensorsEnabledP = false;
  static bool dataSentP = false;
  static bool restingP = false;
  
  static unsigned long lastBlinkAt_ms = 0UL;
  const unsigned long blinkDelay_ms = 500UL;
  
  blinkStatus ( -1 );
  
  if ( Serial && VERBOSE >= 20 ) {
  	Serial.print ( "At " ); Serial.print ( thisCycleBeganAt_ms ); Serial.print ( "ms" );
  	Serial.print ( "; sEP: " ); Serial.print ( sensorsEnabledP );
  	Serial.print ( "; dSP: " ); Serial.print ( dataSentP );
  	Serial.print ( "; r: " );   Serial.print ( restingP );
  	Serial.println (); delay ( 10 );
  }
  
  if ( ! sensorsEnabledP ) {
  
  	// enable the sensors
  	
  	sensorsEnabledP = true;
  	blinkStatus ( 2 );
  	
  } else if ( ( millis() - thisCycleBeganAt_ms ) < warmupPeriodDuration ) {
  	
  	// warming up
  	
    updateIMU();
  	blinkStatus ( 3 );
    
  } else if ( ! dataSentP ) {
  
  	// time to send data
      
    updateIMU();
    vSupp = ( ( float ) analogRead ( paBat ) ) / 1023.0 * 2.0 * 3.3;
  	transmitData ();
  	dataSentP = true;
  	blinkStatus ( 4 );
  	
  } else if ( ! restingP ) {
  
		// prepare for and initiate rest period
		
		restingP = true;
		
		// initiate rest period
		
		#if ALLOW_HARDWARE_SLEEP

  		blinkStatus ( 5 );
			// can prohibit sleep by pulling pin pdSleep to ground
			if ( digitalRead ( pdSleep ) ) {
			
				if ( Serial ) {
					Serial.print ( "Process took " ); 
						Serial.print ( millis() - thisCycleBeganAt_ms ); 
						Serial.println ( " ms" );
				}
				
				/*
					Somehow we occasionally sleep to a period of 51 seconds instead of 60...
					It's not yet clear why or how or even when!
				*/

				// sleep!
				
				sleepFor_ms ( ( thisCycleBeganAt_ms + cycleOverallDuration ) - millis() );
				
				// done sleeping; time to start a new cycle
				
				sensorsEnabledP = false;
				dataSentP = false;
				restingP = false;
				// don't depend on millis() to update during sleep!
				thisCycleBeganAt_ms = millis ();  
				
			}
			
		#endif

	} else {
	
		// resting but not sleeping
		
  	blinkStatus ( 6 );
		if ( ( millis() - lastBlinkAt_ms ) > blinkDelay_ms ) {
			digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
			lastBlinkAt_ms = millis();
		}
		
		// wake up if it's time!
		
		if ( ( millis () - thisCycleBeganAt_ms ) > cycleOverallDuration ) {
			// we're done resting; time to start a new cycle
			sensorsEnabledP = false;
			dataSentP = false;
			restingP = false;
			thisCycleBeganAt_ms = millis ();
		}
	}
        
}



//****************************************************************
//****************************************************************
//****************************************************************  

#if ALLOW_HARDWARE_SLEEP

  void sleepFor_ms ( long remainingSleep_ms ) {
  
  	// thus max sleep is 2,147,483.647 sec = 24.8 days
  	// neg sleep time will not sleep at all...
  	
  	remainingSleep_ms = remainingSleep_ms * LoRa32u4_1_sleep_reduction_factor + 0.5;  // will truncate to integer = round
  
    #if MAINTAIN_SLEEP_LOG
    	// sleepVectorIndex = 0;
    	programmedSleepsTotal_ms = 0;
    #endif
    
    while ( remainingSleep_ms > 0 ) {
  
      /*
        sleep for as large a chunk as possible of the remaining sleep period
      */
    
      unsigned long aboutToSleepFor_ms;
      int intervalIndex = -1;

      if ( remainingSleep_ms >= 8000 ) {
        aboutToSleepFor_ms = 8000; intervalIndex = 9;
      } else if ( remainingSleep_ms >= 4000 ) {
        aboutToSleepFor_ms = 4000; intervalIndex = 8;
      } else if ( remainingSleep_ms >= 2000 ) {
        aboutToSleepFor_ms = 2000; intervalIndex = 7;
      } else if ( remainingSleep_ms >= 1000 ) {
        aboutToSleepFor_ms = 1000; intervalIndex = 6;
      } else if ( remainingSleep_ms >= 500 ) {
        aboutToSleepFor_ms = 500;  intervalIndex = 5;
      } else if ( remainingSleep_ms >= 250 ) {
        aboutToSleepFor_ms = 250;  intervalIndex = 4;
      } else if ( remainingSleep_ms >= 128 ) {
        aboutToSleepFor_ms = 128;  intervalIndex = 3;
      } else if ( remainingSleep_ms >= 64 ) {
        aboutToSleepFor_ms = 64;   intervalIndex = 2;
      } else if ( remainingSleep_ms >= 32 ) {
        aboutToSleepFor_ms = 32;   intervalIndex = 1;
      } else if ( remainingSleep_ms >= 16 ) {
        aboutToSleepFor_ms = 16;   intervalIndex = 0;
      } 
    
      setup_watchdog ( intervalIndex );  // timeout interval index
      
      #if MAINTAIN_SLEEP_LOG
    		// sleepVector_code [ sleepVectorIndex ] = intervalIndex;
    		// sleepVectorIndex++;
    		programmedSleepsTotal_ms += aboutToSleepFor_ms;
    	#endif
    
    
      // **************************** go to bed *********************************

      /* 
        what really happens here?
          WDT times out
          f_wdt is set to 1
          reset to 0
          stuff happens
          system sleeps again
      */

      if ( TESTING && Serial ) {
        Serial.print ( "Entering sleep mode with " );
        	Serial.print ( remainingSleep_ms );
        	Serial.println ( " ms remaining to sleep" );
        delay ( 10 );
      }
      
      // ***************** prepare to sleep, go to sleep
    
      digitalWrite ( pdLED, 0 );
      pinMode ( pdLED, INPUT );      // set all used ports to input to save power
      digitalWrite ( pdStatusLED, 0 );
      pinMode ( pdStatusLED, INPUT );

      system_sleep();                  // we come back to the next line when we awaken

      pinMode ( pdLED, OUTPUT );     // reset set all ports back to pre-sleep states
      digitalWrite ( pdLED, 1 );
      pinMode ( pdStatusLED, OUTPUT );
      digitalWrite ( pdStatusLED, 0 );
    
      // ***************** now we're awake again
    
      if ( TESTING && Serial ) {
        Serial.println ( "Waking back up");
        delay ( 10 );
      }
    
      remainingSleep_ms -= aboutToSleepFor_ms;
      
    }
    
  }
  
  

  //****************************************************************

  /*
    Hmm. Not using timer register 2, but the watchdog timer
  */
  
  void setup_watchdog ( int intervalIndex ) {

  // intervalIndex   time    bb (binary) before setting WDCE
  // 0               16ms           0000 0000
  // 1               32ms           0000 0001
  // 2               64ms           0000 0010
  // 3              128ms           0000 0011
  // 4              250ms           0000 0100
  // 5              500ms           0000 0101
  // 6                1s            0000 0110
  // 7                2s            0000 0111
  // 8                4s            0010 0000
  // 9                8s            0010 0001

    byte bb;
    if ( intervalIndex < 0 ) intervalIndex = 0;
    if ( intervalIndex > 9 ) intervalIndex = 9;
    bb = intervalIndex & 7;
    if ( intervalIndex > 7 ) bb |= ( 1 << 5 );
    // WDTCSR is the watchdog timer control register
    // WDCE is the bit number (4) of the watchdog clock enable in WDTCSR
    bb |= ( 1 << WDCE );
    if ( Serial && VERBOSE >= 20 ) {
      Serial.print ( "bb: 0x" ); 
      if ( bb < 0x10 ) Serial.println ( "0" );
      Serial.println ( bb, HEX );
    }

    // MCUSR is the MCU (main control unit?) status register
    // WDRF is the bit number (3) of the watchdog system reset flag in the MCUSR
    MCUSR &= ~ ( 1 << WDRF );  // make sure the watchdog system reset flag is off
  
    // start timed sequence
  
    // WDE is the bit number (3) of the watchdog system reset enable in WDTCSR
    // turn on the watchdog system reset enable and the watchdog clock enable bits
    WDTCSR |= ( 1 << WDCE ) | ( 1 << WDE );
    // set new watchdog timeout value
    WDTCSR = bb;
    // WDIE is the bit number (6) of the watchdog interrupt enable in WDTCSR
    WDTCSR |= _BV ( WDIE );  // then turn on 

  }

  //****************************************************************  

  void system_sleep() {

  // set system into the sleep state 
  // system wakes up when wtchdog is timed out

    cbi ( ADCSRA, ADEN );                    // switch Analog to Digital converter OFF

    set_sleep_mode ( SLEEP_MODE_PWR_DOWN );  // sleep mode is set here
    sleep_enable ();

    sleep_mode ();                           // System sleeps here

    sleep_disable ();                        // System continues execution here when watchdog timed out 
    sbi ( ADCSRA, ADEN );                    // switch Analog to Digital converter back ON

  }

  //****************************************************************
  
  ISR ( WDT_vect ) {

    // Watchdog Interrupt Service; is executed when watchdog times out
    // wakeupsSinceDoingSomething++;
  
  }

#endif

//****************************************************************
//****************************************************************
//****************************************************************

void updateIMU ( ) {

	if (myIMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01) {  
		myIMU.readAccelData(myIMU.accelCount);  // Read the x/y/z adc values
		myIMU.getAres();

		// Now we'll calculate the accleration value into actual g's
		// This depends on scale being set
		myIMU.ax = (float)myIMU.accelCount[0]*myIMU.aRes; // - accelBias[0];
		myIMU.ay = (float)myIMU.accelCount[1]*myIMU.aRes; // - accelBias[1];
		myIMU.az = (float)myIMU.accelCount[2]*myIMU.aRes; // - accelBias[2];

		myIMU.readGyroData(myIMU.gyroCount);  // Read the x/y/z adc values
		myIMU.getGres();

		// Calculate the gyro value into actual degrees per second
		// This depends on scale being set
		myIMU.gx = (float)myIMU.gyroCount[0]*myIMU.gRes;
		myIMU.gy = (float)myIMU.gyroCount[1]*myIMU.gRes;
		myIMU.gz = (float)myIMU.gyroCount[2]*myIMU.gRes;

		myIMU.readMagData(myIMU.magCount);  // Read the x/y/z adc values
		myIMU.getMres();
		// User environmental x-axis correction in milliGauss, should be
		// automatically calculated
		myIMU.magBias[0] = +470.;
		// User environmental x-axis correction in milliGauss TODO axis??
		myIMU.magBias[1] = +120.;
		// User environmental x-axis correction in milliGauss
		myIMU.magBias[2] = +125.;

		// Calculate the magnetometer values in milliGauss
		// Include factory calibration per data sheet and user environmental corrections
		// Get actual magnetometer value, this depends on scale being set
		myIMU.mx = (float)myIMU.magCount[0]*myIMU.mRes*myIMU.magCalibration[0] -
							 myIMU.magBias[0];
		myIMU.my = (float)myIMU.magCount[1]*myIMU.mRes*myIMU.magCalibration[1] -
							 myIMU.magBias[1];
		myIMU.mz = (float)myIMU.magCount[2]*myIMU.mRes*myIMU.magCalibration[2] -
							 myIMU.magBias[2];
	} // if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)

	// Must be called before updating quaternions!
	myIMU.updateTime();
	/*
		Sensors x (y)-axis of the accelerometer is aligned with the y (x)-axis of
		the magnetometer; the magnetometer z-axis (+ down) is opposite to z-axis
		(+ up) of accelerometer and gyro! We have to make some allowance for this
		orientationmismatch in feeding the output to the quaternion filter. For the
		MPU-9250, we have chosen a magnetic rotation that keeps the sensor forward
		along the x-axis just like in the LSM9DS0 sensor. This rotation can be
		modified to allow any convenient orientation convention. This is ok by
		aircraft orientation standards! Pass gyro rate as rad/s
	*/
	//  MadgwickQuaternionUpdate(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f,  my,  mx, mz);
	MahonyQuaternionUpdate(myIMU.ax, myIMU.ay, myIMU.az, myIMU.gx*DEG_TO_RAD,
												 myIMU.gy*DEG_TO_RAD, myIMU.gz*DEG_TO_RAD, myIMU.my,
												 myIMU.mx, myIMU.mz, myIMU.deltat);

	if ( VERBOSE >= 10 ) {
		Serial.print("        ax = "); Serial.print((int)1000*myIMU.ax);
		Serial.print(" ay = "); Serial.print((int)1000*myIMU.ay);
		Serial.print(" az = "); Serial.print((int)1000*myIMU.az);
		Serial.println(" mg");

		Serial.print("        gx = "); Serial.print( myIMU.gx, 2);
		Serial.print(" gy = "); Serial.print( myIMU.gy, 2);
		Serial.print(" gz = "); Serial.print( myIMU.gz, 2);
		Serial.println(" deg/s");

		Serial.print("        mx = "); Serial.print( (int)myIMU.mx );
		Serial.print(" my = "); Serial.print( (int)myIMU.my );
		Serial.print(" mz = "); Serial.print( (int)myIMU.mz );
		Serial.println(" mG");

		Serial.print("        q0 = "); Serial.print(*getQ());
		Serial.print(" qx = "); Serial.print(*(getQ() + 1));
		Serial.print(" qy = "); Serial.print(*(getQ() + 2));
		Serial.print(" qz = "); Serial.println(*(getQ() + 3));
	}

	/*
		Define output variables from updated quaternion---these are Tait-Bryan
		angles, commonly used in aircraft orientation. In this coordinate system,
		the positive z-axis is down toward Earth. Yaw is the angle between Sensor
		x-axis and Earth magnetic North (or true North if corrected for local
		declination, looking down on the sensor positive yaw is counterclockwise.
		Pitch is angle between sensor x-axis and Earth ground plane, toward the
		Earth is positive, up toward the sky is negative. Roll is angle between
		sensor y-axis and Earth ground plane, y-axis up is positive roll. These
		arise from the definition of the homogeneous rotation matrix constructed
		from quaternions. Tait-Bryan angles as well as Euler angles are
		non-commutative; that is, the get the correct orientation the rotations
		must be applied in the correct order which for this configuration is yaw,
		pitch, and then roll.
		For more see
		http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
		which has additional links.
	*/
	
	myIMU.yaw   = atan2(2.0f * (*(getQ()+1) * *(getQ()+2) + *getQ() *
								*(getQ()+3)), *getQ() * *getQ() + *(getQ()+1) * *(getQ()+1)
								- *(getQ()+2) * *(getQ()+2) - *(getQ()+3) * *(getQ()+3));
	myIMU.pitch = -asin(2.0f * (*(getQ()+1) * *(getQ()+3) - *getQ() *
								*(getQ()+2)));
	myIMU.roll  = atan2(2.0f * (*getQ() * *(getQ()+1) + *(getQ()+2) *
								*(getQ()+3)), *getQ() * *getQ() - *(getQ()+1) * *(getQ()+1)
								- *(getQ()+2) * *(getQ()+2) + *(getQ()+3) * *(getQ()+3));
	myIMU.pitch *= RAD_TO_DEG;
	myIMU.yaw   *= RAD_TO_DEG;
	
	// Declination of SparkFun Electronics (40°05'26.6"N 105°11'05.9"W) is
	//    8° 30' E  ± 0° 21' (or 8.5°) on 2016-07-19
	// - http://www.ngdc.noaa.gov/geomag-web/#declination
	
	
	/*
		My solar panel is at 42°32'04.7"N 72°32'39.2"W = 42.534646, -72.544212 
		On 2017-12-14, declination is 14.05° W  ± 0.36°  
			changing by  0.05° E per year 
	
		Date 	2017-12-14 
		Declination ( + E  | - W ) 	   -14.0543° 	   0.0514°/yr
		Inclination ( + D  | - U )     	67.5665°	  -0.0943°/yr 
		Horizontal Intensity        19,955.9 nT     40.5 nT/yr
		North Comp (+ N  | - S) 	 	19,358.6 nT   	43.6 nT/yr
		East Comp (+ E  | - W) 		  -4,846.1 nT	     7.5 nT/yr
		Vertical Comp (+ D  | - U) 	48,336.4 nT  	-127.5 nT/yr
		Total Field                 52,293.9 nT 	-102.4 nT/yr
		
		Device raw yaw shows +36deg when it's pointed magN, though...
	*/
	// myIMU.yaw   -= 8.5;
	myIMU.yaw   -= -14.0543;  // cbm
	myIMU.roll  *= RAD_TO_DEG;
	
	myIMU.tempCount = myIMU.readTempData();  // Read the adc values
	// Temperature in degrees Centigrade
	myIMU.temperature = ( (float) myIMU.tempCount ) / 333.87 + 21.0;

	if ( VERBOSE >= 6 ) {
		Serial.print("    Yaw, Pitch, Roll: ");
		Serial.print(myIMU.yaw, 2);
		Serial.print(", ");
		Serial.print(myIMU.pitch, 2);
		Serial.print(", ");
		Serial.println(myIMU.roll, 2);

		Serial.print("    rate = ");
		Serial.print((float)myIMU.sumCount/myIMU.sum, 2);
		Serial.println(" Hz");
		
		// Print temperature in degrees Centigrade
		Serial.print("    Temperature is ");  Serial.print(myIMU.temperature, 1);
		Serial.println(" degrees C");
	}

	/*
		With these settings the filter is updating at a ~145 Hz rate using the
		Madgwick scheme and >200 Hz using the Mahony scheme even though the
		display refreshes at only 2 Hz. The filter update rate is determined
		mostly by the mathematical steps in the respective algorithms, the
		processor speed (8 MHz for the 3.3V Pro Mini), and the magnetometer ODR:
		an ODR of 10 Hz for the magnetometer produce the above rates, maximum
		magnetometer ODR of 100 Hz produces filter update rates of 36 - 145 and
		~38 Hz for the Madgwick and Mahony schemes, respectively. This is
		presumably because the magnetometer read takes longer than the gyro or
		accelerometer reads. This filter update rate should be fast enough to
		maintain accurate platform orientation for stabilization control of a
		fast-moving robot or quadcopter. Compare to the update rate of 200 Hz
		produced by the on-board Digital Motion Processor of Invensense's MPU6050
		6 DoF and MPU9150 9DoF sensors. The 3.3 V 8 MHz Pro Mini is doing pretty
		well!
	*/

	myIMU.count = millis();
	myIMU.sumCount = 0;
	myIMU.sum = 0;


}

//****************************************************************
  
void transmitData () {
	/*
		int16_t accel [ 3 ];    // raw values for x, y, z
		lsm303dlh.getAccel ( accel );
		int16_t mag [ 3 ];    // raw values for x, y, z
		lsm303dlh.getMag ( mag );
		float heading;
		lsm303dlh.getHeading ( &heading );
		float tiltMag, tiltAccel;
		lsm303dlh.getTiltHeading ( &tiltMag, &tiltAccel );
	*/

	
	// build packet
	// LoRa packet size is one byte, so packet size limit is 255
	const int packetBufLen = 256;
	static char packetBuf [ packetBufLen ];
	const int pBufSize = 10;
	char pBuf [ pBufSize ];
		
	/*
		JSON structure example:
			 [ 
			 	{
    			"id": "5836d5e1.2046dc",
					"type": "inject",
					"z": "e4c24568.84a368",
					"name": "",
					"topic": "<AppEUI>\/devices\/<DevEUI>\/down",
					"wires": [
						[
							"ec45fdbf.58cce8"
						]
					]
				},
				...
			]

	*/
	
	#if 1
	
	/*
		{
			"devID":"LoRa32u4 1",
			"packetSN":2023,
			"progID":{
				"progName":"solar_tracker_LoRa_tracker",
				"version":"2.2.14",
				"verDate":"2017-12-27"
			 },
			"vSupp":3.819355,
			"tChip":0.929314,
			"tilt":{
				"roll":0.239527,
				"pitch":-0.882794
			},
			"heading":192.6785
		}

		{
			"devID":"LoRa32u4 1",
			"packetSN":2023,
			"progID":{
				"progName":"solar_tracker_LoRa_tracker",
				"version":"2.2.14",
				"verDate":"2017-12-27"
			 },
			"vSupp":3.819355,
			"tChip":0.929314,
			"accelVector":[1.23,2.34,3.45],
			"heading":192.6785
		}
	*/
	
		const size_t jsonBufferSize = 
			// 	JSON_ARRAY_SIZE(15)        // sleeps
			+ JSON_ARRAY_SIZE( 3)        // accel
			+ JSON_ARRAY_SIZE( 3)        // mag
			// + JSON_OBJECT_SIZE(2)        // tilt
			+ JSON_OBJECT_SIZE(3)        // progID
			+ JSON_OBJECT_SIZE(7);       // top level
		DynamicJsonBuffer jsonBuffer(jsonBufferSize);
		
		JsonObject& root = jsonBuffer.createObject ();
		root [ "devID" ]      = uniqueID;
		root [ "packetSN" ]   = packetNum;
		
		JsonObject& progID = root.createNestedObject ( "progID" );
		progID [ "name" ] = PROGNAME;
		progID [ "ver" ]  = VERSION;
		progID [ "verDate" ]  = VERDATE;
		
		root [ "vSupp" ]      = vSupp;
		root [ "tChip" ]      = myIMU.temperature;
		
		// JsonObject& tilt = root.createNestedObject ( "tilt" );
		// tilt [ "roll" ]       = myIMU.roll;
		// tilt [ "pitch" ]      = myIMU.pitch;
		
		JsonArray& accelVector = root.createNestedArray ( "accel" );
		accelVector.add ( myIMU.ax );
		accelVector.add ( myIMU.ay );
		accelVector.add ( myIMU.az );
		
		if ( 1 ) {
			JsonArray& magVector = root.createNestedArray ( "mag" );
			magVector.add ( myIMU.mx );
			magVector.add ( myIMU.my );
			magVector.add ( myIMU.mz );
		}
		
		root [ "heading" ]    = myIMU.yaw;
		
		#if MAINTAIN_SLEEP_LOG
			// JsonArray& sleepVector = root.createNestedArray ( "sleepVector" );
			// for ( int i = 0; i < sleepVectorIndex; i++ )
			// 	sleepVector.add ( sleepVector_code [ i ]);
			root [ "sleepsTotal_ms" ] = programmedSleepsTotal_ms;
		#endif
		
		int len = root.printTo ( packetBuf, packetBufLen );
		// Serial.print ( "packet buf len: " ); Serial.println ( len );
		// delay ( 10 );
		// while ( 1 );

	#elif 0
	
		packetBuf [ 0 ] = '\0';  // effectively clears the (null-terminated) string
		
		strncat ( packetBuf, "{ ",                            packetBufLen - strlen(packetBuf) );
		
		strncat ( packetBuf, "\"devID\": \"",                 packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, uniqueID,                        packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, "\", ",                          packetBufLen - strlen(packetBuf) );
	
		strncat ( packetBuf, "\"packetSN\": ",                packetBufLen - strlen(packetBuf) );
		itoa ( packetNum, pBuf, 10 );
		strncat ( packetBuf, pBuf,                            packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, ", ",                            packetBufLen - strlen(packetBuf) );
	
		strncat ( packetBuf, "\"progID\": { ",                packetBufLen - strlen(packetBuf) );
		// sometimes the string starts over after the text "progName"
		// e.g. starts with '": "solar_tracker_LoRa_tracker", '
		// and omits '{ "devID": "LoRa32u4 1", "packetID": "27", "progID": { "progName"'
		strncat ( packetBuf, "\"progName\": \"",              packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, PROGNAME,                        packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, "\", ",                          packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, "\"version\": \"",               packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, VERSION,                         packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, "\", ",                          packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, "\"verDate\": \"",               packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, VERDATE,                         packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, "\" }, ",                        packetBufLen - strlen(packetBuf) );
	
		strncat ( packetBuf, "\"vSupp\": ",                   packetBufLen - strlen(packetBuf) );
		dtostrf ( vSupp, 4, 2, pBuf );
		strncat ( packetBuf, pBuf,                            packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, ", ",                            packetBufLen - strlen(packetBuf) );
	
		strncat ( packetBuf, "\"tChip\": ",                   packetBufLen - strlen(packetBuf) );
		dtostrf ( myIMU.temperature, 4, 1, pBuf );  
		strncat ( packetBuf, pBuf,                            packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, ", ",                            packetBufLen - strlen(packetBuf) );
	
		strncat ( packetBuf, "\"tilt\": {",                   packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, "\"roll\": ",                    packetBufLen - strlen(packetBuf) );
		dtostrf ( myIMU.roll, 4, 2, pBuf );
		strncat ( packetBuf, pBuf,                            packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, ", ",                            packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, "\"pitch\": ",                   packetBufLen - strlen(packetBuf) );
		dtostrf ( myIMU.pitch, 4, 2, pBuf );
		strncat ( packetBuf, pBuf,                            packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, " }, ",                          packetBufLen - strlen(packetBuf) );
	
		strncat ( packetBuf, "\"heading\": ",                 packetBufLen - strlen(packetBuf) );
		dtostrf ( myIMU.yaw, 5, 1, pBuf );
		strncat ( packetBuf, pBuf,                            packetBufLen - strlen(packetBuf) );
		strncat ( packetBuf, " ",                             packetBufLen - strlen(packetBuf) );
	
		strncat ( packetBuf, "}",                             packetBufLen - strlen(packetBuf) );
	
	#else
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		snprintf ( packetBuf, packetBufLen, "%d %s %s; vSupp: ", 
			packetNum, uniqueID, VERSION );
			
		dtostrf ( vSupp, 4, 2, pBuf );
		strncat ( packetBuf, pBuf,                 packetBufLen - strlen(packetBuf) );

		strncat ( packetBuf, "; (roll, pitch): (", packetBufLen - strlen(packetBuf) );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		dtostrf ( myIMU.roll, 4, 1, pBuf );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		strncat ( packetBuf, pBuf,                 packetBufLen - strlen(packetBuf) );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		strncat ( packetBuf, ", ",                 packetBufLen - strlen(packetBuf) );
		dtostrf ( myIMU.pitch, 4, 1, pBuf );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		strncat ( packetBuf, pBuf,                 packetBufLen - strlen(packetBuf) );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		strncat ( packetBuf, "); heading: ",       packetBufLen - strlen(packetBuf) );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		dtostrf ( myIMU.yaw, 4, 1, pBuf );
		strncat ( packetBuf, pBuf,                 packetBufLen - strlen(packetBuf) );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		strncat ( packetBuf, "); temp: ",          packetBufLen - strlen(packetBuf) );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
		dtostrf ( myIMU.temperature, 4, 1, pBuf );
		strncat ( packetBuf, pBuf,                 packetBufLen - strlen(packetBuf) );
		// Serial.print ( strlen(packetBuf) ); Serial.print ( ": " ); Serial.println ( packetBuf );
	#endif
		
	if ( Serial ) {
		Serial.println ( packetBuf ); 
		delay ( 10 );
	}
	
	// send packet
	LoRa.beginPacket();
	LoRa.print ( packetBuf );
	LoRa.endPacket();

	packetNum++;
}

//****************************************************************

void blinkStatus ( int n, unsigned long rate_ms ) {
	#if TESTING
		const unsigned long pauseDuration_ms = 1000UL;
		static unsigned long lastStateChangeAt_ms = 0UL;
		static int blinkNo = 0;
		static int state = 0;
		static int nBlinksDesired = 0;
	
		if ( n >= 0 ) nBlinksDesired = n;
	
		if ( state == 0 
				&& ( ( millis() - lastStateChangeAt_ms ) < ( rate_ms + pauseDuration_ms ) ) ) {
			return;
		}
	
		if ( ( millis() - lastStateChangeAt_ms ) > rate_ms ) {
			state++;
			if ( state >= nBlinksDesired * 2 + 1 ) state = 0;
			digitalWrite ( pdStatusLED, state % 2 );
			lastStateChangeAt_ms = millis();
		}
	#endif
}
