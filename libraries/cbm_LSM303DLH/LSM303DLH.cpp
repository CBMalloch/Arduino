/*
	LSM303DLH.cpp - library to use a LSM303DLH tilt-compensated compass
	Created by Charles B. Malloch, PhD, June 6, 2013
	Released into the public domain
	
	
	
*/

#include <Arduino.h>
// #include <wiring.h>
#include <Wire.h>
#include <PrintHex.h>

#include <LSM303DLH.h>

#define LSM303DLH_VERBOSE 20

// specific to LSM303DLH - private

#define CTRL_REG1_A 0x20
#define CTRL_REG2_A 0x21
#define CTRL_REG3_A 0x22
#define CTRL_REG4_A 0x23
#define CTRL_REG5_A 0x24
// #define CTRL_REG6_A 0x25
// HP_FILTER_RESET_A is a dummy register
#define HP_FILTER_RESET_A 0x25
#define REFERENCE_A 0x26
#define STATUS_REG_A 0x27
#define OUT_X_L_A 0x28
#define OUT_X_H_A 0x29
#define OUT_Y_L_A 0x2A
#define OUT_Y_H_A 0x2B
#define OUT_Z_L_A 0x2C
#define OUT_Z_H_A 0x2D
// #define FIFO_CTRL_REG_A  0x2e
// #define FIFO_SRC_REG_A   0x2f
#define INT1_CFG_A 0x30
#define INT1_SOURCE_A 0x31
#define INT1_THS_A 0x32
#define INT1_DURATION_A 0x33
#define INT2_CFG_A 0x34
#define INT2_SOURCE_A 0x35
#define INT2_THS_A 0x36
#define INT2_DURATION_A 0x37
// #define CLICK_CFG_A 0x38
// #define CLICK_SRC_A 0x39
// #define CLICK_THS_A 0x3a
// #define TIME_LIMIT_A 0x3b
// #define TIME_LATENCY_A 0x3c
// #define TIME_WINDOW_A 0x3d

#define CRA_REG_M 0x00
#define CRB_REG_M 0x01
#define MR_REG_M 0x02
#define OUT_X_H_M 0x03
#define OUT_X_L_M 0x04
#define OUT_Y_H_M 0x05
#define OUT_Y_L_M 0x06
#define OUT_Z_H_M 0x07
#define OUT_Z_L_M 0x08
#define SR_REG_M 0x09
#define IRA_REG_M 0x0A
#define IRB_REG_M 0x0B
#define IRC_REG_M 0x0C
// #define TEMP_OUT_H_M 0x31
// #define TEMP_OUT_L_M 0x32


/* LSM303DLH Address definitions */
#define REG_BASE_ADDR_MAG  0x1E
// SA0 grounded -> accel base address = 0x18; if pulled up, 0x19
#define SA0_BRIDGE_VALUE 0
#define REG_BASE_ADDR_ACC  ( 0x18 + SA0_bridge_value )

LSM303DLH::LSM303DLH ( Print &print ) {
  _printer = &print;
}

// LSM303DLH::LSM303DLH ( int scale = ACC_SCALER ) {
// init ( scale );
// }

//************************************************************************************************
// 						                             initialization
//************************************************************************************************

void LSM303DLH::init ( int acc_scale, int SA0_bridge_value_in ) {

  // _printer->println ( "init -2" );
  SA0_bridge_value = SA0_bridge_value_in;
  // _printer->println ( "init -1" );
  Wire.begin ();  // Start up I2C, required for LSM303 communication

  // _printer->println ( "init 0" );
  // change to 0x47 for low-power mode output data rate 0.5 Hz
  write ( 0b00100111, REG_BASE_ADDR_ACC, CTRL_REG1_A );  // 0x27 = normal power mode, all accel axes on
  // _printer->println ( "init 1" );
  _acc_scale_bits = 0x00;
  switch ( acc_scale ) {
    case 8:  // 8G full scale
      _acc_scale_bits = 0x03;
      break;
    case 4:  // 4G full scale
      _acc_scale_bits = 0x01;
      break;
    default: // 2G full scale
      _acc_scale_bits = 0x00;
      break;
  }
    
  write ( 0x00, REG_BASE_ADDR_MAG, CRA_REG_M );  // 0x1c = D02-D00; 0x03 = MS1-MS0
  write ( 0x20, REG_BASE_ADDR_MAG, CRB_REG_M );  // 0x20 = GN2-GN0 -> highest gain
  write ( 0x00, REG_BASE_ADDR_MAG, MR_REG_M );   // 0x00 = continouous conversion mode
  // _printer->println ( "init 9" );
  
}



