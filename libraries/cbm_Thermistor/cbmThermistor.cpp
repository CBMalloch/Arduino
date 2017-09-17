#include <math.h>
#include <cbmThermistor.h>
  
cbmThermistor::cbmThermistor() {
  init ( defaultRfixed );
}

cbmThermistor::cbmThermistor( double measuredRfixed ) {
  init ( measuredRfixed );
}

void cbmThermistor::init ( ) {
  init ( defaultRfixed );
}

void cbmThermistor::init ( double measuredRfixed ) {
    kA                               = defaultA;
    kB                               = defaultB;
    kC                               = defaultC;
    
    Rfixed                           = measuredRfixed;
    maxCounts                        = 1023;
    dissipation_constant_mW_per_degC = 1.5;
    v                                = 3.3;
    
    verbose                          = 1;
}

double cbmThermistor::degKfromR ( double R ) {
  if ( R <= 0.0 ) return ( 0.0 );
  double lnR = log ( R );
  double invT = kA + kB * lnR + kC * lnR * lnR * lnR;
  if ( fabs ( invT ) > 1e-12 ) {
    return ( 1.0 / invT );
  } else {
    return ( absoluteZero_degK );
  }
}

double cbmThermistor::degCfromK ( double degK ) {
  return ( degK + absoluteZero_degK );
}

double cbmThermistor::degFfromC ( double degC ) {
  return ( degC * 1.8 + 32.0 );
}
   
double cbmThermistor::degFfromCounts ( int counts ) {
  double R = ( maxCounts - counts ) * Rfixed / counts;
  return ( degFfromC ( degCfromK ( degKfromR ( R ) ) ) );
}

double cbmThermistor::degFfromR ( double R ) {
    return ( degFfromC ( degCfromK ( degKfromR ( R ) ) ) );
}

# Note that, properly calculated now, the dissipation is less than 1 millidegC, and so can be ignored

double cbmThermistor::degKfromR_corr ( double R ) {
  return ( degKfromR ( R ) - deltaT_diss ( diss_mW ( R ) ) );
}
  
double cbmThermistor::diss_mW ( double R ) {
  double i = v / ( R + Rfixed );
  return ( i * i * R );
    
double cbmThermistor::deltaT_diss ( double mW ) {
  return ( mW / dissipation_constant_mW_per_degC );
}
   
double cbmThermistor::degFfromR_corr ( double R ) {
  return ( degFfromC ( degCfromK ( degKfromR_corr ( R ) ) ) );
}
