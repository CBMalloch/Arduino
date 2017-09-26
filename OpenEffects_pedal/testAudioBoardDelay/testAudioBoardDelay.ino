#define PROGNAME  "testAudioBoardDelay"
#define VERSION   "0.2.2"
#define VERDATE   "2017-09-14"

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
AudioSynthKarplusStrong  string1;        //xy=365.0000762939453,428.00000762939453
AudioMixer4              mixer1;         //xy=591.0000610351562,401
AudioMixer4              mixer2;         //xy=704.6666870117188,585
AudioMixer4              mixer9;         //xy=778.0000610351562,598
AudioEffectDelayExternal delayExt1 ( AUDIO_MEMORY_MEMORYBOARD );      //xy=429,587
AudioSynthWaveform       waveform1;      //xy=559,426
AudioEffectBitcrusher    bitcrusher1;    //xy=614,360
AudioEffectMultiply      multiply1;      //xy=697,423
AudioOutputI2S           i2s2;           //xy=980.0000610351562,507
AudioConnection          patchCord1  ( i2s1,         0, mixer1,      0 );
AudioConnection          patchCord2  ( i2s1,         1, mixer1,      1 );
AudioConnection          patchCord3  ( string1,      0, mixer1,      2 );
AudioConnection          patchCord4  ( mixer2,       0, mixer1,      3 );
AudioConnection          patchCord5  ( mixer1,       0, delayExt1,   0 );
AudioConnection          patchCord6  ( delayExt1,    0, mixer2,      0 );
AudioConnection          patchCord7  ( delayExt1,    1, mixer2,      1 );
AudioConnection          patchCord8  ( delayExt1,    2, mixer2,      2 );
AudioConnection          patchCord9  ( delayExt1,    3, mixer2,      3 );
AudioConnection          patchCord10 ( mixer1,       0, bitcrusher1, 0 );
AudioConnection          patchCord11 ( mixer1,       0, multiply1,   0 );
AudioConnection          patchCord12 ( waveform1,    0, multiply1,   1 );
AudioConnection          patchCord13 ( mixer1,       0, mixer9,      0 );
AudioConnection          patchCord14 ( bitcrusher1,  0, mixer9,      1 );
AudioConnection          patchCord15 ( multiply1,    0, mixer9,      2 );
AudioConnection          patchCord16 ( mixer2,       0, mixer9,      3 );
AudioConnection          patchCord17 ( mixer9,       0, i2s2,        0 );
AudioConnection          patchCord18 ( mixer9,       0, i2s2,        1 );
AudioControlSGTL5000     sgtl5000_1;     //xy=647.0000610351562,273
// GUItool: end automatically generated code





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
    sgtl5000_1.lineInLevel ( 15 );     // default 5 is 1.33Vp-p; 0 is 3.12Vp-p; 15 (max) is 0.24Vp-[
  } else {
    sgtl5000_1.inputSelect ( AUDIO_INPUT_MIC );
    sgtl5000_1.micGain ( 31 );        // 0-63 dB
  }
  
  sgtl5000_1.volume ( 0.5 );
  sgtl5000_1.lineOutLevel ( 29 );   // default 29 is 1.29Vp-p; 13 is 3.16Vp-p
  
  const float inputGain = 10.0;      // gain from 0.0 to 32767.0  1.0=straight through
  mixer1.gain ( 0, inputGain );     // i2s input
  mixer1.gain ( 1, inputGain );     // i2s input
  mixer1.gain ( 2,       1.0 );     // string
  mixer1.gain ( 3,       0.0 );     // recycle
  
  mixer9.gain ( 0, 1.0 );           // gain from 0.0 to 32767.0  1.0=straight through
  mixer9.gain ( 1, 0.0 );
  mixer9.gain ( 2, 0.0 );
  mixer9.gain ( 3, 0.0 );
  
  bitcrusher1.bits ( 4 );           // out of 16
  
  waveform1.frequency ( 440 );
  waveform1.amplitude ( 1.0 );
  waveform1.begin ( WAVEFORM_TRIANGLE );
  
  delayExt1.disable ( 4 );
  delayExt1.disable ( 5 );
  delayExt1.disable ( 6 );
  delayExt1.disable ( 7 );

  mixer2.gain ( 0, 1.0 ); 
  mixer2.gain ( 1, 1.0 ); 
  mixer2.gain ( 2, 1.0 ); 
  mixer2.gain ( 3, 1.0 ); 
  
  
  // Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 20 );

}


