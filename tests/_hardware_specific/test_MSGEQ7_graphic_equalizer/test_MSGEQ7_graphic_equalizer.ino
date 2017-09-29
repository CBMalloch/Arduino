/*
  test_MSGEQ7_graphic_equalizer.ino
  Charles B. Malloch, PhD
  2015-02-12
  
*/

enum { BAUDRATE = 115200 };
enum { pd_LED = 13 };
enum { pd_STROBE = 10 };
enum { pd_RESET = 9 };
enum { pa_BAND = 0 };

enum { tr_us = 100 + 50 };
enum { trs_us = 72 + 28 };
enum { ts_us = 18 + 10 };
enum { tss_us = 72 + 28 };
enum { to_us = 36 + 14 };

void setup () {

  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 5000 ) );
  pinMode ( pd_RESET, OUTPUT );
  pinMode ( pd_STROBE, OUTPUT );
  pinMode ( pd_LED, OUTPUT );
  
} 

void loop () {
  int counts = 0;
  
  digitalWrite ( pd_LED, millis() & 0x80 );
  
  /*
    Timing diagram in doc is confusing. Looks like RST high, then RST low, then STR high, then read, then STR low.
    Going to try leaving STR high when not in use, and reading while STR is low.
    A third party shows STR is active low.
  */
  
  digitalWrite ( pd_RESET, 1 );
  digitalWrite ( pd_STROBE, 1 );
  // delayMicroseconds ( ts_us );
  // digitalWrite ( pd_STROBE, 0 );
  // delayMicroseconds ( tr_us - ts_us );
  delayMicroseconds ( tr_us );
  digitalWrite ( pd_RESET, 0 );
  delayMicroseconds ( trs_us );
  
  for ( int i = 0; i < 7; i++ ) {
    // digitalWrite ( pd_STROBE, 1 );
    // delayMicroseconds ( ts_us );
    digitalWrite ( pd_STROBE, 0 );
    delayMicroseconds ( to_us );
    
    counts = analogRead ( pa_BAND );
    delayMicroseconds ( tss_us - to_us );
    
    if ( i > 0 ) {
      Serial.print ( " : " );
    }
    Serial.print ( counts );
    
    digitalWrite ( pd_STROBE, 1 );
    delayMicroseconds ( ts_us );

  }
    
  Serial.println ();
  delay ( 10 );
}