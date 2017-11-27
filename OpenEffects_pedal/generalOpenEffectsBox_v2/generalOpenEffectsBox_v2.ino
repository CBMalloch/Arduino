#define PROGNAME  "generalOpenEffectsBox_v2"
#define VERSION   "2.6.6"
#define VERDATE   "2017-11-22"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// use Paul Stoffregen's NeoPixel library if using Teensy 3.5 or 3.6
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

/*
   NOTE - cannot use standard Serial with OpenEffects or Audio boards
   Actually, as of 2017-11-16, it appears that you can indeed. Maybe
   related to which Teensy -- long vs. short?
*/
// #undef BAUDRATE
#define BAUDRATE 115200
#define VERBOSE 12

/*

  Pot 0 is output volume
  
  Modes selected by left bat switch
    0 - standard operating mode
        knob 0 - output volume
        
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


Note: I have added waveform1 as a 4th input to mixer3, but have not written
it into the code yet.
      
Fun idea: 853Hz and 960Hz for the CONELRAD Attention Signal...

*/


/*
  Please remember that the standard Serial and the standard LED on pin 13
  both interfere with audio board pin mappings. Don't use either of them!
  They will work, but their use will disable the audio board's workings.
*/

/*

                      N O T E S
                      
                      
  I burned out my original OpenEffects board. It is now replaced, but I 
  installed *momentary* switches to the bat switch locations. So now I can
  digitally change states, but I need to have a debounced read of these
  switches and maintain the resulting states.
  Difficulty: the bat switches return analog values, which cannot be used to 
  trigger an interrupt.
        left     right    (switch)
    L    GND      Vcc
    C    Vcc/2    Vcc/2
    R    Vcc      GND
    
  
  I also installed the other bank of input/output jacks; the input is on the 
  R and is 1/4" stereo; the output is on the L and is 1/4" stereo; the two
  center jacks are for expression pedals. Note that the rightmost of these
  is labeled Exp1 but goes to A11 on the Teensy, which is described in the 
  schematic as being connected to CVInput2.
  The expression pedals divide bewteen Vcc and GND and return a value
  somewhere in between. The pinout of the 1/4" stereo jacks for these is
    T - voltage variably divided by the effects pedal
    R - 3V3
    S - GND
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

const int CV1 = A11;
const int CV2 = A10;

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
const int potHysteresis = 10;
int potReadings [ nPots ], oldPotReadings [ nPots ];
bool potChanged [ nPots ];

const int nPresets = 2;


/***************************************************
                   state vector!
 ***************************************************/
 
class OpenEffectsBox {
  public:
  
    OpenEffectsBox ();  // constructor
    
    float mix1_inst_gain;
    float mix1_sin_gain;
    float mix1_tonesweep_gain;
    float mix1_sine_freq;
    float tuning_freq_table [ 5 ];
    float mix1_tonesweep_lf;
    float mix1_tonesweep_hf;
    float mix1_tonesweep_time;

    int bitcrush_bits;
    unsigned long bitcrush_sampleRate;

    int waveshape_selection;
    /*
      0 - ws1_passthru / 2
      1 - ws1_shape1 / 9
      2 - ws1_shape2 / 9
      3 - ws1_shape3 / 9
      4 - ws1_shape4 / 9
    */

    float multiply_dc_gain;
    float multiply_sine_gain;
    float multiply_tonesweep_gain;
    float mpy_dc_amp;    
    float mpy_sine_freq;
    float mpy_tonesweep_lf;
    float mpy_tonesweep_hf;
    float mpy_tonesweep_time;

    int chorus_nVoices;

    int flange_offset;
    int flange_depth;
    int flange_rate;

    float reverbTime_sec;

    int delay_times_ms [ 4 ];

    float mixer2_gains [ 4 ];
    float mixer4_gains [ 4 ];
    
    int currentPreset;
    
    void init ();
    void initializeStateVector ();
    void setPreset ( int presetNumber );
    void sgtl5000_init ();
    
    void sine1_set ( float sine_freq );
    void tonesweep1_set ( float tonesweep_lf, float tonesweep_hf, float tonesweep_time );
    void mixer1_set ( float inst_gain, float sin_gain, float tonesweep_gain );
    void bitcrusher1_set ( int bits, unsigned long sampleRate );
    void waveshape1_set ( int waveshape );
    void dc1_set ( int dc_gain );
    void sine2_set ( float sine_freq );
    void tonesweep2_set ( float tonesweep_lf, float tonesweep_hf, float tonesweep_time );
    void mixer3_set (  
      float dc_gain, 
      float sine_gain, 
      float tonesweep_gain
    );
    void chorus1_set ( int nVoices );
    void flange1_set ( int offset, int depth, int rate );
    void reverb1_set ( float time_sec );
    void delayExt1_set ( int times [ 4 ] );
    void mixer2_set ( float gains [ 4 ] );
    void mixer4_set ( float gains [ 4 ] );
    
