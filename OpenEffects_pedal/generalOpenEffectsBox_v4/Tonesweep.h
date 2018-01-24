#ifndef Tonesweep_h
#define Tonesweep_h

#define Tonesweep_VERSION "0.000.001"

#include "DisplayableModule.h"

#define Tonesweep_VERBOSE_DEFAULT 12
    
class Tonesweep : public DisplayableModule {

  public:
    Tonesweep ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioSynthToneSweep *sine, OpenEffectsBoxHW *oebhw, int verbose = Tonesweep_VERBOSE_DEFAULT );
    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setLFrequency ( float frequency );
    void setHFrequency ( float frequency );
    void setSweepTime ( float sweep_time );
    void tickle ();
  
  protected:
  
  private:
    
    float _lFrequency;
    float _hFrequency;
    float _sweepTime;
    
    AudioSynthToneSweep *_tonesweep;
};

#endif