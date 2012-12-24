#define VERSION "2.2.2"
#define VERDATE "2012-12-19"
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

*/

#include <math.h>

#define BAUDRATE 115200
static int VERBOSE = 2;
#define paVERBOSE 3

#define paMIKE 0
#define paMINTHRESHOLD 4
#define paTHRESHOLD 5
#define BAUDRATE 115200
#define pdMotorA1 2
#define pdMotorA2 3
#define ppMotorA 10
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


void setup () {
  Serial.begin(BAUDRATE);
	snprintf(strBuf, bufLen, "%s: React to Sound via Earphone v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);

  pinMode(pdLED, OUTPUT);
  
  pinMode(pdMotorA1, OUTPUT);
  pinMode(pdMotorA2, OUTPUT);
  pinMode(ppMotorA, OUTPUT);

  digitalWrite (ppMotorA, LOW);
  digitalWrite (pdMotorA1, LOW);
  digitalWrite (pdMotorA2, HIGH);
  analogWrite (ppMotorA, 255);
  delay(1000);
  digitalWrite (ppMotorA, LOW);
  digitalWrite (pdMotorA1, LOW);
  digitalWrite (pdMotorA2, LOW);
}

void loop () {
  
  // variables related to taking a reading
  unsigned long readingBeganAt;
  int nReadings, r;
  float fReading, fVarReading;

  // variables related to evaluating a reading vs our thresholds
  short threshold, minThreshold;
  static short oldThreshold, oldMinThreshold;
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
  
  VERBOSE = analogRead(paVERBOSE) > 512 ? 10 : 2;
  if (analogRead(paVERBOSE) > 256) {
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
  while ((millis() - readingBeganAt) < SAMPLING_PERIOD_MS) {
    nReadings++;
    r = analogRead(paMIKE);
    fReading += r;
    fVarReading += abs( r - centerline );
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
    as in if (amplitude > threshold) {  etc.
    But we want automatically set threshold
    So we use a grand average (centerline) to compare against
    but give it a little wiggle room
  */
  
  if (amplitude > threshold) {
    digitalWrite (pdLED, HIGH);
  } else {
    digitalWrite (pdLED, LOW);
  }
  
  if (score < minThreshold) {
    // do nothing / stop everything
    responseLevel = 0;
    digitalWrite (ppMotorA, LOW);
    digitalWrite (pdMotorA1, LOW);
    digitalWrite (pdMotorA2, LOW);
    lastReversal = 0;
    // Serial.println ("LEVEL 0");
  } else if (score < threshold) {
    // responseLevel 1 - move with speed related to score
    responseLevel = 1;
    digitalWrite (ppMotorA, LOW);
    digitalWrite (pdMotorA1, LOW);
    digitalWrite (pdMotorA2, HIGH);
    analogWrite (ppMotorA, map (score, 0, 100, 0, 255));
    direction = 0;
    lastReversal = 0;
    // Serial.println ("                  LEVEL 1");
  } else {
    // responseLevel 2 - move back and forth at top speed
    responseLevel = 2;
    if ((millis() - lastReversal) > 500) {
      digitalWrite (ppMotorA, LOW);
      if (direction == 0) {
        digitalWrite (pdMotorA2, LOW);
        digitalWrite (pdMotorA1, HIGH);
        direction = 1;
      } else {
        digitalWrite (pdMotorA1, LOW);
        digitalWrite (pdMotorA2, HIGH);
        direction = 0;
      }
      lastReversal = millis();
      analogWrite (ppMotorA, map (score, 0, 100, 0, 255));
    }
    // Serial.println ("                                      LEVEL 2");
  }
  
  // *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=*  
  
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
      Serial.print ("centerline   : "); Serial.println (centerline);
      Serial.print ("amplitude    : "); Serial.println (amplitude);      
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
  
}

void printBar (unsigned short barLen) {
  barLen = constrain (barLen, 0, lineLen);
  for (int i = 0; i < barLen; i++) strBuf[i] = '-';
  strBuf[barLen] = '*';
  for (int i = barLen + 1; i <= lineLen; i++) strBuf[i] = ' ';
  strBuf[bufLen - 2] = '|';
  strBuf[bufLen - 1] = '\0';
  Serial.println (strBuf);
}
