#define BAUDRATE 115200

#define resistance 6.78e3

// using PORT manipulation for pins 6 and 7 which are connected to the clock leads

int pulse_duration_ms = 15;  // 5 works; 15 is better; pretty sensitive; most values don't work
int cycleTime_ms = 1000;  

void setup () {
  Serial.begin ( BAUDRATE );
  
  DDRD |= B11000000;   // set direction of pins 6 and 7 as output
  PORTD &= B00111111;  // set both pins low
  
  
  Serial.println ( "Ready" );
  
}

void loop () {
  static unsigned long previous_cycle_at_ms = millis();
  unsigned long delay_ms;
  static int state = 2;
  
  // pulse_duration_ms = analogRead ( A0 ) / 50;  // 0 to 20
  // Serial.print ( "pulse: " ); Serial.print ( pulse_duration_ms ); Serial.println ( " ms " );
  
  // cycleTime_ms = analogRead ( A0 );
  // if ( cycleTime_ms < 256 ) {
    // cycleTime_ms = ( 16 - int ( cycleTime_ms / 16 ) ) * 1000;
  // } else if ( cycleTime_ms < 768 ) {
    // cycleTime_ms = 1000;
  // } else {
    // cycleTime_ms = 1000 / ( ( ( cycleTime_ms - 768 ) / 16 ) + 1 );
  // }
  
  // Serial.print ( "delay: " ); Serial.print ( delay_ms ); Serial.println ( " ms " );
  // delay_ms = ( cycleTime_ms + previous_cycle_at_ms ) - millis();
  // if ( delay_ms > 30000L ) {
    // delay_ms = 10;  // some arbitrary non-huge delay
    // Serial.println ( " delay problem " );
  // }
  delay ( cycleTime_ms - 17 );  // ( delay_ms - 2 );
  delayMicroseconds ( ( 626 - 48 + 3 ) / 2 );
  previous_cycle_at_ms = millis();    // change states with a pulse

  if ( state == 1 ) {
    PORTD |= B01000000;
    state = 2;
  } else {
    PORTD |= B10000000;
    state = 1;
  }
  
  delay ( pulse_duration_ms );
  
  // turn both pins back off
  PORTD &= B00111111;
  
}