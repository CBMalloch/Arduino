#ifndef Utility_h
#define Utility_h

#define Utility_VERSION "0.001.001"

#include <Arduino.h>
#include <Print.h>

#define Utility_VERBOSE_DEFAULT 2

/*! \brief Utility subroutines.

*/

class Utility {

  public:
    Utility ();  // constructor
    void setVerbose ( int verbose );
    
    static float fmap ( float n, float n0, float n9, float y1, float y9 );
    static float fconstrain ( float x, float y1, float y9 );
    static float fmapc ( float n, float n0, float n9, float y1, float y9 );
    static float expmap ( float x, float fifty_pct_value = 0.2 );

  protected:
      
  private:
    int _verbose;

};

#endif