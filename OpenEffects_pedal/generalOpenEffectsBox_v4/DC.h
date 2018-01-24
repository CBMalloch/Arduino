#ifndef DC_h
#define DC_h

#define DC_VERSION "0.000.001"

#include "DisplayableModule.h"

#define DC_VERBOSE_DEFAULT 12
    
class DC : public DisplayableModule {

  public:
    DC ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioSynthWaveformDc *dc, OpenEffectsBoxHW *oebhw, int verbose = DC_VERBOSE_DEFAULT );
    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setLevel ( float level );    
  
  protected:
  
  private:
    
    float _level;
    
    AudioSynthWaveformDc *_dc;
};

#endif