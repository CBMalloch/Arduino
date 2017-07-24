#define PROGNAME  "testOpenEffectsPedal"
#define VERSION   "0.2.6"
#define VERDATE   "2017-07-24"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// get Paul Stoffregen's NeoPixel library if using Teensy 3.5 or 3.6
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

#define VERBOSE 5
#if VERBOSE >= 10
  // Profiler is my own code-profiling library
  // using it I found that the OLED screen takes 100ms to update
  #include <Profiler.h>
#endif

// currently set up to test the OpenEffects box

//Pinout board rev1
// pa = pin, analog; pd = pin, digital
const int pa_pots [] = { A6, A3, A2, A1 };
// tap pins differ between red board and blue board; I have blue
// pb are the stomp switches "pushbutton"
const int pd_pb1 = 2;
const int pd_pb2 = 3;
const int pd_LED = 13;

// int CV1 = A10;
// int CV2 = A11;
// 

const int pa_batSwitches [] = { A12, A13 };
const char batSwitchStatusIndicator [] = { 'L', 'C', 'R' };


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

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// OLED screen
#define OLED_RESET 4
Adafruit_SSD1306 oled ( OLED_RESET );

volatile bool displayIsStale;
void ( *updateOLEDdisplay) ();

// void cbmTest ( uint8_t wait, uint8_t brightness = 255 );
// void rainbow ( uint8_t wait, uint8_t brightness = 255 );
uint32_t Wheel ( byte WheelPos, uint8_t brightness = 255 );

#if VERBOSE >= 10
  Profile profile [ 10 ];
#endif

Bounce pb1 = Bounce();
Bounce pb2 = Bounce();

