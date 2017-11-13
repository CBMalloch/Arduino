#define PROGNAME  "generalOpenEffectsBox_v2"
#define VERSION   "2.1.11"
#define VERDATE   "2017-11-12"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// use Paul Stoffregen's NeoPixel library if using Teensy 3.5 or 3.6
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

// NOTE - cannot use standard Serial with OpenEffects or Audio boards
#undef BAUDRATE
#define VERBOSE 12

/*

  Pot 0 is output volume
  
  Modes selected by left bat switch
    0 - standard operating mode
        knob 0 - # voices in chorus  --> output volume!!!
        knob 1 - reverb time seconds
        knob 2 - flange frequency
        knob 3 - volume (temporarily)
        
      submode ( right bat switch ) - input to multiply
        0 - DC
        1 - sine
        2 - sweep
        
    1 - mixer1
        knob 0 - output volume
        knob 1 - L&R input gain
        knob 2 - sine1 gain
        knob 3 - tonesweep1 gain
    
    2 - bit crusher
        knob 0 - output volume
        knob 1 - bits
        knob 2 - sample rate
        
    3 - waveshaper
        knob 0 - output volume
        knob 1 - wave shape choice number

    4 - multiply
      knob 0 - output volume
      submodes
        0 - gains
          knob 1 - dc gain
          knob 2 - sine gain
          knob 3 - tonesweep gain
        1 - dc amplitude control
          knob 1 - dc amplitude
        2 - sine frequency control
          knob 1 - sine frequency
        3 - tonesweep control
          knob 1 - tonesweep lower frequency
          knob 2 - tonesweep upper frequency
          knob 3 - tonesweep time
      
    5 - chorus
        knob 0 - output volume
        knob 1 - number of voices

    6 - flanger
        knob 0 - output volume
        knob 1 - offset - fixed distance behind current
        knob 2 - depth - the size of the variation of offset
        knob 3 - rate - the frequency of the variation of offset

    7 - reverb
        knob 0 - output volume
        knob 1 - reverb time
        
    8 - delays
      knob 0 - output volume
      delay 0 is always straight through with no delay
      submodes
        0 - delay times
          knob 1 - delay 1 time (ms)
          knob 2 - delay 2 time (ms)
          knob 3 - delay 3 time (ms)
        1 - gains
          knob 1 - delay 1 gain
          knob 2 - delay 2 gain
          knob 3 - delay 3 gain

      
*/


/*
  Please remember that the standard Serial and the standard LED on pin 13
  both interfere with audio board pin mappings. Don't use either of them!
  They will work, but their use will disable the audio board's workings.
*/

/*
  I burned out my original OpenEffects board. It is now replaced, but I 
  installed *momentary* switches to the bat switch locations. So now I can
  digitally change states, but I need to have a debounced read of these
  switches and maintain the resulting states.
  Difficulty: the bat switches return analog values
        left     right    (switch)
    L    GND      Vcc
    C    Vcc/2    Vcc/2
    R    Vcc      GND
    
  
  I also installed the other bank of input/output jacks; the input is on the 
  R and is 1/4" stereo; the output is on the L and is 1/4" stereo; the two
  center positions are for expression pedals. 
  The expression pedals are expected to use Vcc and GND and to return a value
  somewhere between these. The pinout of the 1/4" stereo jacks for these is
    T
    R
    S
*/

/* *****************************************************
    Hardware-related definitions and assignments
// *****************************************************/

//Pinout board rev1
// pa = pin, analog; pd = pin, digital
const int pa_pots [] = { A6, A3, A2, A1 };
// tap pins differ between red board and blue board; I have blue
// pb are the stomp switches "pushbutton"
const int pd_pb1 = 2;
const int pd_pb2 = 3;
// const int pd_LED = 13;  // no, no, no. Do not use this!

const int pd_relayL = 4;
const int pd_relayR = 5;

const int CV1 = A10;
const int CV2 = A11;

const int pa_batSwitches [] = { A12, A13 };
const unsigned long batSwitchNoticePeriod_ms =  50UL;
const unsigned long batSwitchRepeatPeriod_ms = 500UL;
unsigned long batSwitchLastCenteredAt_ms [] = { 0UL, 0UL };
unsigned long batSwitchLastRepeatAt_ms [] = { 0UL, 0UL };
int batSwitchStatus [] = { 0, 0 };

// pixels

