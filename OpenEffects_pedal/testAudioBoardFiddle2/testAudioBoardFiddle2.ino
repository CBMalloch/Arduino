#define PROGNAME  "testAudioBoardFiddle2a"
#define VERSION   "0.0.2"
#define VERDATE   "2017-09-18"

/*
  Please remember that the standard Serial and the standard LED on pin 13
  both interfere with audio board pin mappings. Don't use either of them!
  They will work, but their use will disable the audio board's workings.
*/

/*

  What IO remains available while using the Teensy Audio Shield?
    See https://www.pjrc.com/store/teensy3_audio.html
    
    Looks like pins 0, 1, 2, 3, 4, 5, 8, 16, 17, 20, 21 should be available.
    Pins 18 and 19 are sharable with other I2C devices
    Pins 7, 12, and 14 are sharable with other SPI devices
    Using the standard Serial object stops the audio.
    Pin 21 is A7; pin 20 is A6; pin 16 is A2; pin 15 is A1
*/

const int paPot = 15;

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=365.0000762939453,384
AudioMixer4              mixer1;         //xy=396,255
AudioMixer4              mixer2;         //xy=624,606
AudioMixer4              mixer9;         //xy=993,270
AudioEffectDelayExternal delayExt1 ( AUDIO_MEMORY_MEMORYBOARD, 4 * 1485 );      //xy=429,587
AudioEffectBitcrusher    bitcrusher1;    //xy=696,288
AudioEffectFlange        flange1;        //xy=663,332
AudioEffectChorus        chorus1;        //xy=667,390
AudioOutputI2S           i2s2;           //xy=980.0000610351562,507
AudioConnection          patchCord1  ( i2s1,         0, mixer1,      0 );
AudioConnection          patchCord2  ( i2s1,         1, mixer1,      1 );
AudioConnection          patchCord4  ( mixer2,       0, mixer1,      3 );
  // AudioConnection          patchCord5  ( mixer1,       0, delayExt1,   0 );
// AudioConnection          patchCord5(mixer1, delayExt1 ( AUDIO_MEMORY_MEMORYBOARD ));
AudioConnection          patchCord5(mixer1, delayExt1 );
  // AudioConnection          patchCord6  ( delayExt1,    0, mixer2,      0 );
  // AudioConnection          patchCord7  ( delayExt1,    1, mixer2,      1 );
  // AudioConnection          patchCord8  ( delayExt1,    2, mixer2,      2 );
  // AudioConnection          patchCord9  ( delayExt1,    3, mixer2,      3 );
// AudioConnection          patchCord6  ( delayExt1_( AUDIO_MEMORY_MEMORYBOARD ), 0, mixer2, 0);
// AudioConnection          patchCord7  ( delayExt1_( AUDIO_MEMORY_MEMORYBOARD ), 1, mixer2, 1);
// AudioConnection          patchCord8  ( delayExt1_( AUDIO_MEMORY_MEMORYBOARD ), 2, mixer2, 2);
// AudioConnection          patchCord9  ( delayExt1_( AUDIO_MEMORY_MEMORYBOARD ), 3, mixer2, 3);
AudioConnection          patchCord6  ( delayExt1, 0, mixer2, 0);
AudioConnection          patchCord7  ( delayExt1, 1, mixer2, 1);
AudioConnection          patchCord8  ( delayExt1, 2, mixer2, 2);
AudioConnection          patchCord9  ( delayExt1, 3, mixer2, 3);
  // AudioConnection          patchCord10 ( mixer1,       0, bitcrusher1, 0 );
AudioConnection          patchCord10 ( mixer1,          bitcrusher1);
AudioConnection          patchCord11 ( mixer1,          flange1);
AudioConnection          patchCord12 ( mixer1,          chorus1);
AudioConnection          patchCord13 ( mixer1,       0, mixer9,      0 );
AudioConnection          patchCord14 ( bitcrusher1,  0, mixer9,      1 );
AudioConnection          patchCord15 ( flange1,      0, mixer9,      2 );
AudioConnection          patchCord16 ( chorus1,      0, mixer9,      3 );

AudioConnection          patchCord17 ( mixer9,       0, i2s2,        0 );
AudioConnection          patchCord18 ( mixer9,       0, i2s2,        1 );
AudioControlSGTL5000     sgtl5000_1;     //xy=647.0000610351562,273
// GUItool: end automatically generated code

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


