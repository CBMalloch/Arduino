// cbmThermistor.h
#ifndef cbmthermistor_h
#define cbmthermistor_h

/*
  Stuff to include into your Arduino program:

#include <cbmThermistor.h>

cbmThermistor thermistorInletA5  ( 9881.9 );
cbmThermistor thermistorOutletA4 ( 9874.5 );

*/

class cbmThermistor {
  /*
    Thermistor, by default a Murata NTSD1XH103FPBxx, 10K @ 25degC
    
    Uses the Steinhart-Hart equation ( see Wikipedia )
    The Steinhart-Hart equation is
      1 / T = a + b ln(R) + c ln(R)^3
      where T is degK and R is ohms
      
    Accepts the case where the thermistor is the upper half of a resistance
    divider whose other half has a fixed resistance Rfixed

    Routines to support blue thermistors NTSD1XH103FPBxx
    10K @ 25degC

    Used Excel to solve for the Steinhart-Hart constants a, b, and c
    given table of temperatures vs. resistances at 
      http://search.murata.co.jp/Ceramy/CatalogframeAction.do?sParam=ntsd1x&sKno=G005&sTblid=A116&sDirnm=A11X&sFilnm=NTH4GG03&sType=0&sLang=en&sNHinnm=NTSD1XH103FPB30&sCapt=Resistance_VS._Temperature
    accessed from 
      http://search.murata.co.jp/Ceramy/CatalogAction.do?sHinnm=?%A0&sNHinnm=NTSD1XH103FPB30&sNhin_key=NTSD1XH103FPB30&sLang=en&sParam=ntsd1x

  */
  public:
  
    cbmThermistor ();
    cbmThermistor ( double Rfixed );
    void init ( );
    void init ( double Rfixed );
    
    double degKfromR ( double R );
    double degFfromR ( double R );
    double degCfromK ( double degK );
    double degFfromC ( double degC );
    double degFfromCounts ( int counts );
    double degKfromR_corr ( double R );
    double diss_mW ( double R );
    double deltaT_diss ( double mW );
    double degFfromR_corr ( double R );
    
  private:
    
    double Rfixed;
    double kA, kB, kC;
    int maxCounts;
    double dissipation_constant_mW_per_degC, v;
    int verbose;
    
    double defaultA = 0.00146534863058156;
    double defaultB = 0.000159278374004741;
    double defaultC = 0.0000005360002751;
  
    double defaultRfixed = 9878.0;

    const double absoluteZero_degK = -273.15;
};
 
#endif
