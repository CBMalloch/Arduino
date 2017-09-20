/*
  Test Teensy sound production without the help of an external chip.
  The Teensy 3.5 and 3.6 can send data directly to ADC pins.
    L channel is A21 is DAC0
    R channel is A22 is DAC1
  
  In phase 2, we will add a software serial output to verify that we can 
  do serial output without interfering with the audio.
  
  Duh. Nothing worked at all until I eventually put in the line
      AudioMemory ( 20 );
  in the setup routine...
  
  The program works perfectly now, using the standard Serial object, 
  the LED on pin D13. I have a 1/8" stereo jack wired thus:
    tip to pin A21 = DAC0 = left channel
    ring to pin A22 = DAC1 = right channel
    sleeve to pin AGND = analog ground via a 10uF 25V electrolytic capacitor,
      which will block the DC
  
*/

/*
  
  "Generally the DAC pins are able to drive the 10K to 100K input impedance 
  of typical line-level inputs on amplifiers & computer speakers.
  They aren't able to drive headphones or speakers, or inputs 
  which emulate the 33 ohm load of a headphone jack." - Paul Stoffregen
  
*/

/*
  ( copied from a program that uses the audio shield, so not fully accurate )
  
  What IO remains available while using the Teensy Audio Shield?
    See https://www.pjrc.com/store/teensy3_audio.html
    
    Looks like pins 0, 1, 2, 3, 4, 5, 8, 16, 17, 20, 21 should be available.
    Pins 18 and 19 are sharable with other I2C devices
    Pins 7, 12, and 14 are sharable with other SPI devices
    Using the standard Serial object stops the audio.
    Pin 21 is A7; pin 20 is A6; pin 16 is A2; pin 15 is A1
    
*/

const int pdLED = 13;

#undef USE_SINE
#define USE_LED
#undef USE_BEGIN_UPDATE
#define BAUDRATE 115200

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin NOT automatically generated code
AudioSynthKarplusStrong  string1;        //xy=459,404
AudioSynthWaveformSine   sine1;          //xy=432,485
AudioOutputAnalogStereo  dacs1;          //xy=621,404
#ifdef USE_SINE
  AudioConnection          patchCord1(sine1, 0, dacs1, 0);
  AudioConnection          patchCord2(sine1, 0, dacs1, 1);
#else
  AudioConnection          patchCord1(string1, 0, dacs1, 0);
  AudioConnection          patchCord2(string1, 0, dacs1, 1);
#endif
// GUItool: end NOT automatically generated code

void setup ()  {

  #ifdef BAUDRATE
    Serial.begin ( BAUDRATE );
    while ( ! Serial && millis() < 8000 );
    Serial.println ( "Hello from testNoAudioBoard.ino" );
  #endif

  AudioMemory ( 20 );

  #ifdef USE_LED
    pinMode ( pdLED, OUTPUT );
  #endif
  
  dacs1.analogReference ( EXTERNAL );  // use 3.3V values
  // dacs1.analogReference ( INTERNAL );
  
  #ifdef USE_BEGIN_UPDATE
    dacs1.begin ();
  #endif
    
  #ifdef USE_SINE
    sine1.frequency ( 440 );
    sine1.amplitude ( 0.75 );
  #endif
  
  delay ( 100 );
}

void loop ()  {
  
  static unsigned long lastSoundAt_ms = 0UL;
  const unsigned long soundInterval_ms = 1000UL;
  
  static unsigned long lastBlinkAt_ms = 0UL;
  const unsigned long blinkInterval_ms = 200UL;
  
  #ifdef USE_BEGIN_UPDATE
    dacs1.update ();
  #endif
  
  #ifndef USE_SINE
    if ( ( millis() - lastSoundAt_ms ) > soundInterval_ms ) {
      string1.noteOn ( 440, 0.75 );  
      lastSoundAt_ms = millis();
    }
  #endif

  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    #ifdef USE_LED
      digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
    #endif
    lastBlinkAt_ms = millis();
  }
  
  delay ( 2 );

}
  
