#ifndef DelayExt_h
#define DelayExt_h

#define DelayExt_VERSION "0.001.002"

#include "DisplayableModule.h"

#define DelayExt_VERBOSE_DEFAULT 12
    
/*! \brief Wrapper for Audio Design Tool -Reverb-.

  See [Paul Stoffregen's Audio Design Tool](https://www.pjrc.com/teensy/gui/)
  
  I have added a 23LC1024 memory chip to the OpenEffects Project hardware board 
  where provision was made for it by Øyvind Mjanger, the developer of the project.
  This chip allows for up to 1 1/2 seconds of delay to be added to the input
  signal, in each of 8 parallel channels. I've programmed only four of these.
  
*/

class DelayExt : public DisplayableModule {

  public:
    DelayExt ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioEffectDelayExternal *delayExt, OpenEffectsBoxHW *oebhw, int verbose = DelayExt_VERBOSE_DEFAULT );
    void init ( int id, char *name, AudioEffectDelayExternal *delayExt, OpenEffectsBoxHW *oebhw, int ch0, int ch1, int ch2, int ch3, int verbose = DelayExt_VERBOSE_DEFAULT );

    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setDelay ( int channel, int delay_ms );    
  
  protected:
  
  private:
    
    static const int nChannels = 4;
    int _delays_ms [ nChannels ];
    
    AudioEffectDelayExternal *_delayExt;
};

#endif