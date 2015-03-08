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
#define paPhotocell              18
#define light_threshold         150
#define light_hysteresis         50
#define full_touch_threshold   1500

float mean, variance, stdev;
int touch_threshold;

int state;
#define manual_override ( ( state >> 2 ) & 0b001 )
#define room_is_bright  ( ( state >> 1 ) & 0b001 )
#define LED             ( ( state >> 0 ) & 0b001 )


// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>


void setup () {
  
  pinMode ( pdBlinkyLED, OUTPUT );
  pinMode ( pdNightLight, OUTPUT );
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 5000 ) ) {
    digitalWrite ( pdBlinkyLED, 1 - digitalRead ( pdBlinkyLED ) );
    delay ( 100 );
  }
  
  Serial.println ( "manual_override_state v1.2.0 2015-03-06" );

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
  
  // set initial state according to the light level in the room
  state = ( analogRead ( paPhotocell ) < light_threshold ) ? 0b001 : 0b010;
    
  Serial.println (); Serial.println ( "Done with initialization" );
  delay ( 1000 );

}

// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>


// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

void loop () {
  static int event_R = 0, event_P = 0, event_T = 0, counts, n;
  static unsigned long lastBlinkAt_ms = 0L;
  static unsigned int blinkInterval_ms = 1000UL;
  
  // begin by determining what event(s) have occurred
  
  // room lighting
  counts = analogRead ( paPhotocell );
  // Serial.print ( "photocell: " ); Serial.println ( counts ); delay ( 10 );
  if (  (     room_is_bright   && ( counts < light_threshold - light_hysteresis ) )
     || ( ( ! room_is_bright ) && ( counts > light_threshold + light_hysteresis ) ) ) {
    // light transition has occurred
    event_R = 1;
    Serial.print ( "room light transition: counts = " ); Serial.println ( counts );
  }
  
  // touching
  // touch will go up as finger approaches sensor;
  // keep reading until counts stops increasing
  counts = 0;
  n = 0;
  while ( n < 10 ) {
    int maxCounts = touchRead ( ptTouch );
    if ( maxCounts > counts ) {
      // keep looking
      counts = maxCounts;
      n = 0;
    } else {
      n++;
    }
    delay ( 20 );
  }
  // Serial.print ( "touch: " ); Serial.println ( counts ); delay ( 10 );
  if ( counts > full_touch_threshold ) { 
    // full touch has occurred
    event_T = 1;
    Serial.println ( "full touch" );
    // wait for the person to let go
    // won't pass through until 10 readings in a row less than the threshold
    int n = 0;
    while ( n < 10 ) {
      if ( touchRead ( ptTouch ) < touch_threshold ) { 
        n++;
      } else {
        n = 0;
      }
      delay ( 20 );
    }
  } else if ( counts > touch_threshold ) {
    // partial touch has occurred
    event_P = 1;
    Serial.println ( "partial touch" );
    // wait for the person to let go
    // won't pass through until 10 readings in a row less than the threshold
    int n = 0;
    while ( n < 10 ) {
      if ( touchRead ( ptTouch ) < touch_threshold ) { 
        n++;
      } else {
        n = 0;
      }
      delay ( 20 );
    }
  }
  
  // now do transitions as required
  
  int event = event_T * 4 + event_P * 2 + event_R;
  if ( event ) {
    state = nextState ( state, event_T * 4 + event_P * 2 + event_R );
    
    Serial.print ( "New state: " );
    Serial.print ( manual_override ? " M " : "~M " );
    Serial.print ( room_is_bright  ? " B " : "~B " );
    Serial.print ( LED             ? " L " : "~L " );
    Serial.println ();
  }
  
  digitalWrite ( pdNightLight, LED );
      
  blinkInterval_ms = manual_override ? 200UL : 500UL;
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    digitalWrite ( pdBlinkyLED, ! digitalRead ( pdBlinkyLED ) );
    lastBlinkAt_ms = millis();
  }
  
  event_R = 0;
  event_P = 0;
  event_T = 0;
  
}

// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

int nextState ( int currentState, int event ) {
  /*
    state is represented as a 3-bit binary number, where the first (fours) bit
    is 1 if we are in manual-override mode, 0 otherwise; the second (twos) bit is 
    0 if the room is dark, 1 if it is brightly lit; the third (ones) bit is 0 if 
    the LED is off and 1 if it is on.
    
    event is a three-bit binary number whose first (fours) bit is 1 if a 
    full touch event has occurred, 0 otherwise; second (twos) bit is 1 if a 
    partial-touch event has occurred, 0 otherwise; and third (ones) bit is 1 if 
    the room has changed its lighting level, 0 otherwise.
  */
  
  int newState;
  
  #define full_touch         ( ( event >> 2 ) & 0b001 )
  #define partial_touch      ( ( event >> 1 ) & 0b001 )
  #define room_light_change  ( ( event >> 0 ) & 0b001 )
  
  Serial.print ( "nextState: entry state = " ); Serial.print ( currentState, BIN );
  Serial.print ( "; entry event = " ); Serial.println ( event, BIN );
  
  newState = currentState;
  
  switch ( currentState ) {
    case 0b000:
      newState = 0b001;
      break;
    case 0b001:
      if ( full_touch ) {
        newState = 0b101;
      } else if ( room_light_change ) {
        newState = 0b010;
      }
      break;
    case 0b010:
      if ( full_touch ) {
        newState = 0b111;
      } else if ( room_light_change ) {
        newState = 0b001;
      }
      break;
    case 0b011:
      newState = 0b010;
      break;
    case 0b100:
      if ( full_touch ) {
        newState = 0b001;
      } else if ( partial_touch ) {
        newState = 0b101;
      } else if ( room_light_change ) {
        newState = 0b110;
      }
      break;
    case 0b101:
      if ( full_touch ) {
        newState = 0b001;
      } else if ( partial_touch ) {
        newState = 0b100;
      } else if ( room_light_change ) {
        newState = 0b111;
      }
      break;
    case 0b110:
      if ( full_touch ) {
        newState = 0b010;
      } else if ( partial_touch ) {
        newState = 0b111;
      } else if ( room_light_change ) {
        newState = 0b100;
      }
      break;
    case 0b111:
      if ( full_touch ) {
        newState = 0b010;
      } else if ( partial_touch ) {
        newState = 0b110;
      } else if ( room_light_change ) {
        newState = 0b101;
      }
      break;
  }

  Serial.print ( "nextState: new state = " ); Serial.println ( newState, BIN );
  
  return ( newState );
}