  private:
    void tuningFreqTable_init ();
};

OpenEffectsBox::OpenEffectsBox() {
}

void OpenEffectsBox::init () {
  sgtl5000_init ();
  tuningFreqTable_init ();
  setPreset ( 0 );
}

OpenEffectsBox oeb;

const int nModes = 9;
int nSubModes = 1;  // set when mode changes
int mode = 0;
bool modeChanged = true;
int subMode = 0;
bool subModeChanged = true;
bool onoff = false;
bool boost = false;

void setVU ( int n, int mode = 1, unsigned long onColor = 0x106060UL, unsigned long offColor = 0x101010UL );

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
AudioEffectReverb        reverb1;        //xy=443,517
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
AudioConnection          patchCord9(mixer1, bitcrusher1);
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
AudioConnection          patchCord24(mixer2, 0, mixer4, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=595,184
// GUItool: end automatically generated code

/* *****************************************************
    audio system definitions and variable setups
// *****************************************************/

#define mixer1_input_L    0
#define mixer1_input_R    1
#define mixer1_sine1      2
#define mixer1_tonesweep1 3

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
  
  Serial.begin ( BAUDRATE ); while ( !Serial && millis () < 2000UL );
  
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

  delay ( 100 );
  oeb.init ();
  
  if ( 1 || Serial ) Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 100 );
}

/* *****************************************************
// *****************************************************/

