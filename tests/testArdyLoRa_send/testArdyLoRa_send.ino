#define PROGNAME  "testArdyLoRa_send"
#define VERSION   "0.1.0"
#define VERDATE   "2017-11-30"

/*
  <https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test>
  <http://www.diymalls.com/lora32u4-lorawan?product_id=84>
  <https://github.com/hj91/nodemcu-esp8266/blob/master/lora_sender/lora_sender.ino>
  
  Chip is ATMEGA32U4
  choose board Arduino Mega ADK? No. Adafruit Feather 32u4, I think.
  
  Note to DIYMall 2017-11-28 11:30
  Listing had link to information, but link
  <https://drive.google.com/open?id=0B8DSGdAr8_31RkQ0UGo2Nzh1YXM> is broken -
  404 from Google. I reported this via the Amazon page. Having spent hours
  searching, found the info I needed - call it an Adafruit Feather to program
  it from the Arduino IDE. But neither board will program, but differently!
  When I try to program it without pushing the reset, one goes into reset
  (throbbing white LED) but the other doesn't. But neither programs
  successfully, even trying the supplied Arduino demo program "Ascii chart".
  The fact that these items are both broken, but differently, indicates that
  the quality is really poor. Can I return them?
*/

#include <SPI.h>
#include <LoRa.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define BAUDRATE 115200
#define VERBOSE 12
#define SLEEP_ENABLE true

#if SLEEP_ENABLE
  #ifndef cbi
    #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
  #endif
  #ifndef sbi
    #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
  #endif
#endif

#define LORA_FREQ_BAND 433E6
// LoRa32u4 ports
const int pdLoRa_SS  =  8;    // CS chip select
const int pdLoRa_RST =  4;    // reset
const int pdLoRa_DI0 =  7;    // IRQ interrupt request

const int pdAwake    = 13;       // LED
const int pdSleep    = 17;
// const int paBat   = 13;       // reads half the battery voltage?
  // in centivolts / count, perhaps

int packetNum = 0;

const long intervalBetweenTransmits_ms = 5000UL;

void setup () {

  pinMode ( pdSleep, INPUT_PULLUP );
  pinMode ( pdLoRa_RST, OUTPUT ); digitalWrite ( pdLoRa_RST, 1 ); 
  
  #if ! SLEEP_ENABLE
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis () < 2000 );
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
    
  if ( Serial ) Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 100 );
  
  #if SLEEP_ENABLE
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

  static unsigned long lastXMitAt_ms = 0UL;
  
  volatile int nSecs = 0;
  
  int counts = 0;
  
  // lastXMitAt_ms will be zero first time through whether or not we sleep
  
  if ( ! lastXMitAt_ms 
      || ( millis() - lastXMitAt_ms ) > intervalBetweenTransmits_ms )  {
  
    if ( Serial ) {
      Serial.println ( "Packet number: " + String ( packetNum ) );
      delay ( 10 );
    }
    
    // counts = analogRead ( paBat );
    
    // build packet
    const int packetBufLen = 20;
    static char packetBuf [ packetBufLen ];
    snprintf ( packetBuf, packetBufLen, "%d; bat: %d; Ardy",
      packetNum, counts );
      
    // send packet
    if ( Serial ) {
      Serial.println ( packetBuf ); 
      delay ( 10 );
    }
    LoRa.beginPacket();
    LoRa.print ( packetBuf );
    LoRa.endPacket();
  
    packetNum++;
    lastXMitAt_ms = millis();
    
  }

  #if SLEEP_ENABLE

    if ( digitalRead ( pdSleep ) ) {
      sleepFor_ms ( intervalBetweenTransmits_ms );
      // done sleeping
      // do the stuff
      lastXMitAt_ms = 0UL;
    }
  
  #endif
      
}



//****************************************************************
//****************************************************************
//****************************************************************  

#if SLEEP_ENABLE

  void sleepFor_ms ( long remainingSleep_ms ) {
  
    while ( remainingSleep_ms > 0 ) {
  
      /*
        either sleeps for as large a chunk as possible of the remaining sleep period
        or returns false, indicating that we're done sleeping for now
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
    
    
    
      // **************************** go to bed *********************************

      /* 
        what really happens here?
          WDT times out
          f_wdt is set to 1
          reset to 0
          stuff happens
          system sleeps again
      */


      if ( Serial ) {
        Serial.print ( "Process took " ); Serial.print ( millis() ); Serial.println ( " ms." );
        Serial.println ( "Entering sleep mode" );
        delay ( 10 );
      }
      
      // ***************** prepare to sleep, go to sleep
    
      digitalWrite ( pdAwake, 0 );
      pinMode ( pdAwake, INPUT );      // set all used ports to input to save power

      system_sleep();                  // we come back to the next line when we awaken

      pinMode ( pdAwake, OUTPUT );     // reset set all ports back to pre-sleep states
      digitalWrite ( pdAwake, 1 );
    
      // ***************** now we're awake again
    
      // testing shows it takes precisely 100ms to go to sleep...
      // delay ( 100 );

      if ( Serial ) {
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
    if ( Serial ) {
      Serial.print ( "bb: " ); 
      Serial.println ( int ( bb ) );
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