const int nPixels    = 10;
const int pdWS2812 = 8;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel( nPixels, pdWS2812, NEO_GRB + NEO_KHZ800 );
//  0 is the singleton
//  1 is R dome
//  2 is L dome
//  3-9 are the rectangular ones in the row, with 3 at R and 9 at L
const int ledSingleton = 0;
const int ledOnOff     = 2;
const int ledBoost     = 1;

/* *****************************************************
    Functionality definitions and assignments
// *****************************************************/

const int brightness = 32;

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// OLED screen
#define OLED_RESET 4
Adafruit_SSD1306 oled ( OLED_RESET );

bool displayIsStale = true;
unsigned long lastOledUpdateAt_ms = 0UL;
unsigned long displayUpdateRate_ms = 200UL;

void init_oled_display ();
void displayOLED_0 ();
void displayOLED_mixer1 ();
void ( * updateOLEDdisplay ) ();

Bounce pb1 = Bounce();
Bounce pb2 = Bounce();

const int nPots = 4;
const int potHysteresis = 5;
int potReadings [ nPots ], oldPotReadings [ nPots ];

/***************************************************
                   state vector!
 ***************************************************/

float mix1gain_inp = 1.0;
float mix1gain_sin440 = 0.0;
float mix1gain_tonesweep = 0.0;

int bitcrush_bits = 16;
unsigned long bitcrush_sampleRate = 44100;

int waveshaper_menu = 0;
/*
  0 - ws1_passthru / 2
  1 - ws1_shape1 / 9
  2 - ws1_shape2 / 9
*/

float multiply_dc_gain = 1.0;
float multiply_sine_gain = 0.0;
float multiply_tonesweep_gain = 0.0;
float mpy_dc_amp = 1.0;
float mpy_sine_freq = 2.0;
float mpy_tonesweep_lf = 0.2;
float mpy_tonesweep_hf = 10000.0;
float mpy_tonesweep_time = 4.0;

int chorus_n = 1;

int flange_offset = 0;
int flange_depth = 0;
int flange_rate = 0;

float reverbTime_sec = 0.0;

int delay_times_ms [ 4 ] = { 0, 0, 0, 0 };

float mixer2_gains [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };

//       end of state vector

const int nModes = 9;
int nSubModes = 1;  // set when mode changes
int mode = 0;
bool modeChanged = true;
int subMode = 0;
bool subModeChanged = true;
bool onoff = false;
bool boost = false;