void loop() {
  
  updateInputs ();
  
  if ( ( oeb.mix1_tonesweep_gain > 0.01 ) && ( ! tonesweep1.isPlaying() ) ) 
    tonesweep1.play ( 1.0, oeb.mix1_tonesweep_lf, oeb.mix1_tonesweep_hf, oeb.mix1_tonesweep_time );
  if ( ( oeb.multiply_tonesweep_gain > 0.01 ) && ( ! tonesweep2.isPlaying() ) ) 
    tonesweep2.play ( 1.0, oeb.mpy_tonesweep_lf, oeb.mpy_tonesweep_hf, oeb.mpy_tonesweep_time );
  
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
  
  /*
         KLUDGE - we're working on seeing short and long presses
         also possibly multiple presses
  */
  
  static unsigned long pb2FellAt_ms;
  if ( pb2.fell() ) {
    pb2FellAt_ms = millis();
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
  
  if ( ! pb2.read() ) {
    // button is down
    unsigned long pressDuration = millis() - pb2FellAt_ms;
    setVU ( pressDuration / 1000UL, 0, 0x401040 ); strip.show();
  }
  
  if ( pb2.rose() ) {
    unsigned long pressDuration_ms = millis() - pb2FellAt_ms;
    static int presetNumber = 0;
    for ( int i = 0; i < 3; i++ ) {
      setVU ( pressDuration_ms / 1000UL, 0, 0x401040 ); strip.show();
      delay ( 200 );
      setVU ( pressDuration_ms / 1000UL, 0, 0x000000 ); strip.show();
      delay ( 200 );
    }
    if ( pressDuration_ms > 500 ) {
      presetNumber += 1;
      if ( presetNumber > nPresets ) presetNumber = 0;
      oeb.setPreset ( presetNumber );
      displayIsStale = true;
    }
  }
  
  // flash the boost LED to indicate the number of the current preset
  static int boostBtnState = 1;
  static int nFlashes = 0;
  static unsigned long lastBoostFlashAt_ms = millis();
  const unsigned long boostFlashLongDuration_ms = 1000UL;
  const unsigned long boostFlashShortDuration_ms = 200UL;
  const unsigned long boostFlashInterval_ms = 200UL;
  const int ledBoostFlashColor = 0x206020;
  const int ledBoostQuietColor = 0x103010;
  const int ledBoostDarkColor  = 0x0c280c;
  // light is on in even states
  // fast blink to indicate the number of the state
  //   on for boostFlashShortDuration_ms, then off for boostFlashInterval_ms
  // then long lit period of boostFlashLongDuration_ms in between
  
  switch ( boostBtnState ) {
    case 0:  // quiet, on
      if ( ( ( millis() - lastBoostFlashAt_ms ) > boostFlashLongDuration_ms )
          && ( nFlashes < oeb.currentPreset ) ) {
          
        if ( Serial && VERBOSE >= 10 ) {
          Serial.print ( "boostBtnState: " ); Serial.print ( boostBtnState );
          Serial.print ( "; nFlashes: " ); Serial.print ( nFlashes );
          Serial.print ( "; currentPreset: " ); Serial.print ( oeb.currentPreset );
          Serial.print ( "; at " ); Serial.print ( millis() - lastBoostFlashAt_ms );
          Serial.println ();
        }
      
        strip.setPixelColor ( ledBoost, ledBoostDarkColor );
        strip.show();
        boostBtnState = 1;
        lastBoostFlashAt_ms = millis();
      
      }
      break;
      
    case 1:  // flashing, off
      if ( ( millis() - lastBoostFlashAt_ms ) > boostFlashInterval_ms ) {
          
          
    Serial.print ( "boostBtnState: " ); Serial.print ( boostBtnState );
    Serial.print ( "; nFlashes: " ); Serial.print ( nFlashes );
    Serial.print ( "; currentPreset: " ); Serial.print ( oeb.currentPreset );
    Serial.print ( "; at " ); Serial.print ( millis() - lastBoostFlashAt_ms );
    Serial.println ();
      
        if ( nFlashes < oeb.currentPreset ) {
          strip.setPixelColor ( ledBoost, ledBoostFlashColor );
          boostBtnState = 2;
          nFlashes++;
        } else {
          strip.setPixelColor ( ledBoost, ledBoostQuietColor );
          boostBtnState = 0;
          nFlashes = 0;
        }
        strip.show();
        lastBoostFlashAt_ms = millis();
      }
      break;
      
    case 2:  // flashing, on
      if ( ( millis() - lastBoostFlashAt_ms ) > boostFlashShortDuration_ms ) {
          
          
    Serial.print ( "boostBtnState: " ); Serial.print ( boostBtnState );
    Serial.print ( "; nFlashes: " ); Serial.print ( nFlashes );
    Serial.print ( "; currentPreset: " ); Serial.print ( oeb.currentPreset );
    Serial.print ( "; at " ); Serial.print ( millis() - lastBoostFlashAt_ms );
    Serial.println ();
      
        strip.setPixelColor ( ledBoost, 0x000000 );
        strip.show();
        boostBtnState = 1;
        lastBoostFlashAt_ms = millis();
      }
      break;
      
    default:
      boostBtnState = 0;
  }
      
  /*
          end of short and long press kludge
  */
  
  if ( potChanged [ 0 ] ) {
    outputVolume = expmap ( potReadings [ 0 ], 0, 1023, 0.01, 2.014 ) - 0.01;
    mixer4.gain ( 0, outputVolume );
    if ( VERBOSE >= 10 ) setVU ( outputVolume * 7 / 2 );   // debug
  }

  static unsigned long lastPeakAt_ms = 0UL;
  const unsigned long peakMeasurementInterval_ms = 100UL;
  
  switch ( mode ) {
    case 0:  // standard operating mode
            
      nSubModes = 1;
      
      if ( 0 && ( 1 || VERBOSE >= 10 ) ) setVU ( outputVolume * 3.5 );   // 0.1<=oV<=2.0
      
      if ( ( millis() - lastPeakAt_ms ) > peakMeasurementInterval_ms ) {
        float peakValue = peak1.read();
        int color = peakValue > 0.7 ? 0x401010 : 0x104010;
        setVU ( round ( peakValue * 7.0 ), 1, color ); 
        strip.show();
        // Serial.println ( peakValue );
        lastPeakAt_ms = millis ();
      }
      
      updateOLEDdisplay = &displayOLED_0;
      break;
  
    case 1:  // control mixer1
    
      nSubModes = 3;
      switch ( subMode ) {
        case 0:  // main gain settings for inputs
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              float val = fmap ( potReadings [ i ], 0, 1023, 0.0, 2.0 );
              switch ( i ) {
                case 1:  // inputs
                  oeb.mix1_inst_gain = val;
                  mixer1.gain ( 0, oeb.mix1_inst_gain );
                  mixer1.gain ( 1, oeb.mix1_inst_gain );
                  break;
                case 2:  // sine1
                  oeb.mix1_sin_gain = val;
                  mixer1.gain ( 2, oeb.mix1_sin_gain );
                  break;
                case 3:  // tonesweep1
                  oeb.mix1_tonesweep_gain = val;
                  mixer1.gain ( 3, oeb.mix1_tonesweep_gain );
                  break;
                default: 
                  break;
              }
              if ( VERBOSE >= 10 ) setVU ( val * 7 / 2 );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_mixer1;
          break;
          
        // no case for i2s - no functions to call!
        
        case 1:  // sine1 frequency
          if ( potChanged [ 1 ] ) {
            oeb.mix1_sine_freq = oeb.tuning_freq_table [ map ( potReadings [ 1 ], 0, 1023, 0, 4 ) ];
            sine1.frequency ( oeb.mix1_sine_freq );
            if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ 1 ], 0, 1023, 1, 5 ) );   // debug
          }
          updateOLEDdisplay = &displayOLED_mix1_sine1_frequency;
          break;
          
        case 2:  // tone sweep
          
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              switch ( i ) {
                case 1:  // tone sweep lower frequency
                  oeb.mix1_tonesweep_lf = expmap ( potReadings [ 1 ], 0, 1023, 1.0, 440.0 );
                  break;
                case 2:  // tone sweep upper frequency
                  oeb.mix1_tonesweep_hf = expmap ( potReadings [ 2 ], 0, 1023, 10.0, 10000.0 );
                  break;
                case 3:  // tone sweep time
                  oeb.mix1_tonesweep_time = expmap ( potReadings [ 3 ], 0, 1023, 0.01, 10.0 );
                  break;
                default:
                  break;
              }
              tonesweep1.play ( 1.0, oeb.mix1_tonesweep_lf, oeb.mix1_tonesweep_hf, oeb.mix1_tonesweep_time );
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_mix1_tone_sweep;
          break;
        default:
          break;
      }

      break;
    
    case 2:  // control bitcrusher
    
      nSubModes = 1;
      
      for ( int i = 1; i <= 2; i++ ) {
        if ( potChanged [ i ] ) {
          switch ( i ) {
            case 1:  // bits
              oeb.bitcrush_bits = map ( potReadings [ i ], 0, 1023, 16, 1 );
              bitcrusher1.bits ( oeb.bitcrush_bits );
              break;
            case 2:  // sample rate
              oeb.bitcrush_sampleRate = fmap ( potReadings [ i ], 0, 1023, 44100UL, 10 );
              bitcrusher1.sampleRate ( oeb.bitcrush_sampleRate );
              break;
            default: 
              break;
          }
          if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
        }
      }

      updateOLEDdisplay = &displayOLED_bitcrusher;
      break;
  
    case 3:  // control waveshaper
    
      nSubModes = 1;
      
      if ( potChanged [ 1 ] ) {
        oeb.waveshape_selection = map ( potReadings [ 1 ], 0, 1023, 0, 4 );
        oeb.waveshape1_set ( oeb.waveshape_selection );
      }

      updateOLEDdisplay = &displayOLED_waveshaper;
      break;
    
    case 4:  // control multiply
    
      nSubModes = 4;
      switch ( subMode ) {
      
        case 0:  // main gain settings for multiply function
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              switch ( i ) {
                case 1:  // dc gain
                  oeb.multiply_dc_gain = fmap ( potReadings [ 1 ], 0, 1023, 0.0, 2.0 );
                  mixer3.gain ( 0, oeb.multiply_dc_gain );
                  break;
                case 2:  // sine gain
                  oeb.multiply_sine_gain = fmap ( potReadings [ 2 ], 0, 1023, 0.0, 2.0 );
                  mixer3.gain ( 1, oeb.multiply_sine_gain );
                  break;
                case 3:  // tone sweep gain
                  oeb.multiply_tonesweep_gain = fmap ( potReadings [ 3 ], 0, 1023, 0.0, 2.0 );
                  mixer3.gain ( 2, oeb.multiply_tonesweep_gain );
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
          if ( potChanged [ 1 ] ) {
            oeb.mpy_dc_amp = fmap ( potReadings [ 1 ], 0, 1023, -1.0, 1.0 );
            dc1.amplitude ( oeb.mpy_dc_amp, 20 );
            if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ 1 ], 0, 1023, 0, 7 ) );   // debug
          }
          updateOLEDdisplay = &displayOLED_mpy_dc_gain;
          break;
          
        case 2:  // sine2 frequency
          if ( potChanged [ 1 ] ) {
            // sine frequency
            oeb.mpy_sine_freq = expmap ( potReadings [ 1 ], 0, 1023, 0.01, 10.0 );
            sine2.frequency ( oeb.mpy_sine_freq );
            if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ 1 ], 0, 1023, 0, 7 ) );   // debug
            }
          updateOLEDdisplay = &displayOLED_mpy_sine2_frequency;
          break;
          
        case 3:  // tone sweep
          
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              switch ( i ) {
                case 1:  // tone sweep lower frequency
                  oeb.mpy_tonesweep_lf = expmap ( potReadings [ 1 ], 0, 1023, 1.0, 440.0 );
                  break;
                case 2:  // tone sweep upper frequency
                  oeb.mpy_tonesweep_hf = expmap ( potReadings [ 2 ], 0, 1023, 10.0, 10000.0 );
                  break;
                case 3:  // tone sweep time
                  oeb.mpy_tonesweep_time = expmap ( potReadings [ 3 ], 0, 1023, 0.1, 10.0 );
                  break;
                default:
                  break;
              }
              tonesweep2.play ( 1.0, oeb.mpy_tonesweep_lf, oeb.mpy_tonesweep_hf, oeb.mpy_tonesweep_time );
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_mpy_tone_sweep;
          break;
      }

      break;
  
    
    case 5:  // control chorus
    
      nSubModes = 1;
      
      if ( potChanged [ 1 ] ) {
        oeb.chorus_nVoices = map ( potReadings [ 1 ], 0, 1023, 1, 5 );
        // chorus1.modify ( map ( potReadings [ 0 ], 0, 1023, 1, 5 ) );
        //   Arduino claims chorus1 has no member "modify"
        chorus1.voices ( oeb.chorus_nVoices );
      }
      
      updateOLEDdisplay = &displayOLED_chorus;
      break;
    
    case 6:  // control flange
    
      nSubModes = 1;
      
      for ( int i = 1; i <= 3; i++ ) {
        if ( potChanged [ i ] ) {
          switch ( i ) {
            case 1:  // offset - fixed distance behind current
              oeb.flange_offset = map ( potReadings [ i ], 0, 1023, 0, FLANGE_DELAY_LENGTH );
              break;
            case 2:  // depth - the size of the variation of offset
              oeb.flange_depth = map ( potReadings [ i ], 0, 1023, 0, FLANGE_DELAY_LENGTH );
              break;
            case 3:  // rate - the frequency of the variation of offset
              oeb.flange_rate = expmap ( potReadings [ i ], 0, 1023, 0.1, 10.0 );
              break;
            default:
              break;
          }
          flange1.voices ( oeb.flange_offset, oeb.flange_depth, oeb.flange_rate );
          if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
        }
      }

      updateOLEDdisplay = &displayOLED_flange;
      break;
    
    case 7:  // control reverb
    
      nSubModes = 1;
      
      if ( potChanged [ 1 ] ) {
        oeb.reverbTime_sec = fmap ( potReadings [ 1 ], 0, 1023, 0.0, 5.0 );
        if ( BAUDRATE && VERBOSE >= 10 ) { 
          Serial.print ( millis() ); Serial.print ( ": " );
          Serial.print ( "reverb: " ); Serial.print ( oeb.reverbTime_sec ); Serial.println ( "sec" );
        }
        reverb1.reverbTime ( oeb.reverbTime_sec );
      }

      updateOLEDdisplay = &displayOLED_reverb;
      break;
    
    case 8:  // control delays
    
      nSubModes = 2;
      
      switch ( subMode ) {
        case 0:  // delay times beyond delay 0
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              oeb.delay_times_ms [ i ] = expmap ( potReadings [ i ], 0, 1023, 0.1, 1485.13 ) - 0.1;
              delayExt1.delay ( i, oeb.delay_times_ms [ i ] );
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_delay_times;
          break;
        case 1:  // gain for each delay beyond 0
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              oeb.mixer2_gains [ i ] = expmap ( potReadings [ i ], 0, 1023, 0.01, 2.013 ) - 0.01;
              mixer2.gain ( i, oeb.mixer2_gains [ i ] );
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_mixer2;
          break;
        default:
          break;
      }
      break;
    
    default:
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

  static unsigned long lastPotChangeAt_ms [ 4 ] = { millis(), millis(), millis(), millis() };
  
  // ----------------------
  // read switches and pots
  // ----------------------
  
  for ( int i = 0; i < nPots; i++ ) {
    potReadings [ i ] = 1023 - analogRead ( pa_pots [ i ] );
    if ( abs ( potReadings [ i ] - oldPotReadings [ i ] ) > potHysteresis ) {
      oldPotReadings [ i ] = potReadings [ i ];
      lastPotChangeAt_ms [ i ] = millis();
    }
    if ( ( millis() - lastPotChangeAt_ms [ i ] ) < 100 ) {
      potChanged [ i ] = true;
      displayIsStale = true;
    } else {
      potChanged [ i ] = false;
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

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
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

  oled.setTextSize ( 3 );
  oled.setCursor ( 40, 20 );
  oled.print ( outputVolume );

  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
  
}

void displayOLED_mixer1 () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once  
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
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
  oled.print ( oeb.mix1_inst_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "inp" );

  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.mix1_sin_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "sin" );

  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.mix1_tonesweep_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "swp" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}

