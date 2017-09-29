#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

enum { BAUDRATE = 115200 };
enum { pdLED = 13 };
enum { pdOneWire = 10 };
enum { paBC2301 = 0 };      // the BC2301 NTC 10K thermistor, gold-orange-black-brown from tip

const float powerRail_v         =     3.3;
const float divider_Rfixed_ohms = 23156.0;
const float zeroC_degK          =   273.15; 
const float rRef_ohms           = 10000.0;  
const float tRef_degK           = 25.0 + zeroC_degK;
// a1, b1, c1, d1 consts for NTC thermistor 
// BC2301 a.k.a. Vishay NTCLE100E3103 Mat A. with Bn = 3977K
const float abcd[] = { 3.354016E-03, 2.569850E-04, 2.620131E-06, 6.383091E-08 };  

OneWire  ds( pdOneWire );  // a 4.7K resistor pullup is necessary

// we will average the thermometer values and do statistics on their 
// deviations from this average value in Kelvin-sized degrees
enum { nMax = 10 };
float temps_degK [ nMax ];
uint32_t ids [ nMax ];
int n [ nMax ];
float x [ nMax ], x2 [ nMax ];


void setup(void) {
  
  pinMode ( pdLED, OUTPUT );
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 10000 ) ) {
    digitalWrite ( pdLED, ( millis() >> 7 ) & 0x01 );
  }
  
  for ( int i = 0; i < nMax; i++ ) {
    ids [ i ] = 0;
    n [ i ] = 0;
    x [ i ] = 0.0;
    x2 [ i ] = 0.0;
  }

}

