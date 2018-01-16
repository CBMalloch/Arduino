#define PROGNAME  "generalOpenEffectsBox_v4"
#define VERSION   "0.0.1"
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
  oeb.init ();
}

void loop () {
  // oeb.printSomething ( millis() );
  oeb.tickle ();
  delay ( 2 );
}

