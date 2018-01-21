#ifndef DisplayableModule_h
#define DisplayableModule_h

#define DisplayableModule_VERSION "0.001.000"

#include <Arduino.h>
#include <Print.h>

#include <Audio.h>

#include "OpenEffectsBoxHW.h"

#define DisplayableModule_VERBOSE_DEFAULT 12

class DisplayableModule {

  public:
    DisplayableModule ();  // constructor
    void setVerbose ( int verbose );
    
    void init ( int id, char *name, OpenEffectsBoxHW *oebhw, int verbose = DisplayableModule_VERBOSE_DEFAULT );
    bool isActive ();
    void activate ( bool val );  // these just query and control the _isActive flag
    
    #define _DisplayableModule_VERBOSE_DEFAULT 2
    virtual void display (); 
  
  protected:
  
    int _verbose;
    int _id;
    #define nameStrLen 8
    char _name [ nameStrLen ];
    OpenEffectsBoxHW *_oebhw;
    
    bool _isActive;
    
  private:
    // since this is a base class, mostly everything needs to be "protected" rather than "private"
};

#endif