#define VERSION "3.1.2"
#define VERDATE "2013-03-19"
#define PROGMONIKER "RSE"

/*
  
  React to sound via earphone
  Charles B. Malloch, PhD  2012-12-06
  Created in support of Hampshire College
  
  2012-12-08 v2.1   fixed overstepping of the array by printBar
  2012-12-13 v2.2   show the threshold bar charts for 30 seconds when 
                    a change is made; otherwise run fast
  2012-12-14 v2.2.1 added loop rate reporting preparatory to decreasing 
                    smoothing of amplitude.
  2012-12-19 v2.2.2 slowed reporting to 4Hz from 10Hz
                    fixed multiply #defined paTHRESHOLD
  2012-12-23 v3.0.0 loop rate adjustable with A3 pot
                    pulled all EWMA stuff out, replaced with a longer read
  2013-01-27 v3.1.0 control pot now has 4 positions:
                    0: motors enabled, not verbose
                    1: motors disabled, not verbose
                    2: motors enabled, verbose
                    3: motors disabled, verbose

*/

#include <math.h>

// disable serial comm if BAUDRATE is undefined
// because serial comm pulls power 5V down by 500mV !!!
// #undef BAUDRATE
#define BAUDRATE 115200
static int VERBOSE = 2;
static int motorsEnabled;
#define paCONTROL 3

#define USE_SERVO_3 1
// 1100
#define SERVO_MIN_PULSE_us 500
// 2410
#define SERVO_MAX_PULSE_us 2500

#define paMIKE 0
#define paMINTHRESHOLD 4
#define paTHRESHOLD 5

/*
  NOTE regarding the motor enable lines - when these are low, the affected outputs are 
  hi-z (tri-stated). This is OK for DC motors, but not so for servos. So to control:
    DC motor - set direction with digital writes to motor pins, control speed with PWM on enables
    servo motor - enable output with digital write, then control position with PWM on motor pin
    stepper motor - enable output, then step using digital writes to motor pins
*/

#define pdMotor1A 3
#define pdMotor2A 5
#define ppMotorEnable12 10

#define pdMotor3A 6
#define pdMotor4A 9
#define ppMotorEnable34 11

#define pdLED 13

// to get a reading, need to average over at least a half-cycle of sound. If min freq is 110, 
// that's 9ms for a full cycle or 4.5ms for a half.
#define SAMPLING_PERIOD_MS 10
#define EXPECTEDMAXSEMIAMPLITUDE 1000

#define BARCHARTREPORTINGINTERVAL_MS ((VERBOSE > 5) ? 300000UL : 15000UL)
#define DELAYAFTERREPORTING_MS ((VERBOSE > 5) ? 1000 : 200)
#define THRESHOLDCHANGETOLERANCE 2

#define lineLen 80
#define bufLen (lineLen + 3)
char strBuf[bufLen];


// the current averages; one slow-moving, the other fast-moving.
float centerline, amplitude;
int minReading, maxReading;


