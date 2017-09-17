/*
	LSM303DLHC.cpp - library to use a LSM303DLHC tilt-compensated compass
	Created by Charles B. Malloch, PhD, June 6, 2013
	Released into the public domain
	
	
	
*/

#include <Arduino.h>
#include <wiring.h>
#include <Wire.h>

#include <LSM303DLHC.h>

// specific to LSM303DLHC - private

#define CTRL_REG1_A 0x20
#define CTRL_REG2_A 0x21
#define CTRL_REG3_A 0x22
#define CTRL_REG4_A 0x23
#define CTRL_REG5_A 0x24
#define CTRL_REG6_A 0x25
#define REFERENCE_A 0x26
#define STATUS_REG_A 0x27
#define OUT_X_L_A 0x28
#define OUT_X_H_A 0x29
#define OUT_Y_L_A 0x2A
#define OUT_Y_H_A 0x2B
#define OUT_Z_L_A 0x2C
#define OUT_Z_H_A 0x2D
#define FIFO_CTRL_REG_A  0x2e
#define FIFO_SRC_REG_A   0x2f
#define INT1_CFG_A 0x30
#define INT1_SOURCE_A 0x31
#define INT1_THS_A 0x32
#define INT1_DURATION_A 0x33
#define INT2_CFG_A 0x34
#define INT2_SOURCE_A 0x35
#define INT2_THS_A 0x36
#define INT2_DURATION_A 0x37
#define CLICK_CFG_A 0x38
#define CLICK_SRC_A 0x39
#define CLICK_THS_A 0x3a
#define TIME_LIMIT_A 0x3b
#define TIME_LATENCY_A 0x3c
#define TIME_WINDOW_A 0x3d

#define CRA_REG_M 0x00
#define CRB_REG_M 0x01
#define MR_REG_M 0x02
#define OUT_X_H_M 0x03
#define OUT_X_L_M 0x04
#define OUT_Z_H_M 0x05
#define OUT_Z_L_M 0x06
#define OUT_Y_H_M 0x07
#define OUT_Y_L_M 0x08
#define SR_REG_M 0x09
#define IRA_REG_M 0x0A
#define IRB_REG_M 0x0B
#define IRC_REG_M 0x0C
#define TEMP_OUT_H_M 0x31
#define TEMP_OUT_L_M 0x32


#define ACC_SCALER 2  // accel full-scale, should be 2, 4, or 8

/* LSM303DLHC Address definitions */
// assuming SA0 grounded or unconnected -- ??
#define REG_BASE_ADDR_MAG  0x1E
#define REG_BASE_ADDR_ACC  0x19

LSM303DLHC::LSM303DLHC () {
}

// LSM303DLHC::LSM303DLHC ( int scale = ACC_SCALER ) {
// init ( scale );
// }

//************************************************************************************************
// 						                             initialization
//************************************************************************************************

void LSM303DLHC::init ( int scale = ACC_SCALER ) {

  Wire.begin ();  // Start up I2C, required for LSM303 communication

  // Serial.println ( "init 0" );
  write(0b00100111, REG_BASE_ADDR_ACC, CTRL_REG1_A);  // 0x27 = normal power mode, all accel axes on
  // Serial.println ( "init 1" );
  if ( ( scale == 8 ) || ( scale == 4 ) ) {
    write ( ( 0x00 | ( scale - scale / 2 - 1 ) << 4 ), REG_BASE_ADDR_ACC, CTRL_REG4_A );  // set full-scale
    // Serial.println ( "init 2.1" );
  } else {
    write ( 0x00, REG_BASE_ADDR_ACC, CTRL_REG4_A );
    // Serial.println ( "init 2.2" );
 }
    
  write ( 0b10010100, REG_BASE_ADDR_MAG, CRA_REG_M );  // 0x14 = mag 30Hz output rate; 0x80 to enable thermometer
  write ( 0x00, REG_BASE_ADDR_MAG, MR_REG_M );  // 0x00 = continouous conversion mode
  // Serial.println ( "init 9" );
}



