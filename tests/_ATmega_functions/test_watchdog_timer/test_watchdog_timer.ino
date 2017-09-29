#define VERSION "1.1.0"
#define VERDATE "2014-03-22"
#define PROGMONIKER "TWD"

/*

  Cloned from Heating system monitor cbm 2010-05-29
  
  Watchdog timer preset value v results in a timeout value of 2^(v-6) seconds, I think,
  so that v = 9 -> 8 sec, 8 -> 4 sec, 7 -> 2 sec, 6 -> 1 sec, 5 -> 1/2 sec, 4 -> 1/4 sec, etc.
  
*/
  
#define VERBOSE 1
#define BAUDRATE 115200

#include <avr/interrupt.h>
#include <avr/wdt.h>  
// see C:\Program Files\arduino-0022\hardware\tools\avr\avr\include\avr\wdt.h

unsigned long loopPeriod;

#define pdLED 13
#define paPOT  1

#define BUFLEN 128
char strBuf [ BUFLEN + 1 ];

void setup() {
  Serial.begin(BAUDRATE);
	snprintf(strBuf, BUFLEN, "\n\n\n%s: Test Watchdog Timer v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);
  
  setAndStartWatchdogTimer ( 9 );
  
}

// commented out 2014-03-22 to try and obtain *hardware* reset rather than this *software* reset
//   which reportedly does not clear the registers
// heh, heh - restarts program, but doesn't hard-reset processor
// ISR(WDT_vect) { 
  // asm("jmp 0");
// };

void loop() {

  unsigned long loopBeganAt;
  
  wdt_reset();  // pet the nice doggy

	int aPOT = analogRead ( paPOT );
  byte scaler;
  scaler = constrain ( aPOT / 100, 0, 9 );
  setAndStartWatchdogTimer ( scaler );
  
  digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
  
  for ( ; ; ) { 
    static int delays[] = { 1, 2, 3, 7, 15, 31, 64, 125, 250, 500 };
    Serial.print ( millis() );
    Serial.print ( " " ); 
    delay ( delays [ scaler ] );
  }
  
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 


// ATmega168 is JUST FINE with all settings of scaler!
// #if defined (__AVR_ATmega168__) ||defined(__AVR_ATmega168P__)
// #error CODE REQUIRES BIGGER PROCESSOR THAN ATMEGA168P
// #endif

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
      
      Valid values from 0 to 9; maybe 8 on ATmega168's
      
      also use 
      
          ISR(WDT_vect) { 
            // heh, heh - restarts program, but doesn't hard-reset processor
            asm("jmp 0");
          };

      and
          wdt_reset();  // pet the nice doggy

  */
  
  Serial.print ( "Scaler: 0x" ); Serial.println ( scaler, HEX );
  
  byte WDTCSR_nextValue = ( 1 << WDIE )                             // bit 6
                        | ( 0 << WDE  )                             // bit 3
                        | ( ( ( scaler >> 3 ) & 0x01 ) << WDP3 )    // bit 5
                        | ( ( ( scaler >> 2 ) & 0x01 ) << WDP2 )    // bit 2
                        | ( ( ( scaler >> 1 ) & 0x01 ) << WDP1 )    // bit 1
                        | ( ( ( scaler >> 0 ) & 0x01 ) << WDP0 );   // bit 0
                                
  Serial.print ( "Next value for WDTCSR: 0x" ); Serial.println ( WDTCSR_nextValue, HEX );
  
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
