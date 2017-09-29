/*
  test driver for mini stepper motor MPJA 30032MS 4-pole
*/

#define BAUDRATE 115200

#define paPot 5

#define pdPhaseA 8
#define pdPhaseB 9
#define pdPhaseC 10
#define pdPhaseD 11

void setup () {
  Serial.begin ( BAUDRATE );
  
  pinMode ( pdPhaseA, OUTPUT ); digitalWrite ( pdPhaseA, 0 );
  pinMode ( pdPhaseB, OUTPUT ); digitalWrite ( pdPhaseB, 0 );
  pinMode ( pdPhaseC, OUTPUT ); digitalWrite ( pdPhaseC, 0 );
  pinMode ( pdPhaseD, OUTPUT ); digitalWrite ( pdPhaseD, 0 );
  
}

void loop () {
  int pot, direction, nextPhase;
  long delay_us;
  static int phase = 0;
  static long location = 0;
  static unsigned long lastPrintAt_ms = 0;
  
  pot = analogRead ( paPot );
  if ( pot == 512 ) {
    direction = 0;
  } else if ( pot > 512 ) {
    direction = 1;
  } else {
    direction = -1;
  }
  delay_us = map ( exp ( - abs ( 512.0 - pot ) / 512.0 ) * 1000, 368, 1000, 1000, 500000 );
  
  if ( ( millis () - lastPrintAt_ms ) > 500 ) {
    Serial.print ( location ); Serial.print ( "; " );
    Serial.print ( delay_us ); Serial.print ( "; " );
    Serial.print ( phase ); Serial.print ( "; " );
    Serial.print ( pot ); Serial.print ( "; " );
    Serial.print ( direction ); Serial.println ();
    lastPrintAt_ms = millis();
  }
  
  nextPhase = ( phase + direction + 4 ) % 4;
  digitalWrite ( nextPhase + pdPhaseA, 1 );
  digitalWrite ( phase     + pdPhaseA, 0 );
  phase = nextPhase;
  
  location += direction;
  
  // delay should go from 100ms at (next to zero) to 1ms at end
  delayMicroseconds ( delay_us );
  
}