//************************************************************************************************
// 						                             interface subroutines
//************************************************************************************************




void LSM303DLHC::getAccel(int * rawValues) {
  int regs [ 3 ];
  regs [ 0 ] = ( ( int ) read ( REG_BASE_ADDR_ACC, OUT_X_H_A ) << 8 ) | (read ( REG_BASE_ADDR_ACC, OUT_X_L_A ) );
  regs [ 1 ] = ( ( int ) read ( REG_BASE_ADDR_ACC, OUT_Y_H_A ) << 8 ) | (read ( REG_BASE_ADDR_ACC, OUT_Y_L_A ) );
  regs [ 2 ] = ( ( int ) read ( REG_BASE_ADDR_ACC, OUT_Z_H_A ) << 8 ) | (read ( REG_BASE_ADDR_ACC, OUT_Z_L_A ) );
  
  /*
  Serial.print ( "\nAccelerometer vector unmapped: ( " );
  for (int i = 0; i < 3; i++) {
    if ( i > 0 ) Serial.print ( ", " );
    Serial.print ( regs [ i ] );
    Serial.print ( " ( " );
    printHex ( regs [ i ], 4 );
    Serial.print ( " ) " );
  }
  Serial.println ( " )" );
  */
  
  // no need to remap those to match the values to the picture in the tech note
  rawValues [ X_COORD ] =   regs [ 0 ];
  rawValues [ Y_COORD ] =   regs [ 1 ];
  rawValues [ Z_COORD ] =   regs [ 2 ];  
}


void LSM303DLHC::getMag ( int * rawValues ) {

  while ( ! ( read ( REG_BASE_ADDR_MAG, SR_REG_M ) & 0x01 ) )
      ;  // wait for the magnetometer readings to be ready
      
  #if 0
  
  Serial.print ("\nMagnetometer registers (byte) 0x00 - 0x09: ( ");
  Wire.beginTransmission ( REG_BASE_ADDR_MAG );
  Wire.write ( 0 );
  Wire.endTransmission ();
  Wire.requestFrom ( REG_BASE_ADDR_MAG, 10 );
  for ( int i = 0; i < 10; i++ ) {
    if ( i > 0 ) Serial.print ( ", " );
    printHex ( Wire.read(), 2 );
  }
  Serial.println ( " )" );
  
  delay ( 10 );
  
  #endif
  
  #if FALSE
  
  Serial.print ("\nMagnetometer registers (byte) 0x03 - 0x08: ( ");
  Wire.beginTransmission ( REG_BASE_ADDR_MAG );
  Wire.write ( OUT_X_H_M );
  Wire.endTransmission ();
  Wire.requestFrom ( REG_BASE_ADDR_MAG, 6 );
  for (int i = 0; i < 6; i++) {
    if ( i > 0 ) Serial.print ( ", " );
    printHex ( Wire.read(), 2 );
  }
  Serial.println ( " )" );
  
  #endif

  #if FALSE

  Serial.print ("\nMagnetometer individual register names: ( ");
  Serial.print ( OUT_X_H_M );
  Serial.print ( ", " );
  Serial.print ( OUT_X_L_M );
  Serial.print ( ", " );
  Serial.print ( OUT_Y_H_M );
  Serial.print ( ", " );
  Serial.print ( OUT_Y_L_M );
  Serial.print ( ", " );
  Serial.print ( OUT_Z_H_M );
  Serial.print ( ", " );
  Serial.print ( OUT_Z_L_M );
  Serial.println ( " )" );

  #endif

  #if FALSE

  Serial.print ("\nMagnetometer individually-named registers ( H first, then L ): ( ");
  printHex ( read ( REG_BASE_ADDR_MAG, OUT_X_H_M ), 2 );
  Serial.print ( ", " );
  printHex ( read ( REG_BASE_ADDR_MAG, OUT_X_L_M ), 2 );
  Serial.print ( ", " );
  printHex ( read ( REG_BASE_ADDR_MAG, OUT_Y_H_M ), 2 );
  Serial.print ( ", " );
  printHex ( read ( REG_BASE_ADDR_MAG, OUT_Y_L_M ), 2 );
  Serial.print ( ", " );
  printHex ( read ( REG_BASE_ADDR_MAG, OUT_Z_H_M ), 2 );
  Serial.print ( ", " );
  printHex ( read ( REG_BASE_ADDR_MAG, OUT_Z_L_M ), 2 );
  Serial.println ( " )" );

  delay ( 10 );
 
  #endif
 
  int regs [ 3 ];
  regs [ 0 ] = ( ( int ) read ( REG_BASE_ADDR_MAG, OUT_X_H_M ) << 8 ) 
                     | ( read ( REG_BASE_ADDR_MAG, OUT_X_L_M ) );
  regs [ 1 ] = ( ( int ) read ( REG_BASE_ADDR_MAG, OUT_Y_H_M ) << 8 ) 
                     | ( read ( REG_BASE_ADDR_MAG, OUT_Y_L_M ) );
  regs [ 2 ] = ( ( int ) read ( REG_BASE_ADDR_MAG, OUT_Z_H_M ) << 8 ) 
                     | ( read ( REG_BASE_ADDR_MAG, OUT_Z_L_M ) );
  
  #if FALSE

  Serial.print ( "\nMagnetometer vector unmapped: ( " );
  for (int i = 0; i < 3; i++) {
    if ( i > 0 ) Serial.print ( ", " );
    Serial.print ( regs [ i ] );
    Serial.print ( " ( " );
    printHex ( regs [ i ], 4 );
    Serial.print ( " ) " );
  }
  Serial.println ( " )" );
  
  #endif
  
  // remap those to match the values to the picture in the tech note
  // NOTE not a right-handed coordinate system
  rawValues [ X_COORD ] =   regs [ 0 ];
  rawValues [ Y_COORD ] =   regs [ 2 ];
  rawValues [ Z_COORD ] =   regs [ 1 ];  

}


