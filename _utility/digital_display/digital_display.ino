#define VERSION "1.1"
#define VERDATE "2013-05-18"
#define PROGMONIKER "UDD"

/*
  Functions:
    1) display time and hot-tub temperature
    2) serve as MODBUS/RTU slave to receive time from, provide the hot tub temperature to a master downstairs
    3) hot tub cover sensor? time-in-tub?
  
  Notes:
    
    SPI ( for communications with numeric display )
    
    Wiring of display: MAXIM MAX7221 (SPI)
      1 DIN - green Arduino 11
      2 DIGIT 0 - display 8
      3 DIGIT 4 - n/c
      4 GND
      5 DIGIT 6 - n/c
      6 DIGIT 2 - display 2
      7 DIGIT 3 - display 1
      8 DIGIT 7 - white display 4 ( colon, etc )
      9 GND
      10 DIGIT 5- n/c
      11 DIGIT 1 - display 6
      12 LOAD/~CS - orange Arduino 10 
      
      13 CLK - brown Arduino 13
      14 SEG A - display 14
      15 SEG F - display 11
      16 SEG B - display 16
      17 SEG G - display 15
      18 ISET - n/c
      19 V+
      20 SEG C - display 13
      21 SEG E - display 5
      22 SEG DP - blue display 7
      23 SEG D - display 3
      24 DOUT - n/c (purple)
    
      
   Arduino connections (digital pins):
      0 RX - reserved for serial comm - left unconnected
      1 TX - reserved for serial comm - left unconnected
      9 status LED
     10 slave select - orange - chip select for MAX7221 SPI LED display driver chip
     11 SDA for SPI - green
     13 SCK for SPI - brown

*/

#define VERBOSE 1

#define BAUDRATE 115200
#define DISPLAY_DURATION_ms 2000

// pin definitions

#define pdSTATUSLED       9
#define pdSS             10
// MISO (pin 12) not used for this display driver.
// MOSI
#define pdSDA            11
#define pdSCK            13

#define paLIGHT            0
#define paPOT              1

#include <SPI.h>

#include <FormatFloat.h>

byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                      // { 0,  pdSS             , -1 },    // handled by SPI
                                      // { 0,  pdSDA            , -1 },    // handled by SPI
                                      // { 0,  pdSCK            , -1 },    // handled by SPI
                                      { 0,  pdSTATUSLED      , -1 }
                                     };


#define LINELEN 80
#define BUFLEN (LINELEN + 3)
char strBuf[BUFLEN];
#define FBUFLEN 8
char fbuf[FBUFLEN];

void displayNumber ( int num, int decimalOnDigit = 0 );

void setup() {

  Serial.begin (BAUDRATE);
  
  // initialize SPI and numeric display
  
  SPI.begin();
  
  SPI.setBitOrder ( MSBFIRST );
  SPI.setDataMode ( SPI_MODE0 );
  SPI.setClockDivider ( SPI_CLOCK_DIV4 );
  
  displayWrite ( 0x0c, 0x01 );  // shutdown mode -> normal operation
  
  displayWrite ( 0x0f, 0x01 );  // display test
  delay ( 1000 );
  displayWrite ( 0x0f, 0x00 );  // turn off display test
  
  displayWrite ( 0x09, 0x1f );  // decode(code B) first 4 characters
  displayWrite ( 0x0a, 0x08 );  // display intensity
  displayWrite ( 0x0b, 0x07 );  // display all characters (digit 7 will be colon etc)
  
  // initialize digital IO
  
  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( int i = 0; i < nPinDefs; i++ ) {
    if ( pinDefs [i] [0] == 1 ) {
      // input pin
      pinMode ( pinDefs [i] [1], INPUT );
      digitalWrite ( pinDefs [i] [1], 1 );  // enable internal 50K pullup
    } else {
      // output pin
      pinMode ( pinDefs [i] [1], OUTPUT );
      digitalWrite ( pinDefs [i] [1], 0 );
    }
  }
  
  
  // report


  Serial.print ( PROGMONIKER );
  Serial.print ( ": Utility - Digital Display v");
  Serial.print ( VERSION );
  Serial.print ( " (" );
  Serial.print ( VERDATE );
  Serial.print ( ")\n" );
  
}
 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
  

  
