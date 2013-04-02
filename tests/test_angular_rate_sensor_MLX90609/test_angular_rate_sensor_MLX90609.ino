/*
 MLX90609 Angular Rate Sensor
 
 Uses the SPI library. For details on the sensor, see:
 http://www.sparkfun.com/commerce/product_info.php?products_id=8161
 
 This sketch adapted from Nathan Seidle's SCP1000 example for PIC:
 http://www.sparkfun.com/datasheets/Sensors/SCP1000-Testing.zip
 
 Circuit:
 
 CSB: pin 10
 MOSI: pin 11
 MISO: pin 12
 SCK: pin 13
 
 created 31 July 2010
 modified 14 August 2010
 by Tom Igoe
 
 modified 22 February 2013 by Charles B. Malloch, PhD to play with MLX90609
 
 Note: from C:\Program Files\arduino-1.0.1\hardware\arduino\variants\standard\pins_arduino.h
    static const uint8_t SS   = 10;
    static const uint8_t MOSI = 11;
    static const uint8_t MISO = 12;
    static const uint8_t SCK  = 13;
 
 */

#include <SPI.h>

#define BAUDRATE 115200
#define pdChipSelect SS

#define STATR 0x88
#define ADCC  0x90
#define CHAN  0x08
#define ADEN  0x04
#define ADCR  0x80

#define bufLen 80
unsigned char strBuf [ bufLen + 2 ];
unsigned char bufPtr = 0;

void setup() {
  Serial.begin(BAUDRATE);
  SPI.begin();

  pinMode ( pdChipSelect, OUTPUT );
  digitalWrite ( pdChipSelect, 1 );  // deselect the chip

  // initialize MLX90609
  bufPtr = transact  ( ADCC | ADEN, 2, strBuf );
  
  // Serial.print ( "activate ADC: reply: " );
  // Serial.println ( bufPtr );
  // for ( int i = 0; i < bufPtr; i++ ) {
    // Serial.print ( " 0x" );
    // Serial.print ( strBuf [ i ], HEX );
  // }
  // Serial.println ();
  
}

void loop() {

  int x;
  float temp_mv, rate_mv, temp, rate;
  
  // start conversion to get temperature
  bufPtr = transact  ( ADCC | CHAN | ADEN, 2, strBuf ); // channel 1 - temperature
  
  // Serial.print ( "Conversion start: reply: " );
  // Serial.println ( bufPtr );
  // for ( int i = 0; i < bufPtr; i++ ) {
    // Serial.print ( " 0x" );
    // Serial.print ( strBuf [ i ], HEX );
  // }
  // Serial.println ();

  // poll and get results (ADC reading)
  delay ( 1 );
  bufPtr = transact  ( ADCR, 2, strBuf );
  
  // Serial.print ( "Conversion result: reply: " );
  // Serial.println ( bufPtr );
  // for ( int i = 0; i < bufPtr; i++ ) {
    // Serial.print ( " 0x" );
    // Serial.print ( strBuf [ i ], HEX );
  // }
  x = ( ( ( strBuf [ 0 ] << 8 ) | strBuf [ 1 ] ) & 0x0ffe ) >> 1;
  // Serial.print ( "  --> " ); Serial.print ( x );
  temp_mv = 25.0 / 16.0 * x + 300.0;
  // Serial.print ( "  --> " ); Serial.print ( temp_mv ); Serial.print ( " temp mv" );
  // Serial.println ();
  temp = ( ( temp_mv - 2500. ) / 10 + 25. ) * 1.8 + 32.0;
 
  bufPtr = transact  ( ADCC | ADEN, 2, strBuf );  // channel 0 - angular rate
  delay ( 1 );
  bufPtr = transact  ( ADCR, 2, strBuf );
  x = ( ( ( strBuf [ 0 ] << 8 ) | strBuf [ 1 ] ) & 0x0ffe ) >> 1;
  rate_mv = 25.0 / 16.0 * x + 400.0;
  rate = ( x - 1008. ) / 12.8;

  Serial.print (" Rate: ");
  Serial.print ( rate_mv ); Serial.print ( " mv" );
  Serial.print ( "   " ); 
  Serial.print ( rate ); Serial.print ( " deg/S" );
  Serial.print ( "       Temp: " ); 
  Serial.print ( temp_mv ); Serial.print ( " mv" );
  Serial.print ( "   " ); 
  Serial.print ( temp ); Serial.print ( " degF" );
  Serial.println ();
  
  delay ( 500 );
}

#define EOC ( strBuf [ 0 ] & 0x40 )
// command accepted
#define nCA ( strBuf [ 0 ] & 0x80 )

int transact ( byte command, int bytesToRead, unsigned char * result ) {

  int i = 0, n = 0;
  
  // Serial.print ( "\nbytesToRead: " ); Serial.println ( bytesToRead );
  result [ 0 ] = 0x80;
  while ( nCA ) {
    digitalWrite(pdChipSelect, LOW);
    SPI.transfer ( command );
    for ( i = 0; i < bytesToRead; i++ ) {
      result [ i ] = SPI.transfer ( 0x00 );
    }
    digitalWrite(pdChipSelect, HIGH);
    // Serial.print ("                          -> ");
    // for ( i = 0; i < bytesToRead; i++ ) {
      // Serial.print ( " 0x" );
      // Serial.print ( result [ i ], HEX );
    // }
    // Serial.println ();
    n++;
    // delay ( 1 );
  }
  
  // Serial.print ( "n: " ); Serial.println ( n );
  return ( i );
  
}
