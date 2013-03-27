/*
  
  Test new method for EWMA, adapting alpha to the actual loop times
  Charles B. Malloch, PhD  2012-12-14
  
*/

#include <math.h>
#include <EWMA.h>

#define BAUDRATE 115200

#define DT_MS 500
#define DT_NOISE 490

#define DESIRED_RESPONSE 0.5
#define DESIRED_RESPONSE_AT 10.0

#define INITIAL_VALUE 1000.0

float static_alpha, dynamic_alpha_internal, dynamic_alpha_external;
float EWMA_static, EWMA_dynamic, EWMA_dynamic_measured;
unsigned long timeBeganAt_ms;
long step = 0;
unsigned long loopBeganAt_ms;

EWMA test_item;

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
  EWMA_dynamic_measured = 0.0;
  
  timeBeganAt_ms = millis();
  loopBeganAt_ms = millis();

}

void loop () {
  // regulate the loop to run at the required rate
  static unsigned long loopTook_ms = 2147483647L;
  int delay_ms;  // must leave as signed!
  static float newVal = INITIAL_VALUE;
  
  dynamic_alpha_internal = test_item.dynamic_alpha ( DESIRED_RESPONSE, DESIRED_RESPONSE_AT );
  Serial.print ("Dynamic alpha internal: "); Serial.println ( dynamic_alpha_internal );
  
  dynamic_alpha_external = test_item.dynamic_alpha ( DESIRED_RESPONSE, DESIRED_RESPONSE_AT, 
                                                     loopTook_ms / 1000.0 );
  Serial.print ("Dynamic alpha external: "); Serial.println ( dynamic_alpha_external );
  
  if ( newVal > 0 ) {
    // first time through
    EWMA_static = newVal;
  } else {
    EWMA_static = ( 1.0 - static_alpha ) * EWMA_static + static_alpha * newVal;
  }
  EWMA_dynamic = ( 1.0 - dynamic_alpha_internal ) * EWMA_dynamic 
                + dynamic_alpha_internal * newVal;
  EWMA_dynamic_measured = ( 1.0 - dynamic_alpha_external ) * EWMA_dynamic_measured 
                + dynamic_alpha_external * newVal;
  
  newVal = 0.0;
  
  Serial.print ("                  Static:      "); Serial.println ( EWMA_static );
  Serial.print ("                  Dynamic Int: "); Serial.println ( EWMA_dynamic );
  Serial.print ("                  Dynamic Ext: "); Serial.print   ( EWMA_dynamic_measured );
  Serial.print (" at "); Serial.print ( ( millis() - timeBeganAt_ms ) / 1000.0 );
  Serial.print (" ( step "); Serial.print ( step ); Serial.println (" )");
 
  
  // calculate required delay
  delay_ms = int ( ( loopBeganAt_ms + DT_MS ) - millis() );
  if ( delay_ms < 0 ) {
    Serial.print ("ERROR: Loop took overlong at (ms) " ); Serial.println ( millis() - loopBeganAt_ms );
  } else {
    delay ( delay_ms + random (DT_NOISE) - DT_NOISE / 2);  // random noise centered at 0
  }
  loopTook_ms = millis() - loopBeganAt_ms;
  loopBeganAt_ms = millis();
  
  step++;
    
}
