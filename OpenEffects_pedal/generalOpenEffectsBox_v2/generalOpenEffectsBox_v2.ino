#define PROGNAME  "generalOpenEffectsBox_v2"
#define VERSION   "2.7.15"
#define VERDATE   "2017-12-09"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// use Paul Stoffregen's NeoPixel library if using Teensy 3.5 or 3.6
#include <Adafruit_NeoPixel.h>
#include <Audio.h>

#include <SPI.h>
#include <Wire.h>
#include <Bounce2.h>

/*
   NOTE - cannot use standard Serial with OpenEffects or Audio boards
   Actually, as of 2017-11-16, it appears that you can indeed. Maybe
   related to which Teensy -- long vs. short?
*/
// #undef BAUDRATE
#define BAUDRATE 115200
#define VERBOSE 12
#undef STRING1_REPLACES_TONESWEEP1

/*

  Pot 0 is output volume
  
  Modes selected by left bat switch
    0 - standard operating mode
        knob 0 - output volume
        
    1 - mixer1
        knob 0 - output volume
        knob 1 - L&R input gain
        knob 2 - tuning_sine_ gain
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
        0 - gains
          knob 1 - delay 1 gain
          knob 2 - delay 2 gain
          knob 3 - delay 3 gain
        1 - delay times
          knob 1 - delay 1 time (ms)
          knob 2 - delay 2 time (ms)
          knob 3 - delay 3 time (ms)


Note: I have added waveform1 as a 4th input to mixer3, but have not written
it into the code yet. Have, too!
      
Fun idea: 853Hz and 960Hz for the CONELRAD Attention Signal...

*/


/*
  Please remember that the standard Serial and the standard LED on pin 13
  both interfere with audio board pin mappings. Don't use either of them!
  They will work, but their use will disable the audio board's workings.
  It turns out that standard Serial is just fine. No problem.
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

// Pinout board rev1
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
Adafruit_NeoPixel strip = Adafruit_NeoPixel ( nPixels, pdWS2812, NEO_GRB + NEO_KHZ800 );
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

const int nPresets = 5;

class OpenEffectsBox {
  public:
  
    OpenEffectsBox ();  // constructor
    
    int currentPreset;
    
    void init ();
    void initializeStateVector ();
    void setPreset ( int presetNumber );
    void sgtl5000_init ();
    
    float tuningFreq_get ( int index );

    void tuning_sine_set ( float sine_freq );
    float tuning_sine_get ( );
    void tonesweep1_set ( float tonesweep_lf, float tonesweep_hf, float tonesweep_time );
    void tonesweep1_lf_set ( float tonesweep_lf );
    void tonesweep1_hf_set ( float tonesweep_hf );
    void tonesweep1_time_set ( float tonesweep_time );
    float tonesweep1_lf_get ();
    float tonesweep1_hf_get ();
    float tonesweep1_time_get ();
    void input_mixer_set ( float inst_gain, float sin_gain, float tonesweep_gain );
    void input_mixer_inst_gain_set ( float inst_gain );
    void input_mixer_sin_gain_set ( float sin_gain );
    void input_mixer_tonesweep_gain_set ( float tonesweep_gain );
    float input_mixer_inst_gain_get ();
    float input_mixer_sin_gain_get ();
    float input_mixer_tonesweep_gain_get ();
    void bitcrusher_set ( int bits, unsigned long sampleRate );
    void bitcrusher_bits_set ( int bits );
    void bitcrusher_sampleRate_set ( unsigned long sampleRate );
    int bitcrusher_bits_get ();
    unsigned long bitcrusher_sampleRate_get ();
    void waveshape_set ( int waveshape );
    int waveshape_get ();
    void mpy_dc_level_set ( float dc_gain );
    float mpy_dc_level_get ();
    void mpy_sine_freq_set ( float sine_freq );
    float mpy_sine_freq_get ();
    void mpy_tonesweep_set ( float tonesweep_lf, float tonesweep_hf, float tonesweep_time );
    void mpy_tonesweep_lf_set ( float tonesweep_lf );
    void mpy_tonesweep_hf_set ( float tonesweep_hf );
    void mpy_tonesweep_time_set ( float tonesweep_time );
    float mpy_tonesweep_lf_get ();
    float mpy_tonesweep_hf_get ();
    float mpy_tonesweep_time_get ();
    void mpy_mixer_set (  
      float dc_gain, 
      float sine_gain, 
      float tonesweep_gain
    );
    void mpy_mixer_dc_gain_set ( float dc_gain );
    void mpy_mixer_sin_gain_set ( float sine_gain );
    void mpy_mixer_tonesweep_gain_set ( float tonesweep_gain );
    void mpy_mixer_set ();
    float mpy_mixer_dc_gain_get ();
    float mpy_mixer_sin_gain_get ();
    float mpy_mixer_tonesweep_gain_get ();
    void chorus_set ( int nVoices );
    int chorus_get ();
    void flange_set ( int offset, int depth, float rate );
    void flange_start ();
    void flange_offset_set ( int offset );
    void flange_depth_set ( int depth );
    void flange_rate_set ( float rate );
    int flange_offset_get ();
    int flange_depth_get ();
    float flange_rate_get ();
    void reverb_set ( float time_sec );
    float reverb_get ();
    void delay_times_set ( int times [ 4 ] );
    void delay_times_set ();
    void delay_times_get ( int times_ms [ 4 ] );
    void delay_mixer_gains_set ( float gains [ 4 ] );
    void delay_mixer_gains_get ( float gains [ 4 ] );
    void output_mixer_gains_set ( float gains [ 4 ] );
    void output_mixer_gains_get ( float gains [ 4 ] );
    
    
  private:
  
    /***************************************************
                       state vector!
     ***************************************************/
 
    void tuningFreqTable_init ();

    float _mix1_inst_gain;
    float _mix1_sin_gain;
    float _mix1_tonesweep_gain;
    float _mix1_sine_freq;
    float _tuning_freq_table [ 5 ];
    float _mix1_tonesweep_lf;
    float _mix1_tonesweep_hf;
    float _mix1_tonesweep_time;

    int _bitcrush_bits;
    unsigned long _bitcrush_sampleRate;

    int _waveshape_selection;
    /*
      0 - _wS1_passthru / 2
      1 - _wS1_shape1 / 9
      2 - _wS1_shape2 / 9
      3 - _wS1_shape3 / 9
      4 - _wS1_shape4 / 9
    */

    float _multiply_dc_gain;
    float _multiply_sine_gain;
    float _multiply_tonesweep_gain;
    float _multiply_dc_amp;    
    float _multiply_sine_freq;
    float _multiply_tonesweep_lf;
    float _multiply_tonesweep_hf;
    float _multiply_tonesweep_time;

    int _chorus_nVoices;

    int _flange_offset;
    int _flange_depth;
    float _flange_rate;

    float _reverbTime_sec;

    int _delay_times_ms [ 4 ];

    float _delay_mixer_gains [ 4 ];
    float _output_mixer_gains [ 4 ];
    
};