static unsigned int lastOledUpdateAt_ms = 0UL;
unsigned long delay_ms;
int potReadings [ 4 ];
int batSwitchReadings [ 2 ];

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=409.75,439.75
AudioSynthWaveformSine   sine1;          //xy=410.75,386.75
AudioMixer4              mixer1;         //xy=571.75,445.75
AudioMixer4              mixer2;         //xy=692.75,311.75
AudioEffectDelay         delay1;         //xy=719.75,552.75
AudioEffectReverb        reverb1;        //xy=724.75,445.75
AudioOutputI2S           i2s1;           //xy=1096.75,391.75
AudioConnection          patchCord1(i2s2, 0, mixer1, 0);
AudioConnection          patchCord2(i2s2, 1, mixer1, 1);
AudioConnection          patchCord3(sine1, 0, mixer2, 0);
AudioConnection          patchCord4(mixer1, reverb1);
AudioConnection          patchCord5(mixer1, delay1);
AudioConnection          patchCord6(mixer2, 0, i2s1, 0);
AudioConnection          patchCord7(mixer2, 0, i2s1, 1);
AudioConnection          patchCord8(mixer2, 0, mixer1, 2);
AudioConnection          patchCord9(delay1, 0, mixer2, 2);
AudioConnection          patchCord10(reverb1, 0, mixer2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=273.75,136.75
// GUItool: end automatically generated code


void setup() {

  AudioMemory ( 250 );
  // AudioMemory_F32(40); //allocate Float32 audio data blocks

  pinMode ( pd_pb1, INPUT );
  pinMode ( pd_pb2, INPUT );
  pinMode ( pd_LED, OUTPUT );
  
  // using Bounce2 library instead of my own, less-functional, debounce routines
  // attachInterrupt(digitalPinToInterrupt(pin), ISR, mode);
  // attachInterrupt ( digitalPinToInterrupt ( pd_pb1 ), ISR_pb1, CHANGE);
  // attachInterrupt ( digitalPinToInterrupt ( pd_pb2 ), ISR_pb2, CHANGE);
  
  pb1.attach(pd_pb1);
  pb1.interval(5); // interval in ms
  pb2.attach(pd_pb2);
  pb2.interval(5); // interval in ms
  
  for ( int i = 0; i < 2; i++ ) {
    digitalWrite ( pd_LED, 1 );
    delay ( 200 );
    digitalWrite ( pd_LED, 0 );
    delay ( 200 );
  }
    

  Serial.begin ( 115200 );
  while ( !Serial ) { delay ( 100 ); }
  
  for ( int i = 0; i < 4; i++ ) {
    digitalWrite ( pd_LED, 1 );
    delay ( 200 );
    digitalWrite ( pd_LED, 0 );
    delay ( 200 );
  }
    
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  init_oled_display ();
  
  #if VERBOSE >= 10
    profile [ 0 ].setup ( "reads", 10, 10 );
    profile [ 1 ].setup ( "oled", 10, 10 );
    profile [ 2 ].setup ( "lightsOuter", 10, 10 );
    profile [ 3 ].setup ( "lightsInner", 10, 10 );
    profile [ 4 ].setup ( "oled_disp", 10, 20 );
  #endif
  
  displayIsStale = true;
  
  sgtl5000_1.enable();  // Enable the audio shield
  sgtl5000_1.inputSelect ( AUDIO_INPUT_LINEIN );
  sgtl5000_1.volume ( 0.8 );
  sgtl5000_1.lineInLevel ( 13 );   // 3.12Vp-p
  sgtl5000_1.lineOutLevel ( 21 ); // 3.16Vp-p
  
  sine1.amplitude ( 0.5 );
  sine1.frequency ( 440 );
  
  mixer1.gain ( 0, 0.8 );
  mixer1.gain ( 1, 0.8 );
  mixer1.gain ( 2, 0.8 );
  reverb1.reverbTime ( 0.25 );
  delay1.delay ( 0, 250 );
  
  Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  delay ( 500 );

}

volatile int state = 0;
volatile bool boost = false;

const int nStates = 4;

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
  
  // volatile int state = 0;
  // volatile bool boost = false;

  static int state = 0;
  static bool boost = false;

  
  pb1.update();
  if ( pb1.fell() ) {
    state = ( state + 1 ) % nStates;
    displayIsStale = true;
  } else if ( pb1.rose() ) {
    displayIsStale = true;
  }
  
  pb2.update();
  if ( pb2.fell() ) {
    boost = !boost;
    displayIsStale = true;
  } else if ( pb2.rose() ) {
    displayIsStale = true;
  }
  
  
  #if VERBOSE >= 10
    profile [ 0 ].start();
  #endif
  
  for ( int i = 0; i < 4; i++ ) {
    potReadings [ i ] = 1024 - analogRead ( pa_pots [ i ] );
  }
  int brightness = potReadings [ 1 ] / 4;
  delay_ms = ( 1023 - potReadings [ 0 ] ) / 10;
  
  sine1.amplitude ( potReadings [ 2 ] / 1024.0 );
  sine1.frequency ( potReadings [ 3 ] );

  for ( int i = 0; i < 2; i++ ) {
    int val = analogRead ( pa_batSwitches [ i ] );
    if ( val < 340 ) batSwitchReadings [ i ] = i ? 2 : 0;
    else if ( val < 680 ) batSwitchReadings [ i ] = 1;
    else batSwitchReadings [ i ] = i ? 0 : 2;
     if ( VERBOSE >= 5 ) {
      Serial.print ( "bat switch" ); 
      Serial.print ( i );
      Serial.print ( ": " );
      Serial.print ( val );
      Serial.print ( " = " );
      Serial.println ( batSwitchStatusIndicator [ batSwitchReadings [ i ] ] );
    }
   // batSwitchReadings [ i ] = val;
  }
  
  if ( boost && batSwitchStatusIndicator [ batSwitchReadings [ 0 ] ] == 'L' ) {
    if ( VERBOSE >= 5 ) Serial.println ( "L" );
    mixer2.gain ( 0, 0.0 );
    mixer2.gain ( 1, 1.0 );
    mixer2.gain ( 2, 0.0 );
    if ( VERBOSE >= 5 ) Serial.println ( "mixer2.gain 1 1.0" );
  } else if ( boost && batSwitchStatusIndicator [ batSwitchReadings [ 0 ] ] == 'R' ) {
    if ( VERBOSE >= 5 ) Serial.println ( "R" );
    mixer2.gain ( 0, 0.0 );
    mixer2.gain ( 1, 0.0 );
    mixer2.gain ( 2, 1.0 );
    if ( VERBOSE >= 5 ) Serial.println ( "mixer2.gain 2 1.0" );
  } else {  // 'C'
    if ( VERBOSE >= 5 ) Serial.println ( "C" );
    mixer2.gain ( 0, 1.0 );
    mixer2.gain ( 1, 0.0 );
    mixer2.gain ( 2, 0.0 );
    if ( VERBOSE >= 5 ) Serial.println ( "mixer2.gain 0 1.0" );
  }
  
  #if VERBOSE >= 10
    profile [ 0 ].stop();
  #endif
  
  #if VERBOSE >= 10
    profile [ 2 ].start();
  #endif

  
  switch ( state ) {
    case 0:
      static unsigned long lastPixelAt_ms = 0UL;
      if ( ( millis() - lastPixelAt_ms ) > delay_ms ) {
        pixelTest_step ( brightness );
        lastPixelAt_ms = millis();
      }
      updateOLEDdisplay = &displayOLED_0;
      break;
    default:
      updateOLEDdisplay = &displayOLED_1;
      break;
  }
  
  
  if ( VERBOSE >= 5 ) Serial.println ( "oled update done" );

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

  if ( displayIsStale || ( millis() - lastOledUpdateAt_ms ) > 2000UL ) {
    updateOLEDdisplay ();
  }
  

}


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void pixelTest_step ( uint8_t brightness ) {
  static int iterator = 0;
  static int state = 0;
  const int fudge = 5;
  
  #if VERBOSE >= 10
    profile [ 3 ].start();
  #endif

  if ( state == 0 ) {
    colorWipe ( strip.Color ( 0, 0, 0 ), 0 );  // clear strip
    state = 1;
    iterator = 0;
  } else if ( state == 1 ) {
    // what was cbmTest
    strip.setPixelColor ( iterator / fudge, strip.Color ( brightness, brightness, brightness ) );
    strip.show();
    iterator++;
    if ( iterator > strip.numPixels() * fudge ) {
      state = 2;
      iterator = 0;
    }
  } else if ( state == 2 ) {
    for ( int i = 0; i < strip.numPixels(); i++ ) {
      strip.setPixelColor ( i, Wheel ( ( i * 1 * 255 / strip.numPixels() + iterator ) & 255, brightness ) );
    }
    strip.show();
    iterator++;
    if ( iterator > strip.numPixels() * 20 ) {
      state = 0;
      iterator = 0;
    }
  }
  
  #if VERBOSE >= 10
    profile [ 3 ].stop();
  #endif

}
  

