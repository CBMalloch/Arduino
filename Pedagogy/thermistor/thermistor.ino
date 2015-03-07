/*
  thermistor NTCLE100E3103JT2 (10K at 25degC)
  as developed for the Introduction to Arduino class
  with some minor enhancements
  Charles B. Malloch, PhD  2015-02-19
*/

#include <ANSI.h>

#define BAUDRATE 115200
#define pdLED 13
#define paThermistor 0

const float zeroC_degK          =   273.15; 

const float powerRail_v         =     3.3;
// divider_Rfixed_ohms is the resistance of the fixed (non-thermistor) resistor
// in class, we used 10K; I have a 22K handy
// and measured it carefully for an actual value
const float divider_Rfixed_ohms = 23156.0;  // nominally 22000
const float rRef_ohms           = 10000.0;
const float tRef_degC           =    25.0;
const float tRef_degK           = tRef_degC + zeroC_degK;

const int line_length = 60;

/*
  If ( as is not quite true ) resistance is inversely proportional to absolute temperature,
  then we need to calculate the constant of proportionality using the known point 
  that the resistance of this particular thermistor is 10K at 25degC.
  r <proportional to> 1 / T
  r = k * 1/T
  k = r * T
    = 10K * tRef_degK
    
  eventually we will use this again as
  k = r * T
  T = k / r
  
  OK. I stink. This relation is not even very close to true. I looked more closely
  at the datasheet and have conceded that we'll use the provided formula. We are given
    R(T) = rRef_ohms * e^(A + B/T + C/(T^2) + D/(T^3))
  and so
    T(R) = 1 / ( A1 + B1 ln ( R / rRef_ohms ) + C1 ln^2 ( R / rRef_ohms ) + D1 ln^3 ( R / rRef_ohms ) )
  where
    R(T) is the resistance at temperature T
    rRef_ohms is the reference resistance ( in our case, 10K )
    A, B, C, and D are coefficients given in the datasheet
    A1, B1, C1, and D1 are a different set of coefficients given in the datashee
    ^ is exponentiation ( e.g. 2^3 is 2 cubed = 8 )
    
*/

#define coeffA1 3.354016E-03
#define coeffB1 2.569850E-04
#define coeffC1 2.620131E-06
#define coeffD1 6.383091E-08

float constantOfProportionality = rRef_ohms * ( tRef_degK );

void setup () {
  pinMode ( pdLED, OUTPUT );
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 10000 ) {
    digitalWrite ( pdLED, ! digitalRead ( pdLED ) );
    delay ( 50 );
  }
  
  Serial.println ( "thermistor.ino 2.0" );
  
}

void loop () {
  int counts, nSpaces;
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
    powerRail_v to get the voltage across the thermistor.
    
    sensed_volts / counts = powerRail_v / 1023
    sensed_volts = powerRail_v * ( counts / 1023 )
    
  */
  // float ( counts ) converts counts to floating-point so everything is now floating point
  sensed_volts = powerRail_v * ( float ( counts ) / 1023.0 );
  if ( 0 ) {
    // changing 0 to 1 will reverse the sense of the measured volts as described above.
    sensed_volts = powerRail_v - sensed_volts;
  }
  
  /*
    Calculate the resistance of the thermistor from the voltage across it.
    thermistor_ohms / sensed_volts = ( thermistor_ohms + divider_Rfixed_ohms ) / powerRail_v
    thermistor_ohms * powerRail_v = ( thermistor_ohms + divider_Rfixed_ohms ) * sensed_volts
    thermistor_ohms * powerRail_v = thermistor_ohms * sensed_volts + divider_Rfixed_ohms * sensed_volts
    thermistor_ohms * powerRail_v - thermistor_ohms * sensed_volts = divider_Rfixed_ohms * sensed_volts
    thermistor_ohms * ( powerRail_v - sensed_volts ) = divider_Rfixed_ohms * sensed_volts
    thermistor_ohms = divider_Rfixed_ohms * sensed_volts / ( powerRail_v - sensed_volts )
  */
  thermistor_ohms = divider_Rfixed_ohms * sensed_volts / ( powerRail_v - sensed_volts );
  
  /* 
    Now use the relation from the top of the program to get the temperature
  */
  // NOT temperature_degK = constantOfProportionality / thermistor_ohms;
  
  // since it's used several times, calculate it once
  float theLog = log ( thermistor_ohms / rRef_ohms );  // note log here is ln, or natural log
  temperature_degK = 1.0 / ( coeffA1 
                           + coeffB1 * theLog 
                           + coeffC1 * theLog * theLog
                           + coeffD1 * theLog * theLog * theLog );
  
  temperature_degC = temperature_degK - zeroC_degK;
  
  temperature_degF = temperature_degC * 9.0 / 5.0 + 32.0;
  
  // 'P' will print the results every half-second,
  // 'S' will print the strip chart,
  // 'B' will do the bouncing ball thing on one line
  
  char myChoice = 'B';
  
  switch ( myChoice ) {
  
    case 'P':
    
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
      break;
      
    case 'S':
  
      // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      //    display results on moving strip chart
      // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

      nSpaces = round ( ( temperature_degF - 60.0 ) * 3.0 );
      // create the space to the left of the asterisk
      for ( int i = 0; i < nSpaces; i++ ) {
        Serial.print ( " " );
      }
      Serial.println ( "*" );
      delay ( 200 );
      break;

    case 'B':
  
      // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      //    display results as bouncing ball
      // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

      nSpaces = round ( ( temperature_degF - 60.0 ) * 3.0 );
      curpos ( 2, 1 );
      clr2EOL ();
      curpos ( 2, nSpaces + 1 );
      Serial.println ( "*" );
      delay ( 50 );
      break;
      
  }
  
  digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
  
}