//************************************************************************************************
// 						                             interface subroutines
//************************************************************************************************


void LSM303DLH::getAccel ( float * _acceleration_vector_components_g ) {
  int16_t regs [ 3 ];
  regs [ 0 ] = ( ( int16_t ) read ( REG_BASE_ADDR_ACC, OUT_X_H_A ) << 8 ) | (read ( REG_BASE_ADDR_ACC, OUT_X_L_A ) );
  regs [ 1 ] = ( ( int16_t ) read ( REG_BASE_ADDR_ACC, OUT_Y_H_A ) << 8 ) | (read ( REG_BASE_ADDR_ACC, OUT_Y_L_A ) );
  regs [ 2 ] = ( ( int16_t ) read ( REG_BASE_ADDR_ACC, OUT_Z_H_A ) << 8 ) | (read ( REG_BASE_ADDR_ACC, OUT_Z_L_A ) );
  
  #if LSM303DLH_VERBOSE >= 30
    _printer->print ( "\nAccelerometer vector unmapped: ( " );
    for (int i = 0; i < 3; i++) {
      if ( i > 0 ) _printer->print ( ", " );
      _printer->print ( regs [ i ] );
      _printer->print ( " ( " );
      printHex ( regs [ i ], 4 );
      _printer->print ( " ) " );
    }
    _printer->println ( " )" );
  #endif
  
  // no need to remap those to match the values to the picture in the tech note
  int rawValues [ 3 ];
  rawValues [ X_COORD ] = ( int ) regs [ 0 ];
  rawValues [ Y_COORD ] = ( int ) regs [ 1 ];
  rawValues [ Z_COORD ] = ( int ) regs [ 2 ];
  
  // 2^15 = 1024 * 32
  float scaler = _acc_scales [ _acc_scale_bits ] / 32768.0;
  _acceleration_vector_components_g [ X_COORD ] = rawValues [ X_COORD ] * scaler;
  _acceleration_vector_components_g [ Y_COORD ] = rawValues [ Y_COORD ] * scaler;
  _acceleration_vector_components_g [ Z_COORD ] = rawValues [ Z_COORD ] * scaler;
  
  #if LSM303DLH_VERBOSE >= 20
    _printer->print ( "\nAccelerometer vector unmapped: ( " );
    for (int i = 0; i < 3; i++) {
      if ( i > 0 ) _printer->print ( ", " );
      _printer->print ( _acceleration_vector_components_g [ i ] );
    }
    _printer->println ( " )" );
  #endif
  
}

