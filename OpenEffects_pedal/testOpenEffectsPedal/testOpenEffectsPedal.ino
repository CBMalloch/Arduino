#define PROGNAME  "testOpenEffectsPedal"
#define VERSION   "0.3.4"
#define VERDATE   "2017-09-26"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// get Paul Stoffregen's NeoPixel library if using Teensy 3.5 or 3.6
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

// NOTE - can use Serial with OpenEffects or Audio boards
#define BAUDRATE 115200

#define VERBOSE 5
#if VERBOSE >= 10
  // Profiler is my own code-profiling library
  // using it I found that the OLED screen takes 100ms to update
  #include <Profiler.h>
#endif

// currently set up to test the OpenEffects box

/*
  Please remember that the standard Serial and the standard LED on pin 13
  both interfere with audio board pin mappings. Don't use either of them!
  They will work, but their use will disable the audio board's workings.
*/

/*
  I burned out my original OpenEffects board. It is now replaced, but I 
  installed *momentary* switches to the bat switch locations. So now I can
  digitally change states, but I need to have a debounced read of these
  switches and maintain the resulting states.
  Difficulty: the bat switches return analog values
        left     right    (switch)
    L    GND      Vcc
    C    Vcc/2    Vcc/2
    R    Vcc      GND
    
  
  I also installed the other bank of input/output jacks; the input is on the 
  R and is 1/4" stereo; the output is on the L and is 1/4" stereo; the two
  center positions are for expression pedals. 
  The expression pedals are expected to use Vcc and GND and to return a value
  somewhere between these. The pinout of the 1/4" stereo jacks for these is
    T
    R
    S
*/

//Pinout board rev1
// pa = pin, analog; pd = pin, digital
const int pa_pots [] = { A6, A3, A2, A1 };
// tap pins differ between red board and blue board; I have blue
// pb are the stomp switches "pushbutton"
const int pd_pb1 = 2;
const int pd_pb2 = 3;
// const int pd_LED = 13;

const int pd_relayL = 4;
const int pd_relayR = 5;

const int pa_CV1 = A11;
const int pa_CV2 = A10;

const int pa_batSwitches [] = { A12, A13 };
const unsigned long batSwitchNoticePeriod_ms =  50UL;
const unsigned long batSwitchRepeatPeriod_ms = 500UL;
unsigned long batSwitchLastCenteredAt_ms [] = { 0UL, 0UL };
unsigned long batSwitchLastRepeatAt_ms [] = { 0UL, 0UL };
int batSwitchStatus [] = { 0, 0 };

int cv1_counts = 0;
int cv2_counts = 0;

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

// OLED screen
#define OLED_RESET 4
Adafruit_SSD1306 oled ( OLED_RESET );

bool displayIsStale = true;
unsigned long lastOledUpdateAt_ms = 0UL;
unsigned long displayUpdateRate_ms = 200UL;

void init_oled_display ();

uint32_t Wheel ( byte WheelPos );
void pixelTest_step ( const uint16_t startPixel  = 0, 
                      const uint16_t n           = nPixels );
void displayOLED_0 ();
void displayOLED_1 ();
void ( *updateOLEDdisplay) ();

#if VERBOSE >= 10
  Profile profile [ 10 ];
#endif

Bounce pb1 = Bounce();
Bounce pb2 = Bounce();

unsigned long stripWheelDelay_ms;
int potReadings [ 4 ];

