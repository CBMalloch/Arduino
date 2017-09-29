/* 
  Sketch written for Teensy with Audio Board
  Author: Charles B. Malloch, PhD
  Date:   2017-09-28
    
  The piezoelectric pickup on my electric fiddle goes, unamplified,
  straight into the AUDIO_INPUT_LINEIN *left* channel
    tip to L, sleeve to GND
  
  I play a note; the notefreq block provides me the frequency of that note;
  I calculate a new frequency a major third (4 semitones) above this and 
  play a sine wave at that frequency.
  
  I'm using a once-per-second A440 string pluck as a verification that the 
  program is still running.
  
  Note to the uninitiated: the stuff marked out by // GUItool can be 
  copied and pasted into Paul Stoffregen's Audio System Design Tool 
  at <https://www.pjrc.com/teensy/gui/> to see a graphical representation
  of the patch
*/

// const int pdLED = 13;
float halfSteps [ 13 ];

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthKarplusStrong  string1;        //xy=291,285
AudioInputI2S            i2s2;           //xy=317,150
AudioSynthWaveformSine   sine1;          //xy=335,233
AudioAnalyzeNoteFrequency notefreq;       //xy=480,112
AudioMixer4              mixer1;         //xy=555,223
AudioOutputI2S           i2s1;           //xy=749,142
AudioConnection          patchCord1(string1, 0, mixer1, 2);
AudioConnection          patchCord2(i2s2, 0, i2s1, 0);
AudioConnection          patchCord3(i2s2, 0, notefreq, 0);
AudioConnection          patchCord4(i2s2, 1, mixer1, 0);
AudioConnection          patchCord5(sine1, 0, mixer1, 1);
AudioConnection          patchCord7(mixer1, 0, i2s1, 0);
AudioConnection          patchCord6(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=320,81
// GUItool: end automatically generated code

//---------------------------------------------------------------------------------------

void setup() {
  // Serial.begin ( 115200 ); while ( !Serial && millis() < 4000 );
  
  halfSteps [ 0 ] = 1.0;
  const double halfStep = exp ( log ( 2.0 ) / 12.0 );
  for ( int i = 1; i < 13; i++ ) {
    halfSteps [ i ] = halfSteps [ i - 1 ] * halfStep;
  }
  
  AudioMemory ( 100 );
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

  mixer1.gain ( 0, 1.0 );
  mixer1.gain ( 1, 1.0 );
  mixer1.gain ( 2, 1.0 );
  mixer1.gain ( 3, 1.0 );

  // Initialize the yin algorithm's absolute threshold, this is good number.

  notefreq.begin (0.15);
  
  // pinMode ( LED_BUILTIN, OUTPUT );
    
  // Serial.println ( "testNoteFrequency.ino v0.0.1  2017-09-28" );
}

void loop () {

   static unsigned long lastNoteAt_ms = 0UL;
   const unsigned long noteDelay_ms = 200UL;
   
   static float note = -1.0;
   static float prob;

  // read back fundamental frequency
  if ( notefreq.available() ) {
    note = notefreq.read ();
    prob = notefreq.probability ();
    if ( prob > 0.98 ) {
      sine1.frequency ( note * halfSteps [ 4 ] );
      sine1.amplitude ( 1.0 );
      lastNoteAt_ms = millis ();
    }
    // Serial.printf("Note: %3.2f | Probability: %.2f\n", note, prob);
  } else {
    if ( ( millis() - lastNoteAt_ms ) > noteDelay_ms ) sine1.amplitude ( 0.0 );
  }

   static unsigned long lastBlinkAt_ms = 0UL;
   const unsigned long blinkDelay_ms = 1000UL;
      
  if ( ( millis() - lastBlinkAt_ms ) > blinkDelay_ms ) {
    // int counts = -1;
    // counts = analogRead ( 3 );
    // Serial.print ( "A3: " ); Serial.println ( counts );
    // digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    if ( ( millis() - lastNoteAt_ms ) > 1000UL ) string1.noteOn ( 440, 0.02 );
    lastBlinkAt_ms = millis();
  }

}