uint32_t Wheel ( byte WheelPos, uint8_t brightness ) {
  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  WheelPos = 255 - WheelPos;
  if ( WheelPos < 85 ) {
   return strip.Color ( ( 255 - WheelPos * 3 ) * brightness / 256, 0, ( WheelPos * 3 ) * brightness / 256 );
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color ( 0, ( WheelPos * 3 ) * brightness / 256, ( 255 - WheelPos * 3 ) * brightness / 256 );
  } else {
   WheelPos -= 170;
   return strip.Color ( ( WheelPos * 3 ) * brightness / 256, ( 255 - WheelPos * 3 ) * brightness / 256, 0 );
  }
}

// ========================================================================

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

// ========================================================================

void displayOLED_0 () {
  static unsigned long lastOledUpdateAt_ms = 0;

#if VERBOSE >= 10
  profile [ 1 ].start();
#endif
  oled.clearDisplay();
  oled.setTextSize ( 1 );
  oled.setTextColor ( WHITE );
  for ( int i = 0; i < 4; i++ ) {
    oled.setCursor ( ( i / 2 ) * 40, ( i % 2 ) * 15 );
    oled.println ( potReadings [ i ] );
  }

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 30 );
  oled.print ( state );
  oled.setCursor ( 40, 30 );
  oled.print ( boost );

  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 45 );
  oled.print ( batSwitchStatusIndicator [ batSwitchReadings [ 0 ] ] );
  // oled.print ( batSwitchReadings [ 0 ] );
  oled.setCursor ( 40, 45 );
  oled.print ( batSwitchStatusIndicator [ batSwitchReadings [ 1 ] ] );
  // oled.print ( batSwitchReadings [ 1 ] );

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

  oled.setTextSize ( 1 );
  oled.setCursor ( 80, 30 );
  oled.print ( delay_ms );
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
}

// ========================================================================

void displayOLED_1 () {
  static unsigned long lastOledUpdateAt_ms = 0;
  return;

  #if VERBOSE >= 10
    profile [ 1 ].start();
  #endif
  oled.clearDisplay();
  oled.setTextSize ( 1 );
  oled.setTextColor ( WHITE );
  for ( int i = 0; i < 4; i++ ) {
    oled.setCursor ( ( i / 2 ) * 40, ( i % 2 ) * 15 );
    oled.println ( potReadings [ i ] );
  }

  oled.setTextSize ( 2 );
  oled.setCursor ( 0, 30 );
  oled.print ( state );
  oled.setCursor ( 40, 30 );
  oled.print ( boost );

  oled.setTextSize ( 1 );
  oled.setCursor ( 0, 45 );
  oled.print ( batSwitchStatusIndicator [ batSwitchReadings [ 0 ] ] );
  // oled.print ( batSwitchReadings [ 0 ] );
  oled.setCursor ( 40, 45 );
  oled.print ( batSwitchStatusIndicator [ batSwitchReadings [ 1 ] ] );
  // oled.print ( batSwitchReadings [ 1 ] );

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

  oled.setTextSize ( 1 );
  oled.setCursor ( 80, 30 );
  oled.print ( delay_ms );
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
}

