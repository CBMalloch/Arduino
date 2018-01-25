#define PROGNAME  "generalOpenEffectsBox_v4"
#define VERSION   "0.2.1"
#define VERDATE   "2018-01-24"



/*! \brief Firmware for OnkartGromt Open Effects Project Hardware

#References:
  [The OpenEffects Project – the source of the open effects boxes](http://openeffectsproject.com/)
  
  [The boxes are powered by Teensy microcontrollers from Paul Stoffregen](https://www.pjrc.com/)
    … and use [the same chip found on the Teensy Audio Shield](https://www.pjrc.com/store/teensy3_audio.html)

  [Paul Stoffregen's Audio Design Tool which generates the starting framework for Teensy audio effects](https://www.pjrc.com/teensy/gui/)
 
#Acknowledgements:
  Many thanks to Øyvind Mjanger, who developed the OpenEffects Project hardware
  and to Paul Stoffregen, who developed the Teensy and especially its audio processing capabilities
  
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// use Paul Stoffregen's NeoPixel library if using Teensy 3.5 or 3.6
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

#include "OpenEffectsBox.h"

OpenEffectsBox oeb;

#define BAUDRATE 115200

void setup () {

  Serial.begin ( BAUDRATE );
  while ( !Serial && millis () < 5000 );

  Serial.print ( "OpenEffectsBox v." ); Serial.println ( OpenEffectsBox_VERSION );
  Serial.print ( "OpenEffectsBoxHW v." ); Serial.println ( OpenEffectsBoxHW_VERSION );
  Serial.print ( "Potentiometer v." ); Serial.println ( Potentiometer_VERSION );
  Serial.print ( "BatSwitch v." ); Serial.println ( BatSwitch_VERSION );
  Serial.print ( "FootSwitch v." ); Serial.println ( FootSwitch_VERSION );
  Serial.print ( "Pedal v." ); Serial.println ( Pedal_VERSION );
  Serial.print ( "Relay v." ); Serial.println ( Relay_VERSION );
  
  Serial.print ( "OLED v." ); Serial.println ( Oled_VERSION );
  
  Serial.print ( "Mode0 v." ); Serial.println ( Mode0_VERSION );
  Serial.print ( "Mixer v." ); Serial.println ( Mixer_VERSION );
  Serial.print ( "Sine v." ); Serial.println ( Sine_VERSION );
  Serial.print ( "Tonesweep v." ); Serial.println ( Tonesweep_VERSION );
  Serial.print ( "DC v." ); Serial.println ( DC_VERSION );
  Serial.print ( "Bitcrusher v." ); Serial.println ( Bitcrusher_VERSION );
  Serial.print ( "Chorus v." ); Serial.println ( Chorus_VERSION );
  Serial.print ( "Flange v." ); Serial.println ( Flange_VERSION );
  Serial.print ( "Filter v." ); Serial.println ( Filter_VERSION );
  Serial.print ( "Reverb v." ); Serial.println ( Reverb_VERSION );
  Serial.print ( "DelayExt v." ); Serial.println ( DelayExt_VERSION );
  
  Serial.print ( "DisplayableModule v." ); Serial.println ( DisplayableModule_VERSION );

  oeb.init ( (char *) VERSION );
  
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 250 );

}

void loop () {
  oeb.tickle ();
  delay ( 2 );
}