void loop () {

  #define nDisplays 3
  
  static unsigned long lastDisplayChangedAt_ms = 0;
  static unsigned long lastBlinkToggleAt_ms = 0;
  static int displayMode = nDisplays;  // 0  temperature; 1 analog 0; 2 analog 1; 3? millis
  
  int aLIGHT = analogRead ( paLIGHT );
  static float light = 0;
  light = 0.02 * aLIGHT + 0.98 * light;
  int aPOT = analogRead ( paPOT );
  byte intensity = ( aPOT >> 6 ) & 0x0f;
  
  displayWrite ( 0x0a, intensity );  // display intensity can be 0x00 to 0x0f

  
  // display the time and the millis
  
  if ( ( millis () - lastDisplayChangedAt_ms ) > DISPLAY_DURATION_ms ) {
    Serial.print ("mode "); Serial.println ( displayMode );
    displayMode = ( displayMode >= ( nDisplays - 1 ) ) ? 0 : displayMode + 1;
    lastDisplayChangedAt_ms = millis ();
  }
  
  if ( displayMode == 0 ) {
    // display time
    int mins = millis() / 60000L;
    int hours = mins / 60;
    mins = mins - 60 * hours;
    hours %= 24;
    displayTime ( hours * 100 + mins );
  } else if ( displayMode == 1 ) {
    displayNumber ( int ( light ) );
  } else if ( displayMode == 2 ) {
    displayNumber ( intensity );
  } else {
    // display millis
    displayNumber ( int ( millis() / 10L % 10000L ) );
  }
  
  delay ( 10 );
  
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 


// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>     subroutines for numeric display device       <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void displayTime ( int timehhmm ) {
  displayNumber ( timehhmm );
  displayWrite ( 7 + 1, 0x60 );  // display colon (digit 7, segs A and B)
}

void displayTemperature ( float temperature ) {
  // assume temperature < 1000 degF
  int t = int ( temperature * 10 + 0.5 );
  displayNumber ( t, 0x01 << 1 );
}

void displayNumber ( int num, int decimalOnDigit ) {
  // decimalOnDigit is mask where a 1 bit puts a decimal on the corresponding digit
  // note that LSB (the 1 bit) corresponds to digit 0 (the rightmost)
  // Serial.println ( decimalOnDigit );
  displayWrite ( 7 + 1, 0x00 );  // remove colon
  for ( int d = 0; d < 4; d++ ) {
    // digit 0 is tenths
    int n1;
    byte r;
    n1 = num / 10;
    r = num - ( n1 * 10 );
    /*
      Serial.print ( d ); 
        if ( decimalOnDigit & ( 0x00000001 << d ) ) Serial.print ( "*" );
        Serial.print ( ": " );
      Serial.print ( num ); Serial.print ( "  " ); 
      Serial.print ( n1 ); Serial.print ( " -> " ); 
      Serial.println ( r );
    */
    if ( decimalOnDigit & ( 0x00000001 << d ) ) r |= 0x80;
    displayWrite ( d + 1, r );  // register for digit 0 is 1
    num = n1;
  }
}

void displayWrite ( byte address, byte value) {
  /*
      16-bits sent with each sending
      D15 is MSB, D0 LSB
      D0-D7 data; D7 is the decimal point
      D8-D11 register address
      D12-D15 ignored
      
  */
   
  // take the SS pin low to select the chip:
  digitalWrite(pdSS,LOW);
  //  send in the address and value via SPI:
  SPI.transfer ( address & 0x0f );
  SPI.transfer ( value & 0xff );
  // take the SS pin high to de-select the chip:
  digitalWrite(pdSS,HIGH); 
}

// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <>               general-purpose subroutines               <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 
// <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> 

void sendMessage (char msg[]) {
  char smBuf[129];
  snprintf (smBuf, 128, "[%s %7lu: %s]\n", PROGMONIKER, millis(), msg);
  Serial.print (smBuf);
}
