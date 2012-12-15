#define VERSION "2.2.1"
#define VERDATE "2012-12-15"
#define PROGMONIKER "RSE"

/*
  
  React to sound via earphone
  Charles B. Malloch, PhD  2012-12-06
  Created in support of Hampshire College
  
  2012-12-08 v2.1   fixed overstepping of the array by printBar
  2012-12-13 v2.2   show the threshold bar charts for 30 seconds when 
                    a change is made; otherwise run fast
  2012-12-14 v2.2.1 added loop rate reporting preparatory to decreasing 
                    smoothing of EWMAFAST.

*/

#include <math.h>

#define BAUDRATE 115200

#define paMIKE 0
#define paTHRESHOLD 3
#define BAUDRATE 115200
#define pdMotorA1 8
#define pdMotorA2 9
#define ppMotorA 10
#define pdLED 13

#define paMINTHRESHOLD 1
#define paTHRESHOLD 2

#define EXPECTEDMAXSEMIAMPLITUDE 100

#define BARCHARTREPORTINGINTERVAL_MS 30000
#define DELAYAFTERREPORTING_MS 100
#define THRESHOLDCHANGETOLERANCE 2

#define lineLen 80
#define bufLen (lineLen + 3)
char strBuf[bufLen];


/*
EWMA, or Exponentially Weighted Moving Average, is a very good tool to 
calculate and continually update an average. The normal averaging technique
of adding n numbers and dividing by n has several disadvantages when looking 
at a stream of numbers. The EWMA sidesteps most of these disadvantages.

Here are some links to articles on EWMA:
http://en.wikipedia.org/wiki/Moving_average
http://en.wikipedia.org/wiki/EWMA_chart

A simplified explanation of how EWMA works is that each time a new 
number arrives from the data stream, it is combined with the previous 
average value to result in a new average value. The only memory required 
is of the current average value.

The average calculated by EWMA can respond more quickly or more slowly to 
sudden changes in the data stream, depending on a parameter alpha, which 
determines the *weight* of each new value relative to the average. 
EWMA is calculated as 
    E(t) = (1 - alpha) * E(t-1) + alpha * new_reading
The persistence of a reading taken at t0 at a later time t1 is then
    p(t0, t1) = (1 - alpha) ^ (t1 - t0), 
since it will be diminished by the factor (1 - alpha) at each time step.
Thus the half-life (measured in time steps) of a reading is calculated by 
    1/2 = (1 - alpha) ^ half_life. Then
    ln(1/2) / ln(1 - alpha) = half_life
And so if we want a half life of n time steps, we get
    ln(1/2) / half_life = ln(1 - alpha)
    1/2 ^ (1 / half_life) = 1 - alpha
    alpha = 1 - (1/2 ^ (1 / half_life))
    
An alternate calculation: if we want the effect of a reading at time t
to be reduced to 1 / r at time t+s, then since (disregarding new readings)
    E(t+1) = (1 - alpha) * E(t), then after k time steps
    E(t+k) = (1 - alpha) ^ k * E(t).
    If loop time is represented by m, then s = k * m, or k = s / m.
    1 / r = E(t+s) / E(t) 
          = ( 1 - alpha ) ^ k
          = ( 1 - alpha ) ^ ( s / m ).
    Solving for alpha,
    ln ( 1 / r ) = - ln (r)
          = ln ( 1 - alpha ) * ( s / m )
    ln ( 1 - alpha ) = - ln (r) / ( s / m )
                     = - m * ln (r) / s
    1 - alpha = exp ( - m * ln (r) / s )
    alpha = 1 - exp ( - m * ln (r) / s )
    
*/

// We want the slow smoothing to have a half-life of 1000 periods
// loop rate is about 40 Hz, so 1000 periods is 25 seconds
#define fSLOW 0.0007
// The fast smoothing should have a half-life of 10 periods = 0.062
// = 1 - exp ( ln (1/2) / nPeriods ) = 1 - (nPeriodsth root of 1/2)
// Revision: should drop to 1% after 10 ms -> alpha = 0.369
//  IF the loop rate is assumed to be 1 kHz

// The Excel formula is =1-EXP(LN(desired_response) / (desired_time / step_interval))
// fFAST is now obsolete
#define fFAST 0.369
#define DESIRED_RESPONSE 0.01
#define DESIRED_RESPONSE_AT 0.01

// #define WIGGLEROOM 1

// the current averages; one slow-moving, the other fast-moving.
float EWMASLOW, EWMAFAST;


