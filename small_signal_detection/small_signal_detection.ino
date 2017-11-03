#define PROGNAME  "small_signal_detection.ino"
#define VERSION   "0.0.3"
#define VERDATE   "2017-09-28"

/*
  I have an LED flashing at 1.029kHz
  I want to detect its flashing despite very overwhelming ambient lighting,
  which will most likely include a DC component (sun) and a 60Hz AC component
  I will look at the 1.029kHz component of what I'm measuring as fast as I can
*/

#define BAUDRATE 115200

const int pdLED = 13;
const int pdPlotEnable = 32;  // corner to L of SD card socket
const int paSensor = A0;

const float pi = 3.1415926536;
const float halfpi = pi / 2.0;
const int sizeCalculatedHalfSine = 20;
const float sinTableScaleFactor = pi / 2.0 / ( sizeCalculatedHalfSine - 1 );
float calculatedHalfSine [ sizeCalculatedHalfSine ];

// frequency and period of the signal we are looking for
const float fSignal_hz = 1029.0;
const float pSignal_s = 1.0 / fSignal_hz;

// we will oversample each half-cycle by this factor
const int oversample = 10;
const unsigned long sampleDelay_us = 1000000.0 * pSignal_s / oversample;

// EWMA
/*
  each iteration reduces the weights of the old values by a factor
  of ( 1 - alpha ), so the Quarter-life happens when ( 1 - alpha ) ^ n = 0.5
  ln ( 1 - alpha ) * n = ln ( 0.5 ) ==> n = ln ( 0.5 ) / ln ( 1 - alpha ) or
  ln ( 1 - alpha ) = ln ( 0.5 ) / n ==> ( 1 - alpha ) = exp ( ln ( 0.5 ) / n )
    ==> alpha = 1 - exp ( ln ( 0.5 ) / n );
  The number of samples in t seconds is t / ( sampleDelay_us / 1e6 )
    = t * 1e6 / sampleDelay_us
  So if we want the Quarter-life to be 100ms, the number of samples will be
    0.1 * 1e6 / sampleDelay_us = 1e5 / sampleDelay_us;
  Then alpha will be 1 - exp ( ln ( 0.5 ) * sampleDelay_us / 1e5;
*/
const float halfLife_ms = 1000.0;
const float alpha = 1.0 - exp ( log ( 0.5 ) * sampleDelay_us / ( halfLife_ms * 1e3 ));


void setup () {
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 4000 );
  
  pinMode ( pdLED, OUTPUT );
  pinMode ( pdPlotEnable, INPUT_PULLUP );
  
  for ( int i = 0; i < sizeCalculatedHalfSine; i++ ) {
    calculatedHalfSine [ i ] = sin ( i * sinTableScaleFactor );
    // Serial.print ( i );
    // Serial.print ( ": sin ( " ); Serial.print ( i * sinTableScaleFactor );
    // Serial.print ( " ) = " ); Serial.print ( calculatedHalfSine [ i ] );
    // Serial.println ();
  }
  
  Serial.print ( "Oversample: " ); Serial.println ( oversample );
  Serial.print ( "Sample delay (us): " ); Serial.println ( sampleDelay_us );
  Serial.print ( "EWMA half life ( ms ): " ); Serial.println ( halfLife_ms );
  Serial.print ( "EWMA alpha * 1e6: " ); Serial.println ( alpha * 1e6 );
  
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 20 );

}