const int nStates = 4;
// volatile int state = 0;
int state = 0;
// volatile bool boost = false;
bool boost = false;

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=385.75,470.75
AudioInputI2S            i2s2;           //xy=387.75,430.75
AudioSynthSimpleDrum     drum1;          //xy=388.75,512.75
AudioMixer4              mixer1;         //xy=560.75,461.75
AudioAnalyzeRMS          rms1;           //xy=694.75,703.75
AudioAnalyzePeak         peak1;          //xy=713.75,665.75
AudioEffectReverb        reverb1;        //xy=752.75,473.75
AudioEffectDelay         delay1;         //xy=763.75,558.75
AudioMixer4              mixer2;         //xy=948.75,426.75
AudioOutputI2S           i2s1;           //xy=1097.75,427.75
AudioConnection          patchCord1(sine1, 0, mixer1, 2);
AudioConnection          patchCord2(i2s2, 0, mixer1, 0);
AudioConnection          patchCord3(i2s2, 1, mixer1, 1);
AudioConnection          patchCord4(drum1, 0, mixer1, 3);
AudioConnection          patchCord5(mixer1, reverb1);
AudioConnection          patchCord6(mixer1, delay1);
AudioConnection          patchCord7(mixer1, peak1);
AudioConnection          patchCord8(mixer1, rms1);
AudioConnection          patchCord9(mixer1, 0, mixer2, 0);
AudioConnection          patchCord10(reverb1, 0, mixer2, 1);
AudioConnection          patchCord11(delay1, 0, mixer2, 2);
AudioConnection          patchCord12(delay1, 1, mixer2, 3);
AudioConnection          patchCord13(mixer2, 0, i2s1, 0);
AudioConnection          patchCord14(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=688.75,326.75
// GUItool: end automatically generated code

void setup() {

  #ifdef BAUDRATE
    Serial.begin ( BAUDRATE );
    while ( !Serial && millis() < 2000 );
  #endif
  AudioMemory ( 250 );
  // AudioMemory_F32 ( 100 ); //allocate Float32 audio data blocks

  pinMode ( pd_pb1, INPUT );
  pinMode ( pd_pb2, INPUT );
  // pinMode ( pd_LED, OUTPUT );
  pinMode ( pd_relayL, OUTPUT );
  pinMode ( pd_relayR, OUTPUT );
  
  // using Bounce2 library instead of my own, less-functional, debounce routines
  // attachInterrupt(digitalPinToInterrupt(pin), ISR, mode);
  // attachInterrupt ( digitalPinToInterrupt ( pd_pb1 ), ISR_pb1, CHANGE);
  // attachInterrupt ( digitalPinToInterrupt ( pd_pb2 ), ISR_pb2, CHANGE);
  
  pb1.attach(pd_pb1);
  pb1.interval(5); // interval in ms
  pb2.attach(pd_pb2);
  pb2.interval(5); // interval in ms
  
  // for ( int i = 0; i < 2; i++ ) {
  //   digitalWrite ( pd_LED, 1 );
  //   delay ( 200 );
  //   digitalWrite ( pd_LED, 0 );
  //   delay ( 200 );
  // }
    

  // Serial.begin ( BAUDRATE );
  // while ( !Serial && millis() < 4000 ) { delay ( 100 ); }
  
  // for ( int i = 0; i < 4; i++ ) {
  //   digitalWrite ( pd_LED, 1 );
  //   delay ( 200 );
  //   digitalWrite ( pd_LED, 0 );
  //   delay ( 200 );
  // }
    
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  init_oled_display ();
  displayIsStale = true;
  
  #if VERBOSE >= 10
    profile [ 0 ].setup ( "reads", 10, 10 );
    profile [ 1 ].setup ( "oled", 10, 10 );
    profile [ 2 ].setup ( "lightsOuter", 10, 10 );
    profile [ 3 ].setup ( "lightsInner", 10, 10 );
    profile [ 4 ].setup ( "oled_disp", 10, 20 );
  #endif
  
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
  
  sine1.amplitude ( 0.5 );
  sine1.frequency ( 440 );
  
  // drum1 left at default settings
  
  mixer1.gain ( 0, 2.0 );   // i2s2 ( line inputs L & R
  mixer1.gain ( 1, 2.0 );   // i2s2 ( line inputs L & R
  mixer1.gain ( 2, 0.0 );   // sine1
  mixer1.gain ( 3, 0.0 );   // drum1
  
  reverb1.reverbTime ( 0.5 );
  delay1.delay ( 0, 0 );
  delay1.delay ( 1, 1000 );
  
  mixer2.gain ( 0, 1.0 );
  mixer2.gain ( 1, 0.0 );
  mixer2.gain ( 2, 0.0 );
  mixer2.gain ( 3, 0.0 );
  
  // Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 500 );

}

// void ISR_pb1 () {
//   const int debounceInterval_ms = 100;
//   static unsigned long lastInterruptAt_ms = 0;
//   unsigned long now_ms = millis();
//   // If interrupts come too fast, assume it's a bounce and ignore
//   if ( now_ms - lastInterruptAt_ms > debounceInterval_ms && ! digitalRead ( pd_pb1 ) ) {
//     state = ( state + 1 ) % nStates;
//   }
//   displayIsStale = true;
//   lastInterruptAt_ms = now_ms;
// }

// void ISR_pb2 () {
//   const int debounceInterval_ms = 100;
//   static unsigned long lastInterruptAt_ms = 0;
//   unsigned long now_ms = millis();
//   // If interrupts come too fast, assume it's a bounce and ignore
//   if ( now_ms - lastInterruptAt_ms > debounceInterval_ms  && ! digitalRead ( pd_pb2 ) ) {
//     boost = !boost;
//   }
//   displayIsStale = true;
//   lastInterruptAt_ms = now_ms;
// }

  
void loop() {
  
  const int maxState = 5;
  
  #if VERBOSE >= 10
    profile [ 0 ].start();
  #endif
  
  // ----------------------
  // read switches and pots
  // ----------------------
  
  const int potHysteresis = 20;
  for ( int i = 0; i < 4; i++ ) {
    int oldReading = potReadings [ i ];
    potReadings [ i ] = 1024 - analogRead ( pa_pots [ i ] );
    if ( abs ( potReadings [ i ] - oldReading ) > potHysteresis ) {
      displayIsStale = true;
      // if ( VERBOSE >= 5 ) {
      //   Serial.print ( "pot" ); 
      //   Serial.print ( i );
      //   Serial.print ( ": " );
      //   Serial.print ( oldReading );
      //   Serial.print ( " -> " );
      //   Serial.println ( potReadings [ i ] );
      // }
    }
  }
  
  // NASTY read the state of the bat switches and update their status
  
  for ( int i = 0; i < 2; i++ ) {
    int val = analogRead ( pa_batSwitches [ i ] );
    if ( i == 1 ) val = 1023 - val;
    if ( val > ( 1024 / 3 ) && val < ( 1024 * 2 / 3 ) ) {
      // centered; no change
      batSwitchLastCenteredAt_ms [ i ] = millis();
      batSwitchLastRepeatAt_ms [ i ] = 0UL;
    } else {
      // have detected actuation
      // notice only if it's been here a while
      if ( ( millis() - batSwitchLastCenteredAt_ms [ i ] ) > batSwitchNoticePeriod_ms ) {
        // it's firmly actuated; hit it and then ignore until it's time to repeat
        if ( ( millis() - batSwitchLastRepeatAt_ms [ i ] ) > batSwitchRepeatPeriod_ms ) {
          // do something
          batSwitchStatus [ i ] += ( val > ( 1024 / 2 ) ) ? 1 : -1;
          batSwitchLastRepeatAt_ms [ i ] = millis();
        }
      }
    }
  }
      
  pb1.update();
  pb2.update();
  
  // expression pedals cv1 and cv2
  
  cv1_counts = analogRead ( pa_CV1 );
  cv2_counts = analogRead ( pa_CV2 );

  // -------------------------------------------------------
  // update variables and displays in accordance with inputs
  // -------------------------------------------------------
  
  if ( pb1.fell() ) {
    state = ( state + 1 ) % nStates;
    // if ( VERBOSE >= 5 ) {
    //   Serial.print ( "State: " );
    //   Serial.println ( state );
    // }
    displayIsStale = true;
  } else if ( pb1.rose() ) {
    displayIsStale = true;
  }
  int colors [] = { 0x000000, 0x400000, 0x004000, 0x000040 };
  strip.setPixelColor ( ledState, colors [ state ] ); 
  
  if ( pb2.fell() ) {
    boost = !boost;
    // if ( VERBOSE >= 5 ) {
    //   Serial.print ( "Boost: " );
    //   Serial.println ( boost );
    // }
    displayIsStale = true;
  } else if ( pb2.rose() ) {
    displayIsStale = true;
  }

  if ( boost ) {
    digitalWrite ( pd_relayR, 1 );
    digitalWrite ( pd_relayL, 1 );
    strip.setPixelColor ( ledBoost, strip.Color ( 16, 96, 16 ) );
  } else {
    digitalWrite ( pd_relayR, 0 );
    digitalWrite ( pd_relayL, 0 );
    strip.setPixelColor ( ledBoost, strip.Color ( 96, 16, 16 ) );
  }
  
  #if VERBOSE >= 10
    profile [ 0 ].stop();
  #endif
  
  #if VERBOSE >= 10
    profile [ 2 ].start();
  #endif

  sine1.amplitude ( potReadings [ 2 ] / 1024.0 );
  sine1.frequency ( potReadings [ 3 ] );

  static unsigned long lastPixelAt_ms = 0UL;
  brightness = potReadings [ 1 ] / 4;
  float amp;
  // amp = peak1.read();  // returns 0.0 to 1.0
  amp = rms1.read();   // returns 0.0 to 1.0

  static unsigned long lastDrumat_ms   = 0UL;
  const  unsigned long drumInterval_ms = 1000UL;
  
  if ( ( millis() - lastDrumat_ms ) > drumInterval_ms ) {
    drum1.noteOn ();
    lastDrumat_ms = millis();
  }

  if ( batSwitchStatus [ 0 ] != state ) {
    displayIsStale = true;
    state = constrain ( batSwitchStatus [ 0 ], 0, maxState );
  }
  
  switch ( state ) {
    case 0:  // inputs
      mixer1.gain ( 0, 2.0 );
      mixer1.gain ( 1, 2.0 );
      mixer1.gain ( 2, 0.0 );
      mixer1.gain ( 3, 0.0 );
      // if ( VERBOSE >= 5 ) Serial.println ( "mixer1.gain 0/1 2.0: inputs" );
      break;
    case 1:  // sine
      mixer1.gain ( 0, 0.0 );
      mixer1.gain ( 1, 0.0 );
      mixer1.gain ( 2, 1.0 );
      mixer1.gain ( 3, 0.0 );
      // if ( VERBOSE >= 5 ) Serial.println ( "mixer1.gain 2 1.0: sine" );
      break;
    case 2:  // drum
      mixer1.gain ( 0, 0.0 );
      mixer1.gain ( 1, 0.0 );
      mixer1.gain ( 2, 0.0 );
      mixer1.gain ( 3, 1.0 );
      // if ( VERBOSE >= 5 ) Serial.println ( "mixer1.gain 3 1.0: drum" );
      break;
    default:
      break;
  }

  switch ( batSwitchStatus [ 1 ] ) {
    case 0:  // straight
      mixer2.gain ( 0, 1.0 );
      mixer2.gain ( 1, 0.0 );
      mixer2.gain ( 2, 0.0 );
      mixer2.gain ( 3, 0.0 );
      // if ( VERBOSE >= 5 ) Serial.println ( "mixer2.gain 0 1.0: straight" );
      break;
    case 1:  // reverb
      mixer2.gain ( 0, 0.0 );
      mixer2.gain ( 1, 1.0 );
      mixer2.gain ( 2, 0.0 );
      mixer2.gain ( 3, 0.0 );
      // if ( VERBOSE >= 5 ) Serial.println ( "mixer2.gain 1 1.0: reverb" );
      break;
    case 2:  // delay
      mixer2.gain ( 0, 0.0 );
      mixer2.gain ( 1, 0.0 );
      mixer2.gain ( 2, 1.0 );
      mixer2.gain ( 3, 0.5 );
      // if ( VERBOSE >= 5 ) Serial.println ( "mixer2.gain 2/3 1.0/0.5: delay" );
      break;
  }

  switch ( state ) {
  
    case 0:
      stripWheelDelay_ms = ( 1023 - potReadings [ 0 ] ) / 10;
  
      if ( ( millis() - lastPixelAt_ms ) > stripWheelDelay_ms ) {
        pixelTest_step ( );
        lastPixelAt_ms = millis();
      }
      updateOLEDdisplay = &displayOLED_0;

      break;
  
    case 1:

      if ( ( millis() - lastPixelAt_ms ) > stripWheelDelay_ms ) {
//        pixelTest_step ( 3 );
        lastPixelAt_ms = millis();
      }
      
      
      
      for ( uint16_t i = 3; i < nPixels; i++ ) {
        strip.setPixelColor ( i, strip.Color ( 0, 0, 0 ) );
      }

      updateOLEDdisplay = &displayOLED_1;
  
      break;
  
    case 2:
      // test VU meter
      { 
        static float amp = 0.0;
        static int repeat = 0;
        const int nRepeats = 4;
        static unsigned long lastVUat_ms = 0UL;
        const unsigned long VUpause_ms = 500UL;
        
        if ( ( millis() - lastVUat_ms ) > VUpause_ms ) {
          /*
            retransmit nRepeats times per change in display
            because we think that the transmissions are
            noisy
          */
          
          if ( repeat == 0 ) {
            // update the display
          
          
            // Serial.print ( "Test VU: " ); Serial.println ( amp );
            setVU ( round ( amp * 7.0 ) );
            // Serial.print ( "barGraph: " ); Serial.println ( barGraph );
            
            amp += 1.0 / 7.0;
            if ( amp > 1.0 ) amp = 0.0;
          }
          repeat++;
          if ( repeat >= nRepeats ) repeat = 0;
          
          strip.setBrightness ( brightness );
          strip.show ();
          lastVUat_ms = millis();
        }
      }
      
      break;
  
    case 3:
      // VU meter
      // Serial.print ( "Amplitude: " ); Serial.println ( amp );
      setVU ( round ( amp * 7.0 ) );
      break;
  
    case 4:
      // VU meter for expression pedal 1
      // Serial.print ( "Expression pedal 1: " ); Serial.println ( cv1_counts );
      setVU ( round ( cv1_counts / 1024.0 * 7.0 ) );
      if ( Serial ) {
        Serial.print ( "EP1: " ); 
        Serial.println ( cv1_counts );
      }
      break;
  
    case 5:
      // VU meter for effects pedal 2
      // Serial.print ( "Expression pedal 2: " ); Serial.println ( cv2_counts );
      setVU ( round ( cv2_counts / 1024.0 * 7.0 ) );
      if ( Serial ) {
        Serial.print ( "EP2: " ); 
        Serial.println ( cv2_counts );
      }
      break;
  
    default:
      break;
      
  }
  
  #if VERBOSE >= 10
    profile [ 2 ].stop();
  
    static unsigned int lastProfilePrintAt_ms = 5000UL;
    if ( 0 && ( millis() - lastProfilePrintAt_ms ) > 2000UL ) {
      for ( int i = 0; i <= 4; i++ ) {
        profile [ i ].report();
      }
      lastProfilePrintAt_ms = millis();
    }
  #endif

  strip.setBrightness ( brightness );
  strip.show ();
  updateOLEDdisplay ();

}

// ========================================================================

// WS2812 3-color LEDs routines

uint32_t Wheel ( byte WheelPos ) {
  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  WheelPos = 255 - WheelPos;
  if ( WheelPos < 85 ) {
   return strip.Color ( 255 - WheelPos * 3, 0, WheelPos * 3 );
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color ( 0, WheelPos * 3, 255 - WheelPos * 3 );
  } else {
   WheelPos -= 170;
   return strip.Color ( WheelPos * 3, 255 - WheelPos * 3, 0 );
  }
}

void pixelTest_step ( const uint16_t startPixel, const uint16_t n ) {
  static int iterator = 0;
  static int pixelTestState = 0;
  const int fudge = 5;
  
  #if VERBOSE >= 10
    profile [ 3 ].start();
  #endif

  int it;
  switch ( pixelTestState ) {
    case 0:
      for ( uint16_t i = startPixel; i < ( startPixel + n ); i++ ) {
        strip.setPixelColor ( i, strip.Color ( 0, 0, 0 ) );
      }
      pixelTestState = 1;
      iterator = startPixel;
      break;
    case 1:
      it = iterator / fudge;
      strip.setPixelColor ( it, 0xffffff );
      iterator++;
      if ( it >= startPixel + n ) {
        pixelTestState = 2;
        iterator = startPixel;
      }
      break;
    case 2:
      for ( int i = startPixel; i < ( startPixel + n ); i++ ) {
        strip.setPixelColor ( i, Wheel ( ( i * 1 * 255 / strip.numPixels() + iterator ) & 255 ) );
      }
      iterator++;
      if ( iterator > strip.numPixels() * 20 ) {
        pixelTestState = 0;
        iterator = startPixel;
      }
      break;
  }
  
  #if VERBOSE >= 10
    profile [ 3 ].stop();
  #endif

}

void setVU ( int n ) {
  const unsigned long offColor = 0x040000UL;
  const unsigned long onColor  = 0x080808UL;
  const int VUfirstPixel = 3;
  const int nVUpixels = 7;
  
  // if ( n < 0 || n > nVUpixels ) {
  //   Serial.println ( "setVU: invalid bargraph value ( 0 - 7 ): " );
  //   Serial.println ( n );
  // }
  for ( int i = 0; i < nVUpixels; i++ ) {
    strip.setPixelColor ( VUfirstPixel + i, ( i < n ) ? onColor : offColor );
  }
  // strip.show ();
}
  
// ========================================================================

// OLED display routines 

void init_oled_display () {
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  // initialize with the I2C addr 0x3D (for the 128x64)
  // 0x3c on Open Effects box
  
  const byte addI2C = 0x3c;
  oled.begin(SSD1306_SWITCHCAPVCC, addI2C);  
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  oled.display ();
  delay ( 500 );

  // Clear the buffer.
  oled.clearDisplay ();
  oled.display ();

}

void displayOLED_0 () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    // if ( VERBOSE >= 9 ) Serial.println ( "oled update skipped" );
    return;
  }

  #if VERBOSE >= 10
    profile [ 1 ].start();
  #endif

  displayUpdateRate_ms = 200UL;
  
  oled.clearDisplay();
  
  // upper left 4 small numbers are the pot readings
  
  oled.setTextSize ( 1 );
  oled.setTextColor ( WHITE );
  for ( int i = 0; i < 4; i++ ) {
    oled.setCursor ( ( i / 2 ) * 40, ( i % 2 ) * 15 );
    oled.println ( potReadings [ i ] );
  }
  
  // large numbers are the state and the boost values

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 30 );
  oled.print ( state );
  oled.setCursor ( 40, 30 );
  oled.print ( boost );
  
  // bottom small letters are the bat switch positions

  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 45 );
  oled.print ( batSwitchStatus [ 0 ] );
  // oled.print ( batSwitchReadings [ 0 ] );
  oled.setCursor ( 40, 45 );
  oled.print ( batSwitchStatus [ 1 ] );
  // oled.print ( batSwitchReadings [ 1 ] );
  
  // very bottom numbers are expression pedal values
  
  oled.setTextSize ( 1 );
  oled.setTextColor ( WHITE );
  oled.setCursor ( 10, 54 );
  oled.print ( cv1_counts );
  oled.print ( " | " );
  oled.print ( cv2_counts );
  
  // blocks to indicate state of the push buttons
  
  // oled.setTextColor ( BLACK, WHITE ); // 'inverted' text
  if ( pb1.read() ) {
    oled.fillRect ( 80, 0, 20, 20, 0);
  } else {
    oled.fillRect ( 80, 0, 20, 20, 1);
  }
  if ( pb2.read() ) {
    oled.fillRect ( 100, 0, 20, 20, 0);
  } else {
    oled.fillRect ( 100, 0, 20, 20, 1);
  }
  
  // small numbers on right: strip wheel delay and loop time

  oled.setTextSize ( 1 );
  oled.setCursor ( 80, 30 );
  oled.print ( stripWheelDelay_ms );
  unsigned long loopTime_ms = millis() - lastOledUpdateAt_ms;
  lastOledUpdateAt_ms = millis();
  oled.setCursor ( 80, 45 );
  oled.print ( loopTime_ms );

  // miniature bitmap display
  // oled.drawBitmap(30, 16,  logo16_glcd_bmp, 16, 16, 1);



  #if VERBOSE >= 10
    profile [ 1 ].stop();
    profile [ 4 ].start();
  #endif
  
  oled.display();  // note takes about 100ms!!!
  
  
  #if VERBOSE >= 10
    profile [ 4 ].stop();
  #endif
  
  
  
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
  
  // if ( VERBOSE >= 9 ) Serial.println ( "oled update done" );

}