void displayOLED_mix1_sine1_frequency () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "sine freq" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.mix1_sine_freq );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "freq" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
        
}
  
void displayOLED_mix1_tone_sweep () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "tone sweep" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.mix1_tonesweep_lf );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "lo f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.mix1_tonesweep_hf );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "hi f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.mix1_tonesweep_time );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "ms" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}
  
void displayOLED_bitcrusher () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  // bitcrusher
  //   0 for bits ( 0 is 16 bits => pass-through )
  //   1 for sampling rate ( 0 is 44100 => pass-through )
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "bitcrush" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.bitcrush_bits );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "bits" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.bitcrush_sampleRate );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "samp" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}

void displayOLED_waveshaper () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "waveshape" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.waveshape_selection );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "menu" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}
  
void displayOLED_multiply () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
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
  oled.print ( oeb.multiply_dc_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "dc" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.multiply_sine_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "sin" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.multiply_tonesweep_gain );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "swp" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}
  
void displayOLED_mpy_dc_gain () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "dc amp" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.mpy_dc_amp );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "val" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}
  
void displayOLED_mpy_sine2_frequency () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "sine freq" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.mpy_sine_freq );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "freq" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}
  
void displayOLED_mpy_tone_sweep () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "tone sweep" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.mpy_tonesweep_lf );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "lo f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.mpy_tonesweep_hf );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "hi f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.mpy_tonesweep_time );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "ms" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}
  
