
#define BAUDRATE 115200

#define pdLED  9
#define pdSS  10
// MOSI. MISO not used for this display driver.
#define pdSDA 11
#define pdSCK 13

#include <SPI.h>

byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                        { 0, pdLED, -1 },
                                        { 0, pdSS,  -1 },
                                        { 0, pdSDA, -1 },
                                        { 0, pdSCK, -1 }
                                      };


#define lineLen 80
#define bufLen (lineLen + 3)
char strBuf[bufLen];

void setup() {
  Serial.begin (BAUDRATE);
  
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
  
  /*
  displayWrite ( 0x01, 0x09 );
  displayWrite ( 0x02, 0x08 );
  displayWrite ( 0x03, 0x07 );
  displayWrite ( 0x04, 0x02 );
  
  for ( int i = 0; i < 8; i++ ) {
    displayWrite ( 0x05, 0x01 << i );  // display colon
    delay ( 1000 );
  }
  */
  delay ( 2 );
}

void displayNumber ( int num, int decimalOnDigit = 0 );

void loop() {
  /*
  displayWrite ( 0x09, 0x00 );  // decode(code B) no characters
  displayWrite ( 0x0b, 0x07 );  // display five characters (digit 4 will be colon etc)
  for ( int i = 0; i < 8; i++ ) {
    for ( int j = 0; j < 8; j++ ) {
      displayWrite ( j + 1, 0x01 << ( 7 - i ) );  // display colon
    }
    delay ( 1000 );
  }
  */
  displayTemperature ( 123.4 );
  delay ( 1000 );
  displayTime ( 4321 );
  delay ( 1000 );
  if ( 0 ) {
    unsigned long startedAt_ms;
    startedAt_ms = millis();
    while ( ( millis() - startedAt_ms ) < 5000 ) {
      displayNumber ( int ( millis() & 0xffff ) );
    }
  }
}

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
  Serial.println ( decimalOnDigit );
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