void loop () {

  static unsigned long lastBlinkAt_ms = 0UL;
  const unsigned long blinkDelay_ms = 500UL;

  static unsigned long lastPrintAt_ms = 0UL;
  const unsigned long printDelay_ms = 100UL;
  static unsigned long printStopAt_ms = millis() + 2000;

  static unsigned long lastSampleAt_us = 0UL;
  
  static float ewma [ 2 ] = { 0.0, 0.0 };  // I / Q demodulation
  const float magnification = 1.0e5;
  
  if ( ( micros() - lastSampleAt_us ) > sampleDelay_us ) {
    unsigned long timingLag_us = ( micros() - lastSampleAt_us - sampleDelay_us );
    int counts;
    float val;
    float phase, sinPhase, cosPhase, sProd, cProd, mag;
    
    lastSampleAt_us = micros();
    counts = analogRead ( paSensor );
    val = counts * 3.3 / 1024.0 * magnification;

    /*
      Choose some time t0; calculate phase relative to that and get the 
      single Fourier component of this. 
    */
    
    phase = fmod ( (float) micros() / 1000000.0, pSignal_s ) * 2.0 * pi / pSignal_s;
    // sinPhase = interpolateSin ( phase );
    // cosPhase = interpolateSin ( phase + halfpi );
    sinPhase = sin ( phase );
    cosPhase = cos ( phase );
    sProd = sinPhase * val;
    cProd = cosPhase * val;
    
    // to do digital I/Q demodulation, LP filter both these products separately
    ewma [ 0 ] = ( alpha * sProd ) + ( 1.0 - alpha ) * ewma [ 0 ];
    ewma [ 1 ] = ( alpha * cProd ) + ( 1.0 - alpha ) * ewma [ 1 ];
    
    mag = sqrt ( ewma [ 0 ] * ewma [ 0 ] + ewma [ 1 ] * ewma [ 1 ] );
    
    // simulate a lock-in amplifier
    // ref http://www.phys.utk.edu/labs/modphys/Lock-In%20Amplifier%20Experiment.pdf
    
    
    #define SLOW 0
    #define FAST 1
    #define PLOTTING SLOW
    
    #if PLOTTING == FAST
    
      // fast plotting: plot for 5 ms every 10 seconds
    
      static unsigned long lastPlotStartedAt_ms = 0UL;
      const unsigned long plotDelay_ms = 1000UL;
      static unsigned long lastPlotStartedAt_us = 0UL;
      const unsigned long plotFor_us = 100000UL;
    
      // activate fast plotting
      if ( digitalRead ( pdPlotEnable ) && ( millis() - lastPlotStartedAt_ms ) > plotDelay_ms ) {
        Serial.println ();
        lastPlotStartedAt_ms = millis();
        lastPlotStartedAt_us = micros();
      }
    
      unsigned long plotTime_us = micros() - lastPlotStartedAt_us;
      if ( plotTime_us < plotFor_us ) {
        const int lineLen = 81;
        char line [ lineLen + 3 ];
        // clear the print line
        for ( int i = 0; i < lineLen + 2; i++ ) {
          line [ i ] = ' ';
        }
        line [ lineLen + 2 ] = '\0';
        line [ 0 ] = '|';
        line [ lineLen + 1 ] = '|';
      
        line [ lineLen / 2 + 1] = ':';
        // plot the reading
        const float rdgRange = 1024.0;
        int rLoc = constrain ( ( counts / rdgRange ) * lineLen + 1, 1, lineLen );
        line [ rLoc ] = 'r';
      
        // raw sin and cos; scale down by setting full range to 4
        const float trigRange = 4.0;
        int tsLoc = constrain ( ( 0.5 + sinPhase / trigRange ) * lineLen + 1, 1, lineLen );
        line [ tsLoc ] = 's';
        int tcLoc = constrain ( ( 0.5 + cosPhase / trigRange ) * lineLen + 1, 1, lineLen );
        line [ tcLoc ] = 'c';
      
        // ewmas
        const float ewmaRange = 5000.0;
        int sLoc = constrain ( ( 0.5 + ewma [ 0 ] / ewmaRange ) * lineLen + 1, 1, lineLen );
        line [ sLoc ] = 'S';
        int cLoc = constrain ( ( 0.5 + ewma [ 1 ] / ewmaRange ) * lineLen + 1, 1, lineLen );
        line [ cLoc ] = 'C';
      
        // mag
        const float magRange = 5000.0;
        int mLoc = constrain ( ( mag / magRange ) * lineLen + 1, 1, lineLen );
        line [ mLoc ] = '*';
      
        if ( plotTime_us < 10UL ) Serial.print ( ' ' );
        if ( plotTime_us < 100UL ) Serial.print ( ' ' );
        if ( plotTime_us < 1000UL ) Serial.print ( ' ' );
        if ( plotTime_us < 10000UL ) Serial.print ( ' ' );
        if ( plotTime_us < 100000UL ) Serial.print ( ' ' );
        if ( plotTime_us < 1000000UL ) Serial.print ( ' ' );
        Serial.print ( plotTime_us );
        Serial.print ( " " );
        Serial.println ( line );
      }
    
    #endif   // fast PLOTTING
  
    #if PLOTTING == SLOW
  
      // slow plotting
    
      static unsigned long lastSlowPlotAt_ms = 0UL;
      const unsigned long slowPlotDelay_ms = 10UL;
    
      // activate slow plotting
      if ( digitalRead ( pdPlotEnable ) && ( ( millis() - lastSlowPlotAt_ms ) > slowPlotDelay_ms ) ) {
        lastSlowPlotAt_ms = millis();

        const int lineLen = 81;
        char line [ lineLen + 3 ];
        // clear the print line
        for ( int i = 0; i < lineLen + 2; i++ ) {
          line [ i ] = ' ';
        }
        line [ lineLen + 2 ] = '\0';
        line [ 0 ] = '|';
        line [ lineLen + 1 ] = '|';
      
        line [ lineLen / 2 + 1] = ':';
      
        // ewmas
        const float ewmaRange = 5000.0;
        int sLoc = constrain ( ( 0.5 + ewma [ 0 ] / ewmaRange ) * lineLen + 1, 1, lineLen );
        line [ sLoc ] = 'S';
        int cLoc = constrain ( ( 0.5 + ewma [ 1 ] / ewmaRange ) * lineLen + 1, 1, lineLen );
        line [ cLoc ] = 'C';
      
        // mag
        const float magRange = 5000.0;
        int mLoc = constrain ( ( mag / magRange ) * lineLen + 1, 1, lineLen );
        line [ mLoc ] = '*';
      
        Serial.print ( "Slow: " );
        Serial.println ( line );
      }
   
      if ( ( 0 && ( millis() - lastPrintAt_ms ) > printDelay_ms ) ) {
        if ( millis() > printStopAt_ms ) {
          Serial.print ( "timing lag (us) = " ); Serial.print ( timingLag_us );
          Serial.print ( "; ewma = ( " ); Serial.print ( ewma [ 0 ] );
            Serial.print ( ", " ); Serial.print ( ewma [ 1 ] );
            Serial.print ( " )" );
          Serial.print ( "; mag = " ); Serial.print ( mag );
          Serial.println ();
        } else {
          Serial.print ( "at " ); Serial.print ( micros() );
          Serial.print ( "us: phase = " ); Serial.print ( phase );
          Serial.print ( "; timing lag (us) = " ); Serial.print ( timingLag_us );
          Serial.print ( "; counts = " ); Serial.print ( counts );
          Serial.print ( "; val = " ); Serial.print ( val );
          Serial.print ( "; sinPhase = " ); Serial.print ( sinPhase );
          Serial.print ( "; cosPhase = " ); Serial.print ( cosPhase );
          Serial.print ( "; sProd = " ); Serial.print ( sProd );
          Serial.print ( "; cProd = " ); Serial.print ( cProd );
          Serial.print ( "; ewma = ( " ); Serial.print ( ewma [ 0 ] );
            Serial.print ( ", " ); Serial.print ( ewma [ 1 ] );
            Serial.print ( " )" );
          Serial.print ( "; mag = " ); Serial.print ( mag );
          Serial.println ();
        }
        lastPrintAt_ms = millis();
      }

    #endif   // SLOW PLOTTING
    
  }  // it was time to sample 
    
    
  if ( ( millis() - lastBlinkAt_ms ) > blinkDelay_ms ) {
    digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    lastBlinkAt_ms = millis();
  }
  
}