OpenEffectsBox::OpenEffectsBox() {}

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

#define input_mixer_input_L    0
#define input_mixer_input_R    1
#define input_mixer_tuning_sine_      2
#define input_mixer_tonesweep1 3

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
  
  if ( ( oeb.input_mixer_tonesweep_gain_get () > 0.01 ) && ( ! tonesweep1.isPlaying() ) ) 
    tonesweep1.play ( 1.0, oeb.tonesweep1_lf_get (), oeb.tonesweep1_hf_get (), oeb.tonesweep1_time_get () );
  if ( ( oeb.mpy_mixer_tonesweep_gain_get () > 0.01 ) && ( ! tonesweep2.isPlaying() ) ) 
    tonesweep2.play ( 1.0, oeb.mpy_tonesweep_lf_get (), oeb.mpy_tonesweep_hf_get (), oeb.mpy_tonesweep_time_get () );
    
  #ifdef STRING1_REPLACES_TONESWEEP1
    static unsigned long lastString1At_ms = 0UL;
    if ( ( millis() - lastString1At_ms ) > 1000UL ) {
      string1.noteOn ( 440.0, 0.5 );
      lastString1At_ms = millis ();
    }
  #endif
  
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
      if ( presetNumber >= nPresets ) presetNumber = 0;
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
  const int ledBoostDarkColor  = 0x103010;  // 0x0c280c;
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
      
        strip.setPixelColor ( ledBoost, ledBoostQuietColor );
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
                  oeb.input_mixer_inst_gain_set ( val );
                  break;
                case 2:  // tuning_sine_
                  oeb.input_mixer_sin_gain_set ( val );
                  break;
                case 3:  // tonesweep1
                  oeb.input_mixer_tonesweep_gain_set ( val );
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
        
        case 1:  // tuning_sine_ frequency
          if ( potChanged [ 1 ] ) {
            oeb.tuning_sine_set ( oeb.tuningFreq_get ( map ( potReadings [ 1 ], 0, 1023, 0, 4 ) ) );
            if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ 1 ], 0, 1023, 1, 5 ) );   // debug
          }
          updateOLEDdisplay = &displayOLED_mix1_tuning_sine__frequency;
          break;
          
        case 2:  // tone sweep
          
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              switch ( i ) {
                case 1:  // tone sweep lower frequency
                  oeb.tonesweep1_lf_set ( expmap ( potReadings [ 1 ], 0, 1023, 1.0, 440.0 ) );
                  break;
                case 2:  // tone sweep upper frequency
                  oeb.tonesweep1_hf_set ( expmap ( potReadings [ 2 ], 0, 1023, 10.0, 10000.0 ) );
                  break;
                case 3:  // tone sweep time
                  oeb.tonesweep1_time_set ( expmap ( potReadings [ 3 ], 0, 1023, 0.01, 10.0 ) );
                  break;
                default:
                  break;
              }
              tonesweep1.play ( 1.0, oeb.tonesweep1_lf_get (), oeb.tonesweep1_hf_get (), oeb.tonesweep1_time_get () );
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
              oeb.bitcrusher_bits_set ( map ( potReadings [ i ], 0, 1023, 16, 1 ) );
              break;
            case 2:  // sample rate
              oeb.bitcrusher_sampleRate_set ( fmap ( potReadings [ i ], 0, 1023, 44100UL, 10 ) );
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
        oeb.waveshape_set ( map ( potReadings [ 1 ], 0, 1023, 0, 4 ) );
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
                  oeb.mpy_mixer_dc_gain_set ( fmap ( potReadings [ 1 ], 0, 1023, 0.0, 2.0 ) );
                  break;
                case 2:  // sine gain
                  oeb.mpy_mixer_sin_gain_set ( fmap ( potReadings [ 2 ], 0, 1023, 0.0, 2.0 ) );
                  break;
                case 3:  // tone sweep gain
                  oeb.mpy_mixer_tonesweep_gain_set ( fmap ( potReadings [ 3 ], 0, 1023, 0.0, 2.0 ) );
                  break;
                default:
                  break;
              }
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_multiply;
          break;
          
        case 1:  // dc level
          if ( potChanged [ 1 ] ) {
            oeb.mpy_dc_level_set ( fmap ( potReadings [ 1 ], 0, 1023, -1.0, 1.0 ) );
            if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ 1 ], 0, 1023, 0, 7 ) );   // debug
          }
          updateOLEDdisplay = &displayOLED_mpy_dc_level;
          break;
          
        case 2:  // sine2 frequency
          if ( potChanged [ 1 ] ) {
            // sine frequency
            oeb.mpy_sine_freq_set ( expmap ( potReadings [ 1 ], 0, 1023, 0.01, 10.0 ) );
            if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ 1 ], 0, 1023, 0, 7 ) );   // debug
            }
          updateOLEDdisplay = &displayOLED_mpy_sine_frequency;
          break;
          
        case 3:  // tone sweep
          
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              switch ( i ) {
                case 1:  // tone sweep lower frequency
                  oeb.mpy_tonesweep_lf_set ( expmap ( potReadings [ 1 ], 0, 1023, 1.0, 440.0 ) );
                  break;
                case 2:  // tone sweep upper frequency
                  oeb.mpy_tonesweep_hf_set ( expmap ( potReadings [ 2 ], 0, 1023, 10.0, 10000.0 ) );
                  break;
                case 3:  // tone sweep time
                  oeb.mpy_tonesweep_time_set ( expmap ( potReadings [ 3 ], 0, 1023, 0.1, 10.0 ) );
                  break;
                default:
                  break;
              }
              tonesweep2.play ( 1.0, oeb.mpy_tonesweep_lf_get (), oeb.mpy_tonesweep_hf_get (), oeb.mpy_tonesweep_time_get () );
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
        oeb.chorus_set ( map ( potReadings [ 1 ], 0, 1023, 1, 5 ) );
      }
      
      updateOLEDdisplay = &displayOLED_chorus;
      break;
    
    case 6:  // control flange
    
      nSubModes = 1;
      
      for ( int i = 1; i <= 3; i++ ) {
        if ( potChanged [ i ] ) {
          switch ( i ) {
            case 1:  // offset - fixed distance behind current
              oeb.flange_offset_set ( map ( potReadings [ i ], 0, 1023, 0, FLANGE_DELAY_LENGTH ) );
              break;
            case 2:  // depth - the size of the variation of offset
              oeb.flange_depth_set ( map ( potReadings [ i ], 0, 1023, 0, FLANGE_DELAY_LENGTH ) );
              break;
            case 3:  // rate - the frequency of the variation of offset
              oeb.flange_rate_set ( expmap ( potReadings [ i ], 0, 1023, 0.1, 10.0 ) );
              break;
            default:
              break;
          }
          oeb.flange_start ();
          if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
        }
      }

      updateOLEDdisplay = &displayOLED_flange;
      break;
    
    case 7:  // control reverb
    
      nSubModes = 1;
      
      if ( potChanged [ 1 ] ) {
        oeb.reverb_set ( fmap ( potReadings [ 1 ], 0, 1023, 0.0, 5.0 ) );
        if ( BAUDRATE && VERBOSE >= 10 ) { 
          Serial.print ( millis() ); Serial.print ( ": " );
          Serial.print ( "reverb: " ); Serial.print ( oeb.reverb_get () ); Serial.println ( "sec" );
        }
      }

      updateOLEDdisplay = &displayOLED_reverb;
      break;
    
    case 8:  // control delays
    
      nSubModes = 2;
      
      switch ( subMode ) {
        case 0:  // gain for each delay beyond 0
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              float delay_mixer_gains [ 4 ];
              oeb.delay_mixer_gains_get ( delay_mixer_gains );
              delay_mixer_gains [ i ] = expmap ( potReadings [ i ], 0, 1023, 0.01, 2.013 ) - 0.01;
              oeb.delay_mixer_gains_set ( delay_mixer_gains );
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_mixer2;
          break;
        case 1:  // delay times beyond delay 0
          for ( int i = 1; i <= 3; i++ ) {
            if ( potChanged [ i ] ) {
              int delay_times_ms [ 4 ];
              oeb.delay_times_get ( delay_times_ms );
              delay_times_ms [ i ] = expmap ( potReadings [ i ], 0, 1023, 0.1, 1485.13 ) - 0.1;
              oeb.delay_times_set ( delay_times_ms );
              if ( VERBOSE >= 10 ) setVU ( map ( potReadings [ i ], 0, 1023, 0, 7 ) );   // debug
            }
          }
          updateOLEDdisplay = &displayOLED_delay_times;
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
  //   2 is tuning_sine_
  //   3 is tonesweep1
  
  // 3 gains, pots 1-3
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 0 );
  oled.print ( "input mix" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 16 );
  oled.print ( oeb.input_mixer_inst_gain_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "inp" );

  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.input_mixer_sin_gain_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "sin" );

  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.input_mixer_tonesweep_gain_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "swp" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}