void LSM303DLH::getMag ( float * _magnetic_field_components_gauss ) {

  while ( ! ( read ( REG_BASE_ADDR_MAG, SR_REG_M ) & 0x01 ) )
      ;  // wait for the magnetometer readings to be ready
      
  #if 0
  
    _printer->print ("\nMagnetometer registers (byte) 0x00 - 0x09: ( ");
    Wire.beginTransmission ( REG_BASE_ADDR_MAG );
    Wire.write ( 0 );
    Wire.endTransmission ();
    Wire.requestFrom ( REG_BASE_ADDR_MAG, 10 );
    for ( int i = 0; i < 10; i++ ) {
      if ( i > 0 ) _printer->print ( ", " );
      printHex ( Wire.read(), 2 );
    }
    _printer->println ( " )" );
  
    delay ( 10 );
  
  #endif
  
  #if FALSE
  
    _printer->print ("\nMagnetometer registers (byte) 0x03 - 0x08: ( ");
    Wire.beginTransmission ( REG_BASE_ADDR_MAG );
    Wire.write ( OUT_X_H_M );
    Wire.endTransmission ();
    Wire.requestFrom ( REG_BASE_ADDR_MAG, 6 );
    for (int i = 0; i < 6; i++) {
      if ( i > 0 ) _printer->print ( ", " );
      printHex ( Wire.read(), 2 );
    }
    _printer->println ( " )" );
  
  #endif

  #if FALSE

    _printer->print ("\nMagnetometer individual register names: ( ");
    _printer->print ( OUT_X_H_M );
    _printer->print ( ", " );
    _printer->print ( OUT_X_L_M );
    _printer->print ( ", " );
    _printer->print ( OUT_Y_H_M );
    _printer->print ( ", " );
    _printer->print ( OUT_Y_L_M );
    _printer->print ( ", " );
    _printer->print ( OUT_Z_H_M );
    _printer->print ( ", " );
    _printer->print ( OUT_Z_L_M );
    _printer->println ( " )" );

  #endif

  #if FALSE

    _printer->print ("\nMagnetometer individually-named registers ( H first, then L ): ( ");
    printHex ( read ( REG_BASE_ADDR_MAG, OUT_X_H_M ), 2 );
    _printer->print ( ", " );
    printHex ( read ( REG_BASE_ADDR_MAG, OUT_X_L_M ), 2 );
    _printer->print ( ", " );
    printHex ( read ( REG_BASE_ADDR_MAG, OUT_Y_H_M ), 2 );
    _printer->print ( ", " );
    printHex ( read ( REG_BASE_ADDR_MAG, OUT_Y_L_M ), 2 );
    _printer->print ( ", " );
    printHex ( read ( REG_BASE_ADDR_MAG, OUT_Z_H_M ), 2 );
    _printer->print ( ", " );
    printHex ( read ( REG_BASE_ADDR_MAG, OUT_Z_L_M ), 2 );
    _printer->println ( " )" );

  delay ( 10 );
 
  #endif
 
  int16_t regs [ 3 ];
  regs [ 0 ] = ( ( int16_t ) read ( REG_BASE_ADDR_MAG, OUT_X_H_M ) << 8 ) 
                         | ( read ( REG_BASE_ADDR_MAG, OUT_X_L_M ) );
  regs [ 1 ] = ( ( int16_t ) read ( REG_BASE_ADDR_MAG, OUT_Y_H_M ) << 8 ) 
                         | ( read ( REG_BASE_ADDR_MAG, OUT_Y_L_M ) );
  regs [ 2 ] = ( ( int16_t ) read ( REG_BASE_ADDR_MAG, OUT_Z_H_M ) << 8 ) 
                         | ( read ( REG_BASE_ADDR_MAG, OUT_Z_L_M ) );
  
  #if LSM303DLH_VERBOSE >= 30
    _printer->print ( "\n                                      Magnetometer vector unmapped: " );
    for (int i = 0; i < 3; i++) {
      if ( i > 0 ) _printer->print ( ", " );
      _printer->print ( regs [ i ] );
      _printer->print ( " ( " );
      printHex ( regs [ i ], 4 );
      _printer->print ( " ) " );
    }
    _printer->println ( " )" );
  #endif
  
  // // remap those to match the values to the picture in the tech note
  // // NOTE not a right-handed coordinate system
  int rawValues [ 3 ];
  rawValues [ X_COORD ] = ( int ) regs [ 0 ];
  rawValues [ Y_COORD ] = ( int ) regs [ 1 ];
  rawValues [ Z_COORD ] = ( int ) regs [ 2 ];  
  
  // 1100 LSB/gauss X,Y; 980 LSB/gauss Z
  _magnetic_field_components_gauss [ X_COORD ] = rawValues [ X_COORD ] / 1100.0;
  _magnetic_field_components_gauss [ Y_COORD ] = rawValues [ Y_COORD ] / 1100.0;
  _magnetic_field_components_gauss [ Z_COORD ] = rawValues [ Z_COORD ] / 980.0;

  #if LSM303DLH_VERBOSE >= 20
    _printer->print ( "\n                                      Magnetometer vector unmapped: " );
    for (int i = 0; i < 3; i++) {
      if ( i > 0 ) _printer->print ( ", " );
      _printer->print ( _magnetic_field_components_gauss [ i ] );
    }
    _printer->println ( " )" );
  #endif
  
}

float LSM303DLH::getHeading ( float * magValue ) {
  // see section 1.2 in app note AN3192
  float heading = 180.0 * atan2 ( magValue [ Y_COORD ], magValue [ X_COORD ] ) / PI;  // assume pitch, roll are 0
  
  if ( heading < 0 )
    heading += 360;
  
  #if LSM303DLH_VERBOSE >= 10
    _printer->print ( "Uncorrected heading: " ); _printer->println ( heading, 3 );
  #endif
  
  return heading;
}

