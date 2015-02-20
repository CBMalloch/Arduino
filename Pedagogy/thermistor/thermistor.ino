/*
  thermistor NTCLE100E3103JT2 (10K at 25degC)
  as developed for the Introduction to Arduino class
  with some minor enhancements
  Charles B. Malloch, PhD  2015-02-19
*/

#define BAUDRATE 115200
#define pdLED 13
#define paThermistor 0

#define full_volts 3.3
// rFixed is the resistance of the fixed (non-thermistor) resistor
// in class, we used 10K; I have a 22K handy
#define rFixed 22000
#define rFixed 23156.0

#define absoluteZero_degC -273.15

/*
  If ( as is not quite true ) resistance is inversely proportional to absolute temperature,
  then we need to calculate the constant of proportionality using the known point 
  that the resistance of this particular thermistor is 10K at 25degC.
  r <proportional to> 1 / T
  r = k * 1/T
  k = r * T
    = 10K * ( 25 - absoluteZero_degC )
    
  eventually we will use this again as
  k = r * T
  T = k / r
  
  OK. I stink. This relation is not even very close to true. I looked more closely
  at the datasheet and have conceded that we'll use the provided formula. We are given
    R(T) = Rref * e^(A + B/T + C/(T^2) + D/(T^3))
  and so
    T(R) = 1 / ( A1 + B1 ln ( R / Rref ) + C1 ln^2 ( R / Rref ) + D1 ln^3 ( R / Rref ) )
  where
    R(T) is the resistance at temperature T
    Rref is the reference resistance ( in our case, 10K )
    A, B, C, and D are coefficients given in the datasheet
    A1, B1, C1, and D1 are a different set of coefficients given in the datashee
    ^ is exponentiation ( e.g. 2^3 is 2 cubed = 8 )
    
*/

#define rRef 10000.0
#define coeffA1 3.354016E-03
#define coeffB1 2.569850E-04
#define coeffC1 2.620131E-06
#define coeffD1 6.383091E-08

float constantOfProportionality = 10000.0 * ( 25.0 - absoluteZero_degC );

void setup () {
  pinMode ( pdLED, OUTPUT );
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 10000 ) {
    digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
    delay ( 200 );
  }
}

void loop () {
  int counts;
  float sensed_volts, thermistor_ohms, temperature_degK, temperature_degC, temperature_degF;
  
  counts = analogRead ( paThermistor );
  /*
    We have assumed that sensed_volts is the voltage across the thermistor,
    which stems from the assumption that the thermistor is what is between 
    the sense wire ( connected to analog pin 0 ) and ground.
    If in fact the thermistor is connected between the sense wire and power
    ( meaning the fixed resistor is between sense and ground ) we have to reverse
    this. This is accomplished by noting that the full voltage is the sum of the voltage
    across the thermistor and the voltage across the fixed resistor, so if what we 
    currently have is the voltage across the fixed resistor, we subtract this from 
    full_volts to get the voltage across the thermistor.
    
    sensed_volts / counts = full_volts / 1023
    sensed_volts = full_volts * ( counts / 1023 )
    
  */
  // float ( counts ) converts counts to floating-point so everything is now floating point
  sensed_volts = full_volts * ( float ( counts ) / 1023.0 );
  if ( 0 ) {
    // changing 0 to 1 will reverse the sense of the measured volts as described above.
    sensed_volts = full_volts - sensed_volts;
  }
  
  /*
    Calculate the resistance of the thermistor from the voltage across it.
    thermistor_ohms / sensed_volts = ( thermistor_ohms + rFixed ) / full_volts
    thermistor_ohms * full_volts = ( thermistor_ohms + rFixed ) * sensed_volts
    thermistor_ohms * full_volts = thermistor_ohms * sensed_volts + rFixed * sensed_volts
    thermistor_ohms * full_volts - thermistor_ohms * sensed_volts = rFixed * sensed_volts
    thermistor_ohms * ( full_volts - sensed_volts ) = rFixed * sensed_volts
    thermistor_ohms = rFixed * sensed_volts / ( full_volts - sensed_volts )
  */
  thermistor_ohms = rFixed * sensed_volts / ( full_volts - sensed_volts );
  
  /* 
    Now use the relation from the top of the program to get the temperature
  */
  // NOT temperature_degK = constantOfProportionality / thermistor_ohms;
  
  // since it's used several times, calculate it once
  float theLog = log ( thermistor_ohms / rRef );  // note log here is ln, or natural log
  temperature_degK = 1.0 / ( coeffA1 
                           + coeffB1 * theLog 
                           + coeffC1 * theLog * theLog
                           + coeffD1 * theLog * theLog * theLog );
  
  temperature_degC = temperature_degK + absoluteZero_degC;
  
  temperature_degF = temperature_degC * 9.0 / 5.0 + 32.0;
  
  // 1 = logic TRUE. So 1 will print the results every half-second, and 
  // 0 will activate the else part which will do the bouncing ball thing.
  
  if ( 1 ) {
  
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    //    print results every half-second
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    Serial.print ( "cts: " );
    Serial.print ( counts );
    Serial.print ( "; v: ");
    Serial.print ( sensed_volts );
    Serial.print ( "; r: " );
    Serial.print ( thermistor_ohms );
    Serial.print ( "; K: " );
    Serial.print ( temperature_degK );
    Serial.print ( "; C: " );
    Serial.print ( temperature_degC );
    Serial.print ( "; F: " );
    Serial.print ( temperature_degF );
    Serial.println ();
    
    delay ( 500 );
  
  } else {
  
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    //    display results as bouncing ball
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

    const int line_length = 60;
    int nSpaces;
    nSpaces = round ( ( temperature_degF - 60.0 ) * 3.0 );
    // create the space to the left of the asterisk
    for ( int i = 0; i < nSpaces; i++ ) {
      Serial.print ( " " );
    }
    Serial.println ( "*" );
    delay ( 200 );
   
  }
  
}