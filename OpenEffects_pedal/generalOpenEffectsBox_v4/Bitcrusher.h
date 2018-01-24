#ifndef Bitcrusher_h
#define Bitcrusher_h

#define Bitcrusher_VERSION "0.001.000"

#include "DisplayableModule.h"

#define Bitcrusher_VERBOSE_DEFAULT 12
    
class Bitcrusher : public DisplayableModule {

  public:
    Bitcrusher ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, AudioEffectBitcrusher *_bitcrusher, OpenEffectsBoxHW *oebhw, int verbose = Bitcrusher_VERBOSE_DEFAULT );

    void notify ( int channel, float value );
    void display ( int mode, int subMode, bool force = false );
    
    void setNBits ( int nBits );
    void setSampleRate ( unsigned int sampleRate );
  
  protected:
  
  private:
  /*
    int _verbose;
    int _id;
    #define nameStrLen 8
    char _name [ nameStrLen ];
    OpenEffectsBoxHW *_oebhw;
  */
    
    static const int nChannels = 2;
    int _nBits;
    unsigned int _sampleRate;
    
    AudioEffectBitcrusher *_bitcrusher;
};

#endif