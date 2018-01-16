/*
	OpenEffectsBox - library to abstract the software of the AudioEffects
	web development toolkit
	for application to the Open Effects Project being developed by Øyvind Mjanger
	
	Created by Charles B. Malloch, PhD, January 10, 2018
	Released into the public domain
	
	Structure:
	  OpenEffectsBox - the overall open effects box
	  OpenEffectsBoxHW - defines and handles the switches, knobs, etc.
	  OpenEffectsBoxFW - the firmware - runs things, handles events, etc
	  OpenEffectsBoxModule - base type for individual effects
	
*/

#include <Arduino.h>
#include <Print.h>

#ifndef OpenEffectsBox_h
#define OpenEffectsBox_h

#include "OpenEffectsBoxHW.h"

#define OpenEffectsBox_VERSION "4.000.001"

class OpenEffectsBox {
  public:
  
    OpenEffectsBox ();  // constructor
    
    void init ();
    void tickle ();    
    
  protected:
    
  private:

    int _verbose;

    OpenEffectsBoxHW hw;
    void cbPots ( int pot, int newValue );
    void cbBat0 ( int newValue );
    void cbBat1 ( int newValue );
    void cbPBs ( int pb, int newValue );
    
};

class OpenEffectsBoxFW {

  public:
  
  protected:
  
  private:
  
};

class OpenEffectsBoxModule {

  public:
  
  protected:
  
  private:
  
};



#endif