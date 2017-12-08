#define BAUDRATE 115200

// #define IMPULSEINTERVAL_ms 2000

#define mmtInterval_ms          20
#define reportInterval_ms       20
#define graphChangeInterval_ms 100

#define max_counts 1023
#define ref_v 5.0

#define rSense_ohms 20.12

const int paVr = A4;  // Vr - voltage across the sense resistor
const int paVt = A5;  // Vt - total voltage across both LED and sense resistor

const float volts_per_count = ref_v / float ( max_counts );

float vt, ir;
#define sizePowers ( 1000 / mmtInterval_ms )
float powers [ sizePowers ];
int pPowers = 0;

void setup() {
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000UL ) delay ( 20 );
  
  for ( int i = 0; i < sizePowers; i++ ) {
    powers [ i ] = 0.0;
  }
  pPowers = 0;
  
  Serial.println ( "optimize piezo scratch v0.1 cbm 2016-10-22" );
}

void loop () {
  static unsigned long lastMmtAt_ms    = 0;
  static unsigned long lastReportAt_ms = 0;
  static unsigned long lastChangeAt_ms = 0;
  int Vt_cts, Vr_cts;
  static float lastVt = 0.0, lastIr = 0.0, lastPower_W = 0.0, lastEnergy_J = 0.0;
  
  if ( ( millis() - lastMmtAt_ms ) > mmtInterval_ms ) {
    Vt_cts = analogRead ( paVt );
    vt = Vt_cts * volts_per_count;
    Vr_cts  = analogRead ( paVr  );
    
    
    
    
    Serial.println ( Vr_cts );
    
    
    
    
    
    ir = Vr_cts * volts_per_count / rSense_ohms;
    powers [ pPowers ] = vt * ir;
    pPowers = ( pPowers >= sizePowers ) ? 0 : pPowers + 1;
    lastMmtAt_ms = millis();
  }
  
  if ( ( millis() - lastReportAt_ms ) > reportInterval_ms ) {
    // integrate powers wrt time to get energy
    float peakPower_W = 0.0;
    float energy_J = 0.0;
    for ( int i = 0; i < sizePowers; i++ ) {
      if ( powers [ i ] > peakPower_W ) peakPower_W = powers [ i ];
      energy_J += powers [ i ];  // multiply later by the interval size
    }
    energy_J *= float ( sizePowers ) * float ( mmtInterval_ms ) / 1000.0;  // seconds
    
    // detect changes in values
    if ( (   //  ( fabs ( vt          - lastVt        ) > 1e-2 )
              ( vt > lastVt )
            
            
            
           || 1
           
           
           
           
           
           || ( fabs ( ir          - lastIr       ) > 1e-6 )
           || ( fabs ( peakPower_W - lastPower_W  ) > 1e-3 )
           || ( fabs ( energy_J    - lastEnergy_J ) > 1e-3 ) ) ) {
      // something changed. we don't care what.
      lastVt      = vt;
      lastIr       = ir;
      lastPower_W  = peakPower_W;
      lastEnergy_J = energy_J;
      lastChangeAt_ms = millis();
    }
    
    if ( ( millis() - lastChangeAt_ms ) < graphChangeInterval_ms ) {
      // something changed recently, so report.
      // report units: mA, V, mW, mJ
      printValues ( ir * 1000.0, vt, peakPower_W * 1000.0, energy_J * 1000.0 );
      lastReportAt_ms = millis();
    }
  }

}

#define YMAX 5.0

float output_value ( float value, float origin, float scale ) {
  return ( YMAX * value / scale + origin );
}

void printValues ( float c0, float c1,  float c2, float c3 ) {
  if ( 1 ) {
    // Serial.print ( millis() ); Serial.print ( " :: " );
    Serial.print ( 0.0 );
    Serial.print ( " " );
    Serial.print ( YMAX );
    Serial.print ( " " );
    Serial.print ( c0, 6 );
    Serial.print ( " " );
    Serial.print ( c1, 6 );
    Serial.print ( " " );
    Serial.print ( c2, 6 );
    Serial.print ( " " );
    Serial.print ( c3, 6 );

    Serial.println ();
  } else if ( 0 ) {
    
  }
}