#ifndef Chorus_h
#define Chorus_h

#define Chorus_VERSION "0.000.001"

#include "DisplayableModule.h"

#define Chorus_VERBOSE_DEFAULT 12
    
/*! \brief Wrapper for Audio Design Tool -Chorus-.

  See [Paul Stoffregen's Audio Design Tool](https://www.pjrc.com/teensy/gui/)
 
*/

class Chorus : public DisplayableModule {

  public:
    Chorus ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioEffectChorus *chorus, OpenEffectsBoxHW *oebhw, int verbose = Chorus_VERBOSE_DEFAULT );
    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setNVoices ( int nVoices );    
  
  protected:
  
  private:
    
    static const int CHORUS_DELAY_LENGTH = ( 16 * AUDIO_BLOCK_SAMPLES );
    short _chorusDelayLine [ CHORUS_DELAY_LENGTH ];

    int _nVoices;
    bool _inited;
    
    AudioEffectChorus *_chorus;
};

#endif