void loop() {

  static int state = 0;
  const int nStates = 5;
  static unsigned long lastStateChangeAt_ms = 0UL;

  static unsigned long lastDinkAt_ms = 0UL;
  const unsigned long dinkRate_ms = 1000UL;
  
  static unsigned long lastBlinkAt_ms = 0UL;
  const unsigned long blinkRate_ms = 1000UL;
  
  
  static int potValue = -1;
  const int potHysteresis = 4;
  
  int newPotValue = analogRead ( paPot );
  if ( abs ( newPotValue - potValue ) > potHysteresis ) {
    potValue = newPotValue;
    // mixer1.gain ( 3, ( (float) potValue ) * 1.1 / 1024.0 );
    state = potValue * nStates / 1024;
    lastDinkAt_ms = 0UL;
    lastStateChangeAt_ms = millis();
  }
  
    
  // if ( ( millis() - lastChangeAt_ms ) > changeRate_ms ) {
    // state++;
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
        // multiply
        mixer1.gain ( 3, 0.0 );           // kill recycling
        mixer9.gain ( 0, 0.0 );           // gain from 0.0 to 32767.0  1.0=straight through
        mixer9.gain ( 1, 0.0 );
        mixer9.gain ( 2, 1.0 );
        mixer9.gain ( 3, 0.0 );
        break;
      case 3:
        // delay up to 8 sec
        delayExt1.delay ( 0,  300 );
        delayExt1.delay ( 1, 2400 );
        delayExt1.delay ( 2, 3600 );
        delayExt1.delay ( 3, 8750 );

        mixer1.gain ( 3, 0.25 );
        mixer9.gain ( 0, 0.0 );           // gain from 0.0 to 32767.0  1.0=straight through
        mixer9.gain ( 1, 0.0 );
        mixer9.gain ( 2, 0.0 );
        mixer9.gain ( 3, 1.0 );
        break;
      case 4:
        // test delay
        delayExt1.delay ( 0, 8000 );
        delayExt1.disable ( 1 );
        delayExt1.disable ( 2 );
        delayExt1.disable ( 3 );
        if ( ( millis() - lastStateChangeAt_ms ) < 8000UL ) {
          mixer1.gain ( 2, 1.0 );           // from string1
          mixer1.gain ( 3, 0.0 );           // from mixer2
          mixer9.gain ( 0, 1.0 );           // gain from 0.0 to 32767.0  1.0=straight through
          mixer9.gain ( 1, 0.0 );
          mixer9.gain ( 2, 0.0 );
          mixer9.gain ( 3, 0.0 );
          if ( ( millis() - lastDinkAt_ms ) > dinkRate_ms ) {
            // string1.noteOn ( 512.0, 0.8 );
            float pitch = 440.0;
            for ( int i = 0; i < ( millis() - lastStateChangeAt_ms ) / 1000; i++ ) {
              pitch *= 1.18920712;  // 1.05946309436 ^ 3;
            }
            string1.noteOn ( pitch, 0.8 );
            lastDinkAt_ms = millis();
          }
        } else {
          mixer1.gain ( 2, 0.0 );           // from string1; disable more dinks
          mixer1.gain ( 3, 0.25 );          // from mixer2
          mixer9.gain ( 0, 0.0 );           // gain from 0.0 to 32767.0  1.0=straight through
          mixer9.gain ( 1, 0.0 );
          mixer9.gain ( 2, 0.0 );
          mixer9.gain ( 3, 1.0 );           // from mixer2
        }
        break;
      }
      
  if ( ( millis() - lastBlinkAt_ms ) > blinkRate_ms ) {
    // digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    if ( state < 4 && potValue > 32 ) string1.noteOn ( 440, 0.25 );
    lastBlinkAt_ms = millis();
  }
      
}