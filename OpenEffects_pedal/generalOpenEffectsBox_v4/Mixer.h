#ifndef Mixer_h
#define Mixer_h

#define Mixer_VERSION "0.001.002"

#include "DisplayableModule.h"

#define Mixer_VERBOSE_DEFAULT 12
    
/*! \brief Wrapper for Audio Design Tool -Filter-.

  See [Paul Stoffregen's Audio Design Tool](https://www.pjrc.com/teensy/gui/)
 
*/

class Mixer : public DisplayableModule {

  public:
    Mixer ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioMixer4 *mixer, OpenEffectsBoxHW *oebhw, int verbose = Mixer_VERBOSE_DEFAULT );
    void init ( int id, char *name, AudioMixer4 *mixer, OpenEffectsBoxHW *oebhw, int ch0, int ch1, int ch2, int ch3, int verbose = Mixer_VERBOSE_DEFAULT );

    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setGain ( int channel, float gain );    
  
  protected:
  
  private:
  /*
    int _verbose;
    int _id;
    #define nameStrLen 8
    char _name [ nameStrLen ];
    OpenEffectsBoxHW *_oebhw;
  */
    
    static const int nChannels = 4;
    float _gains [ nChannels ];
    
    AudioMixer4 *_mixer;
};

#endif