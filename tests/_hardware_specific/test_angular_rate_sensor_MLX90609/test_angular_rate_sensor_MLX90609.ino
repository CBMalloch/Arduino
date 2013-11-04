/*
 MLX90609 Angular Rate Sensor
 
 Uses the SPI library. For details on the sensor, see:
 http://www.sparkfun.com/commerce/product_info.php?products_id=8161
 
 This sketch adapted from Nathan Seidle's SCP1000 example for PIC:
 http://www.sparkfun.com/datasheets/Sensors/SCP1000-Testing.zip
 
 NOTE:
 This device has far too much drift to be used as a heading device. OK for non-integrative applications,
 but at any rate of reading, its integral drifts at a rate related to the reading frequency, but on the order of
 30deg per hour.
 
 Circuit:
 
 CSB:  pin 10  G
 MOSI: pin 11  W
 MISO: pin 12  B
 SCK:  pin 13  O
 
 
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

#define VERBOSE 10

#define BAUDRATE 115200
#define print_interval_ms 500

#define pdMLXCS SS

#define MLX_STATR 0x88
#define MLX_ADCC  0x90
#define MLX_CHAN  0x08
#define MLX_ADEN  0x04
#define MLX_ADCR  0x80

// end-of-conversion (ADC complete)
#define MLX_EOC ( strBuf [ 0 ] & 0x40 )
// not command accepted
#define MLX_nCA ( strBuf [ 0 ] & 0x80 )
// still busy
#define MLX_BUSY ( ( strBuf [ 0 ] & 0x88 ) == 0x88 )


#define bufLen 80
unsigned char strBuf [ bufLen + 2 ];
unsigned char bufPtr = 0;

float biasDifferenceFrom1008 = 0.0;

void setup() {
  Serial.begin(BAUDRATE);
  SPI.begin();

  pinMode ( pdMLXCS, OUTPUT );
  digitalWrite ( pdMLXCS, 1 );  // deselect the chip
  
  // configureSPI_GYRO ();  // no MLX_CHANge when enabled

  // initialize MLX90609
  bufPtr = transact  ( MLX_ADCC | MLX_ADEN, 2, strBuf );  // 0x94
  
  if ( VERBOSE > 1 ) {
    Serial.print ( "activate ADC: reply (length " );
    Serial.print ( bufPtr );
    Serial.println ( ")" );
    for ( int i = 0; i < bufPtr; i++ ) {
      Serial.print ( " 0x" );
      Serial.print ( strBuf [ i ], HEX );
    }
    Serial.println ();
  }
  
  if ( MLX_nCA ) {
    Serial.println ("Halting because initialization refused"); for ( ;; ) { }
  }

  // Serial.println ("Halting"); for ( ;; ) { }
  
  Serial.println ( "Calibrating MLX90609..." );

  
  // calibrate MLX90609 for zero angular velocity reading
  
  int MLX_nCAlibrationIterations = 1000;
  for (int i = 0; i < MLX_nCAlibrationIterations; i++) {
    int x;
    float rate_mv;
    strBuf [ 0 ] = 0; strBuf [ 1 ] = 0;
    bufPtr = transact  ( MLX_ADCC | MLX_ADEN, 2, strBuf );  // MLX_CHANnel 0 - angular rate  0x94 (again)
    
    if ( ! ( ( strBuf [ 0 ] & 0xa0 ) == 0x20 && ( strBuf [ 1 ] & 0x06 ) == 0x04 ) ) {
      Serial.print ("Halting because of bad reply to request for ADC: "); 
      Serial.print ( "0x" ); Serial.print ( strBuf [ 0 ], HEX ); 
      Serial.print ( " 0x" ); Serial.print ( strBuf [ 1 ], HEX ); 
      for ( ;; ) { }
    } 
    delay ( 1 );
    strBuf [ 0 ] = 0; strBuf [ 1 ] = 0;
    bufPtr = transact  ( MLX_ADCR, 2, strBuf );
    
    if ( 1 ) {
      Serial.print ( "Reply: " );
      for ( int k = 0; k < 8; k++ ) {
        Serial.print ( ( strBuf [ 0 ]  >> ( 7 - k )) & 0x01 );
      }
      Serial.print ( " " );
      for ( int k = 0; k < 8; k++ ) {
        Serial.print ( ( strBuf [ 1 ]  >> ( 7 - k )) & 0x01 );
      }
      Serial.print ( "B: " );
    }
    
    x = ( ( int ( strBuf [ 0 ] & 0x0f ) << 8 ) + strBuf [ 1 ] ) >> 1;
    // rate_mv = 25.0 / 12.0 * x + 400.0;
    Serial.print ("x: "); Serial.println (x);
    biasDifferenceFrom1008 += ( x - 1008 );
    delay ( 10 );
  }
  biasDifferenceFrom1008 = biasDifferenceFrom1008 / float ( MLX_nCAlibrationIterations );
  Serial.print ("Bias difference: "); Serial.println (biasDifferenceFrom1008);
  
  if ( abs ( biasDifferenceFrom1008 ) > 200 ) {
    Serial.println ("Halting from excessive bias difference"); for ( ;; ) { }
  }
  
  // Serial.println ("Halting"); for ( ;; ) { }
}

void configureSPI_GYRO()
{
  SPCR = /* Configure SPI mode: */
    (1<<SPE) |  /* to enable SPI */
    (1<<MSTR) | /* to set Master SPI mode */
    (1<<CPOL) | /* SCK is high when idle */
    (1<<CPHA) | /* data is sampled on the trailing edge of the SCK */
    (1<<SPR0);  /* It sets SCK freq. in 8 times less than a system clock */    
}

