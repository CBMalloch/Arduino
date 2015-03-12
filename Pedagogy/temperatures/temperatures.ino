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
enum { paBC2301 = 14 };      // the BC2301 NTC 10K thermistor, gold-orange-black-brown from tip

const float powerRail_v         =     3.3;
const float divider_Rfixed_ohms = 20064.0; // 23156.0; // 996.15;
const float zeroC_degK          =   273.15; 
const float rRef_ohms           = 10000.0;  
const float tRef_degK           = 25.0 + zeroC_degK;
// a1, b1, c1, d1 consts for NTC thermistor 
// BC2301 a.k.a. Vishay NTCLE100E3103 Mat A. with Bn = 3977K
const float abcd[] = { 3.354016E-03, 2.569850E-04, 2.620131E-06, 6.383091E-08 };  

OneWire  ds( pdOneWire );  // a 4.7K resistor pullup is necessary

void setup(void) {
  
  pinMode ( pdLED, OUTPUT );
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && ( millis() < 10000 ) ) {
    digitalWrite ( pdLED, ( millis() >> 7 ) & 0x01 );
  }
}

void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
  
  // run through all the visible one-wire devices
  
  while ( ds.search ( addr ) ) {
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
  }  // walking through one-wire devices
  
  // Serial.println("No more addresses.");
  // Serial.println();
  ds.reset_search();
  
  // read thermistor
  int counts = analogRead ( paBC2301 );
  float vMeas = powerRail_v * float ( counts ) / 1024.0;  // voltage across the thermistor
  float aMeas = ( powerRail_v - vMeas ) / divider_Rfixed_ohms;
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
  delay(250);
 
}
