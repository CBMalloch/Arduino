/****************************************************************
 * Watchdog Sleep Example 
 * Demonstrate the Watchdog and Sleep Functions
 
 * KHM 2008 / Lab3/  Martin Nawrath nawrath@khm.de
 * Kunsthochschule fuer Medien Koeln
 * Academy of Media Arts Cologne
 
 * Modified by Charles B. Malloch, PhD
 
****************************************************************/

// cbm note: works OK on ATmega168P

#include <avr/sleep.h>
#include <avr/wdt.h>

#define pdAwake 13

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

volatile boolean f_wdt = 1;

void setup(){

  Serial.begin ( 115200 );
  
  pinMode ( pdAwake, OUTPUT ); digitalWrite ( pdAwake, 1 );
  
  Serial.println ( "WDT_sleep" );

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

  setup_watchdog ( 7 );  // arg sets the watchdog timeout interval
  
}

//****************************************************************

void loop(){

  static int nint = 0;

  /* 
    what really happens here?
      WDT times out
      f_wdt is set to 1
      reset to 0
      prints happen
      system sleeps again
      
      as demonstrated by disabling the condition test
      no need to wait for timed-out watchdog - we sleep until it times out!
  */
  if ( 1 || f_wdt == 1 ) {  // wait for timed out watchdog; flag is set when a watchdog timeout occurs
  
    f_wdt = 0;       // reset flag
    
    nint++;
    Serial.print ( millis() ); Serial.print ( ": sleep # " ); Serial.println ( nint );
    // note that each sleep advances the clock only 12684/4894 = 2.6 ms,
    // of which the following delay accounts for 2.0
    
    // at 115200 baud, each character of 8 bits + 1 start bit + 2 stop bits = 11 bits
    // takes 11/115200 = 95.5 us; we are printing about 22 characters, which takes 2.1 ms
    // delay ( 1 ) DOES NOT WORK; delay ( 2 ) seems a little low, but it works OK.
    delay ( 2 );                     // wait for the characters to be sent before sleeping
    
    // ***************** prepare to sleep, go to sleep
    
    digitalWrite ( pdAwake, 0 );
    pinMode ( pdAwake, INPUT );      // set all used ports to input to save power

    system_sleep();                  // we come back to the next line when we awaken

    pinMode ( pdAwake, OUTPUT );     // reset set all ports back to pre-sleep states
    digitalWrite ( pdAwake, 1 );
    
    // ***************** now we're awake again
    

  }

}


//****************************************************************
//****************************************************************
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
  Serial.println ( int ( bb ) );

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

ISR ( WDT_vect ) {

// Watchdog Interrupt Service; is executed when watchdog times out

  f_wdt = 1;  // set global flag
  
}


