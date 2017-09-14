#define PROGNAME  "testAudioBoardEffects"
#define VERSION   "0.0.1"
#define VERDATE   "2017-09-10"

/*
  Please remember that the standard Serial and the standard LED on pin 13
  both interfere with audio board pin mappings. Don't use either of them!
  They will work, but their use will disable the audio board's workings.
*/

const int paPot = 15;

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=216.6666717529297,329.00001525878906
AudioSynthKarplusStrong  string1;        //xy=216.6666717529297,373.00001525878906
AudioMixer4              mixer1;         //xy=442.6666717529297,346.00001525878906
AudioMixer4              mixer2;         //xy=704.6666870117188,585
AudioMixer4              mixer3;         //xy=998.7333984375,353.73333740234375
AudioEffectEnvelope      envelope1;      //xy=671.7333374023438,396.73333740234375
AudioEffectFlange        flange1;        //xy=678.75,446.75
AudioEffectDelay         delay1;         //xy=410.6666717529297,583.0000457763672
AudioOutputI2S           i2s2;           //xy=1198.6666259765625,356
AudioConnection          patchCord1  ( i2s1,      0, mixer1, 0 );
AudioConnection          patchCord2  ( i2s1,      1, mixer1, 1 );
AudioConnection          patchCord3  ( string1,   0, mixer1, 2 );
AudioConnection          patchCord4  ( mixer1,    delay1       ); 
AudioConnection          patchCord5  ( mixer1,    0, mixer3, 0 );
AudioConnection          patchCord6  ( mixer1,    envelope1    ); 
AudioConnection          patchCord7  ( mixer1,    flange1      ); 
AudioConnection          patchCord8  ( envelope1, 0, mixer3, 1 );
AudioConnection          patchCord9  ( flange1,   0, mixer3, 2 );
AudioConnection          patchCord10 ( delay1,    0, mixer2, 0 );
AudioConnection          patchCord11 ( delay1,    1, mixer2, 1 );
AudioConnection          patchCord12 ( delay1,    2, mixer2, 2 );
AudioConnection          patchCord13 ( delay1,    3, mixer2, 3 );
AudioConnection          patchCord14 ( mixer2,    0, mixer1, 3 );
AudioConnection          patchCord15 ( mixer2,    0, mixer3, 3 );
AudioConnection          patchCord16 ( mixer3,    0, i2s2,   0 );
AudioConnection          patchCord17 ( mixer3,    0, i2s2,   1 );
AudioControlSGTL5000     sgtl5000_1;     //xy=498.6666717529297,218.00001525878906
// GUItool: end automatically generated code


void setup () {

  // needs 1/3 block for each millisecond of delay = 67 blocks for 200 ms
  AudioMemory ( 150 );
  // AudioMemory_F32 ( 100 ); //allocate Float32 audio data blocks

  sgtl5000_1.enable();  // Enable the audio shield

  sgtl5000_1.inputSelect ( AUDIO_INPUT_LINEIN );
  sgtl5000_1.lineInLevel ( 12 );     // default 5 is 1.33Vp-p; 0 is 3.12Vp-p

  // sgtl5000_1.inputSelect ( AUDIO_INPUT_MIC );
  // sgtl5000_1.micGain ( 31 );        // 0-63 dB
  
  sgtl5000_1.volume ( 0.5 );
  sgtl5000_1.lineOutLevel ( 29 );   // default 29 is 1.29Vp-p; 13 is 3.16Vp-p
  
  const float inputGain = 1.0;      // gain from 0.0 to 32767.0  1.0=straight through
  mixer1.gain ( 0, inputGain );     // i2s input
  mixer1.gain ( 1, inputGain );     // i2s input
  mixer1.gain ( 2,       1.0 );     // string
  mixer1.gain ( 3,       0.2 );     // recycle
  
  envelope1
  
  delay1.delay ( 0,   0 );
  delay1.delay ( 1,  50 );
  delay1.delay ( 2, 100 );
  delay1.delay ( 3, 150 );
  delay1.disable ( 4 );
  delay1.disable ( 5 );
  delay1.disable ( 6 );
  delay1.disable ( 7 );

  mixer2.gain ( 0, 1.0 );           // gain from 0.0 to 32767.0  1.0=straight through
  mixer2.gain ( 1, 0.9 );
  mixer2.gain ( 2, 0.8 );
  mixer2.gain ( 3, 0.7 );
  
  // Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 20 );

}


void loop() {
  static int potValue = -1;
  const int potHysteresis = 4;
  const float mixer1GainMax = 0.6;
  
  int newPotValue = analogRead ( paPot );
  if ( abs ( newPotValue - potValue ) > potHysteresis ) {
    potValue = newPotValue;
    mixer1.gain ( 3, ( (float) potValue ) * mixer1GainMax / 1024.0 );
  }
        
}