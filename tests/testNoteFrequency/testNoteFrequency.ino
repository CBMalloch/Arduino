/* 
  Sketch written for Teensy with Audio Board
  Author: Charles B. Malloch, PhD
  Date:   2017-09-28
    
  The piezoelectric pickup on my electric fiddle goes, unamplified,
  straight into the AUDIO_INPUT_LINEIN *left* channel
    tip to L, sleeve to GND
  
  I play a noteFreq; the notefreq block provides me the frequency of that noteFreq;
  I calculate a new frequency a major third (4 semitones) above this and 
  play a sine wave at that frequency.
  
  I'm using a once-per-second A440 string pluck as a verification that the 
  program is still running.
  
  noteFreq to the uninitiated: the stuff marked out by // GUItool can be 
  copied and pasted into Paul Stoffregen's Audio System Design Tool 
  at <https://www.pjrc.com/teensy/gui/> to see a graphical representation
  of the patch
*/

// const int pdLED = 13;
const double halfStep = exp ( log ( 2.0 ) / 12.0 );
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

const int bufLen = 3 + 1;
char strBuf [ bufLen ];
// void noteDetails ( char * pNoteNameString, int cents, float f );

//---------------------------------------------------------------------------------------

void setup() {
  Serial.begin ( 115200 ); while ( !Serial && millis() < 4000 );
  
  halfSteps [ 0 ] = 1.0;
  for ( int i = 1; i < 13; i++ ) {
    halfSteps [ i ] = halfSteps [ i - 1 ] * halfStep;
  }
  
  if ( 0 ) {
    int cents;
    noteDetails ( strBuf, &cents, 440.0 );  // A4
    Serial.printf ( "A4: %s + %d cents\n", strBuf, cents );
    noteDetails ( strBuf, &cents, 220.0 );  // A3
    Serial.printf ( "A3: %s + %d cents\n", strBuf, cents );
    noteDetails ( strBuf, &cents, 110.0 * halfSteps [ 2 ] );  // B2
    Serial.printf ( "B2: %s + %d cents\n", strBuf, cents );
    noteDetails ( strBuf, &cents, 880.0 * halfSteps [ 11 ] );  // G#6
    Serial.printf ( "Ab6: %s + %d cents\n", strBuf, cents );
    noteDetails ( strBuf, &cents, 890.25 );  // ??
    Serial.printf ( "A5, a little sharp: %s + %d cents\n", strBuf, cents );
    noteDetails ( strBuf, &cents, 860.0 );  // ??
    Serial.printf ( "A5, a little flat: %s + %d cents\n", strBuf, cents );
    while ( 1 );
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
   
   static float noteFreq = -1.0;
   static float noteProb;

  // read back fundamental frequency
  if ( notefreq.available() ) {
    noteFreq = notefreq.read ();
    noteProb = notefreq.probability ();
    if ( noteProb > 0.98 ) {
      sine1.frequency ( noteFreq * halfSteps [ 4 ] );
      sine1.amplitude ( 1.0 );
      lastNoteAt_ms = millis ();
    }
    if ( 1 ) {
      int cents;
      noteDetails ( strBuf, &cents, noteFreq );
      Serial.printf("noteFreq: %3s + %d cents ( %3.2fHz ) p=%.2f\n", strBuf, cents, noteFreq, noteProb);
    }
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

void noteDetails ( char *pNoteNameString, int *pNoteCents, float f ) {
  const int VERBOSE = 0;
  const double halfStep = exp ( log ( 2.0 ) / 12.0 );
  const char noteNames [ 12 ] [ 3 ] = { "A", "Bb", "B", "C", "C#", "D", 
                                  "Eb", "E", "F", "F#", "G", "Ab" };
  const int octaveNumbers [] = { 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5 };
  
  // the distance from A440 in half-steps
  float fromA440 = log ( f / 440.0 ) / log ( halfStep );
  float remainder = fromA440;
  
  int baseOctave, halfSteps;
  float fcents;
  baseOctave = round ( ( remainder + 600 ) / 12.0 ) - 50.0;
  remainder -= 12 * baseOctave;
  halfSteps = round ( remainder );
  fcents = ( remainder - ( float ) halfSteps ) * 100.0;
  *pNoteCents = round ( fcents );
  
  snprintf ( pNoteNameString, 4, "%s%d", 
              noteNames [ halfSteps ], baseOctave + octaveNumbers [ halfSteps ] );
  
  if ( VERBOSE >= 4 ) {
    Serial.printf ( "f: %2.1f\nfromA440: %2.2f\nbaseOctave: %2d\nhalf-steps: %2d\n",
                  f, fromA440, baseOctave, halfSteps );
    Serial.printf ( "fcents: %2.1f\n", fcents );
    Serial.printf ( "Note name: %s\n", pNoteNameString );
  }
  
  return;
}

