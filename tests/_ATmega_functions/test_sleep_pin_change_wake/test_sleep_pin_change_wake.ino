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

#define pdAwake  13
// #define pdWakeUp 12
// kludge - Arduino pin D12 is PCINT4
#define pcintWakeUp 4

/*  pin   PCINT
     D0     16
     D1     17
     D2     18
     D3     19
     D4     20
     D5     21
     D6     22
     D7     23
     D8      0
     D9      1
     D10     2
     D11     3
     D12     4
     D13     5
     A0      8
     A1      9
     A2     10
     A3     11
     A4     12
     A5     13
*/

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

volatile unsigned long n_PCINT0 = 0;
volatile unsigned long n_PCINT1 = 0;
volatile unsigned long n_PCINT2 = 0;

void setup(){

  Serial.begin ( 115200 );
  
  pinMode ( pdAwake, OUTPUT ); digitalWrite ( pdAwake, 1 );
  
  Serial.println ( "sleep_until_pin_change" );

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

  setup_pin_change_interrupt ( pcintWakeUp );  // arg sets the watchdog timeout interval
  
}

//****************************************************************

void loop(){

  static int nint = 0;

  /* 
    what really happens here?
      a pin changes
      n_PCINT is bumped by the interrupt routine
      prints happen
      goes back to sleep
      
    no need to wait for timed-out watchdog - we sleep until it times out!
      as demonstrated by disabling the if
  */
  
  Serial.print ( millis() ); 
  Serial.print ( ": sleep # " ); 
  Serial.print ( n_PCINT0 ); Serial.print ( " / " );
  Serial.print ( n_PCINT1 ); Serial.print ( " / " );
  Serial.print ( n_PCINT2 ); Serial.println ();
  
  // at 115200 baud, each character of 8 bits + 1 start bit + 2 stop bits = 11 bits
  // takes 11/115200 = 95.5 us
  delay ( 4 );                     // wait for the characters to be sent before sleeping
  
  // ***************** prepare to sleep, go to sleep
  
  digitalWrite ( pdAwake, 0 );
  pinMode ( pdAwake, INPUT );      // set all used ports to input to save power

  system_sleep();                  // we come back to the next line when we awaken

  pinMode ( pdAwake, OUTPUT );     // reset set all ports back to pre-sleep states
  digitalWrite ( pdAwake, 1 );
  
  // ***************** now we're awake again
    

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

void setup_external_interrupt ( int senseControl0, int senseControl1 ) {
}

void setup_pin_change_interrupt ( int pin ) {

  if ( pin <  0 ) pin =  0;
  if ( pin > 23 ) pin = 23;

  // PCICR is the pin change interrupt control register
  if ( pin <= 7 ) {
    // PCI0
    PCICR = 1 << PCIE0;
    PCMSK0 |= ( 1 << ( pin % 8 ) );
  } else if ( pin <= 14 ) {
    // PCI1
    PCICR = 1 << PCIE1;
    PCMSK1 |= ( 1 << ( pin % 8 ) );
  } else {
    // PCI2
    PCICR = 1 << PCIE2;
    PCMSK2 |= ( 1 << ( pin % 8 ) );
  }

  // PCIFR is the pin change interrupt flag register
  PCIFR = 0;  // clear the interrupt flags

}

//****************************************************************  

ISR ( PCINT0_vect ) {

// Pin Change Interrupt Service; is executed when an enabled pin changes

  n_PCINT0++;  // set global flag
  
}

ISR ( PCINT1_vect ) {

// Pin Change Interrupt Service; is executed when an enabled pin changes

  n_PCINT1++;  // set global flag
  
}
ISR ( PCINT2_vect ) {

// Pin Change Interrupt Service; is executed when an enabled pin changes

  n_PCINT2++;  // set global flag
  
}

