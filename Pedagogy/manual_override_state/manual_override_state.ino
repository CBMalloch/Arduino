/*
  manual_override_state.ino
  Demonstration of simplified state machine with two inputs:
    a photocell and a touch sensor
  The photocell controls the state of a night light, unless manually overridden
  by a touch to the touch sensor. A second touch rescinds the override.
  
  The problem as we approached it in class offers an incorrect solution. 
  Specifically, if we represent each state with three characters, one each for 
  override, room, and LED, with
    override: 0 off, 1 on
    room: 0 dark, 1 lit
    LED: 0 off, 1 on
  we have the following state transition chart:
  
        000   <--T-->   101 <-
         v                     \
      > 001   <--T-->   100 <\  |
     /                        R R
    R   011   <--T-->   110 </  |
     \   v                     /
      > 010   <--T-->   111 <-
     
  Illustrating that a touch on the manual override moves back or forth between
  the left and right columns, that state 000 immediately transitions to 001,
  011 similarly transistions to 010, and that a change in the room lighting 
  moves between 001 and 010, or between 110 and 111 or between 101 and 111.
  
  The states that don't persist, namely 000 and 011, could be safely removed.
  
  This is the state transistion picture I was trying to create. However, there is 
  some desired behavior that cannot be achieved with the set of events that are 
  listed -- namely a desired transition between 101 and 100 or between 110 and 111
  -- which is translated as "we're in manual override, and wish to turn the LED on 
  or off.
  
  Wah.
  
  Imposing the additional rule that T will not cause entry into the nonpersistent
  states 000 and 011 would help by using T to transistion from 101 to 100 or from 110 to 111,
  but not the other way 'round.
  
  We need to add one more event type to allow for the required additional transisitons:
  perhaps a near pass but not a touch to the touch sensor, or perhaps ( more difficult to 
  code ) a double-touch on the touch sensor.
  
  If we encode a partial touch as "P", we could have the following state transistion
  diagram, which would solve the problem completely:
  
         000   <--T-->   101 <-
          v               ^     \
          v               P     |
          v               v     |
      -> 001   <--T-->   100    |
    /                     ^     |
    |                     R     R
    |                     v     |
    R    011   <--T-->   110    |
    |     v               ^     |
    |     v               P     |
    \     v               v     /
      -> 010   <--T-->   111 <-

  Having now defined the states, the events, and the actions to be taken
  for each event and state, we can now code the states and transitions. 
  
  This program will illustrate one possible instantiation.
  
*/

#define BAUDRATE 115200

#define ptTouch                  16
#define pdNightLight              6
#define pdBlinkyLED              13
#define paPhotocell              14
#define light_threshold         365
#define light_hysteresis          5
#define full_touch_threshold   1500

float mean, variance, stdev;
int touch_threshold;

int manual_override = 0, room_is_bright = 1, LED = 0;

void setup () {
  
  pinMode ( pdBlinkyLED, OUTPUT );
  pinMode ( pdNightLight, OUTPUT );
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 5000 ) ) {
    digitalWrite ( pdBlinkyLED, 1 - digitalRead ( pdBlinkyLED ) );
    delay ( 100 );
  }
  
  Serial.println ( "manual_override_state v1.0.0 2015-03-05" );

  const int nLoops = 1000;
  unsigned long sum = 0, sum2 = 0;
  for ( int i = 0; i < nLoops; i++ ) {
    int v = touchRead ( ptTouch );
    sum += v;
    sum2 += v * v;
    digitalWrite ( pdBlinkyLED, ! digitalRead ( pdBlinkyLED ) );
    delay ( 2 );
  }
  
  mean = sum / nLoops;
  variance = sum2 / nLoops - mean * mean;
  stdev = sqrt ( variance );

  touch_threshold = int ( mean + 2 * stdev );
  Serial.print ( "mean: " ); Serial.println ( mean );
  Serial.print ( "variance: " ); Serial.println ( variance );
  Serial.print ( "std dev: " ); Serial.println ( stdev );
  Serial.print ( "touch threshold: " ); Serial.println ( touch_threshold );
    
  Serial.println (); Serial.println ( "Done with initialization" );
  delay ( 1000 );

}

#define printInterval_ms  500UL
#define blinkInterval_ms 1000UL

void loop () {
  int event_R, event_P, event_T, counts;
  static unsigned long lastPrintAt_ms = 0L, lastBlinkAt_ms = 0L;
  
  event_R = 0;
  event_P = 0;
  event_T = 0;
  
  // begin by determining what event(s) have occurred
  
  // room lighting
  counts = analogRead ( paPhotocell );
  // Serial.print ( "photocell: " ); Serial.println ( counts ); delay ( 10 );
  if (  (   room_is_bright && ( counts < light_threshold - light_hysteresis ) )
     || ( ! room_is_bright && ( counts > light_threshold + light_hysteresis ) ) ) {
    // light transition has occurred
    event_R = 1;
    room_is_bright = 1 - room_is_bright;
    Serial.println ( room_is_bright ? "room is bright" : "room is dark" );
  }
  
  // touching
  counts = touchRead ( ptTouch );
  // Serial.print ( "touch: " ); Serial.println ( counts ); delay ( 10 );
  if ( counts > full_touch_threshold ) { 
    // full touch has occurred
    event_T = 1;
    Serial.println ( "full touch" );
    while ( touchRead ( ptTouch ) > touch_threshold ) {
      // wait for the person to let go
      delay ( 10 );
    }
    // and a little more just for good measure
    delay ( 100 ); 
  } else if ( counts > touch_threshold ) {
    // partial touch has occurred
    event_P = 1;
    Serial.println ( "partial touch" );
    while ( touchRead ( ptTouch ) > touch_threshold ) {
      // wait for the person to let go
      delay ( 10 );
    }
    // and a little more just for good measure
    delay ( 100 ); 
  }
  
  // now do transitions as required
  
  if ( event_T ) {
    // full touch switches override value
    manual_override = 1 - manual_override;
  }

  if ( manual_override && event_P ) {
    // partial touch inverts LED
    LED = 1 - LED;
  }
  
  if ( ! manual_override && event_R ) {
    // room lighting changed, so invert LED
    LED = 1 - LED;
  }
  
  digitalWrite ( pdNightLight, LED );
  
  if ( ( millis() - lastPrintAt_ms ) > printInterval_ms ) {
    
    Serial.print ( manual_override ? " M " : "~M " );
    Serial.print ( room_is_bright  ? " B " : "~B " );
    Serial.print ( LED             ? " L " : "~L " );
    Serial.println ();
  
    lastPrintAt_ms = millis();
  }
    
  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    digitalWrite ( pdBlinkyLED, ! digitalRead ( pdBlinkyLED ) );
    lastBlinkAt_ms = millis();
  }
  
}
