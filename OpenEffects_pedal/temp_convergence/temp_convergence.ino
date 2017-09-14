#define PROGNAME  "temp_convergence"
#define VERSION   "0.0.1"
#define VERDATE   "2017-09-10"

const int pdLED = 13;

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=351.66668701171875,369.8666687011719
AudioSynthKarplusStrong  string1;        //xy=351.66668701171875,413.8666687011719
AudioMixer4              mixer1;         //xy=577.6666870117188,386.8666687011719
AudioEffectDelay         delay1;         //xy=602.6666870117188,605.8666687011719
AudioMixer4              mixer2;         //xy=764.6666870117188,583.8666687011719
AudioOutputI2S           i2s2;           //xy=966.6666870117188,492.8666687011719
AudioConnection          patchCord1(i2s1, 0, i2s2, 0);
AudioConnection          patchCord2(i2s1, 1, i2s2, 1);
AudioConnection          patchCord3(string1, 0, mixer1, 2);
AudioConnection          patchCord4(delay1, 0, mixer2, 0);
AudioConnection          patchCord5(delay1, 1, mixer2, 1);
AudioConnection          patchCord6(delay1, 2, mixer2, 2);
AudioConnection          patchCord7(delay1, 3, mixer2, 3);
AudioConnection          patchCord8(mixer2, 0, mixer1, 3);
AudioControlSGTL5000     sgtl5000_1;     //xy=633.6666870117188,258.8666687011719
// GUItool: end automatically generated code


void setup () {

  // Serial.begin ( 9600 );
  // while ( !Serial && millis() < 4000 ) { delay ( 100 ); }
  
  // pinMode ( pdLED, OUTPUT );
  // digitalWrite ( pdLED, 0 );
  // pinMode ( pdLED, INPUT );
    
  // needs 1/3 block for each millisecond of delay = 67 blocks for 200 ms
  AudioMemory ( 80 );
  // AudioMemory_F32 ( 100 ); //allocate Float32 audio data blocks

  sgtl5000_1.enable();  // Enable the audio shield

  sgtl5000_1.inputSelect ( AUDIO_INPUT_LINEIN );
  sgtl5000_1.lineInLevel ( 5 );     // default 5 is 1.33Vp-p; 0 is 3.12Vp-p

  // sgtl5000_1.inputSelect ( AUDIO_INPUT_MIC );
  // sgtl5000_1.micGain ( 31 );        // 0-63 dB
  
  sgtl5000_1.volume ( 0.5 );
  sgtl5000_1.lineOutLevel ( 29 );   // default 29 is 1.29Vp-p; 13 is 3.16Vp-p
  
  const float inputGain = 10.0;      // gain from 0.0 to 32767.0  1.0=straight through
  mixer1.gain ( 0, inputGain );     // i2s input
  mixer1.gain ( 1, inputGain );     // i2s input
  mixer1.gain ( 2,       1.0 );     // string
  mixer1.gain ( 3,       0.2 );     // recycle
  
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
  static unsigned long lastBlinkAt_ms = 0UL;
  const unsigned long blinkRate_ms = 1000UL;
  
  if ( ( millis() - lastBlinkAt_ms ) > blinkRate_ms ) {
    digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    // string1.noteOn ( 440, 0.25 );
    lastBlinkAt_ms = millis();
  }
      
}