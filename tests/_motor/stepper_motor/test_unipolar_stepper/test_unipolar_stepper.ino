/*
  test driver for stepper motor 
    
  OOPS - it's unipolar - power through center tap, then 
*/

#define BAUDRATE 115200

// PHASE_SKIP 0 for half-stepping; 1 for full stepping
#define PHASE_SKIP      0
// PHASE_START 0 for one coil at a time, 1 for two coils at a time
#define PHASE_START     0
// INACTIVE_STATE to be the state for inactive coils
//   0 for N-channel MOSFETs on the low side of the coils and power to common
//   1 for NPN 2N3904 controlling the gate of an N-channel MOSFET on the low side
//          adapted from http://www.edutek.ltd.uk/Circuit_Pages/Bridge_Driver_MOSFET.html
#define INACTIVE_STATE  1

#define paPot 5

// the motor is expected to have connections   1A - C - 2A   and  3A - C - 4A
#define pdMotor1A 3
#define pdMotor2A 5
// #define ppMotorEnable12 10

#define pdMotor3A 6
#define pdMotor4A 9
// #define ppMotorEnable34 11

int wires[] = { 
                 pdMotor1A, 
                 pdMotor2A, 
                 pdMotor3A, 
                 pdMotor4A 
              };
int nWires;

int phases[] = { 
                 B00000001,
                 B00000101,
                 B00000100,
                 B00000110,
                 B00000010,
                 B00001010,
                 B00001000,
                 B00001001
               };
int nPhases;

void setup () {
  Serial.begin ( BAUDRATE );
  while (!Serial);
  
  nWires = sizeof ( wires ) / sizeof ( wires [ 0 ] );
  nPhases = sizeof ( phases ) / sizeof ( phases [ 0 ] );
  
  for ( int i = 0; i < nWires; i++ ) {
    pinMode ( wires[i], OUTPUT ); digitalWrite ( wires[i], 0 );
  }
  
  // pinMode ( ppMotorEnable12, OUTPUT ); digitalWrite ( ppMotorEnable12, 1 );
  // pinMode ( ppMotorEnable34, OUTPUT ); digitalWrite ( ppMotorEnable34, 1 );
  
}

void loop () {
  // half-stepping by design - change pair1 
  int pot;
  static int direction;
  static float loops_per_sec = 1.0;
  float command_loops_per_sec;
  unsigned long delay_us;
  static int oldPot = -10;
  static int phase = PHASE_START;
  static long location = 0;
  static unsigned long lastPrintAt_ms = 0;
  
  pot = analogRead ( paPot );
  // if ( abs ( pot - oldPot ) > 2 ) {
    if ( pot == 512 ) {
      direction = 0;
    } else if ( pot > 512 ) {
      direction = 1;
    } else {
      direction = -1;
    }
 
    /*
    delay_us = map ( constrain ( exp ( - abs ( 512.0 - pot ) / 80.0 ) * 1000,
                                 2, 1000 ) , 
                     2, 1000, 20, 100000L );
           */
                     
    // the pot at 512 counts will square to 262144
    // delay at center (512) will be 262344, 200 at ends
    
    /*
    delay_us = ( 262144L - ( ( 512L - pot ) * ( 512L - pot ) ) ) / 4 + 200;
    
    Serial.print ( "Pot: " ); Serial.print ( pot );
    Serial.print ( "; delay: " ); Serial.print ( delay_us ); Serial.println ( "us" );

    // max acceleration is 60% of change per second
    // which is (60% of change) * old_delay_us * 1e6  per loop
    delay_us = old_delay_us + ( delay_us - old_delay_us ) * 6e-7 * old_delay_us;
    */
    
    // new loops_per_sec is loops_per_sec + desired_value * 60% / loops_per_sec
    command_loops_per_sec = max ( 0.0, abs ( 512.0 - pot ) - 5.0 ) * 100.0;
    loops_per_sec = max ( 0.0, loops_per_sec + ( command_loops_per_sec - loops_per_sec ) * 0.60 / loops_per_sec );
    delay_us = 1e6 / loops_per_sec;
    
    oldPot = pot;
  // }
  
  if ( ( millis () - lastPrintAt_ms ) > 0 ) {
    Serial.print ( "command: " ); Serial.print ( command_loops_per_sec ); Serial.print ( "; " );
    Serial.print ( "loops per second: " ); Serial.print ( loops_per_sec ); Serial.print ( "; " );
    Serial.print ( "accel delay: " ); Serial.print ( delay_us ); Serial.println ( "us" );
    lastPrintAt_ms = millis();
  }

  // delay ( 2000 );
  
  // if ( ( millis () - lastPrintAt_ms ) > 500 ) {
    // Serial.print ( location ); Serial.print ( "; " );
    // Serial.print ( delay_us ); Serial.print ( "; " );
    // Serial.print ( phase ); Serial.print ( "; " );
    // Serial.print ( pot ); Serial.print ( "; " );
    // Serial.print ( direction ); Serial.println ();
    // lastPrintAt_ms = millis();
  // }
  
  phase = ( phase + nPhases + ( direction * ( 1 + PHASE_SKIP ) ) ) % nPhases ;
  
  for ( int i = 0; i < nWires; i++ ) {
    // set the active ones to INACTIVE_STATE, the others to !INACTIVE_STATE
    digitalWrite ( wires[i], ( ( phases [ phase ] >> i ) & 0x01 ) ^ INACTIVE_STATE );
    // Serial.print ( "   wire " ); Serial.print ( wires[i] );
    // Serial.print ( ": " ); Serial.println ( ( phases [ phase ] >> i ) & 0x01 );
    // delay ( 1000 );
  }
    
  if ( delay_us > 5000 ) {
    Serial.print ( "Phase: " ); Serial.print ( phase ); Serial.print ( "; " );
    // Serial.print ( "Enable: " ); Serial.print ( pairs[phase][2] ); Serial.print ( "; " );
    Serial.print ( "Direction: " ); Serial.print ( direction ); Serial.print ( "; " );
    Serial.print ( "Mask: B" ); Serial.print ( phases [ phase ], BIN ); Serial.println ();
  }
  
  location += direction;
  
  delay_us = constrain ( delay_us, 100, 2000000L );
  
  // delay should go from 1000ms at center to 1ms at ends
  if ( delay_us < 5000 ) {
    delayMicroseconds ( delay_us );
  } else {
    delay ( delay_us / 1000L );
  }

  
}