void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data [ 12 ];
  byte addr [ 8 ];
  float celsius, fahrenheit;
  
  // run through all the visible one-wire devices
  
  int therm = 0;
  for ( int i = 0; i < nMax; i++ ) {
    temps_degK [ i ] = 0.0;
  }
  
  while ( ds.search ( addr ) ) {
    
    /*
      Format of addr ( 8 bytes )
         0 : CRC
        1-6: 48 bit serial number
         7 : family code ( 0x28 for DS18B20 )
    */
    
    /*
    Serial.print("ROM =");
    for( i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(addr[i], HEX);
    }
    */

    if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
    }
    // Serial.println();
   
    /*
    // the first ROM byte indicates which chip
    switch (addr[0]) {
      case 0x10:
        Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
      case 0x28:
        Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;
      case 0x22:
        Serial.println("  Chip = DS1822");
        type_s = 0;
        break;
      default:
        Serial.println("Device is not a DS18x20 family device.");
        return;
    } 
    */

    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end
    
    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.
    
    present = ds.reset();
    ds.select(addr);
    ds.write(0xBE);         // "read scratchpad memory"

    // Serial.print("  Data = ");
    // Serial.print(present, HEX);
    // Serial.print(" ");
    
    /*
      Scratchpad format ( 9 bytes ):
        0: temperature LSB
        1: temperature MSB
        2: TH register or user byte 1
        3: TL register or user byte 2
          The T register is for alarm signaling
        4: configuration register
          0 | R1 | R0 | 1 | 1 | 1 | 1 | 1
            binary R1R0 is additional bits resolution beyond 9 
            ( so R1 = 1, R0 = 0 => 11 bits res )
        5: reserved
        6: reserved
        7: reserved
        8: CRC
    */
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
      // Serial.print(data[i], HEX);
      // Serial.print(" ");
    }
    // Serial.print(" CRC=");
    // Serial.print(OneWire::crc8(data, 8), HEX);
    // Serial.println();

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      // note interesting way to make a mask: ~7 should be same as 0xFFF8
      //   is there a guarantee that ~7 is interpreted as 16-bit value?
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float) raw / 16.0;
    fahrenheit = celsius * 1.8 + 32.0;
    Serial.print("  Temperature = ");
    Serial.print ( celsius + zeroC_degK );
    Serial.print ( "degK; " );
    Serial.print(celsius);
    Serial.print("degC; ");
    Serial.print(fahrenheit);
    Serial.println("degF");
    
    memcpy ( ( void * ) & ( ids [ therm ] ), ( void * ) & ( addr [ 1 ] ), 4 );
    temps_degK [ therm ] = celsius;
    therm++;
    
    digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
  
  }  // walking through one-wire devices
  
  // Serial.println("No more addresses.");
  // Serial.println();
  ds.reset_search();
  
  // read thermistor
  int counts = analogRead ( paBC2301 );
  float vMeas = powerRail_v * float ( counts ) / 1024.0;  // voltage across the thermistor
  float aMeas = ( powerRail_v - vMeas ) / divider_Rfixed_ohms;  // through Rfixed
  float rMeas = vMeas / aMeas;
  if ( 0 ) {
    Serial.print ( "  Thermistor counts: " ); Serial.println ( counts );
    Serial.print ( "  Thermistor voltage drop: " ); Serial.println ( vMeas );
    Serial.print ( "  Current (milliamps): " ); Serial.println ( aMeas * 1000.0 );
    Serial.print ( "  Thermistor resistance: " ); Serial.println ( rMeas );
  }
  
  float lnR = log ( rMeas / rRef_ohms );  // where rRef_ohms is the reference temp of 25degC; in c, log is natural log
  float invT = abcd[0] + abcd[1] * lnR + abcd[2] * lnR * lnR + abcd[3] * lnR * lnR * lnR;
  float T_K = 1 / invT;
  float T_C = T_K - zeroC_degK;
  float T_F = T_C * 1.8 + 32.0;
  
  Serial.print ( "  Thermistor  = " );
  Serial.print ( T_K );
  Serial.print ( "degK; " );
  Serial.print ( T_C );
  Serial.print ( "degC; " );
  Serial.print ( T_F );
  Serial.print ( "degF" );

  Serial.print ( "\r\n\r\n" );
  
  temps_degK [ therm ] = T_C;
  therm++;
  
  digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
  
  // now do statistics on the temperatures
  
  // average them:
  float sum = 0.0;
  for ( int i = 0; i < therm; i++ ) {
    sum += temps_degK [ i ];
  }
  
  float avg = sum / therm;
  // Serial.print ( "Avg temp: " ); Serial.println ( avg );
  
  for ( int i = 0; i < therm; i++ ) {
    n [ i ] ++;
    x [ i ] += temps_degK [ i ] - avg;
    x2 [ i ] += ( temps_degK [ i ] - avg ) * ( temps_degK [ i ] - avg );
  }
  
  // now report the stats from the moments
  
  for ( int i = 0; i < therm; i++ ) {
    float m, stDev;
		if ( n [ i ] > 0 ) {
      m = x [ i ] / n [ i ];
			if ( n [ i ] > 1 ) {
				// from WikiPedia; checks with Excel
				stDev = sqrt ( ( x2 [ i ]  - ( x [ i ] * x [ i ] ) / n [ i ] ) / ( n [ i ] - 1 ) );
			} else {
        stDev = 0.0;
      }
		} else {
      m = -5000.0;
      stDev = -5000.0;
    }
    
    Serial.print ( "n: " ); Serial.print ( n [ i ] );
    Serial.print ( "; id = " ); printID ( ( uint8_t * ) & ids [ i ] );
    Serial.print ( "; i = " ); Serial.print ( i );
    Serial.print ( ": m = " ); Serial.print ( m );
    Serial.print ( "; s = " ); Serial.print ( stDev );
    Serial.println ();
  }  
  
  digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
  
  Serial.println (); Serial.println ();
  delay(250);
 
}

void printID (uint8_t *data) {
  // note little-endian byte order
  Serial.print ( "0x" ); 
  for ( int i = 3; i >= 0; i-- ) { 
    if ( data [ i ] < 0x10 ) Serial.print("0");
    Serial.print( data [ i ], HEX );
  }
}
