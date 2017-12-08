// Direct Digital Synthesis for Arduino / Teensy
// Charles B. Malloch, PhD
// 2016-02-04

enum { BAUDRATE = 115200 };
enum { pdLED2   = 10 };
enum { pdButton = 12 };
enum { pdLED    = 13 };
enum { paPIEZO  = A14 };  // max is 4096

// initialize waveTable to square wave
int waveTable [ ] = {    0,    0,    0,    0,    0,    0,    0,    0, 
                         0,    0,    0,    0,    0,    0,    0,    0, 
                      4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
                      4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095  };
int waveTableLen;

const unsigned long sampleRate_us = 1e6 / ( 440 * 32 );
// const unsigned long sampleRate_us = 1e6 * 8 / ( 32 );

enum { bufLen = 80 };
int bufPtr = 0;
char strBuf [bufLen];

void setup () {

  Serial.begin( BAUDRATE );
  
  pinMode ( pdLED2, OUTPUT );
  pinMode ( pdButton, INPUT );
  pinMode ( pdLED, OUTPUT );
    
  for ( int i = 0; i < 5; i++ ) {
    digitalWrite ( pdLED, 1 );
    digitalWrite ( pdLED2, 1 );
    delay ( 200 );
    digitalWrite ( pdLED, 0 );
    digitalWrite ( pdLED2, 0 );
    delay ( 200 );
  }
  
  while ( ( millis() < 5000 ) && !Serial ) {
    delay ( 500 );
  }
  
  analogWriteResolution(12);
  waveTableLen = sizeof ( waveTable ) / sizeof ( int );
  initializeWaveTable ( waveTable, waveTableLen );
  
  Serial.println ( "direct_digital_synthesis v0.01 2016-02-04" );
  
  for ( int i = 0; i < waveTableLen; i++ ) {
    Serial.print ( i ); Serial.print ( ": " ); Serial.println ( waveTable [ i ] );
  }
}

void loop () {
  const unsigned long blinkInterval_ms = 2000;
  static unsigned long lastBlinkAt_ms = 0;
  static int waveTableIndex = 0;


  if ( ( millis() - lastBlinkAt_ms ) > blinkInterval_ms ) {
    LEDToggle ();
    lastBlinkAt_ms = millis();
  }
  
  if ( digitalRead ( pdButton ) ) {
    digitalWrite ( pdLED2, 1 );
    analogWrite ( paPIEZO, waveTable [ waveTableIndex++ ] );
    waveTableIndex = ( waveTableIndex >= waveTableLen ) ? 0 : ( waveTableIndex + 1 );
    delayMicroseconds ( sampleRate_us );
  } else {
    digitalWrite ( pdLED2, 0 );
  }
  
}

void initializeWaveTable ( int waveTable[], int waveTableLen ) {
  float phase = 0;
  float deltaPhase = 2.0 * 3.14159265358979 / float ( waveTableLen );
  for ( int i = 0; i < waveTableLen; i++ ) {
    waveTable [ i ] = 2048 + int ( 2047.0 * cos ( phase ) );
    phase += deltaPhase;
  }
}


void LEDToggle() {
  digitalWrite( pdLED, 1 - digitalRead ( pdLED ) );
}