/* *****************************************************
    Audio system definitions and assignments
// *****************************************************/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=72,129
AudioSynthWaveformSine   sine1;          //xy=73,174
AudioSynthToneSweep      tonesweep1;     //xy=86,221
AudioSynthWaveformSine   sine2;          //xy=96,523
AudioSynthWaveformDc     dc1;            //xy=98,477
AudioSynthToneSweep      tonesweep2;     //xy=108,570
AudioMixer4              mixer1;         //xy=226,150
AudioMixer4              mixer3;         //xy=246,498
AudioEffectBitcrusher    bitcrusher1;    //xy=335,225
AudioEffectWaveshaper    waveshape1;     //xy=364,279
AudioEffectMultiply      multiply1;      //xy=396,338
AudioEffectChorus        chorus1;        //xy=406,399
AudioEffectFlange        flange1;        //xy=422,456
AudioEffectReverb        reverb1;        //xy=443,517
AudioEffectDelayExternal delayExt1;      //xy=588,425
AudioMixer4              mixer2;         //xy=742,380
AudioOutputI2S           i2s2;           //xy=878,379
AudioConnection          patchCord1(i2s1, 0, mixer1, 0);
AudioConnection          patchCord2(i2s1, 1, mixer1, 1);
AudioConnection          patchCord3(sine1, 0, mixer1, 2);
AudioConnection          patchCord4(tonesweep1, 0, mixer1, 3);
AudioConnection          patchCord5(sine2, 0, mixer3, 1);
AudioConnection          patchCord6(dc1, 0, mixer3, 0);
AudioConnection          patchCord7(tonesweep2, 0, mixer3, 2);
AudioConnection          patchCord8(mixer1, bitcrusher1);
AudioConnection          patchCord9(mixer3, 0, multiply1, 1);
AudioConnection          patchCord10(bitcrusher1, waveshape1);
AudioConnection          patchCord11(waveshape1, 0, multiply1, 0);
AudioConnection          patchCord12(multiply1, chorus1);
AudioConnection          patchCord13(chorus1, flange1);
AudioConnection          patchCord14(flange1, reverb1);
AudioConnection          patchCord15(reverb1, delayExt1);
AudioConnection          patchCord16(delayExt1, 0, mixer2, 0);
AudioConnection          patchCord17(delayExt1, 1, mixer2, 1);
AudioConnection          patchCord18(delayExt1, 2, mixer2, 2);
AudioConnection          patchCord19(delayExt1, 3, mixer2, 3);
AudioConnection          patchCord20(mixer2, 0, i2s2, 0);
AudioConnection          patchCord21(mixer2, 0, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=120,48
// GUItool: end automatically generated code


/* *****************************************************
    audio system definitions and variable setups
// *****************************************************/

#define mixer1_input_L    0
#define mixer1_input_R    1
#define mixer1_sine1      2
#define mixer1_tonesweep1 3

float ws1_passthru [] = { -1.0, 1.0 };
float ws1_shape1 [] = { -1.0, -0.3,  -0.2,  -0.1, 0.0, 0.1, 0.2,  0.3,  1.0 };
float ws1_shape2 [] = { -1.0, -0.98, -0.95, -0.1, 0.0, 0.1, 0.95, 0.98, 1.0 };
float ws1_shape3 [] = {  1.0,  0.3,   0.2,   0.1, 0.0, 0.1, 0.2,  0.3,  1.0 };
float ws1_shape4 [] = {  1.0,  0.98,  0.5,   0.1, 0.0, 0.1, 0.5,  0.98, 1.0 };

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
#define mixer3_DC          0
#define mixer3_sine2       1
#define mixer3_tonesweep2  2

float outputVolume = 1.0;

/* *****************************************************
// *****************************************************/

void setup() {

  AudioMemory ( 250 );

  pinMode ( pd_pb1, INPUT );
  pinMode ( pd_pb2, INPUT );
  pinMode ( pd_relayL, OUTPUT );
  pinMode ( pd_relayR, OUTPUT );
  
  pb1.attach ( pd_pb1 );
  pb1.interval ( 5 ); // debounce interval in ms
  pb2.attach ( pd_pb2 );
  pb2.interval ( 5 ); // debounce interval in ms
  
  strip.begin();
  strip.setPixelColor ( ledSingleton, 0x106010 );
  strip.setPixelColor ( ledOnOff, 0x601010 );
  strip.setPixelColor ( ledBoost, 0x601010 );
  strip.show(); // Initialize all pixels to 'off'
  
  init_oled_display ();
  displayIsStale = true;
  
  sgtl5000_1.enable();
  digitalWrite ( pd_relayL, 0 );
  digitalWrite ( pd_relayR, 0 );
  openEffectsBox_init ();
  
  // Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 100 );
}

/* *****************************************************
// *****************************************************/

void loop() {
  
  updateInputs ();
  
  // -------------------------------------------------------
  // update variables and displays in accordance with inputs
  // -------------------------------------------------------
  
  if ( pb1.fell() ) {
    onoff = !onoff;
    if ( onoff ) {
      digitalWrite ( pd_relayL, 1 );
      digitalWrite ( pd_relayR, 1 );
      strip.setPixelColor ( ledOnOff, 0x106010 );
    } else {
      digitalWrite ( pd_relayL, 0 );
      digitalWrite ( pd_relayR, 0 );
      strip.setPixelColor ( ledOnOff, 0x601010 );
    }
  }
  
  if ( pb2.fell() ) {
    boost = !boost;
    if ( boost ) {
      // digitalWrite ( pd_relayL, 1 );
      // digitalWrite ( pd_relayR, 1 );
      strip.setPixelColor ( ledBoost, 0x106010 );
    } else {
      // digitalWrite ( pd_relayL, 0 );
      // digitalWrite ( pd_relayR, 0 );
      strip.setPixelColor ( ledBoost, 0x601010 );
    }
  }
  
  outputVolume = expmap ( potReadings [ 0 ], 0, 1023, 0.01, 2.0 );
      
  switch ( mode ) {
    case 0:  // standard operating mode
            
      nSubModes = 1;
      
      mixer2.gain ( 0, outputVolume );
      if ( 1 || VERBOSE >= 10 ) setVU ( outputVolume * 3.5 );   // 0.1<=oV<=2.0
      
      updateOLEDdisplay = &displayOLED_0;
      
      break;
  
    case 1:  // control mixer1

      nSubModes = 1;
      
      for ( int i = 1; i <= 3; i++ ) {
        if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
          float val = fmap ( potReadings [ i ], 0, 1023, 0.0, 2.0 );
          switch ( i ) {
            case 1:  // inputs
              mix1gain_inp = val;
              mixer1.gain ( 0, mix1gain_inp );
              mixer1.gain ( 1, mix1gain_inp );
              break;
            case 2:  // sine1
              mix1gain_sin440 = val;
              mixer1.gain ( 2, mix1gain_sin440 );
              break;
            case 3:  // tonesweep1
              mix1gain_tonesweep = val;
              mixer1.gain ( 3, mix1gain_tonesweep );
              break;
            default: 
              break;
          }
          if ( VERBOSE >= 10 ) setVU ( val * 7 / 2 );   // debug
        }
      }
    
      mixer2.gain ( 0, outputVolume );
      // setVU ( outputVolume );   // debug

      updateOLEDdisplay = &displayOLED_mixer1;

      break;
  
    case 2:  // control bitcrusher
    
      nSubModes = 1;
      
      for ( int i = 1; i <= 2; i++ ) {
        if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
          switch ( i ) {
            case 1:  // bits
              bitcrush_bits = map ( potReadings [ i ], 0, 1023, 16, 1 );
              bitcrusher1.bits ( bitcrush_bits );
              break;
            case 2:  // sample rate
              bitcrush_sampleRate = fmap ( potReadings [ i ], 0, 1023, 44100UL, 10 );
              bitcrusher1.sampleRate ( bitcrush_sampleRate );
              break;
            default: 
              break;
          }
          if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
        }
      }

      mixer2.gain ( 0, outputVolume );
      // setVU ( outputVolume );   // debug
      
      updateOLEDdisplay = &displayOLED_bitcrusher;

      break;
  
    case 3:  // control waveshaper
    
      nSubModes = 1;
      
      if ( abs ( potReadings [ 1 ] - oldPotReadings [ 1 ] ) > potHysteresis ) {
        waveshaper_menu = map ( potReadings [ 1 ], 0, 1023, 0, 4 );
        switch ( waveshaper_menu ) {
          case 0:
            waveshape1.shape ( ws1_passthru, 2 );
            break;
          case 1:
            waveshape1.shape ( ws1_shape1, 9 );
            break;
          case 2:
            waveshape1.shape ( ws1_shape2, 9 );
            break;
          case 3:
            waveshape1.shape ( ws1_shape3, 9 );
            break;
          case 4:
            waveshape1.shape ( ws1_shape4, 9 );
            break;
          default:
            break;
        }
      }

      mixer2.gain ( 0, outputVolume );
      // setVU ( outputVolume );   // debug
      
      updateOLEDdisplay = &displayOLED_waveshaper;

      break;
    
    case 4:  // control multiply
    
      nSubModes = 4;
      switch ( subMode ) {
        case 0:  // main gain settings for multiply function
          for ( int i = 1; i <= 2; i++ ) {
            if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
              switch ( i ) {
                case 1:  // dc gain
                  multiply_dc_gain = fmap ( potReadings [ 1 ], 0, 1023, 0.0, 2.0 );
                  mixer3.gain ( 0, multiply_dc_gain );
                  break;
                case 2:  // sine gain
                  multiply_sine_gain = fmap ( potReadings [ 2 ], 0, 1023, 0.0, 2.0 );
                  mixer3.gain ( 1, multiply_sine_gain );
                  break;
                case 3:  // sine gain
                  multiply_tonesweep_gain = fmap ( potReadings [ 3 ], 0, 1023, 0.0, 2.0 );
                  mixer3.gain ( 1, multiply_tonesweep_gain );
                  break;
                default:
                  break;
              }
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_multiply;
          break;
        case 1:  // dc amplitude
          if ( abs ( potReadings [ 1 ] - oldPotReadings [ 1 ] ) > potHysteresis ) {
            mpy_dc_amp = fmap ( potReadings [ 1 ], 0, 1023, -1.0, 1.0 );
            dc1.amplitude ( 0, mpy_dc_amp );
            if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ 1 ], 0, 1023, 0, 7 ) );   // debug
          }
          updateOLEDdisplay = &displayOLED_mpy_dc_gain;
          break;
        case 2:  // sine2 frequency
          if ( abs ( potReadings [ 1 ] - oldPotReadings [ 1 ] ) > potHysteresis ) {
            // sine frequency
            mpy_sine_freq = expmap ( potReadings [ 1 ], 0, 1023, 0.01, 10.0 );
            sine2.frequency ( mpy_sine_freq );
            if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ 1 ], 0, 1023, 0, 7 ) );   // debug
            }
          updateOLEDdisplay = &displayOLED_mpy_sine2_frequency;
          break;
        case 3:  // tone sweep
        
          // incomplete - need to start and restart tone sweeps
          
          for ( int i = 1; i <= 3; i++ ) {
            if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
              switch ( i ) {
                case 1:  // tone sweep lower frequency
                  mpy_sine_freq = expmap ( potReadings [ 1 ], 0, 1023, 10.0, 10000.0 );
                  sine2.frequency ( mpy_sine_freq );
                  break;
                case 2:  // tone sweep upper frequency
                  mpy_sine_freq = expmap ( potReadings [ 1 ], 0, 1023, 0.01, 1.0 );
                  sine2.frequency ( mpy_sine_freq );
                  break;
                case 3:  // tone sweep time
                  mpy_sine_freq = expmap ( potReadings [ 1 ], 0, 1023, 0.01, 1.0 );
                  sine2.frequency ( mpy_sine_freq );
                  break;
                default:
                  break;
              }
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_mpy_tone_sweep;
          break;
      }

      mixer2.gain ( 0, outputVolume );
      // setVU ( outputVolume );   // debug

      break;
  
    
    case 5:  // control chorus
    
      nSubModes = 1;
      
      if ( abs ( potReadings [ 1 ] - oldPotReadings [ 1 ] ) > potHysteresis ) {
        chorus_n = map ( potReadings [ 1 ], 0, 1023, 1, 5 );
        // chorus1.modify ( map ( potReadings [ 0 ], 0, 1023, 1, 5 ) );
        //   Arduino claims chorus1 has no member "modify"
        chorus1.voices ( chorus_n );
      }
      
      mixer2.gain ( 0, outputVolume );
      // setVU ( outputVolume );   // debug
      
      updateOLEDdisplay = &displayOLED_chorus;

      break;
    
    case 6:  // control flange
    
      nSubModes = 1;
      
      for ( int i = 1; i <= 3; i++ ) {
        if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
          switch ( i ) {
            case 1:  // offset - fixed distance behind current
              flange_offset = map ( potReadings [ i ], 0, 1023, 0, FLANGE_DELAY_LENGTH );
              break;
            case 2:  // depth - the size of the variation of offset
              flange_depth = map ( potReadings [ i ], 0, 1023, 0, FLANGE_DELAY_LENGTH );
              break;
            case 3:  // rate - the frequency of the variation of offset
              flange_rate = expmap ( potReadings [ i ], 0, 1023, 0.1, 10.0 );
              break;
            default:
              break;
          }
          flange1.voices ( flange_offset, flange_depth, flange_rate );
          if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
        }
      }

      mixer2.gain ( 0, outputVolume );
      // setVU ( outputVolume );   // debug
      
      updateOLEDdisplay = &displayOLED_flange;

      break;
    
    case 7:  // control reverb
    
      nSubModes = 1;
      
      if ( abs ( potReadings [ 1 ] - oldPotReadings [ 1 ] ) > potHysteresis ) {
        reverbTime_sec = fmap ( potReadings [ 1 ], 0, 1023, 0.0, 5.0 );
        reverb1.reverbTime ( reverbTime_sec );
      }

      mixer2.gain ( 0, outputVolume );
      // setVU ( outputVolume );   // debug
      
      updateOLEDdisplay = &displayOLED_reverb;

      break;
    
    case 8:  // control delays
    
      nSubModes = 2;
      
      switch ( subMode ) {
        case 0:  // delay times beyond delay 0
          for ( int i = 1; i <= 3; i++ ) {
            if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
              delay_times_ms [ i ] = expmap ( potReadings [ i ], 0, 1023, 0.1, 1485.0 );
              delayExt1.delay ( i, delay_times_ms [ i ] );
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_delay_times;
          break;
        case 1:  // gain for each delay beyond 0
          for ( int i = 1; i <= 3; i++ ) {
            if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
              mixer2_gains [ i ] = expmap ( potReadings [ i ], 0, 1023, 0.001, 2.0 );
              mixer2.gain ( i, mixer2_gains [ i ] );
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_mixer2;
          break;
        default:
          break;
      }

      mixer2.gain ( 0, outputVolume );
      // setVU ( outputVolume );   // debug

      break;
    
    default:
      if ( modeChanged ) {
        // clear stuff from other modes
        openEffectsBox_init ();
        nSubModes = 1;
      }
      
      mixer2.gain ( 0, outputVolume );
      setVU ( outputVolume );   // debug
      
      break;
      
  }
  
  strip.setBrightness ( brightness );
  strip.show ();
  updateOLEDdisplay ();
  
  modeChanged = false;
  subModeChanged = false;
}