void LSM303DLHC::getTemp ( int * pTemp ) {
  int temp;
  // temp is returned as eighths of a degC, 2's complement, 12-bit resolution, LEFT-JUSTIFIED (needs to shift R 4)
  temp = ( ( int ) ( read ( REG_BASE_ADDR_MAG, TEMP_OUT_H_M ) << 8 ) 
                 | ( read ( REG_BASE_ADDR_MAG, TEMP_OUT_L_M ) << 0 ) ) >> 4 ;
  // Serial.print ( "Get Temp: " ); Serial.println ( temp );
  *pTemp = temp;
}


float LSM303DLHC::getHeading(float * magValue) {
  // see section 1.2 in app note AN3192
  float heading = 180 * atan2 ( float ( magValue [ Y_COORD ] ), float ( magValue [ X_COORD ] ) ) / PI;  // assume pitch, roll are 0
  
  if ( heading < 0 )
    heading += 360;
  
  // Serial.print ( "Uncorrected heading: " ); Serial.println ( heading, 3 );
  return heading;
}

float LSM303DLHC::getTiltHeading(float * magValue, float * accelValue) {

  // see appendix A in app note AN3192 
  float pitch =   asin ( accelValue [ X_COORD ] );
  float roll  = - asin ( accelValue [ Y_COORD ] / cos ( pitch ) );
  // Serial.print ( "Pitch, roll (deg): " ); Serial.print ( pitch * 180.0 / PI );
  // Serial.print ( ", " ); Serial.println ( roll * 180.0 / PI );
  
  // to correct these, we rotate our coordinate system which rolls and pitches the mag vector the same way
  float theta = + roll;
  float phi   = + pitch;
  
  float x = magValue[X_COORD];
  float y = magValue[Y_COORD];
  float z = magValue[Z_COORD];
  
  float sinPhi = sin ( phi );
  float cosPhi = cos ( phi );
  float sinTheta = sin ( theta );
  float cosTheta = cos ( theta );
  
  float xh = x * cosPhi
            + y * sinTheta * sinPhi
            - z * cosTheta * sinPhi;
  float yh = x * 0
            + y * cosTheta
            + z * sinTheta;
  float zh = x * sinPhi
            - y * sinTheta * cosPhi
            + z * cosTheta * cosPhi;

  #if 0
  
  Serial.print ("\nMag vector corrected for pitch and roll: ( ");
  Serial.print ( xh, 1 );
  Serial.print ( ", " );
  Serial.print ( yh, 1 );
  Serial.print ( ", " );
  Serial.print ( zh, 1 );
  Serial.println ( " )" );
  
  #endif

  #if 0
  
  float magnitude = sqrt ( xh * xh + yh * yh + zh * zh );
  Serial.print ("Mag vector corrected and normalized: ( ");
  Serial.print ( xh / magnitude, 3);
  Serial.print ( ", " );
  Serial.print ( yh / magnitude, 3 );
  Serial.print ( ", " );
  Serial.print ( zh / magnitude, 3 );
  Serial.println ( " )" );
  
  #endif

  float heading_deg = atan2 ( yh, xh ) * 180.0 / PI;

  // Serial.print ( "Corrected heading: " ); Serial.println ( heading_deg, 3 );

  if ( yh >= 0 )
    return heading_deg;
  else
    return ( 360 + heading_deg );

  //*/
  
  /*
  // get dot product with acceleration vector:
  float projectedMag [ 3 ], magnitude_accel;
  float dot [ 3 ];
  int i;
  
  magnitude_accel = 0;
  for ( i = 0; i < 3; i++ ) {
    dot [ i ] = magValue [ i ] * accelValue [ i ];
    magnitude_accel += accelValue [ i ] * accelValue [ i ];
  }
  magnitude_accel = sqrt ( magnitude_accel );
  Serial.print ( "Magnitude of acceleration: " ); Serial.println ( magnitude_accel, 3 );
  for ( i = 0; i < 3; i++ ) {
    dot [ i ] /= magnitude_accel;
  }
  
  #if 1
  
  Serial.print ("\nDot product: ( ");
  for (i = 0; i < 3; i++) {
    if ( i > 0 ) Serial.print ( ", " );
    Serial.print ( dot [ i ], 2 );
  }
  Serial.println ( " )" );
   
  #endif
  
  // now project heading into horizontal plane
  for ( i = 0; i < 3; i++ ) {
    projectedMag [ i ] = float ( magValue [ i ] ) - dot [ i ];
  }
  
  #if 1
  
  Serial.print ("\nProjected magnetic vector: ( ");
  for (i = 0; i < 3; i++) {
    if ( i > 0 ) Serial.print ( ", " );
    Serial.print ( projectedMag [ i ], 2 );
  }
  Serial.println ( " )" );
   
  #endif
  
  
  float heading = 180 * atan2 ( projectedMag [ 1 ], projectedMag [ 0 ] ) / PI;

  return ( heading );
  
  */
  
}





//************************************************************************************************
// 						                             basic subroutines
//************************************************************************************************



byte LSM303DLHC::read ( int device_address, byte register_address ) {

  byte temp;
  
  Wire.beginTransmission ( device_address );
  Wire.write ( register_address );
  Wire.endTransmission ();
  
  Wire.requestFrom ( device_address, 1 );
  while ( ! Wire.available() ) ;
  temp = Wire.read ();
  
  return temp;
}

void LSM303DLHC::write ( byte data, byte device_address, byte register_address ) {

  Wire.beginTransmission ( device_address );
    
  Wire.write ( register_address );
  Wire.write ( data );
  Wire.endTransmission ();
  
}
