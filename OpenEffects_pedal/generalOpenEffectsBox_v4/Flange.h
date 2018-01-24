#ifndef Flange_h
#define Flange_h

#define Flange_VERSION "0.000.001"

#include "DisplayableModule.h"

#define Flange_VERBOSE_DEFAULT 12
    
class Flange : public DisplayableModule {

  public:
    Flange ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioEffectFlange *flange, OpenEffectsBoxHW *oebhw, int verbose = Flange_VERBOSE_DEFAULT );
    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setOffset ( int offset );    
    void setDepth ( int depth );    
    void setDelayFreq ( float delayFreq );    
  
  protected:
  
  private:
    
    #if 1
    static const int FLANGE_DELAY_LENGTH = ( 2 * AUDIO_BLOCK_SAMPLES );
    int s_idx = 2 * FLANGE_DELAY_LENGTH / 4;
    int s_depth = FLANGE_DELAY_LENGTH / 4;
    double s_freq = 3;
    #else
    static const int FLANGE_DELAY_LENGTH = ( 12 * AUDIO_BLOCK_SAMPLES );
    int s_idx = 3 * FLANGE_DELAY_LENGTH / 4;
    int s_depth = FLANGE_DELAY_LENGTH / 8;
    double s_freq = 0.0625;
    #endif
    short _flangeDelayLine [ FLANGE_DELAY_LENGTH ];

    int _offset;
    int _depth;
    float _delayFreq;
    bool _inited;
    
    void go ();
    
    AudioEffectFlange *_flange;
};

#endif