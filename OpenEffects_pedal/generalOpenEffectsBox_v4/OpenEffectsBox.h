#ifndef OpenEffectsBox_h
#define OpenEffectsBox_h

#include "OpenEffectsBoxHW.h"

#define OpenEffectsBox_VERSION "4.000.007"

#include <Arduino.h>
#include <Print.h>

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Mode0.h"
#include "Mixer.h"
#include "Sine.h"
#include "Tonesweep.h"
#include "Bitcrusher.h"
#include "DC.h"
#include "Chorus.h"
#include "Flange.h"
#include "Filter.h"
#include "Reverb.h"
#include "DelayExt.h"

#define OpenEffectsBox_VERBOSE_DEFAULT 12
#define OpenEffectsBox_NMODES 11
    
/*!
	\brief OpenEffectsBox - main class to abstract the firmware of the AudioEffects web development toolkit
	for application to the OpenEffects Project being developed by Øyvind Mjanger.
	
	Created by Charles B. Malloch, PhD, January 10, 2018 and released into the public domain
	
	Structure:
	- OpenEffectsBox - the overall open effects box
	- OpenEffectsBoxHW - defines and handles the switches, knobs, etc.
	- OpenEffectsBoxFW - the firmware - runs things, handles events, etc
	- OpenEffectsBoxModule - base type for individual effects
	
*/

class OpenEffectsBox {
  public:
  
    OpenEffectsBox ();  // constructor
    void setVerbose ( int i );
    
    void init ( char * version, int verbose = OpenEffectsBox_VERBOSE_DEFAULT );
    void tickle ();    
    
  protected:
    
  private:

    int _verbose;
    #define _nModes OpenEffectsBox_NMODES
    int _nSubModes;
    int _mode, _subMode;
    
    void setDisplay ( DisplayableModule *who );
    DisplayableModule *_displayableModule;
    
    bool _onOffP, _boostP;
    
    OpenEffectsBoxHW _hw;
    
    void selectDisplay ();
    
    void sgtl5000_init ();

    void cbPots ( int pot, float newValue );
    void cbBat0 ( int newValue );
    void cbBat1 ( int newValue );
    void cbPBs ( int pb, int newValue );
    void cbPBOnOff ( int newValue );
    void cbPBBoost ( int newValue );
    void cbPedals ( int pedal, float newValue );
    
    Mode0 _mode0;
    #define _nMixers 5
    Mixer _mixers [ _nMixers ];
    #define _nSines 2
    Sine _sines [ _nSines ];
    #define _nTonesweeps 2
    Tonesweep _tonesweeps [ _nTonesweeps ];
    #define _nBitcrushers 1
    Bitcrusher _bitcrushers [ _nBitcrushers ];
    #define _nDCs 1
    DC _dcs [ _nDCs ];
    #define _nChori 1
    Chorus _chori [ _nChori ];
    #define _nFlanges 1
    Flange _flanges [ _nFlanges ];
    #define _nFilters 1
    Filter _filters [ _nFilters ];
    #define _nReverbs 1
    Reverb _reverbs [ _nReverbs ];
    #define _nDelayExts 1
    DelayExt _delayExts [ _nDelayExts ];
    
    void setOutputVolume ( float volume );
    void setWah ( float wah );

    /* *****************************************************
        audio system definitions and variable setups
    // *****************************************************/

    const static int idx_inputMixer               = 0;
    const static int idx_mpyMixer                 = 1;
    const static int idx_filterMixer              = 2;
    const static int idx_delayMixer               = 3;
    const static int idx_outputMixer              = 4;
    
    const static int idx_inputSine                = 0;
    const static int idx_mpy_sine                 = 1;
    
    const static int idx_inputTonesweep           = 0;
    const static int idx_mpyTonesweep             = 1;
    
    const static int idx_Bitcrusher               = 0;
    const static int idx_mpyDC                    = 0; 
    const static int idx_Chorus                   = 0;
    const static int idx_Flange                   = 0;
    const static int idx_Filter                   = 0;
    const static int idx_Reverb                   = 0;
    const static int idx_DelayExt                 = 0;
    
    const static int ch_inputMixer_input_L        = 0;
    const static int ch_inputMixer_input_R        = 1;
    const static int ch_inputMixer_tuning_sine    = 2;
    const static int ch_inputMixer_tonesweep1     = 3;
    
    const static int ch_mpyMixer_dc               = 0;
    const static int ch_mpyMixer_sine             = 1;
    const static int ch_mpyMixer_sweep            = 2;
    const static int ch_mpyMixer_wave             = 3;
    
    const static int ch_filterMixer_Dry           = 0;
    const static int ch_filterMixer_LP            = 1;
    const static int ch_filterMixer_BP            = 2;
    const static int ch_filterMixer_HP            = 3;
    
    const static int ch_delayMixer0               = 0;
    const static int ch_delayMixer1               = 1;
    const static int ch_delayMixer2               = 2;
    const static int ch_delayMixer3               = 3;
    
    const static int ch_outputMixer               = 0;
    

    float _outputVolume = 1.0;
    float _wah = 0.0;
    
    /*
    unsigned long _lastUpdateAt_ms = 0UL;  // never really used!
    unsigned long _displayUpdatePeriod_ms = 200UL;
    const unsigned long _displayMinimumUpdatePeriod_ms = 500UL;
    */

};

#endif