/* *****************************************************
                  Subroutine definitions
// *****************************************************/

// ========================================================================

// upper-level functions

void updateInputs () {

  // ----------------------
  // read switches and pots
  // ----------------------
  
  for ( int i = 0; i < nPots; i++ ) {
    oldPotReadings [ i ] = potReadings [ i ];
    potReadings [ i ] = 1023 - analogRead ( pa_pots [ i ] );
    if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
      displayIsStale = true;
    }
  }
  
  // NASTY read the state of the bat switches and update their status
  
  for ( int i = 0; i < 2; i++ ) {
    int val = analogRead ( pa_batSwitches [ i ] );
    if ( i == 1 ) val = 1023 - val;
    if ( val > ( 1024 / 3 ) && val < ( 1024 * 2 / 3 ) ) {
      // centered; no change
      batSwitchLastCenteredAt_ms [ i ] = millis();
      batSwitchLastRepeatAt_ms [ i ] = 0UL;
    } else {
      // have detected actuation
      // notice only if it's been here a while
      if ( ( millis() - batSwitchLastCenteredAt_ms [ i ] ) > batSwitchNoticePeriod_ms ) {
        // it's firmly actuated; hit it and then ignore until it's time to repeat
        if ( ( millis() - batSwitchLastRepeatAt_ms [ i ] ) > batSwitchRepeatPeriod_ms ) {
          // do something
          batSwitchStatus [ i ] += ( val > ( 1024 / 2 ) ) ? 1 : -1;
          batSwitchStatus [ i ] = constrain ( batSwitchStatus [ i ], 
                                              0,
                                              i == 0 ? nModes - 1 : nSubModes - 1 );
          batSwitchLastRepeatAt_ms [ i ] = millis();
        }
      }
    }
  }
  
  if ( mode != batSwitchStatus [ 0 ] ) {
    displayIsStale = true;
    modeChanged = true;
    mode = batSwitchStatus [ 0 ];
    batSwitchStatus [ 1 ] = 0;   // reset submode when mode changes
  }

  if ( subMode != batSwitchStatus [ 1 ] ) {
    displayIsStale = true;
    subModeChanged = true;
    subMode = batSwitchStatus [ 1 ];
  }
      
  pb1.update();
  pb2.update();
  
}