void displayOLED_chorus () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "chorus" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 64, 16 );
  oled.print ( oeb.chorus_nVoices );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "voices" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}

void displayOLED_flange () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "flange" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.flange_offset );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "ofst" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.flange_depth );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "dpth" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.flange_rate );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "rate" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}

void displayOLED_reverb () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "reverb" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.reverbTime_sec );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "sec" );
    
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}

void displayOLED_delay_times () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "delays" );
  
  for ( int i = 1; i < 4; i++ ) {
    int y = ( i - 1 ) * 16 + 16;
    oled.setTextSize ( 1 );
    oled.setCursor ( 30, y );
    oled.print ( oeb.delay_times_ms [ i ] );
    oled.setTextSize ( 1 );
    oled.setCursor ( 0, y );
    oled.print ( "ms" );
  }
    
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}

void displayOLED_mixer2 () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    return;
  }

  // update display, but only once
  displayUpdateRate_ms = 0xFFFFFFFF; // 200UL;
  
  displayOLED_common ();  // displays mode

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "delay gain" );
  
  for ( int i = 1; i < 4; i++ ) {
    int y = ( i - 1 ) * 16 + 16;
    oled.setTextSize ( 1 );
    oled.setCursor ( 30, y );
    oled.print ( oeb.mixer2_gains [ i ] );
    // oled.setTextSize ( 1 );
    // oled.setCursor ( 0, y );
    // oled.print ( "" );
  }
    
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}

