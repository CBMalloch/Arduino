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

#ifndef OpenEffectsBox_h
#define OpenEffectsBox_h

#include "OpenEffectsBoxHW.h"

#define OpenEffectsBox_VERSION "4.000.004"

#include <Arduino.h>
#include <Print.h>

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Mixer.h"
#include "Sine.h"

#define OpenEffectsBox_VERBOSE_DEFAULT 12
#define OpenEffectsBox_NMODES 7
    
class OpenEffectsBox {
  public:
  
    OpenEffectsBox ();  // constructor
    void setVerbose ( int i );
    
    void init ( int verbose = OpenEffectsBox_VERBOSE_DEFAULT );
    void tickle ();    
    
  protected:
    
  private:

    int _verbose;
    #define nModes OpenEffectsBox_NMODES
    int _nSubModes;
    int _mode, _subMode;
    
    void setDisplay ( DisplayableModule *who );
    DisplayableModule *_displayableModule;
    
    bool _onOffP, _boostP;
    
    OpenEffectsBoxHW _hw;
    
    void sgtl5000_init ();

    void cbPots ( int pot, int newValue );
    void cbBat0 ( int newValue );
    void cbBat1 ( int newValue );
    void cbPBs ( int pb, int newValue );
    void cbPBOnOff ( int newValue );
    void cbPBBoost ( int newValue );
    void cbPedals ( int pedal, int newValue );
    
    #define _nMixers 4
    Mixer _mixers [ _nMixers ];
    #define _nSines 1
    Sine _sines [ _nSines ];
    
    void setOutputVolume ( float volume );

    /* *****************************************************
        audio system definitions and variable setups
    // *****************************************************/

    #define inputMixer              0
    #define input_mixer_input_L     0
    #define input_mixer_input_R     1
    #define input_mixer_tuning_sine 2
    #define input_mixer_tonesweep1  3
    #define mpyMixer                2
    #define delayMixer              1
    #define outputMixer             3
    
    #define inputSine               0

    #if 1
    #define FLANGE_DELAY_LENGTH ( 2 * AUDIO_BLOCK_SAMPLES )
    int s_idx = 2 * FLANGE_DELAY_LENGTH / 4;
    int s_depth = FLANGE_DELAY_LENGTH / 4;
    double s_freq = 3;
    #else
    #define FLANGE_DELAY_LENGTH ( 12 * AUDIO_BLOCK_SAMPLES )
    int s_idx = 3 * FLANGE_DELAY_LENGTH / 4;
    int s_depth = FLANGE_DELAY_LENGTH / 8;
    double s_freq = 0.0625;
    #endif
    short flangeDelayLine [ FLANGE_DELAY_LENGTH ];
    #define CHORUS_DELAY_LENGTH ( 16 * AUDIO_BLOCK_SAMPLES )
    short chorusDelayLine [ CHORUS_DELAY_LENGTH ];
    #define mpy_mixer_DC          0
    #define mpy_mixer_sine2       1
    #define mpy_mixer_tonesweep2  2

    float _outputVolume = 1.0;
    
    bool _displayIsStale = true;
    unsigned long _lastUpdateAt_ms = 0UL;
    unsigned long _displayUpdatePeriod_ms = 200UL;
    const unsigned long _displayMinimumUpdatePeriod_ms = 500UL;

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