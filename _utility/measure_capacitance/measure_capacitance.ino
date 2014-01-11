#define VERSION "0.0.1"
#define VERDATE "2013-08-05"
#define PROGMONIKER "CAP"

// measure capacitance
// try to have a time constant 1-10 sec, so choose a resistance 1.0 / C to 10.0 / C
// 1000 uF -> 1K - 10K
// 10 uF -> 100K - 1M

#define BAUDRATE 115200

#define resistance 6.78e3

#define pdCharge     8
#define pdChargeBtn 12
#define pdChargeLED 11

#define paMeasure   A0

#define BUFLEN 128
char strBuf [ BUFLEN + 1 ];

void setup () {
  Serial.begin ( BAUDRATE );
  while ( !Serial );
  
  Serial.begin(BAUDRATE);
	snprintf(strBuf, BUFLEN, "%s: Capacitance Measurement v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);
 
 analogReference ( DEFAULT );
  
  pinMode ( pdCharge,     INPUT );
  pinMode ( pdChargeBtn,  INPUT );
  pinMode ( pdChargeLED, OUTPUT );
  
  digitalWrite ( pdCharge,    0 );
  digitalWrite ( pdChargeBtn, 1 );
  digitalWrite ( pdChargeLED, 0 );
  
}

void loop () {
  
  static unsigned long chargeLastClimbedAt_ms = 0, beganTestingAt_ms = 0, beganMeasuringAt_ms = 0;
  static int charging = 0, testing = 0, measuring = 0;
  static int chargeReading = 0, valueAtTestStart = 0;
  static float chargeReadingEWMA, peakChargeEWMA, peakChargeReadingEWMA;
  static float tcAVG;
  static int half_life = 0.1;
  static float alpha = 1.0 - pow ( 0.5, 1.0 / half_life );
  
  #define MAXLOOPS 6
  static float elapsedTimes [ MAXLOOPS ], targetValues [ MAXLOOPS ];
  static int loopCt;
  #define tc_factor 0.5
  // decay should be governed by v = v0 x e(  - ( t-t0 ) / tc )
  // so at tcf * tc, we should be at factor e ( - ( tcf * tc ) / tc ) or e ( - tcf )
  // should reach targetValue of 0.6065 x v0 at tc/2; 0.3679 x v0 at tc
  static float targetFactor = exp ( - tc_factor );
  static float targetValue;
  
  if ( ( ! digitalRead ( pdChargeBtn ) ) && ( ! charging ) && ( ! measuring ) ) {
    // button is down. start the cycle
    testing = 0;
    measuring = 0;
    peakChargeReadingEWMA = 0.0;
    if ( ! charging ) {
      pinMode ( pdCharge, OUTPUT );
      digitalWrite ( pdCharge, 1 );
      charging = 1;
      chargeLastClimbedAt_ms = millis();
      digitalWrite ( pdChargeLED, 1 );
      chargeReadingEWMA = analogRead ( paMeasure );
      Serial.println ( "charging" );
    }
    while ( ! digitalRead ( pdChargeBtn ) ) {
      delay ( 200 );
    }
  }
    
  if ( charging ) {
    // we're in the process of charging
    chargeReading = analogRead ( paMeasure );
    analogWrite ( pdChargeLED, int ( chargeReadingEWMA ) / 4 );
    chargeReadingEWMA = ( 1.0 - alpha ) * chargeReadingEWMA + alpha * chargeReading;
    if ( chargeReadingEWMA > peakChargeReadingEWMA ) {
      peakChargeReadingEWMA = chargeReadingEWMA;
    }
    if ( chargeReadingEWMA > peakChargeReadingEWMA + 0.5 ) {
      // climbing
      chargeReadingEWMA = ( 1.0 - alpha ) * chargeReadingEWMA + alpha * chargeReading;
      chargeLastClimbedAt_ms = millis();
      // Serial.print ( "climbing: " ); Serial.print ( chargeReading );
      // Serial.print ( " > " ); Serial.println ( chargeReadingEWMA );
    } else {
      // we've peaked. 
      if ( chargeReadingEWMA > 500.0 ) { 
        // good charge. start the measurement process
        peakChargeEWMA = chargeReadingEWMA;
        charging = 0;
        digitalWrite ( pdChargeLED, 0 );
        pinMode ( pdCharge, INPUT );
        digitalWrite ( pdCharge, 0 );
        testing = 1;
        beganTestingAt_ms = millis();
        valueAtTestStart = chargeReading;
        // Serial.println ( "charged -> testing" );
      } else if ( ( millis () - chargeLastClimbedAt_ms ) > 20000L ) {
        Serial.print ( "Incomplete charge of " ); Serial.print ( chargeReadingEWMA );
        Serial.print ( " after " ); Serial.print ( millis () - chargeLastClimbedAt_ms );
        Serial.println ( " ms" );
        charging = 0;
        digitalWrite ( pdChargeLED, 0 );
      }
      // else just keep waiting
    }  // peaked
  }  // charging
  
  if ( testing ) {
    chargeReading = analogRead ( paMeasure );
    // analogWrite ( pdChargeLED, int ( chargeReadingEWMA ) / 4 );
    // chargeReadingEWMA = ( 1.0 - alpha ) * chargeReadingEWMA + alpha * chargeReading;
  
    if ( ( millis () - beganTestingAt_ms ) > 10000L ) {
      // done testing phase
      Serial.print ( "Leakage over 10 s: " ); 
      Serial.print ( ( valueAtTestStart - chargeReading ) * 5.0 / 1024.0 );
      Serial.println ( "v" );
      testing = 0;
      measuring = 1;
      targetValue = chargeReading * targetFactor;
      // Serial.print ( "Current value: " ); Serial.println ( chargeReading );
      // Serial.print ( "Target value: " ); Serial.println ( targetValue );
      pinMode ( pdCharge, OUTPUT );
      digitalWrite ( pdCharge, 0 );
      beganMeasuringAt_ms = millis();
      // targetValue = peakChargeEWMA * targetFactor;
      loopCt = 0;
      // Serial.println ( "tested -> measuring" );
      digitalWrite ( pdChargeLED, 1 );
    }
  }

  if ( measuring ) {
    chargeReading = analogRead ( paMeasure );
    if ( ( chargeReading < targetValue ) && ( loopCt < MAXLOOPS ) ) {
      // expecting that tc_factor * tc time has elapsed
      unsigned long elapsed = millis() - beganMeasuringAt_ms;
      // Serial.print ( "Elapsed time " ); Serial.print ( elapsed );
      // Serial.print ( " ms; " ); Serial.print ( targetValue );
      // Serial.println ();
      elapsedTimes [ loopCt ] = elapsed;
      targetValues [ loopCt ] = targetValue;
      beganMeasuringAt_ms = millis();
      targetValue = targetValue * targetFactor;
      if ( loopCt == 0 ) tcAVG = 0.0;
      tcAVG += elapsed;
      loopCt++;
      digitalWrite ( pdChargeLED, 1 - digitalRead ( pdChargeLED ) );
    }
    if ( loopCt >= MAXLOOPS ) {
      #define VOLTS_PER_COUNT ( 5.0 / 1024.0 )
      Serial.print ( "Peak charging voltage: " ); Serial.println ( peakChargeEWMA * VOLTS_PER_COUNT );
      for ( int i = 0; i < MAXLOOPS; i++ ) {
        Serial.print ( "Voltage level " ); Serial.print ( targetValues [ i ] * VOLTS_PER_COUNT );
        Serial.print ( ": elapsed time " ); Serial.print ( elapsedTimes [ i ] );
        Serial.print ( " ms; " );
        Serial.println ();
      }
      tcAVG = tcAVG / tc_factor / float ( loopCt );
      Serial.print ( "tc: " ); Serial.println ( tcAVG );
      // tc is in ms, so divide by 1e3; want output in uF, so multiply by 1e6; result, multiply by 1e3
      Serial.print ( "capacitance measured: " );
      Serial.print ( tcAVG / resistance * 1e3 ); Serial.println ( " uF" );
      measuring = 0;
      digitalWrite ( pdChargeLED, 0 );
      Serial.println ( "Done." );
    }
  }
  
  // delay ( 5 );
  
}
