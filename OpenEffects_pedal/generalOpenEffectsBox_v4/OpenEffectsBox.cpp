#include "OpenEffectsBox.h"

    
/* *****************************************************
    Audio system definitions and assignments
// *****************************************************/

// GUItool: begin automatically generated code
AudioSynthKarplusStrong  string1;        //xy=70,270
AudioInputI2S            i2s1;           //xy=72,129
AudioSynthWaveformSine   sine1;          //xy=73,174
AudioSynthToneSweep      tonesweep1;     //xy=82,224
AudioSynthWaveformSine   sine2;          //xy=88,380
AudioSynthWaveformDc     dc1;            //xy=90,339
AudioSynthWaveform       waveform1;      //xy=97,463
AudioSynthToneSweep      tonesweep2;     //xy=100,421
AudioMixer4              mixer1;         //xy=226,150
AudioMixer4              mixer3;         //xy=245,400
AudioEffectBitcrusher    bitcrusher1;    //xy=335,225
AudioEffectWaveshaper    waveshape1;     //xy=364,279
AudioAnalyzePeak         peak1;          //xy=379,135
AudioEffectMultiply      multiply1;      //xy=396,338
AudioEffectChorus        chorus1;        //xy=406,399
AudioEffectFlange        flange1;        //xy=422,456
AudioEffectReverb        reverb1;        //xy=443,514
AudioEffectDelayExternal delayExt1;      //xy=590,333
AudioMixer4              mixer4;         //xy=661,546
AudioMixer4              mixer2;         //xy=729,316
AudioOutputI2S           i2s2;           //xy=794,547
AudioConnection          patchCord1(i2s1, 0, mixer1, 0);
AudioConnection          patchCord2(i2s1, 1, mixer1, 1);
AudioConnection          patchCord3(sine1, 0, mixer1, 2);
AudioConnection          patchCord4(tonesweep1, 0, mixer1, 3);
AudioConnection          patchCord5(sine2, 0, mixer3, 1);
AudioConnection          patchCord6(dc1, 0, mixer3, 0);
AudioConnection          patchCord7(waveform1, 0, mixer3, 3);
AudioConnection          patchCord8(tonesweep2, 0, mixer3, 2);



AudioConnection          patchCord9(mixer1, 0, mixer4, 0);



AudioConnection          patchCord10(mixer1, peak1);
AudioConnection          patchCord11(mixer3, 0, multiply1, 1);
AudioConnection          patchCord12(bitcrusher1, waveshape1);
AudioConnection          patchCord13(waveshape1, 0, multiply1, 0);
AudioConnection          patchCord14(multiply1, chorus1);
AudioConnection          patchCord15(chorus1, flange1);
AudioConnection          patchCord16(flange1, reverb1);
AudioConnection          patchCord17(reverb1, delayExt1);
AudioConnection          patchCord18(delayExt1, 0, mixer2, 0);
AudioConnection          patchCord19(delayExt1, 1, mixer2, 1);
AudioConnection          patchCord20(delayExt1, 2, mixer2, 2);
AudioConnection          patchCord21(delayExt1, 3, mixer2, 3);
AudioConnection          patchCord22(mixer4, 0, i2s2, 0);
AudioConnection          patchCord23(mixer4, 0, i2s2, 1);



// AudioConnection          patchCord24(mixer2, 0, mixer4, 0);



AudioControlSGTL5000     sgtl5000_1;     //xy=595,184
// GUItool: end automatically generated code

OpenEffectsBox::OpenEffectsBox() {
}

void OpenEffectsBox::setVerbose ( int verbose ) {
  _verbose = verbose;
  Serial.print ( "OEB VERBOSE: " );
  Serial.println ( _verbose );
}