float interpolateSin ( float t ) {
  // note: cos ( t ) = sin ( t + pi / 2 )
  const float pi = 3.1415926536;
  const float halfpi = pi / 2.0;
  // place t into [ 0, 2 * pi )
  t = fmod ( t, 2.0 * pi );
  if ( t < 0 ) t += 2.0 * pi;
  // now get quadrant
  int quadrant = trunc ( t / halfpi );
  int sign;
  float tr;   // t, reduced to an interval [ 0, halfpi )
  switch ( quadrant ) {
    case 0:
      sign = 1;
      tr = t;
      break;
    case 1:
      sign = 1;
      tr = pi - t;
      break;
    case 2:
      sign = -1;
      tr = t - pi;
      break;
    case 3:
      sign = -1;
      tr = 2 * pi - t;
      break;
    default:
      sign = 0;
      tr = 0.0;
      break;
  }

  // now interpolate into table
  
  float tScaled = tr / sinTableScaleFactor;  // table index, start of interval FLOAT
  float tFactor = fmod ( tScaled, 1.0 );
  int ti = trunc ( tScaled );
  if ( ti < 0 ) ti = 0;
  if ( ti >= sizeCalculatedHalfSine ) ti = sizeCalculatedHalfSine;
  int tiNext = ti + 1;
  if ( tiNext > sizeCalculatedHalfSine ) tiNext = sizeCalculatedHalfSine;
  
  float y0 = calculatedHalfSine [ ti ];
  float y9 = calculatedHalfSine [ tiNext ];
  float retVal = ( ( 1.0 - tFactor ) * y0 + tFactor * y9 ) * ( float ) sign;
  
  return ( retVal );
  
}

