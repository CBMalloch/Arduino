#ifndef Reverb_h
#define Reverb_h

#define Reverb_VERSION "0.000.002"

#include "DisplayableModule.h"

#define Reverb_VERBOSE_DEFAULT 12
    
/*! \brief Wrapper for Audio Design Tool -Reverb-.

  See [Paul Stoffregen's Audio Design Tool](https://www.pjrc.com/teensy/gui/)
   
*/

class Reverb : public DisplayableModule {

  public:
    Reverb ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioEffectReverb *reverb, OpenEffectsBoxHW *oebhw, int verbose = Reverb_VERBOSE_DEFAULT );
    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setReverbTime ( float reverbTime_s );    
  
  protected:
  
  private:
    
    float _reverbTime_s;
    
    AudioEffectReverb *_reverb;
};

#endif