void loop() {

  int x;
  float temp_mv, rate_mv, temp, rate;
  static float delta_heading, heading = 0;
  static unsigned long heading_read_at_us = 0UL, last_print_at_ms = 0;
  
  // start conversion to get temperature
  bufPtr = transact  ( MLX_ADCC | MLX_CHAN | MLX_ADEN, 2, strBuf ); // MLX_CHANnel 1 - temperature
  
  // Serial.print ( "Conversion start: reply: " );
  // Serial.println ( bufPtr );
  // for ( int i = 0; i < bufPtr; i++ ) {
    // Serial.print ( " 0x" );
    // Serial.print ( strBuf [ i ], HEX );
  // }
  // Serial.println ();

  // poll and get results (ADC reading)
  delay ( 1 );
  bufPtr = transact  ( MLX_ADCR, 2, strBuf );
  
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
 
  bufPtr = transact  ( MLX_ADCC | MLX_ADEN, 2, strBuf );  // MLX_CHANnel 0 - angular rate
  delay ( 1 );
  bufPtr = transact  ( MLX_ADCR, 2, strBuf );
  x = ( ( ( strBuf [ 0 ] << 8 ) | strBuf [ 1 ] ) & 0x0ffe ) >> 1;
  rate_mv = 25.0 / 12.0 * x + 400.0;
  rate = ( x - ( biasDifferenceFrom1008 + 1008.0 ) ) / 12.8;
  
  if ( heading_read_at_us != 0 ) {
    // this is not the first rate reading, so we can now calculate delta heading and then heading
    unsigned long delta_t_us;
    delta_t_us = micros() - heading_read_at_us;
    heading_read_at_us = micros();
    delta_heading = rate * delta_t_us / 1.0e6;
    heading += delta_heading;
  } else {
    heading_read_at_us = micros();
  }
  
  if ( ( millis() - last_print_at_ms ) > print_interval_ms ) {

    Serial.print (" Rate: ");
    Serial.print ( rate_mv ); Serial.print ( " mv" );
    Serial.print ( "   " ); 
    Serial.print ( rate ); Serial.print ( " deg/S" );
    Serial.print ( "       Temp: " ); 
    Serial.print ( temp_mv ); Serial.print ( " mv" );
    Serial.print ( "   " ); 
    Serial.print ( temp ); Serial.print ( " degF" );
    Serial.println ();
    
    Serial.print ( "        Heading: " );
    Serial.print ( heading ); Serial.println ( " deg" );
    
    last_print_at_ms = millis();
  }
  
  delay ( 50 );  // 50 ms delay is about 20 readings per second
}

int transact ( byte command, int bytesToRead, unsigned char * result ) {

  int i = 0, n = 0;
  
  // Serial.print ( "\nbytesToRead: " ); Serial.println ( bytesToRead );
  result [ 0 ] = 0x80;
  while ( MLX_nCA ) {
    digitalWrite(pdMLXCS, LOW);
    SPI.transfer ( command );
    for ( i = 0; i < bytesToRead; i++ ) {
      result [ i ] = SPI.transfer ( 0x00 );
    }
    digitalWrite(pdMLXCS, HIGH);
    // Serial.print ("                          -> ");
    // for ( i = 0; i < bytesToRead; i++ ) {
      // Serial.print ( " 0x" );
      // Serial.print ( result [ i ], HEX );
    // }
    // Serial.println ();
    n++;
    // delay ( 1 );
  }
  
  if ( MLX_BUSY ) {
    Serial.println ( "Reply MLX_BUSY" );
  }
  
  // Serial.print ( "n: " ); Serial.println ( n );
  return ( i );
  
}
