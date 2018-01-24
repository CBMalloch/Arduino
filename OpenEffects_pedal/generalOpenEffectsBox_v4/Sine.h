#ifndef Sine_h
#define Sine_h

#define Sine_VERSION "0.000.001"

#include "DisplayableModule.h"

#define Sine_VERBOSE_DEFAULT 12
    
class Sine : public DisplayableModule {

  public:
    Sine ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioSynthWaveformSine *sine, OpenEffectsBoxHW *oebhw, int verbose = Sine_VERBOSE_DEFAULT );
    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setFrequency ( float frequency );    
  
  protected:
  
  private:
    
    float _frequency;
    
    AudioSynthWaveformSine *_sine;
};

#endif