void setup () {
  Serial.begin(BAUDRATE);
	snprintf(strBuf, bufLen, "%s: React to Sound via Earphone v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);

  pinMode(pdLED, OUTPUT);
  EWMASLOW = -5;
  EWMAFAST = -5;
  
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

  short reading, level;
  short threshold, minThreshold;
  static short oldThreshold, oldMinThreshold;
  static unsigned long thresholdChangedAt = 0UL;

  int PWMval, score;
  static unsigned long lastReversal = millis();
  static unsigned short direction = 0;
  
  static unsigned long loopBeganAt_us = 0;
  unsigned long loopTook_us;
  
  loopTook_us = micros() - loopBeganAt_us;
  loopBeganAt_us = micros();
  
  reading = analogRead(paMIKE);
  minThreshold = map ( analogRead (paMINTHRESHOLD), 0, 1023, 0, 100 );
  threshold = map ( analogRead (paTHRESHOLD), 0, 1023, 0, 100 );
  
  if (   ( threshold - oldThreshold )       > THRESHOLDCHANGETOLERANCE
      || ( minThreshold - oldMinThreshold ) > THRESHOLDCHANGETOLERANCE ) {
    oldThreshold = threshold;
    oldMinThreshold = minThreshold;
    thresholdChangedAt = millis();
  }
  
  /*
    EWMASLOW is a very slowly-moving average
  */
  if (EWMASLOW < 0) {
    EWMASLOW = reading;
  } else {
    EWMASLOW = (1 - fSLOW) * EWMASLOW + fSLOW * reading;
  }
  /*
    EWMAFAST is a quickly-moving average
      ** WHICH AVERAGES THE DEVIATION OF THE SIGNAL FROM ITS CENTERLINE **
    That deviation is the absolute value of the difference of the reading
    from EWMASLOW (which approximates the centerline)
  */
  if (0) {
    // old way
    if (EWMAFAST < 0) {
      EWMAFAST = abs(reading - EWMASLOW);
    } else {
      EWMAFAST = (1 - fFAST) * EWMAFAST + fFAST * abs(reading - EWMASLOW);
    }
  } else {
    // new way
    float alpha = EWMA_alpha (DESIRED_RESPONSE, DESIRED_RESPONSE_AT);
    EWMAFAST = (1 - alpha) * EWMAFAST + alpha * abs(reading - EWMASLOW);
  }
  // *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=*
  
  /*
    constrain constrains a value to a certain range; if it tries to exceed the range, 
    is set to the nearest endpoint e.g. constrain(195, 0, 170) returns 170.
    map maps a range of numbers into a new (either expanded or diminished) range.
    e.g. map(12, 0, 100, 0, 1000) returns 120.
    The allowable range for PWM is 0-255, so we remap to that range.
  */
  
  score = map(constrain(EWMAFAST, 0, EXPECTEDMAXSEMIAMPLITUDE), 0, EXPECTEDMAXSEMIAMPLITUDE, 0, 100);
  // now we have a score between 0 and 100
  // we can compare this with minThreshold and threshold
  
    
  // *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=* *=*  
  
  
  /*
    One good test would be with the manual threshold
    as in if (EWMAFAST > threshold) {  etc.
    But we want automatically set threshold
    So we use a grand average (EWMASLOW) to compare against
    but give it a little wiggle room
  */
  
  if (EWMAFAST > threshold) {
    digitalWrite (pdLED, HIGH);
  } else {
    digitalWrite (pdLED, LOW);
  }
  
  if (score < minThreshold) {
    // do nothing / stop everything
    level = 0;
    digitalWrite (ppMotorA, LOW);
    digitalWrite (pdMotorA1, LOW);
    digitalWrite (pdMotorA2, LOW);
    lastReversal = 0;
    // Serial.println ("LEVEL 0");
  } else if (score < threshold) {
    // level 1 - move with speed related to score
    level = 1;
    digitalWrite (ppMotorA, LOW);
    digitalWrite (pdMotorA1, LOW);
    digitalWrite (pdMotorA2, HIGH);
    analogWrite (ppMotorA, map (score, 0, 100, 0, 255));
    direction = 0;
    lastReversal = 0;
    // Serial.println ("                  LEVEL 1");
  } else {
    // level 2 - move back and forth at top speed
    level = 2;
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
    // Serial.print (EWMASLOW);
    // Serial.print ("  ");
    // Serial.print (EWMAFAST);
    // Serial.print ("  ");
    // Serial.print (score);
    // Serial.print ("  ");
    // Serial.print (reading);
    // Serial.println ();
    
    Serial.println ();
    Serial.print ("LEVEL "); Serial.println (level);
    Serial.print ("score   : "); printBar (score);
    Serial.print ("min thr : "); printBar (minThreshold);
    Serial.print ("thr     : "); printBar (threshold);
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

float EWMA_alpha (float desired_response, float desired_time) {
  // The Excel formula is =1-EXP(LN(desired_response) / (desired_time / step_interval))
  static unsigned long lastCalledAt_us = 0;
  unsigned long step_interval_us;
  float alpha;

  if ( lastCalledAt_us == 0 ) {
    // never called before -- force initialization of y(t+1) to new value
    alpha = 1.0;
  } else {
    step_interval_us = micros() - lastCalledAt_us;
    lastCalledAt_us = micros();
    // note log is natural log
    alpha = 1 - exp (   log ( desired_response ) 
                      / ( ( desired_time * 1e6) / float ( step_interval_us ) ) );
  }
  
  lastCalledAt_us = micros();
  
  return ( alpha );
}