void displayOLED_1 () {

  if ( ! ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > displayUpdateRate_ms ) ) {
    // if ( VERBOSE >= 9 ) Serial.println ( "oled update skipped" );
    return;
  }

  #if VERBOSE >= 10
    profile [ 1 ].start();
  #endif
  
  // update display, but only once
  
  displayUpdateRate_ms = 0xFFFFFFFF;
  
  oled.clearDisplay();
  
  // upper left 4 small numbers are the pot readings
  
  oled.setTextSize ( 1 );
  oled.setTextColor ( WHITE );
  for ( int i = 0; i < 4; i++ ) {
    oled.setCursor ( ( i / 2 ) * 40, ( i % 2 ) * 15 );
    oled.println ( potReadings [ i ] );
  }
  
  // next lower, large numbers are the state and the boost values

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 30 );
  oled.print ( state );
  oled.setCursor ( 40, 30 );
  oled.print ( boost );
  
  // bottom small letters are the bat switch positions

  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 45 );
  oled.print ( batSwitchStatus [ 0 ] );
  // oled.print ( batSwitchReadings [ 0 ] );
  oled.setCursor ( 40, 45 );
  oled.print ( batSwitchStatus [ 1 ] );
  // oled.print ( batSwitchReadings [ 1 ] );

  // blocks to indicate state of the push buttons
  
  // oled.setTextColor ( BLACK, WHITE ); // 'inverted' text
  if ( pb1.read() ) {
    oled.fillRect ( 80, 0, 20, 20, 0);
  } else {
    oled.fillRect ( 80, 0, 20, 20, 1);
  }
  if ( pb2.read() ) {
    oled.fillRect ( 100, 0, 20, 20, 0);
  } else {
    oled.fillRect ( 100, 0, 20, 20, 1);
  }
  
  // small numbers on right: strip wheel delay and loop time

  oled.setTextSize ( 1 );
  oled.setCursor ( 80, 30 );
  oled.print ( stripWheelDelay_ms );
  unsigned long loopTime_ms = millis() - lastOledUpdateAt_ms;
  lastOledUpdateAt_ms = millis();
  oled.setCursor ( 80, 45 );
  oled.print ( loopTime_ms );



  #if VERBOSE >= 10
    profile [ 1 ].stop();
    profile [ 4 ].start();
  #endif
  
  oled.display();  // note takes about 100ms!!!
  
  #if VERBOSE >= 10
    profile [ 4 ].stop();
  #endif
  
  
  
  displayIsStale = false;
  lastOledUpdateAt_ms = millis();
    
  // if ( VERBOSE >= 9 ) Serial.println ( "oled update done" );

}

