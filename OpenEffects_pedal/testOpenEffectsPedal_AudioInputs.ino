#define PROGNAME  "testOpenEffectsPedal_AudioInputs"
#define VERSION   "0.0.1"
#define VERDATE   "2017-08-01"


// currently set up to test the OpenEffects box

const int pd_relayL = 4;
const int pd_relayR = 5;


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=387.75,430.75
AudioOutputI2S           i2s1;           //xy=568.75,432.75
AudioConnection          patchCord1(i2s2, 0, i2s1, 0);
AudioConnection          patchCord2(i2s2, 1, i2s1, 1);
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
    
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 500 );

}


void loop() {
  static unsigned long lastSwitchAt_ms = 0UL;
  const unsigned long switchingRate_ms = 1000UL;
  static int relayState = 0;
  
  if ( ( millis() - lastSwitchAt_ms ) > switchingRate_ms ) {
    
    relayState = 1 - relayState;
    digitalWrite ( pd_relayR, relayState );
    digitalWrite ( pd_relayL, relayState );
    Serial.print ( "Relays are now " );
    Serial.println ( relayState ? "activated" : "deactivated" );

    lastSwitchAt_ms = millis();
  }

}