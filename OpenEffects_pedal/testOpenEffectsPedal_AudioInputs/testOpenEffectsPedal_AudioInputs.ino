#define PROGNAME  "testOpenEffectsPedal_AudioInputs"
#define VERSION   "0.0.4"
#define VERDATE   "2017-08-01"


// currently set up to test the OpenEffects box

/*
  Please remember that the standard Serial and the standard LED on pin 13
  both interfere with audio board pin mappings. Don't use either of them!
  They will work, but their use will disable the audio board's workings.
*/

const int pd_relayL = 4;
const int pd_relayR = 5;


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=355.75,605.75
AudioInputI2S            i2s2;           //xy=387.75,430.75
AudioMixer4              mixer1;         //xy=566.75,462.75
AudioMixer4              mixer2;         //xy=566.75,547.75
AudioOutputI2S           i2s1;           //xy=765.75,432.75
AudioConnection          patchCord1(sine1, 0, mixer1, 1);
AudioConnection          patchCord2(sine1, 0, mixer2, 1);
AudioConnection          patchCord3(i2s2, 0, mixer1, 0);
AudioConnection          patchCord4(i2s2, 1, mixer2, 0);
AudioConnection          patchCord5(mixer1, 0, i2s1, 0);
AudioConnection          patchCord6(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=397.75,368.75
// GUItool: end automatically generated code


void setup () {

  AudioMemory ( 250 );
  // AudioMemory_F32 ( 100 ); //allocate Float32 audio data blocks

  pinMode ( pd_relayL, OUTPUT );
  pinMode ( pd_relayR, OUTPUT );
  
  Serial.begin ( 115200 );
  while ( !Serial && millis() < 4000 ) { delay ( 100 ); }
    
  sgtl5000_1.enable();  // Enable the audio shield
  
  #if 1
    sgtl5000_1.inputSelect ( AUDIO_INPUT_LINEIN );
    sgtl5000_1.lineInLevel ( 0 );   // default 0 is 3.12Vp-p
  #else
    sgtl5000_1.inputSelect ( AUDIO_INPUT_MIC );
    sgtl5000_1.micGain ( 20 );      // dB
  #endif
  
  sgtl5000_1.volume ( 1.0 );
  sgtl5000_1.lineOutLevel ( 13 );   // default 13 is 3.16Vp-p
  
  sine1.amplitude ( 1.0 );
  sine1.frequency ( 261 ); // .6 );
  
  mixer1.gain ( 0, 1.0 );
  mixer1.gain ( 1, 1.0 );
  
  mixer2.gain ( 0, 1.0 );
  mixer2.gain ( 1, 1.0 );
    
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 500 );

}


void loop() {
  static unsigned long lastSwitchAt_ms = 0UL;
  const unsigned long switchingRate_ms = 1000UL;
  static int state = 0;
  
  if ( ( millis() - lastSwitchAt_ms ) > switchingRate_ms ) {
    
    state++;
    if ( state > 3 ) state = 0;
    
    /*
      internal sine not working now
    */
    
    
    
    
    
    
    int stateR = ( ( state >> 1 ) ^ state ) & 0x01;
    digitalWrite ( pd_relayR, stateR );
    Serial.print ( "  R: " ); Serial.print ( stateR );
    
    int stateL = ( state >> 1 ) & 0x01;
    digitalWrite ( pd_relayL, stateL );
    Serial.print ( "  L: " ); Serial.print ( stateL );
    
    Serial.println ( );

    lastSwitchAt_ms = millis();
  }

}