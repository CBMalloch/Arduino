#define PROGNAME  "testOpenEffectsPedal_AudioInputs"
#define VERSION   "0.0.5"
#define VERDATE   "2017-09-26"


// currently set up to test the OpenEffects box

/*
  Please remember that the standard Serial and the standard LED on pin 13
  both interfere with audio board pin mappings. Don't use either of them!
  They will work, but their use will disable the audio board's workings.
*/

// Working perfectly now. 
// Input always sounds, either through processor or directly.
//   But when processed, it is multiplied by a 55Hz sine
// Sine sounds only while processor is patched in by relays

#undef BAUDRATE

const int pd_relayL = 4;
const int pd_relayR = 5;


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine2;          //xy=233,312
AudioInputI2S            i2s2;           //xy=240,240
AudioSynthWaveformSine   sine1;          //xy=415,394
AudioEffectMultiply      multiply2;      //xy=416,307
AudioEffectMultiply      multiply1;      //xy=418,234
AudioMixer4              mixer1;         //xy=615,269
AudioMixer4              mixer2;         //xy=615,354
AudioOutputI2S           i2s1;           //xy=814,239
AudioConnection          patchCord1(sine2, 0, multiply1, 1);
AudioConnection          patchCord2(sine2, 0, multiply2, 1);
AudioConnection          patchCord3(i2s2, 0, multiply1, 0);
AudioConnection          patchCord4(i2s2, 1, multiply2, 0);
AudioConnection          patchCord5(sine1, 0, mixer1, 1);
AudioConnection          patchCord6(sine1, 0, mixer2, 1);
AudioConnection          patchCord7(multiply2, 0, mixer2, 0);
AudioConnection          patchCord8(multiply1, 0, mixer1, 0);
AudioConnection          patchCord9(mixer1, 0, i2s1, 0);
AudioConnection          patchCord10(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=250,178
// GUItool: end automatically generated code

#define USE_PIXELS 1
#if USE_PIXELS == 1
  #include <Adafruit_NeoPixel.h>
  //const int nPixels    = 60;
  const int nPixels    = 10;
  // const int pdWS2812   = 12;   // is GPIO12; pin is D6 on NodeMCU
  const int pdWS2812 = 8;

  // Parameter 1 = number of pixels in strip
  // Parameter 2 = Arduino pin number (most are valid)
  // Parameter 3 = pixel type flags, add together as needed:
  //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
  //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
  //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
  Adafruit_NeoPixel strip = Adafruit_NeoPixel( nPixels, pdWS2812, NEO_GRB + NEO_KHZ800 );
  //  0 is the singleton
  //  1 is R dome
  //  2 is L dome
  //  3-9 are the rectangular ones in the row, with 3 at R and 9 at L
  const int ledSingleton = 0;
  const int ledState     = 2;
  const int ledBoost     = 1;
  int brightness = 64;

  // IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
  // pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
  // and minimize distance between Arduino and first pixel.  Avoid connecting
  // on a live circuit...if you must, connect GND first.
#endif

void setup () {

  AudioMemory ( 250 );
  // AudioMemory_F32 ( 100 ); //allocate Float32 audio data blocks

  pinMode ( pd_relayL, OUTPUT );
  pinMode ( pd_relayR, OUTPUT );
  
  // Serial.begin ( 115200 );
  // while ( !Serial && millis() < 4000 ) { delay ( 100 ); }
    
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
  
  sine2.amplitude ( 1.0 );
  sine2.frequency ( 55 );
  
  sine1.amplitude ( 1.0 );
  sine1.frequency ( 261 ); // .6 );
  
  mixer1.gain ( 0, 1.0 );
  mixer1.gain ( 1, 1.0 );
  
  mixer2.gain ( 0, 1.0 );
  mixer2.gain ( 1, 1.0 );
    
  #if USE_PIXELS == 1
    strip.begin();
    for ( int i = 0; i < nPixels; i++ ) {
      strip.setPixelColor ( i, 0x00000000 );
    }
    strip.setBrightness ( 0x40 );
    strip.show();
    // Serial.print ( "\nUsing pixels...\n" );
  #endif
  
  // Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 500 );

}


void loop() {
  static unsigned long lastSwitchAt_ms = 0UL;
  const unsigned long switchingRate_ms = 1000UL;
  static int state = 0;
  
  if ( ( millis() - lastSwitchAt_ms ) > switchingRate_ms ) {
    
    state++;
    if ( state > 3 ) state = 0;
        
    int stateR = ( ( state >> 1 ) ^ state ) & 0x01;
    digitalWrite ( pd_relayR, stateR );
    // Serial.print ( "  R: " ); Serial.print ( stateR );
    
    int stateL = ( state >> 1 ) & 0x01;
    digitalWrite ( pd_relayL, stateL );
    // Serial.print ( "  L: " ); Serial.print ( stateL );
    
    // Serial.println ( );

    #if USE_PIXELS == 1
      // for ( int i = 0; i < nPixels; i++ ) {
      //   strip.setPixelColor ( i, ( unsigned long ) 0x00000000 );
      // }
      strip.setPixelColor ( 3, stateL ? 0x00101010 : 0x00000000 );
      strip.setPixelColor ( 4, stateR ? 0x00101010 : 0x00000000 );
      strip.show();
    #endif
    
    lastSwitchAt_ms = millis();
  }

}