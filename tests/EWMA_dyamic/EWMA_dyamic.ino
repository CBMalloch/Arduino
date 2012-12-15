/*
  
  Test new method for EWMA, adapting alpha to the actual loop times
  Charles B. Malloch, PhD  2012-12-14
  
*/

#include <math.h>

#define BAUDRATE 115200

#define DT_MS 500
#define DT_NOISE 490

#define DESIRED_RESPONSE 0.5
#define DESIRED_RESPONSE_AT 10.0

#define INITIAL_VALUE 1000.0

float static_alpha, dynamic_alpha, EWMA_static, EWMA_dynamic;
unsigned long timeBeganAt_ms;
long step = 0;
unsigned long loopBeganAt_ms;

void setup () {

  Serial.begin ( BAUDRATE );
  // The fast smoothing should have a half-life of 10 periods = 0.062
  // = 1 - exp ( ln (1/2) / nPeriods ) = 1 - (nPeriodsth root of 1/2)
  // Revision: should drop to 1% after 10 ms -> alpha = 0.369
  //  IF the loop rate is assumed to be 1 kHz

  // The Excel formula is =1-EXP(LN(desired_response) / (desired_time / step_interval))
  
  // We want a response of DESIRED_RESPONSE at time DESIRED_RESPONSE_AT,
  // which is DESIRED_RESPONSE_AT * 1000.0 / DT_MS periods

  static_alpha = 1.0 - exp ( log (0.5) / ( DESIRED_RESPONSE_AT * 1000.0 / DT_MS ) );
  Serial.print ("Static alpha: "); Serial.println ( static_alpha );
  
  EWMA_static = 0.0;
  EWMA_dynamic = 0.0;
  
  timeBeganAt_ms = millis();
  loopBeganAt_ms = millis();

}

void loop () {
  // regulate the loop to run at the required rate
  unsigned long loopTook_ms;
  int delay_ms;  // must leave as signed!
  static float newVal = INITIAL_VALUE;
  
  dynamic_alpha = EWMA_alpha ( DESIRED_RESPONSE, DESIRED_RESPONSE_AT );
  Serial.print ("Dynamic alpha: "); Serial.println ( dynamic_alpha );
  
  if ( newVal > 0 ) {
    // first time through
    EWMA_static = newVal;
  } else {
    EWMA_static = ( 1.0 - static_alpha ) * EWMA_static + static_alpha * newVal;
  }
  EWMA_dynamic = ( 1.0 - dynamic_alpha ) * EWMA_dynamic + dynamic_alpha * newVal;
  
  newVal = 0.0;
  
  Serial.print ("                  Static:  "); Serial.println ( EWMA_static );
  Serial.print ("                  Dynamic: "); Serial.print ( EWMA_dynamic );
  Serial.print (" at "); Serial.print ( ( millis() - timeBeganAt_ms ) / 1000.0 );
  Serial.print (" ( step "); Serial.print ( step ); Serial.println (" )");
 
  
  // calculate required delay
  delay_ms = int ( ( loopBeganAt_ms + DT_MS ) - millis() );
  if ( delay_ms < 0 ) {
    Serial.print ("ERROR: Loop took overlong at (ms) " ); Serial.println ( millis() - loopBeganAt_ms );
  } else {
    delay ( delay_ms + random (DT_NOISE) - DT_NOISE / 2);  // random noise centered at 0
  }
  loopBeganAt_ms = millis();
  
  step++;
    
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