float LSM303DLH::getTiltHeading ( float * magValue, float * accelValue ) {

  // see appendix A in app note AN3192 
  float pitch =   asin ( accelValue [ X_COORD ] );
  float roll  = - asin ( accelValue [ Y_COORD ] / cos ( pitch ) );
  
  #if LSM303DLH_VERBOSE >= 15
    _printer->print ( "Pitch, roll (deg): " ); _printer->print ( pitch * 180.0 / PI );
    _printer->print ( ", " ); _printer->println ( roll * 180.0 / PI );
  #endif
  
  // to correct these, we rotate our coordinate system which rolls and pitches the mag vector the same way
  float theta = + roll;
  float phi   = + pitch;
  
  float x = magValue [ X_COORD ];
  float y = magValue [ Y_COORD ];
  float z = magValue [ Z_COORD ];
  
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

  float heading_deg = atan2 ( yh, xh ) * 180.0 / PI;
  if ( heading_deg < 0.0 ) heading_deg += 360.0;

  #if LSM303DLH_VERBOSE >= 15
  
  _printer->print ("\nMag vector corrected for pitch and roll: ( ");
  _printer->print ( xh, 1 );
  _printer->print ( ", " );
  _printer->print ( yh, 1 );
  _printer->print ( ", " );
  _printer->print ( zh, 1 );
  _printer->println ( " )" );
  
  float magnitude = sqrt ( xh * xh + yh * yh + zh * zh );
  _printer->print ("Mag vector corrected and normalized: ( ");
  _printer->print ( xh / magnitude, 3);
  _printer->print ( ", " );
  _printer->print ( yh / magnitude, 3 );
  _printer->print ( ", " );
  _printer->print ( zh / magnitude, 3 );
  _printer->println ( " )" );
  
  #endif

  #if LSM303DLH_VERBOSE >= 10
   _printer->print ( "Corrected heading: " ); _printer->println ( heading_deg, 3 );
  #endif

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
  _printer->print ( "Magnitude of acceleration: " ); _printer->println ( magnitude_accel, 3 );
  for ( i = 0; i < 3; i++ ) {
    dot [ i ] /= magnitude_accel;
  }
  
  #if 1
  
  _printer->print ("\nDot product: ( ");
  for (i = 0; i < 3; i++) {
    if ( i > 0 ) _printer->print ( ", " );
    _printer->print ( dot [ i ], 2 );
  }
  _printer->println ( " )" );
   
  #endif
  
  // now project heading into horizontal plane
  for ( i = 0; i < 3; i++ ) {
    projectedMag [ i ] = float ( magValue [ i ] ) - dot [ i ];
  }
  
  #if 1
  
  _printer->print ("\nProjected magnetic vector: ( ");
  for (i = 0; i < 3; i++) {
    if ( i > 0 ) _printer->print ( ", " );
    _printer->print ( projectedMag [ i ], 2 );
  }
  _printer->println ( " )" );
   
  #endif
  
  
  float heading = 180 * atan2 ( projectedMag [ 1 ], projectedMag [ 0 ] ) / PI;

  return ( heading );
  
  */
  
}

//************************************************************************************************
// 						                             basic subroutines
//************************************************************************************************



byte LSM303DLH::read ( int device_address, byte register_address ) {

  byte data;
  
  #if LSM303DLH_VERBOSE >= 30
    _printer->print ( "R: d " );
    printHex ( device_address, 2 );
    _printer->print ( "; r " );
    printHex ( register_address, 2 );
  #endif

  Wire.beginTransmission ( device_address );
  Wire.write ( register_address );
  Wire.endTransmission ();
  
  #if LSM303DLH_VERBOSE >= 30
    _printer->print ( " <== " );
  #endif

  Wire.requestFrom ( device_address, 1 );
  while ( ! Wire.available() ) ;
  data = Wire.read ();
  
  #if LSM303DLH_VERBOSE >= 30
    printHex ( data, 2 );
    _printer->println ();
  #endif

  return data;
}

void LSM303DLH::write ( byte data, byte device_address, byte register_address ) {
  #if LSM303DLH_VERBOSE >= 30
    _printer->print ( "W: d " );
    printHex ( device_address, 2 );
    _printer->print ( "; r " );
    printHex ( register_address, 2 );
    _printer->print ( " <== " );
    printHex ( data, 2 );
    _printer->println ();
  #endif
  
  Wire.beginTransmission ( device_address );
    
  Wire.write ( register_address );
  Wire.write ( data );
  Wire.endTransmission ();
  
}
