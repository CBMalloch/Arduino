#include <Arduino.h>
#include <Print.h>

#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// use Paul Stoffregen's NeoPixel library if using Teensy 3.5 or 3.6
#include <Adafruit_NeoPixel.h>
#include <Audio.h>

#include "Potentiometer.h"
#include "BatSwitch.h"
#include "FootSwitch.h"
#include "Pedal.h"
#include "Relay.h"

#ifndef OpenEffectsBoxHW_h
#define OpenEffectsBoxHW_h

// needed for callbacks
// #include "OpenEffectsBox.h"

#define OpenEffectsBoxHW_VERSION "0.000.005"

/* ======================================================================== //

                           hardware definitions
                           
// ======================================================================== */

/*

                      N O T E S
                      
                      
  I burned out my original OpenEffects board. It is now replaced, but I 
  installed *momentary* switches to the bat switch locations. So now I can
  digitally change states, but I need to have a debounced read of these
  switches and maintain the resulting states.
  Difficulty: the bat switches return analog values, which cannot be used to 
  trigger an interrupt.
        left     right    (switch)
    L    GND      Vcc
    C    Vcc/2    Vcc/2
    R    Vcc      GND
    
  
  I also installed the other bank of input/output jacks; the input is on the 
  R and is 1/4" stereo; the output is on the L and is 1/4" stereo; the two
  center jacks are for expression pedals. Note that the rightmost of these
  is labeled Exp1 but goes to A11 on the Teensy, which is described in the 
  schematic as being connected to CVInput2.
  The expression pedals divide bewteen Vcc and GND and return a value
  somewhere in between. The pinout of the 1/4" stereo jacks for these is
    T - voltage variably divided by the effects pedal
    R - 3V3
    S - GND
    
*/





/*

  Observations:
    o should create separate objects for knob, bat, foot switch, relay
  Planned work:
  
*/




/* ------------------------------------------------------------------------ //
                              pin assignments                       
// ------------------------------------------------------------------------ */

// Pinout board rev1
// pa = pin, analog; pd = pin, digital

// pots
const int pa_pots [] = { A6, A3, A2, A1 };

// bat switches
const int pa_bats [] = { A12, A13 };

// stomp "pb" pushbuttons
//   pins differ between red board and blue board; I have blue
const int pd_pbs [] = { 2, 3 };

// relays
const int pd_relays [] = { 4, 5 };

// pedals
const int pa_pedals [] = { A11, A10 };

// NeoPixels
const int pdWS2812 = 8;


/* ------------------------------------------------------------------------ //
                                 constants                       
// ------------------------------------------------------------------------ */

// pots
const int nPots = 4;

// bat switches
const int nBats = 2;
const int batL = 0;
const int batR = 1;

// stomp "pb" pushbuttons
const int nPBs = 2;
const int pbL = 0;
const int pbR = 1;

// relays
const int nRelays = 2;
const int relayL = 0;
const int relayR = 1;

// pedals
const int nPedals = 2;

// NeoPixels
const int nNeoPixels    = 10;
const int ledSingleton = 0;
const int ledOnOff     = 2;
const int ledBoost     = 1;
const int VUfirstPixel = 3;
const int nVUpixels = 7;

// OLED screen



class OpenEffectsBoxHW {
  public:
  
    OpenEffectsBoxHW ();  // constructor
    
    void init ();
    void tickle ();
    
    // void register_cbOnBatChanged ( int which, void ( OpenEffectsBox::*cb ) ( int * newValue ) );
    // void register_cbOnBatChanged ( int which, void ( * cb ) ( int * ) );
        
    Potentiometer pot [ nPots ];
    BatSwitch bat [ nBats ];
    FootSwitch pb [ nPBs ];
    Relay relay [ nRelays ];
    Pedal pedal [ nPedals ];
    
    void setLED ( 
      int led, 
      unsigned long color = 0x101010UL
    );

    void setVU ( 
      int n, 
      int mode = 1, 
      unsigned long onColor = 0x106060UL, 
      unsigned long offColor = 0x101010UL
    );

  private:

    void updateInputs ();
    
    int _verbose;
    
    // relays

    // pedals

    // NeoPixels
    // Parameter 1 = number of pixels in strip
    // Parameter 2 = Arduino pin number (most are valid)
    // Parameter 3 = pixel type flags, add together as needed:
    //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
    //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
    //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
    //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
    Adafruit_NeoPixel _strip = Adafruit_NeoPixel ( nNeoPixels, pdWS2812, NEO_GRB + NEO_KHZ800 );
    //  0 is the singleton
    //  1 is R dome
    //  2 is L dome
    //  3-9 are the rectangular ones in the row, with 3 at R and 9 at L
    void init_NeoPixel_strip ();

    // OLED screen
    bool _displayIsStale = true;
    unsigned long _lastOledUpdateAt_ms = 0UL;
    unsigned long _displayUpdateRate_ms = 200UL;

    void init_oled_display ();
    // void displayOLED ();
    // void ( * updateOLEDdisplay ) ();

    Adafruit_SSD1306 _oled = Adafruit_SSD1306 ();
    
};

#endif