/*

Run EMMA with RS232 commands

*/

#define BAUDRATE 115200
#define pdLED 13
#define paDUR 0
#define paSPD 1

// values 0 - 1000; motor won't move much below min speed
#define MAXSPEED 800
#define MINSPEED 120
#define MAX_TEST_DURATION_ms 60000L

#define LOOP_DELAY_ms 100

#define ANALOG_READ_TOLERANCE_cts 5

void setup () {

  Serial.begin ( BAUDRATE );
  pinMode ( pdLED, OUTPUT );
  digitalWrite ( pdLED, 0 );
  
  // set timeout value (ms)
  //   robot will stop automatically if no commands come for this interval
  Serial.println ( "^rwd 1000" );
  // mixing mode 0 (separate) or 1 (ch 1 speed, ch2 steering)
  Serial.println ( "^mxmd 0" );
  Serial.println ( "%eesav" );
  

}

void loop () {
  
  int s1, s2;
  int maxSpeedAboveMin;
  unsigned long test_duration_ms;

  static int aDUR_old = -10, aSPD_old = -10, aDUR = 0, aSPD = 0;
  static unsigned long lastChangeAt_ms = millis();
  
  if ( ( abs ( aDUR - aDUR_old ) > ANALOG_READ_TOLERANCE_cts ) 
    || ( abs ( aSPD - aSPD_old ) > ANALOG_READ_TOLERANCE_cts ) ) {
    lastChangeAt_ms = millis();
    aDUR_old = aDUR;
    aSPD_old = aSPD;
  }
  while ( ( millis() - lastChangeAt_ms ) < 5000 ) {
    aDUR = analogRead ( paDUR );
    aSPD = analogRead ( paSPD );
    test_duration_ms = MAX_TEST_DURATION_ms * (unsigned long) aDUR / 1024L;
    Serial.print ( "duration " ); Serial.print ( aDUR ); 
      Serial.print ( " / " ); Serial.print ( test_duration_ms );
    maxSpeedAboveMin = int ( ( MAXSPEED - MINSPEED ) * (unsigned long) aSPD / 1024L );
    Serial.print ( "; max spd above min " ); Serial.print ( aSPD ); 
      Serial.print ( " / " ); Serial.println ( maxSpeedAboveMin );
  }
  
  /*
  
    It may be that the channels used in the !g GO command go to the mixing mode,
    with channel 1 being speed and channel 2 being steering.
    
    Build Arduino board with RS-232 output, powered by DB-15 +5VDC. Make this power
    available on the ribbon bus. Ribbon connectors and ribbon-to-PC-board connectors.
    *P  P  P  Sp  MOSI
     G  G  G  Sp  MISO
    
    Set mixing mode to 1 using RoboRun or ^MXMD 1
                    or 0 (separate)
    
    Need to swap the leads of one motor.
    
  */

  static unsigned long loopBeganAt_ms;
  
  // run robot for a little while
  
  // this block will run straight and slow
  // ramp up
  loopBeganAt_ms = millis();
  while ( ( millis () - loopBeganAt_ms ) * 2 < test_duration_ms ) {
    s1 = ( ( millis () - loopBeganAt_ms ) * 2 * maxSpeedAboveMin / test_duration_ms ) + MINSPEED;
    Serial.print ( "!g 1 "); Serial.println ( s1 );  // motor 1 is facing the other way...
    Serial.print ( "!g 2 "); Serial.println ( s1 );
    digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    delay (LOOP_DELAY_ms);
  }
  // ramp down
  loopBeganAt_ms = millis();
  while ( ( millis () - loopBeganAt_ms ) * 2 < test_duration_ms ) {
    s1 = ( ( millis () - loopBeganAt_ms ) * 2 * maxSpeedAboveMin / test_duration_ms ) + MINSPEED;
    s2 = maxSpeedAboveMin - ( s1 - MINSPEED ) + MINSPEED;
    Serial.print ( "!g 1 "); Serial.println ( s2 );  // motor 1 is facing the other way...
    Serial.print ( "!g 2 "); Serial.println ( s2 );
    digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    delay (LOOP_DELAY_ms);
  }
  
  // this block will do an s-curve
  // while ( ( millis () - loopBeganAt_ms ) < test_duration_ms ) {
    // s1 = ( ( millis () - loopBeganAt_ms ) * maxSpeedAboveMin / test_duration_ms ) + MINSPEED;
    // s2 = maxSpeedAboveMin - ( s1 - MINSPEED ) + MINSPEED;
    // Serial.print ( "!g 1 "); Serial.println ( s1 );  // motor 1 is facing the other way...
    // Serial.print ( "!g 2 "); Serial.println ( s2 );
    // digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    // delay (LOOP_DELAY_ms);
  // }
  
  // and now stop forever
  for (;;) {
    delay ( 1000 );
  }

}