// ========================================================================

// OLED display routines 

void init_oled_display () {
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  // initialize with the I2C addr 0x3D (for the 128x64)
  // 0x3c on Open Effects box
  
  const byte addI2C = 0x3c;
  oled.begin(SSD1306_SWITCHCAPVCC, addI2C);  
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  oled.display ();
  delay ( 500 );

  // Clear the buffer.
  oled.clearDisplay ();
  oled.display ();

}

void displayOLED_0 () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  displayUpdateRate_ms = 200UL;
  
  displayOLED_common ();
  
  #if 1
    // top 4 small numbers are the pot readings  
    oled.setTextSize ( 1 );
    oled.setTextColor ( WHITE );
    for ( int i = 0; i < nPots; i++ ) {
      oled.setCursor ( i * 32, 0 );
      oled.println ( potReadings [ i ] );
    }
  #endif
  
  // version bottom
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 56 );
  oled.print ( VERSION );

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 20 );
  oled.print ( outputVolume );

  oled.setCursor ( 64, 20 );
  oled.print ( chorus_n );
  
  oled.setCursor ( 96, 20 );
  oled.print ( reverbTime_sec );

  oled.display();  // note takes about 100ms!!!
  
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
  
}

void displayOLED_mixer1 () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  // mixer1 has 4 inputs, controlled as 3
  //   0 & 1 are L and R channels input
  //   2 is sine1
  //   3 is tonesweep1
  
  // 3 gains, pots 1-3
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "input mix" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( mix1gain_inp );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "inp" );

  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( mix1gain_sin440 );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "sin" );

  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( mix1gain_tonesweep );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "swp" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}