void OpenEffectsBox::init ( int verbose ) {

  AudioMemory ( 250 );

  setVerbose ( verbose );
  
  _displayableModule = NULL;
  
  _displayIsStale = true;
  _lastUpdateAt_ms = 0UL;
  _displayUpdatePeriod_ms = 0xFFFFFFFF;  // never update...

  _mode = 0;
  _subMode = 0;
  
  _hw.init ();
  
  if ( 1 ) {
    // sort of a self-test
    for ( int i = 0; i < 2; i++ ) {
      _hw.relay [ relayL ].setValue ( 0 );
      _hw.setLED ( ledOnOff, 0x102010 );
      delay ( 100 );
      _hw.relay [ relayL ].setValue ( 1 );
      _hw.setLED ( ledOnOff, 0x201010 );
      delay ( 100 );
    }
  
    for ( int i = 0; i < 2; i++ ) {
      _hw.relay [ relayR ].setValue ( 0 );
      _hw.setLED ( ledBoost, 0x102010 );
      delay ( 100 );
      _hw.relay [ relayR ].setValue ( 1 );
      _hw.setLED ( ledBoost, 0x201010 );
      delay ( 100 );
    }
  
    for ( int i = 0; i < 3; i++ ) {
      for ( int j = 0; j < 8; j++ ) {
        _hw.setVU ( j );
        delay ( 20 );
      }
      for ( int j = 0; j < 8; j++ ) {
        _hw.setVU ( 7 - j );
        delay ( 20 );
      }
    }

    for ( int i = 0; i < 2; i++ ) {
      for ( int j = 0; j < 8; j++ ) {
        _hw.setVU ( j, 1, 5 );
        delay ( 25 );
      }
      delay ( 50 );
      for ( int j = 0; j < 8; j++ ) {
        _hw.setVU ( 7 - j, -1, 5 );
        delay ( 25 );
      }
      delay ( 50 );
    }

  }  // self-test display
  
  sgtl5000_1.enable();
  sgtl5000_init ();
  
  _mixers [ inputMixer ].init ( inputMixer, (char *) "input", &mixer1, &_hw, 1.0, 1.0, 0.0, 0.0 );
  _mixers [ mpyMixer ].init ( mpyMixer, (char *) "mpy", &mixer3, &_hw, 1.0, 0.0, 0.0, 0.0 );
  _mixers [ delayMixer ].init ( delayMixer,  (char *) "delay",  &mixer2, &_hw, 1.0, 0.0, 0.0, 0.0 );
  _mixers [ outputMixer ].init ( outputMixer, (char *) "outpt", &mixer4, &_hw, 1.0, 0.0, 0.0, 0.0 );
  
  _sines [ inputSine ].init ( inputSine, (char *) "tuner", &sine1, &_hw );

  _onOffP = false; _hw.relay [ 0 ].setValue ( _onOffP );
  _boostP = false; _hw.relay [ 1 ].setValue ( _boostP );
  
  
  if ( 1 ) {
  
    _sines [ inputSine ].setFrequency ( 880.0 );

    _mixers [ inputMixer ].setGain ( input_mixer_tuning_sine, 1.0 );
    bitcrusher1.bits ( 16 );
    bitcrusher1.sampleRate ( 44100 );

    mixer2.gain ( 0, 1.0 );
    dc1.amplitude ( 0.2 );
    mixer3.gain ( 0, 1.0 );
    mixer4.gain ( 0, 1.0 );
  }

  Serial.println ( "OEB Init complete." );
  
}

void OpenEffectsBox::tickle () {

  _hw.tickle ();
  
  for ( int i = 0; i < nPots; i++ ) {
    if ( _hw.pot [ i ].changed () ) {
      OpenEffectsBox::cbPots ( i, _hw.pot [ i ].getValue () );
    }
  }
  
  if ( _hw.bat [ 0 ].changed () ) {
    OpenEffectsBox::cbBat0 ( _hw.bat [ 0 ].getValue () );
    _displayIsStale = true;
  }
  
  if ( _hw.bat [ 1 ].changed () ) {
    OpenEffectsBox::cbBat1 ( _hw.bat [ 1 ].getValue () );
    _displayIsStale = true;
  }
  
  for ( int i = 0; i < nPBs; i++ ) {
    if ( _hw.pb [ i ].changed () )
      OpenEffectsBox::cbPBs ( i, _hw.pb [ i ].getValue () );
  }

  for ( int i = 0; i < nPedals; i++ ) {
    if ( _hw.pedal [ i ].changed () ) {
      OpenEffectsBox::cbPedals ( i, _hw.pedal [ i ].getValue () );
      _hw.setVU ( map ( _hw.pedal [ i ].getValue (), 0, 1023, 0, 7 ) );
    }
  }
  


/*
How do we determine when the display goes stale? We need to know what's currently being
displayed, and what just changed, and that will tell us whether that's something on-screen.
Each module know its own mode / submode, and get a call when something changes?
Display list? Boolean array of all things currently displayed? Or send change notifications 
to all objects?



Also set a maximum display update rate, about 100ms. -> 500ms.

How do we do screen update in the background, or when nothing else is happening?

Perhaps only the pots and mode switches changing should stale the screen

Looks like we have a working solution; need to blog it!!!!

*/











  setDisplay ( & _sines [ inputSine ] );
  
  /*
    Updating the screen takes ~100ms, so we can't just do it any time we'd like
    So we've set criteria for updating:
      Never more frequently than a minimum update period
      Only when there's something new to show
        This can be set for a single update of a stale display
        Or it can be periodic for some special purpose, with a period
          set by _displayUpdatePeriod_ms. If that's set to 0xffffffff ( 4294967296 )
          that means don't periodically update.
  */
  
  if ( ( ( millis() - _lastUpdateAt_ms ) > _displayMinimumUpdatePeriod_ms )
       && ( _displayIsStale
          || ( ( ( millis() - _lastUpdateAt_ms ) > _displayUpdatePeriod_ms ) 
                 && ( _displayUpdatePeriod_ms != 0xFFFFFFFF ) 
             )
          )
     ) {
    
    _hw.oled.displayCommon ( _mode, _subMode );
  
    _displayableModule->display ();
    _displayIsStale = false;
  }
  
  
  
  
  
  
  
}