void setup () {
#ifdef BAUDRATE
  Serial.begin(BAUDRATE);
	snprintf(strBuf, bufLen, "%s: React to Sound via Earphone v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);
#endif

  pinMode(pdLED, OUTPUT);
  
  pinMode(pdMotor1A, OUTPUT);
  pinMode(pdMotor2A, OUTPUT);
  pinMode(ppMotorEnable12, OUTPUT);

  digitalWrite (ppMotorEnable12, LOW);
  digitalWrite (pdMotor1A, LOW);
  digitalWrite (pdMotor2A, HIGH);
  analogWrite (ppMotorEnable12, 255);
  delay(1000);
  digitalWrite (ppMotorEnable12, LOW);
  digitalWrite (pdMotor1A, LOW);
  digitalWrite (pdMotor2A, LOW);

  pinMode(pdMotor3A, OUTPUT);
  pinMode(pdMotor4A, OUTPUT);
  pinMode(ppMotorEnable34, OUTPUT);

  digitalWrite (ppMotorEnable34, LOW);
  digitalWrite (pdMotor3A, LOW);
  digitalWrite (pdMotor4A, HIGH);
  analogWrite (ppMotorEnable34, 255);
  delay(1000);
  digitalWrite (ppMotorEnable34, LOW);
  digitalWrite (pdMotor3A, LOW);
  digitalWrite (pdMotor4A, LOW);
  
  #ifdef USE_SERVO_3xxx
  // don't use PWM for servos - they need 50Hz and pulse width of 500-2500us
  digitalWrite (ppMotorEnable34, 1);
  for (int i = 0; i <= 100; i++) {
    analogWrite (pdMotor3A, map (i, 0, 100, 00, 1000));
#ifdef BAUDRATE
    Serial.print ("Servo 3: "); Serial.println (map (i, 0, 100, 0, 180));
#endif
    delay (500);
  }
  #endif

  
  motorsEnabled = 0;

}

void loop () {
  
  // variables related to taking a reading
  unsigned long readingBeganAt;
  int nReadings, r;
  float fReading, fVarReading;

  // variables related to evaluating a reading vs our thresholds
  int threshold, minThreshold;
  static int oldThreshold, oldMinThreshold;
  static unsigned long thresholdChangedAt = 0UL;
  short responseLevel;

  // variables related to motor control
  int PWMval, score;
  static unsigned long lastReversal = millis();
  static unsigned short direction = 0;
  
  // variables related to pacing the loop
  static unsigned long loopBeganAt_us = 0;
  unsigned long loopTook_us;
  
  loopTook_us = micros() - loopBeganAt_us;
  loopBeganAt_us = micros();
  
  r = analogRead(paCONTROL);
  VERBOSE = r > 512 ? 10 : 2;
  motorsEnabled = ! ( r & 0x0100 );
  if (analogRead(paCONTROL) > 256) {
    thresholdChangedAt = millis();
  }
  
  minThreshold = map ( analogRead (paMINTHRESHOLD), 0, 1023, 0, 100 );
  threshold = map ( analogRead (paTHRESHOLD), 0, 1023, 0, 100 );
  
  if (   ( threshold - oldThreshold )       > THRESHOLDCHANGETOLERANCE
      || ( minThreshold - oldMinThreshold ) > THRESHOLDCHANGETOLERANCE ) {
    oldThreshold = threshold;
    oldMinThreshold = minThreshold;
    thresholdChangedAt = millis();
  }
  
  // *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=*

  nReadings = 0;
  readingBeganAt = millis();
  fReading = 0.0;
  fVarReading = 0.0;
  minReading = 2000;
  maxReading = -5;
  while ((millis() - readingBeganAt) < SAMPLING_PERIOD_MS) {
    nReadings++;
    r = analogRead(paMIKE);
    fReading += r;
    fVarReading += abs( r - centerline );
    if (r < minReading) minReading = r;
    if (r > maxReading) maxReading = r;
  }
  amplitude = int (20.0 * fVarReading / nReadings);
  centerline = int (fReading / nReadings);            // the new centerline will be used next loop

  // *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=*
  
  /*
    constrain constrains a value to a certain range; if it tries to exceed the range, 
    is set to the nearest endpoint e.g. constrain(195, 0, 170) returns 170.
    map maps a range of numbers into a new (either expanded or diminished) range.
    e.g. map(12, 0, 100, 0, 1000) returns 120.
    The allowable range for PWM is 0-255, so we remap to that range.
  */
  
  score = map(constrain(amplitude, 0, EXPECTEDMAXSEMIAMPLITUDE), 0, EXPECTEDMAXSEMIAMPLITUDE, 0, 100);
  // now we have a score between 0 and 100
  // we can compare this with minThreshold and threshold
  
    
  // *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=*  
  
  
  /*
    One good test would be with the manual threshold
    as in if (score > threshold) {  etc.
    But we want automatically set threshold
    So we use a grand average (centerline) to compare against
    but give it a little wiggle room
  */
  if (motorsEnabled > 0) {
    if (score > threshold) {
      digitalWrite (pdLED, HIGH);
    } else {
      digitalWrite (pdLED, LOW);
    }
    
    if (score < minThreshold) {
      // do nothing / stop everything
      responseLevel = 0;
      digitalWrite (ppMotorEnable12, LOW);
      digitalWrite (pdMotor1A, LOW);
      digitalWrite (pdMotor2A, LOW);
      lastReversal = 0;
#ifdef BAUDRATE
      // Serial.println ("LEVEL 0");
#endif
    } else if (score < threshold) {
      // responseLevel 1 - move with speed related to score
      responseLevel = 1;
      // digitalWrite (ppMotorEnable12, LOW);
      digitalWrite (pdMotor1A, LOW);
      digitalWrite (pdMotor2A, HIGH);
      analogWrite (ppMotorEnable12, map (score, 0, 100, 0, 255));
      direction = 0;
      lastReversal = 0;
#ifdef BAUDRATE
      // Serial.println ("                  LEVEL 1");
#endif
    } else {
      // responseLevel 2 - move back and forth at top speed
      responseLevel = 2;
      if ((millis() - lastReversal) > 500) {
        // digitalWrite (ppMotorEnable12, LOW);
        if (direction == 0) {
          digitalWrite (pdMotor2A, LOW);
          digitalWrite (pdMotor1A, HIGH);
          direction = 1;
        } else {
          digitalWrite (pdMotor1A, LOW);
          digitalWrite (pdMotor2A, HIGH);
          direction = 0;
        }
        lastReversal = millis();
        analogWrite (ppMotorEnable12, map (score, 0, 100, 0, 255));
      }
      // Serial.println ("                                      LEVEL 2");
    }
    
    
    
    
    #ifdef USE_SERVO_3
    analogWrite (ppMotorEnable34, 255);
    // need to get a servo pulse width between SERVO_MIN_PULSE_us and SERVO_MAX_PULSE_us
    // from commanded angle, given that we know the PWM frequency
    int commandedAngle = map (score, 0, 100, 0, 180);
    int pulseWidth_us = map (commandedAngle, 0, 180, SERVO_MIN_PULSE_us, SERVO_MAX_PULSE_us);
    // PWM frequency is about 500Hz (period 20ms) so pulse width = period * duty cycle
    int duty_cycle_x_1000 = pulseWidth_us / 20;  // will be between 0 and 1000
    analogWrite (pdMotor3A, duty_cycle_x_1000);
#ifdef BAUDRATE
    Serial.print ("Servo 3: "); 
    Serial.print ("command a: "); ( commandedAngle );
    Serial.print ("; width us: "); ( pulseWidth_us );
    Serial.print ("; dc * 1000: "); ( duty_cycle_x_1000 );
    Serial.println ( map (score, 0, 100, 0, 180) );
#endif
    #endif
  } else {
    // motors not enabled
    digitalWrite (ppMotorEnable12, LOW);
    digitalWrite (pdMotor1A, LOW);
    digitalWrite (pdMotor2A, LOW);
  }
  
  // *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=*  
  
#ifdef BAUDRATE
  if ( ( millis() - thresholdChangedAt ) < BARCHARTREPORTINGINTERVAL_MS ) {
    // Serial.print (centerline);
    // Serial.print ("  ");
    // Serial.print (amplitude);
    // Serial.print ("  ");
    // Serial.print (score);
    // Serial.print ("  ");
    // Serial.print (reading);
    // Serial.println ();
    
    Serial.println ();
    if (VERBOSE > 5) {
      Serial.print ("verbose        : "); Serial.println (VERBOSE);
      Serial.print ("motors enabled : "); Serial.println (motorsEnabled);
      Serial.print ("centerline     : "); Serial.println (centerline);
      Serial.print ("amplitude      : "); Serial.println (amplitude);
      Serial.print ("min            : "); Serial.println (minReading);
      Serial.print ("max            : "); Serial.println (maxReading);
    }
    Serial.print ("min thr      : "); printBar (minThreshold);
    Serial.print ("thr          : "); printBar (threshold);
    Serial.print ("score        : "); printBar (score);
    Serial.print ("response level: "); Serial.println (responseLevel);
    if (loopTook_us > DELAYAFTERREPORTING_MS * 1000UL) {
      loopTook_us -= DELAYAFTERREPORTING_MS * 1000UL;
    }
    Serial.print ("Loop took (us): "); Serial.println (loopTook_us);
    unsigned long loopRate_Hz;
    loopRate_Hz = long ( 1000000UL / loopTook_us );
    Serial.print ("Loop rate (Hz): "); Serial.println (loopRate_Hz);
    
    // delay a little so printing is visible
    delay (DELAYAFTERREPORTING_MS);
    
  }
#endif
  
}

#ifdef BAUDRATE
void printBar (unsigned short barLen) {
  barLen = constrain (barLen, 0, lineLen);
  for (int i = 0; i < barLen; i++) strBuf[i] = '-';
  strBuf[barLen] = '*';
  for (int i = barLen + 1; i <= lineLen; i++) strBuf[i] = ' ';
  strBuf[bufLen - 2] = '|';
  strBuf[bufLen - 1] = '\0';
  Serial.println (strBuf);
}
#endif