void displayOLED_bitcrusher () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  // bitcrusher
  //   0 for bits ( 0 is 16 bits => pass-through )
  //   1 for sampling rate ( 0 is 44100 => pass-through )
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "bitcrush" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( bitcrush_bits );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "bits" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( bitcrush_sampleRate );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "samp" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}

void displayOLED_waveshaper () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "waveshape" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( waveshaper_menu );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "menu" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}
  
void displayOLED_multiply () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  // bitcrusher
  //   0 for bits ( 0 is 16 bits => pass-through )
  //   1 for sampling rate ( 0 is 44100 => pass-through )
  
  // 3 gains, pots 1-3
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "multiply" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( multiply_dc_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "dc" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( multiply_sine_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "sin" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( multiply_tonesweep_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "swp" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}
  
void displayOLED_mpy_dc_gain () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "dc amp" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( mpy_dc_amp );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "val" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}
  
void displayOLED_mpy_sine2_frequency () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "sine freq" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( mpy_sine_freq );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "freq" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}
  
void displayOLED_mpy_tone_sweep () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "tone sweep" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( mpy_tonesweep_lf );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "lo f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( mpy_tonesweep_lf );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "hi f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( mpy_tonesweep_time );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "ms" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}
  
void displayOLED_chorus () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "chorus" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 64, 16 );
  oled.print ( chorus_n );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "voices" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}

