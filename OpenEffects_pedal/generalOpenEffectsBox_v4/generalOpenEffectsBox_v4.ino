#define PROGNAME  "generalOpenEffectsBox_v4"
#define VERSION   "0.1.0"
#define VERDATE   "2018-01-10"

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
  
  Serial.print ( "Mixer v." ); Serial.println ( Mixer_VERSION );
  Serial.print ( "Sine v." ); Serial.println ( Sine_VERSION );
  Serial.print ( "DisplayableModule v." ); Serial.println ( DisplayableModule_VERSION );

  oeb.init ();
  
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 250 );

}

void loop () {
  oeb.tickle ();
  delay ( 2 );
}