void displayOLED_common () {

  displayIsStale = false;

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

void setVU ( int n, int mode, unsigned long onColor, unsigned long offColor ) {
  const int VUfirstPixel = 3;
  const int nVUpixels = 7;
  
  // if ( n < 0 || n > nVUpixels ) {
  //   Serial.println ( "setVU: invalid bargraph value ( 0 - 7 ): " );
  //   Serial.println ( n );
  // }
  for ( int i = 0; i < nVUpixels; i++ ) {
    unsigned long theColor = offColor;
    switch ( mode ) {
      case -1:  // from right
        if ( i > n ) theColor = onColor;
        break;
      case 0:   // just one led on
        if ( i == n ) theColor = onColor;
        break;
      case 1:   // normal mode: from left
        if ( i < n ) theColor = onColor;
        break;
      default:
        break;
    }
    strip.setPixelColor ( VUfirstPixel + i, theColor );
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

void OpenEffectsBox::sgtl5000_init () {
  #if 1
    sgtl5000_1.inputSelect ( AUDIO_INPUT_LINEIN );
    // 0 is 3.12Vp-p; default 5 is 1.33Vp-p; min 15 is 0.24Vp-p
    sgtl5000_1.lineInLevel ( 10 );   
  #else
    sgtl5000_1.inputSelect ( AUDIO_INPUT_MIC );
    sgtl5000_1.micGain ( 20 );      // dB
  #endif
  sgtl5000_1.volume ( 0.8 );  // headphones only, not line-level outputs
  // max 13 is 3.16Vp-p; default 29 is 1.29Vp-p; min 31 is 1.16Vp-p
  sgtl5000_1.lineOutLevel ( 13 );   
}

void OpenEffectsBox::sine1_set ( float sine_freq ) {
  mix1_sine_freq = sine_freq;
  sine1.amplitude ( 1.0 );
  sine1.frequency ( mix1_sine_freq );
}

// tonesweep1: play will be automatic when the gain is positive
void OpenEffectsBox::tonesweep1_set ( float tonesweep_lf, float tonesweep_hf, float tonesweep_time ) {
  mix1_tonesweep_lf = tonesweep_lf;
  mix1_tonesweep_hf = tonesweep_hf;
  mix1_tonesweep_time = tonesweep_time;
}

void OpenEffectsBox::mixer1_set ( float inst_gain, float sin_gain, float tonesweep_gain ) {
  mix1_inst_gain = inst_gain;
  mix1_sin_gain = sin_gain;
  mix1_tonesweep_gain = tonesweep_gain;
  mixer1.gain ( mixer1_input_L, mix1_inst_gain );   // i2s2 ( line inputs L & R )
  mixer1.gain ( mixer1_input_R, mix1_inst_gain );   // i2s2 ( line inputs L & R )
  mixer1.gain ( mixer1_sine1, mix1_sin_gain );   // sine1
  mixer1.gain ( mixer1_tonesweep1, mix1_tonesweep_gain );   // tonesweep1
}

void OpenEffectsBox::bitcrusher1_set ( int bits, unsigned long sampleRate ) {
  bitcrush_bits = bits;
  bitcrush_sampleRate = sampleRate;
  bitcrusher1.bits ( bitcrush_bits );
  bitcrusher1.sampleRate ( bitcrush_sampleRate );
}
  
void OpenEffectsBox::waveshape1_set ( int waveshape ) {
    
    static float waveShapes [ 5 ] [ 9 ] = { 
      { -1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
      { -1.0, -0.3,  -0.2,  -0.1, 0.0, 0.1, 0.2,  0.3,  1.0 },
      { -1.0, -0.98, -0.95, -0.1, 0.0, 0.1, 0.95, 0.98, 1.0 },
      {  1.0,  0.3,   0.2,   0.1, 0.0, 0.1, 0.2,  0.3,  1.0 },
      {  1.0,  0.98,  0.5,   0.1, 0.0, 0.1, 0.5,  0.98, 1.0 }
    };
    
  waveshape_selection = waveshape;
  waveshape1.shape ( waveShapes [ waveshape_selection ], waveshape_selection ? 9 : 2 );
}
  
void OpenEffectsBox::dc1_set ( int dc_gain ) {
  multiply_dc_gain = dc_gain;
  dc1.amplitude ( multiply_dc_gain );
}

void OpenEffectsBox::sine2_set ( float sine_freq ) {
  mpy_sine_freq = sine_freq;
  sine2.amplitude ( 1.0 );
  sine2.frequency ( mpy_sine_freq );
}

// tonesweep2: play will be automatic when the gain is positive
void OpenEffectsBox::tonesweep2_set ( float tonesweep_lf, float tonesweep_hf, float tonesweep_time ) {
  mpy_tonesweep_lf = tonesweep_lf;
  mpy_tonesweep_hf = tonesweep_hf;
  mpy_tonesweep_time = tonesweep_time;
}

void OpenEffectsBox::mixer3_set ( 
    float dc_gain, 
    float sine_gain, 
    float tonesweep_gain ) {
    
  multiply_dc_gain = dc_gain;
  multiply_sine_gain = sine_gain;
  multiply_tonesweep_gain = tonesweep_gain;
  
  mixer3.gain ( 0, multiply_dc_gain );
  mixer3.gain ( 1, multiply_sine_gain );
  mixer3.gain ( 2, multiply_tonesweep_gain );
    
}

void OpenEffectsBox::chorus1_set ( int chorus_n ) {
  chorus_nVoices = chorus_n;
  chorus1.begin ( chorusDelayLine, CHORUS_DELAY_LENGTH, chorus_nVoices );  // 1 is passthru
}

void OpenEffectsBox::flange1_set ( int offset, int depth, int rate) {
  flange_offset = offset;
  flange_depth = depth;
  flange_rate = rate;
  flange1.begin ( flangeDelayLine, FLANGE_DELAY_LENGTH, 
    flange_offset, flange_depth, flange_rate );
}

void OpenEffectsBox::reverb1_set ( float time_sec ) {
  reverbTime_sec = time_sec;
  if ( BAUDRATE && VERBOSE >= 10 ) { 
    Serial.print ( millis() ); Serial.print ( ": " );
    Serial.print ( "reverb: " ); Serial.print ( reverbTime_sec ); Serial.println ( "sec" );
  }
  reverb1.reverbTime ( reverbTime_sec );
}

void OpenEffectsBox::delayExt1_set ( int times_ms [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    delay_times_ms [ i ] = times_ms [ i ];
    delayExt1.delay ( i , delay_times_ms [ i ] );
  }
  for ( int i = 4; i < 8; i++ ) {
    delayExt1.disable ( i );
  }
}
  
void OpenEffectsBox::mixer2_set ( float gains [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    mixer2_gains [ i ] = gains [ i ];
    mixer2.gain ( i, mixer2_gains [ i ] );
  }
}

void OpenEffectsBox::mixer4_set ( float gains [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    mixer4_gains [ i ] = gains [ i ];
    mixer4.gain ( i, mixer4_gains [ i ] );
  }
}

void OpenEffectsBox::tuningFreqTable_init () {
  tuning_freq_table [ 0 ] = 440.00;  // A4
  tuning_freq_table [ 1 ] = 293.66;  // D4
  tuning_freq_table [ 2 ] = 196.00;  // G3
  tuning_freq_table [ 3 ] = 130.81;  // C3
  tuning_freq_table [ 4 ] = 659.25;  // E5
}

void OpenEffectsBox::setPreset ( int ps ) {
  currentPreset = ps;
  switch ( currentPreset ) {
    case 0: // reinitialize board to straight through
      {
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        mixer1_set ( 1.0, 0.0, 0.0 );
        // mix1_sine_freq
        sine1_set ( 440.0 );
        
        //  tonesweep_lf, tonesweep_hf, tonesweep_time                       
        tonesweep1_set ( 55.0, 3520.0, 4.0 );

        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher1_set ( 16, 44100 );

        /*
          0 - ws1_passthru / 2
          1 - ws1_shape1 / 9
          2 - ws1_shape2 / 9
          3 - ws1_shape3 / 9
          4 - ws1_shape4 / 9
        */
        waveshape1_set ( 0 );

        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        dc1_set ( 1.0 );
        // mpy_sine_freq
        sine2_set ( 2.0 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mixer3_set ( 1.0, 0.0, 0.0 );

        //  tonesweep_lf, tonesweep_hf, tonesweep_time                       
        tonesweep2_set ( 55.0, 3520.0, 4.0 );

        // chorus_nVoices
        chorus1_set ( 1 );

        // flange_offset, flange_depth, flange_rate
        flange1_set ( 0, 0, 0 );

        // reverbTime_sec
        reverb1_set ( 0.0 );

        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 0, 0, 0 };
        delayExt1_set ( dt_ms );

        // mixer2 mixes delays
        float mg [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        mixer2_set ( mg );
        mixer4_set ( mg );
      }
      break;
      
    case 1: // preset 1 - nice with reverb but mostly straight
      {
        // mix1_sine_freq
        sine1_set ( 440.0 );
        // tonesweep needs no init; parameters are explicit in start
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        mixer1_set ( 1.0, 0.0, 0.0 );
        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher1_set ( 16, 44100 );
        // waveshape_selection
        waveshape1_set ( 0 );
  
        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        dc1_set ( 0.5 );
        // mpy_sine_freq
        sine2_set ( 0.25 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mixer3_set ( 1.0, 0.1, 0.0 );
  
        // chorus_nVoices
        chorus1_set ( 2 );
        // flange_offset, flange_depth, flange_rate
        flange1_set ( 0, 0, 0 );
        // reverbTime_sec
        reverb1_set ( 1.0 );
        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 100, 200, 300 };
        delayExt1_set ( dt_ms );
        // mixer2 mixes delays
        float m2g [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        mixer2_set ( m2g );
        // mixer4 is output volume
        float m4g [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        mixer4_set ( m4g );
      }
      break;

    case 2: // preset 2 a little wild
      {
        // mix1_sine_freq
        sine1_set ( 440.0 );
        // tonesweep needs no init; parameters are explicit in start
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        mixer1_set ( 1.0, 0.0, 0.0 );
        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher1_set ( 16, 44100 );
        // waveshape_selection
        waveshape1_set ( 2 );
  
        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        dc1_set ( 0.5 );
        // mpy_sine_freq
        sine2_set ( 6.0 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mixer3_set ( 1.0, 0.5, 0.0 );
  
        // chorus_nVoices
        chorus1_set ( 2 );
        // flange_offset, flange_depth, flange_rate
        flange1_set ( 0, 0, 0 );
        // reverbTime_sec
        reverb1_set ( 1.0 );
        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 100, 200, 300 };
        delayExt1_set ( dt_ms );
        // mixer2 mixes delays
        float m2g [ 4 ] = { 1.0, 0.8, 0.6, 0.4 };
        mixer2_set ( m2g );
        // mixer4 is output volume
        float m4g [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        mixer4_set ( m4g );
      }
      break;
        
    case 3: // preset 3 for tuning
      {
        // mix1_sine_freq
        sine1_set ( 440.0 );
        // tonesweep needs no init; parameters are explicit in start
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        mixer1_set ( 1.0, 0.10, 0.0 );
        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher1_set ( 16, 44100 );
        // waveshape_selection
        waveshape1_set ( 2 );
  
        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        dc1_set ( 0.5 );
        // mpy_sine_freq
        sine2_set ( 6.0 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mixer3_set ( 1.0, 0.5, 0.0 );
  
        // chorus_nVoices
        chorus1_set ( 2 );
        // flange_offset, flange_depth, flange_rate
        flange1_set ( 0, 0, 0 );
        // reverbTime_sec
        reverb1_set ( 1.0 );
        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 100, 200, 300 };
        delayExt1_set ( dt_ms );
        // mixer2 mixes delays
        float m2g [ 4 ] = { 1.0, 0.8, 0.6, 0.4 };
        mixer2_set ( m2g );
        // mixer4 is output volume
        float m4g [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        mixer4_set ( m4g );
      }
      break;
    default:
      break;
  }
}