void displayOLED_flange () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "flange" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( flange_offset );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "ofst" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( flange_depth );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "dpth" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( flange_rate );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "rate" );
  
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}

void displayOLED_reverb () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "reverb" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( reverbTime_sec );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "sec" );
    
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}

void displayOLED_delay_times () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "delays" );
  
  for ( int i = 1; i < 4; i++ ) {
    int y = ( i - 1 ) * 16 + 16;
    oled.setTextSize ( 1 );
    oled.setCursor ( 30, y );
    oled.print ( delay_times_ms [ i ] );
    oled.setTextSize ( 1 );
    oled.setCursor ( 0, y );
    oled.print ( "ms" );
  }
    
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}

void displayOLED_mixer2 () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  
  // displayUpdateRate_ms = 0xFFFFFFFF;
  displayUpdateRate_ms = 100UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "delay gain" );
  
  for ( int i = 1; i < 4; i++ ) {
    int y = ( i - 1 ) * 16 + 16;
    oled.setTextSize ( 1 );
    oled.setCursor ( 30, y );
    oled.print ( mixer2_gains [ i ] );
    // oled.setTextSize ( 1 );
    // oled.setCursor ( 0, y );
    // oled.print ( "" );
  }
    
  lastOledUpdateAt_ms = millis();
  oled.display();  // note takes about 100ms!!!
    
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
}

void displayOLED_common () {

  // 128h x 64v
  
  oled.clearDisplay();
  oled.setTextColor ( WHITE );
    
  // large numbers are the bat switch positions

  oled.setTextSize ( 1 );
  oled.setCursor ( 96, 56 );
  oled.print ( mode );
  oled.print ( "." );
  oled.print ( subMode );
  
}

// ========================================================================

// WS2812 3-color LEDs routines

void setVU ( int n ) {
  const unsigned long offColor = 0x101010UL;
  const unsigned long onColor  = 0x106060UL;
  const int VUfirstPixel = 3;
  const int nVUpixels = 7;
  
  // if ( n < 0 || n > nVUpixels ) {
  //   Serial.println ( "setVU: invalid bargraph value ( 0 - 7 ): " );
  //   Serial.println ( n );
  // }
  for ( int i = 0; i < nVUpixels; i++ ) {
    strip.setPixelColor ( VUfirstPixel + i, ( i < n ) ? onColor : offColor );
  }
  // strip.show ();
}
  
// ========================================================================

// general routines

float fmap ( float x, float x0, float x9, float t0, float t9 ) {
  return ( t0 + ( t9 - t0 ) * ( ( x - x0 ) / ( x9 - x0 ) ) );
}

float expmap ( float x, float x0, float x9, float t0, float t9 ) {
  // to simulate things like straight log pots
  float lt0 = log ( t0 );
  float lt9 = log ( t9 );
  float interp = ( lt0 + ( lt9 - lt0 ) * ( ( x - x0 ) / ( x9 - x0 ) ) );
  return ( exp ( interp ) );
}

