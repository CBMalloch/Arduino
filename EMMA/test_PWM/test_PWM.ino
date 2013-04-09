#define BAUDRATE 115200

#define pdPWM0 3
// #define pdBRAKE 4
#define pdPWM1 6
#define paPOT0 0
#define paPOT1 1

#define pdLED 13

#define interPrintPeriod_ms 2000
#define interAnalogReadPeriod_ms 300
// lcm 2000 and 300 is 60000, so every six seconds we print the results of a loop
// *with* analog read; the other two times we print *without*.
// except that these independent things *wander*. So we never get a hit.

int pulseDuration0_us, pulseDuration1_us;

void setup () {

  Serial.begin ( BAUDRATE );
  
  pinMode ( pdPWM0, OUTPUT );
  digitalWrite ( pdPWM0, 0 );
  
  pinMode ( pdPWM1, OUTPUT );
  digitalWrite ( pdPWM1, 0 );
  
  // pinMode ( pdBRAKE, OUTPUT );
  // digitalWrite ( pdBRAKE, 0 );
  
  pinMode ( pdLED, OUTPUT );
  digitalWrite ( pdLED, 0 );
  
}

void loop () {

  static int status = 0;
  static unsigned long lastChangeAt_ms = 0, lastAnalogReadAt_ms = 0;
  static unsigned long loopBeganAt_us = 0;
  
  static unsigned long pulse0BeganAt_us = 0,   pulse1BeganAt_us = 0;
  static unsigned long pulsePeriod0_us = 10000, pulsePeriod1_us = 10000;
  static int status0 = 0, status1 = 0;
    
  if ( ( ! status0 ) && ( ( micros() - pulse0BeganAt_us ) > pulsePeriod0_us ) ) {
    // it's zero and it's time to begin a new cycle
    digitalWrite (pdPWM0, 1);
    status0 = 1;
    pulse0BeganAt_us = micros();
  }
  
  if ( ( ! status1 ) && ( ( micros() - pulse1BeganAt_us ) > pulsePeriod1_us ) ) {
    // it's zero and it's time to begin a new cycle
    digitalWrite (pdPWM1, 1);
    status1 = 1;
    pulse1BeganAt_us = micros();
  }
  
  if ( ( status0 ) && ( ( micros() - pulse0BeganAt_us ) > pulseDuration0_us ) ) {
    // it's one and it's time to begin a new cycle
    digitalWrite (pdPWM0, 0);
    status0 = 0;
  }
  
  if ( ( status1 ) && ( ( micros() - pulse1BeganAt_us ) > pulseDuration1_us ) ) {
    // it's one and it's time to begin a new cycle
    digitalWrite (pdPWM1, 0);
    status1 = 0;
  }
  
  if ( ( millis() - lastAnalogReadAt_ms ) > interAnalogReadPeriod_ms ) {
    // loop time with the next two statements in: 1076; with them out: 430, 
    // so 640 for both, 320 for each
    // so we read only every half-second
    pulseDuration0_us = map (analogRead (paPOT0), 0, 1023, 0, 900) + 1050;  // takes 320us
    pulseDuration1_us = map (analogRead (paPOT1), 0, 1023, 0, 900) + 1050;
    // pulseDuration0_us = analogRead (paPOT0) + 1000;  // takes 225us
    // pulseDuration1_us = analogRead (paPOT1) + 1000;
    lastAnalogReadAt_ms = millis();
  }

  if ( ( millis() - lastChangeAt_ms ) > interPrintPeriod_ms ) {
    Serial.print (" 0: "); Serial.println (pulseDuration0_us);
    Serial.print (" 1: "); Serial.println (pulseDuration1_us);
    Serial.print ( "Loop time: " ); Serial.println ( micros() - loopBeganAt_us );
    Serial.println ();
    status = 1 - status;
    digitalWrite ( pdLED, status );
    // digitalWrite ( pdBRAKE, status );
    lastChangeAt_ms = millis();
    
  }
  loopBeganAt_us = micros();
}