void displayOLED_mix1_tuning_sine__frequency () {

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
  oled.print ( oeb.tuning_sine_get () );
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
  oled.print ( oeb.tonesweep1_lf_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "lo f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.tonesweep1_hf_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "hi f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.tonesweep1_time_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "s" );
  
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
  oled.print ( oeb.bitcrusher_bits_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "bits" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.bitcrusher_sampleRate_get () );
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
  oled.print ( oeb.waveshape_get () );
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
  oled.print ( oeb.mpy_mixer_dc_gain_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "dc" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.mpy_mixer_sin_gain_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "sin" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.mpy_mixer_tonesweep_gain_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "swp" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}
  
void displayOLED_mpy_dc_level () {

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
  oled.print ( oeb.mpy_mixer_dc_gain_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "val" );
  
  oled.display();  // note takes about 100ms!!!
  lastOledUpdateAt_ms = millis ();
    
}
  
void displayOLED_mpy_sine_frequency () {

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
  oled.print ( oeb.mpy_sine_freq_get () );
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
  oled.print ( oeb.mpy_tonesweep_lf_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "lo f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.mpy_tonesweep_hf_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "hi f" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.mpy_tonesweep_time_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 52 );
  oled.print ( "s" );
  
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
  oled.print ( oeb.chorus_get () );
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
  oled.print ( oeb.flange_offset_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 20 );
  oled.print ( "ofst" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 32 );
  oled.print ( oeb.flange_depth_get () );
  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 36 );
  oled.print ( "dpth" );
  
  oled.setTextSize ( 2 );
  oled.setCursor ( 30, 48 );
  oled.print ( oeb.flange_rate_get () );
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
  oled.print ( oeb.reverb_get () );
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
  
  int delay_times [ 4 ];
  oeb.delay_times_get ( delay_times );
  for ( int i = 1; i < 4; i++ ) {
    int y = ( i - 1 ) * 16 + 16;
    oled.setTextSize ( 1 );
    oled.setCursor ( 30, y );
    oled.print ( delay_times [ i ] );
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
  
  float mixer_gains [ 4 ];
  oeb.delay_mixer_gains_get ( mixer_gains );
  for ( int i = 1; i < 4; i++ ) {
    int y = ( i - 1 ) * 16 + 16;
    oled.setTextSize ( 1 );
    oled.setCursor ( 30, y );
    oled.print ( mixer_gains [ i ] );
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

// inits, setters, and getters for audio objects

void OpenEffectsBox::init () {
  sgtl5000_init ();
  tuningFreqTable_init ();
  setPreset ( 0 );
}

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

void OpenEffectsBox::tuning_sine_set ( float sine_freq ) {
  _mix1_sine_freq = sine_freq;
  sine1.amplitude ( 0.2 );
  sine1.frequency ( _mix1_sine_freq );
}

float OpenEffectsBox::tuning_sine_get () {
  return ( _mix1_sine_freq );
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

// tonesweep1: play will be automatic when the gain is positive
void OpenEffectsBox::tonesweep1_set ( float tonesweep_lf, float tonesweep_hf, float tonesweep_time ) {
  _mix1_tonesweep_lf = tonesweep_lf;
  _mix1_tonesweep_hf = tonesweep_hf;
  _mix1_tonesweep_time = tonesweep_time;
}

void OpenEffectsBox::tonesweep1_lf_set ( float tonesweep_lf ) {
  _mix1_tonesweep_lf = tonesweep_lf;
}

void OpenEffectsBox::tonesweep1_hf_set ( float tonesweep_hf ) {
  _mix1_tonesweep_hf = tonesweep_hf;
}

void OpenEffectsBox::tonesweep1_time_set ( float tonesweep_time ) {
  _mix1_tonesweep_time = tonesweep_time;
}

float OpenEffectsBox::tonesweep1_lf_get () {
  return ( _mix1_tonesweep_lf );
}

float OpenEffectsBox::tonesweep1_hf_get () {
  return ( _mix1_tonesweep_hf );
}

float OpenEffectsBox::tonesweep1_time_get () {
  return ( _mix1_tonesweep_time );
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

// string1: no getter or setter; for testing purposes only

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::input_mixer_set ( float inst_gain, float sin_gain, float tonesweep_gain ) {
  _mix1_inst_gain = inst_gain;
  _mix1_sin_gain = sin_gain;
  _mix1_tonesweep_gain = tonesweep_gain;
  mixer1.gain ( input_mixer_input_L, _mix1_inst_gain );   // i2s2 ( line inputs L & R )
  mixer1.gain ( input_mixer_input_R, _mix1_inst_gain );   // i2s2 ( line inputs L & R )
  mixer1.gain ( input_mixer_tuning_sine_, _mix1_sin_gain );   // tuning_sine_
  mixer1.gain ( input_mixer_tonesweep1, _mix1_tonesweep_gain );   // tonesweep1
}

void OpenEffectsBox::input_mixer_inst_gain_set ( float inst_gain ) {
  _mix1_inst_gain = inst_gain;
  mixer1.gain ( input_mixer_input_L, _mix1_inst_gain );   // i2s2 ( line inputs L & R )
  mixer1.gain ( input_mixer_input_R, _mix1_inst_gain );   // i2s2 ( line inputs L & R )
}

void OpenEffectsBox::input_mixer_sin_gain_set ( float sin_gain ) {
  _mix1_sin_gain = sin_gain;
  mixer1.gain ( input_mixer_tuning_sine_, _mix1_sin_gain );   // tuning_sine_
}

void OpenEffectsBox::input_mixer_tonesweep_gain_set ( float tonesweep_gain ) {
  _mix1_tonesweep_gain = tonesweep_gain;
  mixer1.gain ( input_mixer_tonesweep1, _mix1_tonesweep_gain );   // tuning_sine_
}

float OpenEffectsBox::input_mixer_inst_gain_get () {
  return ( _mix1_inst_gain );
}

float OpenEffectsBox::input_mixer_sin_gain_get () {
  return ( _mix1_sin_gain );
}

float OpenEffectsBox::input_mixer_tonesweep_gain_get () {
  return ( _mix1_tonesweep_gain );
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::bitcrusher_set ( int bits, unsigned long sampleRate ) {
  _bitcrush_bits = bits;
  _bitcrush_sampleRate = sampleRate;
  bitcrusher1.bits ( _bitcrush_bits );
  bitcrusher1.sampleRate ( _bitcrush_sampleRate );
}

void OpenEffectsBox::bitcrusher_bits_set ( int bits ) {
  _bitcrush_bits = bits;
  bitcrusher1.bits ( _bitcrush_bits );
}

void OpenEffectsBox::bitcrusher_sampleRate_set ( unsigned long sampleRate ) {
  _bitcrush_sampleRate = sampleRate;
  bitcrusher1.sampleRate ( _bitcrush_sampleRate );
}

int OpenEffectsBox::bitcrusher_bits_get () {
  return ( _bitcrush_bits );
}

unsigned long OpenEffectsBox::bitcrusher_sampleRate_get () {
  return ( _bitcrush_sampleRate );
}
  
// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::waveshape_set ( int waveshape ) {
    
    static float waveShapes [ 5 ] [ 9 ] = { 
      { -1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
      { -1.0, -0.3,  -0.2,  -0.1, 0.0, 0.1, 0.2,  0.3,  1.0 },
      { -1.0, -0.98, -0.95, -0.1, 0.0, 0.1, 0.95, 0.98, 1.0 },
      {  1.0,  0.3,   0.2,   0.1, 0.0, 0.1, 0.2,  0.3,  1.0 },
      {  1.0,  0.98,  0.5,   0.1, 0.0, 0.1, 0.5,  0.98, 1.0 }
    };
  _waveshape_selection = waveshape;
  waveshape1.shape ( waveShapes [ _waveshape_selection ], _waveshape_selection ? 9 : 2 );
}
 
int OpenEffectsBox::waveshape_get () {
  return ( _waveshape_selection );
}
 
// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::mpy_dc_level_set ( float dc_gain ) {
  _multiply_dc_gain = dc_gain;
  dc1.amplitude ( dc_gain );
}

float OpenEffectsBox::mpy_dc_level_get () {
  return ( _multiply_dc_gain );
}
 
// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::mpy_sine_freq_set ( float sine_freq ) {
  _multiply_sine_freq = sine_freq;
  sine2.amplitude ( 1.0 );
  sine2.frequency ( _multiply_sine_freq );
}

float OpenEffectsBox::mpy_sine_freq_get () {
  return ( _multiply_sine_freq );
}
 
// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

// tonesweep2: play will be automatic when the gain is positive
void OpenEffectsBox::mpy_tonesweep_set ( float tonesweep_lf, float tonesweep_hf, float tonesweep_time ) {
  _multiply_tonesweep_lf = tonesweep_lf;
  _multiply_tonesweep_hf = tonesweep_hf;
  // note: despite documentation, tonesweep_time is (float) seconds, not (int) ms
  _multiply_tonesweep_time = tonesweep_time;
}

void OpenEffectsBox::mpy_tonesweep_lf_set ( float tonesweep_lf ) {
  _multiply_tonesweep_lf = tonesweep_lf;
}

void OpenEffectsBox::mpy_tonesweep_hf_set ( float tonesweep_hf ){
  _multiply_tonesweep_hf = tonesweep_hf;
}

void OpenEffectsBox::mpy_tonesweep_time_set ( float tonesweep_time ) {
  _multiply_tonesweep_time = tonesweep_time;
}

float OpenEffectsBox::mpy_tonesweep_lf_get () {
  return ( _multiply_tonesweep_lf );
}

float OpenEffectsBox::mpy_tonesweep_hf_get () {
  return ( _multiply_tonesweep_hf );
}

float OpenEffectsBox::mpy_tonesweep_time_get () {
  return ( _multiply_tonesweep_time );
}
 
// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::mpy_mixer_set ( 
    float dc_gain, 
    float sine_gain, 
    float tonesweep_gain ) {
    
  _multiply_dc_gain = dc_gain;
  _multiply_sine_gain = sine_gain;
  _multiply_tonesweep_gain = tonesweep_gain;
  
  mpy_mixer_set ();
}

void OpenEffectsBox::mpy_mixer_set () {
  mixer3.gain ( 0, _multiply_dc_gain );
  mixer3.gain ( 1, _multiply_sine_gain );
  mixer3.gain ( 2, _multiply_tonesweep_gain );
}


void OpenEffectsBox::mpy_mixer_dc_gain_set ( float dc_gain ) {
  _multiply_dc_gain = dc_gain;
  mpy_mixer_set ();
}

void OpenEffectsBox::mpy_mixer_sin_gain_set ( float sine_gain ) {
  _multiply_sine_gain = sine_gain;
  mpy_mixer_set ();
}

void OpenEffectsBox::mpy_mixer_tonesweep_gain_set ( float tonesweep_gain ) {
  _multiply_tonesweep_gain = tonesweep_gain;
  mpy_mixer_set ();
}

float OpenEffectsBox::mpy_mixer_dc_gain_get () {
  return ( _multiply_dc_gain );
}

float OpenEffectsBox::mpy_mixer_sin_gain_get () {
  return ( _multiply_sine_gain );
}

float OpenEffectsBox::mpy_mixer_tonesweep_gain_get () {
  return ( _multiply_tonesweep_gain );
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::chorus_set ( int chorus_n ) {
  _chorus_nVoices = chorus_n;
  chorus1.begin ( chorusDelayLine, CHORUS_DELAY_LENGTH, _chorus_nVoices );  // 1 is passthru
}

int OpenEffectsBox::chorus_get () {
  return ( _chorus_nVoices );
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::flange_set ( int offset, int depth, float rate) {
  _flange_offset = offset;
  _flange_depth = depth;
  _flange_rate = rate;
  flange_start ();
}

void OpenEffectsBox::flange_start () {
  flange1.begin ( flangeDelayLine, FLANGE_DELAY_LENGTH, 
    _flange_offset, _flange_depth, _flange_rate );
}

void OpenEffectsBox::flange_offset_set ( int offset ) {
  _flange_offset = offset;
  flange_start ();
}

void OpenEffectsBox::flange_depth_set ( int depth ) {
  _flange_depth = depth;
  flange_start ();
}

void OpenEffectsBox::flange_rate_set ( float rate ) {
  _flange_rate = rate;
  flange_start ();
}

int OpenEffectsBox::flange_offset_get () {
  return ( _flange_offset );
}

int OpenEffectsBox::flange_depth_get () {
  return ( _flange_depth );
}

float OpenEffectsBox::flange_rate_get () {
  return ( _flange_rate );
}


// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::reverb_set ( float time_sec ) {

/*
  // extra, trying to trace reverb problem
  _reverbTime_sec = 2.0;
  if ( BAUDRATE && VERBOSE >= 10 ) { 
    Serial.print ( millis() ); Serial.print ( ": " );
    Serial.print ( "reverb: " ); Serial.print ( _reverbTime_sec ); Serial.println ( "sec" );
  }
  reverb1.reverbTime ( _reverbTime_sec );
*/  
  
  _reverbTime_sec = time_sec;
  // sidestep bug - reverb 0.0 is effectively 1.0 or so
  if ( _reverbTime_sec == 0.0 ) _reverbTime_sec = 1e-4;  
  if ( 0 && BAUDRATE && VERBOSE >= 10 ) { 
    Serial.print ( millis() ); Serial.print ( ": " );
    Serial.print ( "reverb: " ); Serial.print ( _reverbTime_sec ); Serial.println ( "sec" );
  }
  reverb1.reverbTime ( _reverbTime_sec );
}

float OpenEffectsBox::reverb_get () {
  return ( _reverbTime_sec );
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::delay_times_set ( int times_ms [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    _delay_times_ms [ i ] = times_ms [ i ];
  }
  delay_times_set ();
}

void OpenEffectsBox::delay_times_set () {
  for ( int i = 0; i < 4; i++ ) {
    delayExt1.delay ( i , _delay_times_ms [ i ] );
  }
  for ( int i = 4; i < 8; i++ ) {
    delayExt1.disable ( i );
  }
}

void OpenEffectsBox::delay_times_get ( int times_ms [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    times_ms [ i ] = _delay_times_ms [ i ];
  }
  return;
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::delay_mixer_gains_set ( float gains [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    _delay_mixer_gains [ i ] = gains [ i ];
    mixer2.gain ( i, _delay_mixer_gains [ i ] );
  }
}

void OpenEffectsBox::delay_mixer_gains_get ( float gains [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    gains [ i ] = _delay_mixer_gains [ i ];
  }
  return;
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::output_mixer_gains_set ( float gains [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    _output_mixer_gains [ i ] = gains [ i ];
    mixer4.gain ( i, _output_mixer_gains [ i ] );
  }
}

void OpenEffectsBox::output_mixer_gains_get ( float gains [ 4 ] ) {
  for ( int i = 0; i < 4; i++ ) {
    gains [ i ] = _output_mixer_gains [ i ];
  }
  return;
}

// ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '

void OpenEffectsBox::tuningFreqTable_init () {
  _tuning_freq_table [ 0 ] = 440.00;  // A4
  _tuning_freq_table [ 1 ] = 293.66;  // D4
  _tuning_freq_table [ 2 ] = 196.00;  // G3
  _tuning_freq_table [ 3 ] = 130.81;  // C3
  _tuning_freq_table [ 4 ] = 659.25;  // E5
}

float OpenEffectsBox::tuningFreq_get ( int index ) {
  return ( _tuning_freq_table [ index ] );
}

// ^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v
// v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^
// ^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v

void OpenEffectsBox::setPreset ( int ps ) {
  currentPreset = ps;
  switch ( currentPreset ) {
    case 0: // reinitialize board to straight through
      {
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        input_mixer_set ( 1.0, 0.0, 0.0 );
        // mix1_sine_freq
        tuning_sine_set ( 440.0 );
        
        //  tonesweep_lf, tonesweep_hf, tonesweep_time                       
        tonesweep1_set ( 55.0, 3520.0, 4.0 );

        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher_set ( 16, 44100 );

        /*
          0 - wS1_passthru / 2
          1 - wS1_shape1 / 9
          2 - wS1_shape2 / 9
          3 - wS1_shape3 / 9
          4 - wS1_shape4 / 9
        */
        waveshape_set ( 0 );

        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        mpy_dc_level_set ( 1.0 );
        // multiply_sine_freq
        mpy_sine_freq_set ( 2.0 );
        //  tonesweep_lf, tonesweep_hf, tonesweep_time                       
        mpy_tonesweep_set ( 55.0, 3520.0, 4.0 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mpy_mixer_set ( 1.0, 0.0, 0.0 );

        // chorus_nVoices
        chorus_set ( 1 );

        // flange_offset, flange_depth, flange_rate
        flange_set ( 0, 0, 0.0 );

        // reverbTime_sec
        reverb_set ( 0.0 );

        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 0, 0, 0 };
        delay_times_set ( dt_ms );

        float mg [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        // mixer2 mixes delays
        delay_mixer_gains_set ( mg );
        // mixer43 is output master gain
        output_mixer_gains_set ( mg );
      }
      break;
      
    case 1: // preset 1 for tuning
      {
        // mix1_sine_freq
        tuning_sine_set ( 440.0 );
        // tonesweep needs no init; parameters are explicit in start
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        input_mixer_set ( 1.0, 0.10, 0.0 );
        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher_set ( 16, 44100 );
        // waveshape_selection
        waveshape_set ( 2 );
  
        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        mpy_dc_level_set ( 0.5 );
        // multiply_sine_freq
        mpy_sine_freq_set ( 6.0 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mpy_mixer_set ( 1.0, 0.5, 0.0 );
  
        // chorus_nVoices
        chorus_set ( 2 );
        // flange_offset, flange_depth, flange_rate
        flange_set ( 0, 0, 0 );
        // reverbTime_sec
        reverb_set ( 1.0 );
        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 100, 200, 300 };
        delay_times_set ( dt_ms );
        // mixer2 mixes delays
        float m2g [ 4 ] = { 1.0, 0.8, 0.6, 0.4 };
        delay_mixer_gains_set ( m2g );
        // mixer4 is output volume
        float m4g [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        output_mixer_gains_set ( m4g );
      }
      break;

    case 2: // preset 2 - nice with reverb but mostly straight
      {
        // mix1_sine_freq
        tuning_sine_set ( 440.0 );
        // tonesweep needs no init; parameters are explicit in start
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        input_mixer_set ( 1.0, 0.0, 0.0 );
        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher_set ( 16, 44100 );
        // waveshape_selection
        waveshape_set ( 0 );
  
        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        mpy_dc_level_set ( 0.5 );
        // multiply_sine_freq
        mpy_sine_freq_set ( 0.25 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mpy_mixer_set ( 1.0, 0.1, 0.0 );
  
        // chorus_nVoices
        chorus_set ( 2 );
        // flange_offset, flange_depth, flange_rate
        flange_set ( 0, 0, 0 );
        // reverbTime_sec
        reverb_set ( 1.0 );
        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 100, 200, 300 };
        delay_times_set ( dt_ms );
        // mixer2 mixes delays
        float m2g [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        delay_mixer_gains_set ( m2g );
        // mixer4 is output volume
        float m4g [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        output_mixer_gains_set ( m4g );
      }
      break;

    case 3: // preset 3 a little wild
      {
        // mix1_sine_freq
        tuning_sine_set ( 440.0 );
        // tonesweep needs no init; parameters are explicit in start
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        input_mixer_set ( 1.0, 0.0, 0.0 );
        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher_set ( 16, 44100 );
        // waveshape_selection
        waveshape_set ( 2 );
  
        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        mpy_dc_level_set ( 0.5 );
        // multiply_sine_freq
        mpy_sine_freq_set ( 6.0 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mpy_mixer_set ( 1.0, 0.5, 0.0 );
  
        // chorus_nVoices
        chorus_set ( 2 );
        // flange_offset, flange_depth, flange_rate
        flange_set ( 0, 0, 0 );
        // reverbTime_sec
        reverb_set ( 1.0 );
        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 100, 200, 300 };
        delay_times_set ( dt_ms );
        // mixer2 mixes delays
        float m2g [ 4 ] = { 1.0, 0.8, 0.6, 0.4 };
        delay_mixer_gains_set ( m2g );
        // mixer4 is output volume
        float m4g [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        output_mixer_gains_set ( m4g );
      }
      break;
        
    case 4: // interesting flange / reverb effect
      {
        // mix1_inst_gain, mix1_sin_gain, mix1_tonesweep_gain
        input_mixer_set ( 1.0, 0.0, 0.3 );
        // mix1_sine_freq
        tuning_sine_set ( 440.0 );
        
        //  tonesweep_lf, tonesweep_hf, tonesweep_time                       
        tonesweep1_set ( 20.0, 90.0, 1.7 );

        // bitcrush_bits, bitcrush_sampleRate
        bitcrusher_set ( 16, 44100 );

        waveshape_set ( 0 );

        // dc1, sine2, and tonesweep2 are inputs to mixer3, in turn to multiply
        // multiply_dc_gain
        mpy_dc_level_set ( 1.0 );
        // multiply_sine_freq
        mpy_sine_freq_set ( 2.0 );
        //  tonesweep_lf, tonesweep_hf, tonesweep_time                       
        mpy_tonesweep_set ( 55.0, 3520.0, 4.0 );
        // tonesweep2: tonesweeps play only once, when called
        // multiply_dc_gain, multiply_sine_gain,  multiply_tonesweep_gain
        mpy_mixer_set ( 1.0, 0.0, 0.0 );

        // chorus_nVoices
        chorus_set ( 1 );  // changing this is interesting

        // flange_offset, flange_depth, flange_rate
        flange_set ( 32, 43, 10.0 );

        // reverbTime_sec
        reverb_set ( 1.0 );

        // delay_times_ms
        int dt_ms [ 4 ] = { 0, 0, 0, 0 };
        delay_times_set ( dt_ms );

        float mg [ 4 ] = { 1.0, 0.0, 0.0, 0.0 };
        // mixer2 mixes delays
        delay_mixer_gains_set ( mg );
        // mixer43 is output master gain
        output_mixer_gains_set ( mg );
      }
      break;
      
    default:
      break;
  }
}