// ========================================================================

// inits for audio objects

void openEffectsBox_init () {
  sgtl5000_init ();
  sine1_init ();
  // tonesweep1: tonesweeps play only once, when called
  mixer1_init ();
  bitcrusher1_init ();
  waveshape1_init ();
  multiply1_init ();
  chorus1_init ();
  flange1_init ();
  reverb1_init ();
  delayExt1_init ();
  mixer2_init ();
}

void sgtl5000_init () {
  #if 1
    sgtl5000_1.inputSelect ( AUDIO_INPUT_LINEIN );
    sgtl5000_1.lineInLevel ( 0 );   // default 0 is 3.12Vp-p; min 15 is 0.24Vp-p
  #else
    sgtl5000_1.inputSelect ( AUDIO_INPUT_MIC );
    sgtl5000_1.micGain ( 20 );      // dB
  #endif
  sgtl5000_1.volume ( 0.8 );  // headphones only, not line-level outputs
  sgtl5000_1.lineOutLevel ( 13 );   // 13 is 3.16Vp-p; default 29 is 1.29Vp-p 
}

void sine1_init () {
  sine1.amplitude ( 1.0 );
  sine1.frequency ( 440.0 );
}

void mixer1_init () {
  mix1gain_inp = 1.0;
  mix1gain_sin440 = 0.0;
  mix1gain_tonesweep = 0.0;

  mixer1.gain ( mixer1_input_L, mix1gain_inp );   // i2s2 ( line inputs L & R )
  mixer1.gain ( mixer1_input_R, mix1gain_inp );   // i2s2 ( line inputs L & R )
  mixer1.gain ( mixer1_sine1, mix1gain_sin440 );   // sine1
  mixer1.gain ( mixer1_tonesweep1, mix1gain_tonesweep );   // tonesweep1
}

void bitcrusher1_init () {
  bitcrush_bits = 16;
  bitcrush_sampleRate = 44100;
  bitcrusher1.bits ( bitcrush_bits );
  bitcrusher1.sampleRate ( bitcrush_sampleRate );
}
  
void waveshape1_init () {
  waveshaper_menu = 0;
  waveshape1.shape ( ws1_passthru, 2 );
}
  
void dc1_init () {
  mpy_dc_amp = 1.0;
  dc1.amplitude ( mpy_dc_amp );
}

void sine2_init () {
  mpy_sine_freq = 2.0;
  sine2.amplitude ( 1.0 );
  sine2.frequency ( mpy_sine_freq );
}

void mixer3_init () {

  multiply_dc_gain = 1.0;
  multiply_sine_gain = 0.0;
  multiply_tonesweep_gain = 0.0;
  
  mixer3.gain ( 0, multiply_dc_gain );
  mixer3.gain ( 1, multiply_sine_gain );
  mixer3.gain ( 2, multiply_tonesweep_gain );
    
}

void multiply1_init () {
  
  // dc1, sine2, and tonesweep2 are inputs mixer3, in turn to the multiply
  dc1_init ();
  sine2_init ();
  // tonesweep2: tonesweeps play only once, when called
  mpy_tonesweep_lf = 0.2;
  mpy_tonesweep_hf = 10000.0;
  mpy_tonesweep_time = 4.0;
  
  mixer3_init ();
  
}

void chorus1_init () {
  chorus_n = 1;
  chorus1.begin ( chorusDelayLine, CHORUS_DELAY_LENGTH, chorus_n );  // 1 is passthru
}

void flange1_init () {
  flange_offset = 0;
  flange_depth = 0;
  flange_rate = 0.0;
  flange1.begin ( flangeDelayLine, FLANGE_DELAY_LENGTH, 
    flange_offset, flange_depth, flange_rate );
}

void reverb1_init () {
  reverbTime_sec = 0.0;
  reverb1.reverbTime ( reverbTime_sec );
}

void delayExt1_init () {
  for ( int i = 0; i < 4; i++ ) {
    delay_times_ms [ i ] = 0.0;
    delayExt1.delay ( i , delay_times_ms [ i ] );
  }
  for ( int i = 4; i < 8; i++ ) {
    delayExt1.disable ( i );
  }
}
  
void mixer2_init () {
  for ( int i = 0; i < 4; i++ ) {
    mixer2.gain ( i, i ? 0.0 : outputVolume );
  }
}