void OpenEffectsBox::setDisplay ( DisplayableModule *who ) {
  if ( _displayableModule != NULL ) _displayableModule->activate ( false );
  _displayableModule = who;
  _displayableModule->activate ( true );
}


/* ======================================================================== //

                          pseudo-callbacks
                       
// ======================================================================== */


void OpenEffectsBox::cbPots ( int pot, int newValue ) {
  if ( _verbose >= 12 ) {
    Serial.print ( "Callback pot " );
    Serial.print ( pot );
    Serial.print ( ": new value " ); 
    Serial.println ( newValue );
  }
  
  // the following fails because it's not the pot that is or is not active...
  // maybe we need "uses" as a function of the display
  // if ( isActive ( _hw.pot [ pot ] ) {
    _displayIsStale = true;
  // }
  
  
  
  
  _hw.pot [ pot ].clearState ();
}

void OpenEffectsBox::cbBat0 ( int newValue ) {

  _mode = constrain ( _mode + newValue, 0, nModes - 1 );

  if ( _verbose >= 12 ) {
    Serial.print ( "Callback bat0: new value " ); 
    Serial.print ( newValue );
    Serial.print ( "; mode = " );
    Serial.println ( _mode );
  }
  
  _hw.bat [ 0 ].clearState ();
}

void OpenEffectsBox::cbBat1 ( int newValue ) {
  if ( _verbose >= 12 ) {
    Serial.print ( "Callback bat1: new value " ); 
    Serial.println ( newValue );
  }
  _hw.bat [ 1 ].clearState ();
}

void OpenEffectsBox::cbPBs ( int pb, int newValue ) {

  pb == 0 ? cbPBOnOff ( newValue ) : cbPBBoost ( newValue );
  if ( _verbose >= 12 ) {
    Serial.print ( "Callback pushbutton " );
    Serial.print ( pb );
    Serial.print ( ": new value " ); 
    Serial.println ( newValue );
  }
  _hw.pb [ pb ].clearState ();
}

void OpenEffectsBox::cbPedals ( int pedal, int newValue ) {
  if ( _verbose >= 12 ) {
    Serial.print ( "Callback pedal " );
    Serial.print ( pedal );
    Serial.print ( ": new value " ); 
    Serial.println ( newValue );
  }
  
  switch ( pedal ) {
    case pedal0:
      setOutputVolume ( float ( newValue ) * 1.5 / 1023.0 );
      break;
    case pedal1:
      break;
    default:
      Serial.print ( "Callback with invalid pedal ( " );
      Serial.print ( pedal );
      Serial.println ( ")" );
      break;
  }
  _hw.pedal [ pedal ].clearState ();
}

// probably should migrate to OpenEffectsBoxHW, or even better FootSwitch, 
// but it's behavior...
void OpenEffectsBox::cbPBOnOff ( int newValue ) {
  if ( ! newValue ) return;
  _onOffP = !_onOffP;
  _hw.relay [ 0 ].setValue ( _onOffP );
  _hw.relay [ 1 ].setValue ( _onOffP );
  _hw.setLED ( ledOnOff, _onOffP ? 0x102010 : 0x201010 );
}

void OpenEffectsBox::cbPBBoost ( int newValue ) {
  if ( ! newValue ) return;
  _boostP = !_boostP;
  _hw.setLED ( ledBoost, _boostP ? 0x102010 : 0x201010 );
}


// ========================================================================

void OpenEffectsBox::sgtl5000_init () {
  sgtl5000_1.enable ();  // ummm... it seems to make a difference if you enable!
  
  // input
  
  #if 1
    sgtl5000_1.inputSelect ( AUDIO_INPUT_LINEIN );
    // 0 is 3.12Vp-p; default 5 is 1.33Vp-p; min 15 is 0.24Vp-p
    // sgtl5000_1.lineInLevel ( 10 );
    sgtl5000_1.lineInLevel ( 5 );       // maybe this will work now TESTING
  #else
    sgtl5000_1.inputSelect ( AUDIO_INPUT_MIC );
    sgtl5000_1.micGain ( 20 );      // dB
  #endif
  
  // output
  
  sgtl5000_1.volume ( 0.8 );  // headphones only, not line-level outputs
  
  // max 13 is 3.16Vp-p; default 29 is 1.29Vp-p; min 31 is 1.16Vp-p
  sgtl5000_1.lineOutLevel ( 13 );
  // sgtl5000_1.lineOutLevel ( 29 );       // maybe this will work now TESTING
}

// ========================================================================

void OpenEffectsBox::setOutputVolume ( float volume ) {
  _outputVolume = volume;
  if ( _verbose >= 22 ) {
    Serial.print ( "setOutputVolume: new value " ); 
    Serial.println ( _outputVolume );
  }
  _hw.setVU ( int ( volume * 7.01 ) );
  _mixers [ outputMixer ].setGain ( 0, _outputVolume );
}