void setup () {

  // Serial.begin ( 115200 );
  // while ( !Serial && millis() < 4000 ) { delay ( 100 ); }
  // 
  // pinMode ( pdLED, OUTPUT );
  // digitalWrite ( pdLED, 0 );
    
  // needs 1/3 block for each millisecond of delay = 67 blocks for 200 ms
  AudioMemory ( 150 );
  // AudioMemory_F32 ( 100 ); //allocate Float32 audio data blocks

  sgtl5000_1.enable();  // Enable the audio shield

  if ( 1 ) {
    sgtl5000_1.inputSelect ( AUDIO_INPUT_LINEIN );
    sgtl5000_1.lineInLevel ( 0 );     // default 5 is 1.33Vp-p; 0 is 3.12Vp-p; 15 (max) is 0.24Vp-[
  } else {
    sgtl5000_1.inputSelect ( AUDIO_INPUT_MIC );
    sgtl5000_1.micGain ( 31 );        // 0-63 dB
  }
  
  sgtl5000_1.volume ( 0.5 );
  sgtl5000_1.lineOutLevel ( 29 );   // default 29 is 1.29Vp-p; 13 is 3.16Vp-p
  
  const float inputGain = 4.0;      // gain from 0.0 to 32767.0  1.0=straight through
  mixer1.gain ( 0, inputGain );     // i2s input
  mixer1.gain ( 1, inputGain );     // i2s input
  mixer1.gain ( 3,       0.0 );     // recycle
  
  mixer9.gain ( 0, 1.0 );           // gain from 0.0 to 32767.0  1.0=straight through
  mixer9.gain ( 1, 0.0 );
  mixer9.gain ( 2, 0.0 );
  mixer9.gain ( 3, 0.0 );
  
  bitcrusher1.bits ( 4 );           // out of 16
  
  flange1.begin ( flangeDelayLine, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq );
  // flange1.voices ( s_idx, s_depth, s_freq );
  
  chorus1.begin ( chorusDelayLine, CHORUS_DELAY_LENGTH, 4 );
  
  if ( 0 ) {
    delayExt1.delay ( 0,  300 );
    delayExt1.delay ( 1, 2400 );
    delayExt1.delay ( 2, 3600 );
    delayExt1.delay ( 3, 8750 );
  } else {
    delayExt1.delay ( 0,  200 );
    delayExt1.delay ( 1,  400 );
    delayExt1.delay ( 2,  600 );
    delayExt1.delay ( 3,  800 );
  }
  delayExt1.disable ( 4 );
  delayExt1.disable ( 5 );
  delayExt1.disable ( 6 );
  delayExt1.disable ( 7 );

  mixer2.gain ( 0, 0.8 ); 
  mixer2.gain ( 1, 0.7 ); 
  mixer2.gain ( 2, 0.6 ); 
  mixer2.gain ( 3, 0.5 ); 
  
  
  // Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 20 );

}


void loop() {

  static int state = 0;
  const int nStates = 5;

  static int potValue;
  const int potHysteresis = 4;
  
  int newPotValue = analogRead ( paPot );
  if ( abs ( newPotValue - potValue ) > potHysteresis ) {
    potValue = newPotValue;
    // mixer1.gain ( 3, ( (float) potValue ) * 1.1 / 1024.0 );
    state = potValue * nStates / 1024;

    switch ( state ) {
      case 0:
        // straight through
        mixer1.gain ( 3, 0.0 );           // kill recycling
        mixer9.gain ( 0, 1.0 );           // gain from 0.0 to 32767.0  1.0=straight through
        mixer9.gain ( 1, 0.0 );
        mixer9.gain ( 2, 0.0 );
        mixer9.gain ( 3, 0.0 );
        break;
      case 1:
        // bit crusher
        mixer1.gain ( 3, 0.0 );           // kill recycling
        mixer9.gain ( 0, 0.0 );           // gain from 0.0 to 32767.0  1.0=straight through
        mixer9.gain ( 1, 1.0 );
        mixer9.gain ( 2, 0.0 );
        mixer9.gain ( 3, 0.0 );
        break;
      case 2:
        // flange
        mixer1.gain ( 3, 0.0 );           // kill recycling
        mixer9.gain ( 0, 0.0 );           // gain from 0.0 to 32767.0  1.0=straight through
        mixer9.gain ( 1, 0.0 );
        mixer9.gain ( 2, 1.0 );
        mixer9.gain ( 3, 0.0 );
        break;
      case 3:
        // chorus
        mixer1.gain ( 3, 0.0 );
        mixer9.gain ( 0, 0.0 );           // gain from 0.0 to 32767.0  1.0=straight through
        mixer9.gain ( 1, 0.0 );
        mixer9.gain ( 2, 0.0 );
        mixer9.gain ( 3, 1.0 );
        break;
      case 4:
        // delay up to 8 sec
        mixer1.gain ( 3, 0.3 );
        mixer9.gain ( 0, 0.5 );           // gain from 0.0 to 32767.0  1.0=straight through
        mixer9.gain ( 1, 0.0 );
        mixer9.gain ( 2, 0.0 );
        mixer9.gain ( 3, 0.0 );
        break;
    }
  }   
}