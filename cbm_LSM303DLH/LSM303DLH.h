/*
	LSM303DLH.cpp - library to use a LSM303DLH tilt-compensated compass
	Created by Charles B. Malloch, PhD, June 6, 2013
	Released into the public domain
	
	Sensors on board the compass:
    3-axis accelerometer
    3-axis magnetometer
    thermometer
	
	NOTE: ESP-01 needs SDA and SCL pin specification for init
	
*/

#ifndef LSM303DLH_h
#define LSM303DLH_h

#define LSM303DLH_VERSION "1.002.000"

#include <Arduino.h>
#include <Wire.h>

#define X_COORD 0
#define Y_COORD 1
#define Z_COORD 2

class LSM303DLH {

	public:
		LSM303DLH ( Print &print = Serial );
    void init ( int acc_scale = 2, int SA0_bridge_value = 0,
                int pdSDA = -1, int pdSCL = -1 );
    
    // void setAccelScale ( int scale );
        
    void getAccel ( float * _acceleration_vector_components_g );
    void getMag ( float * _magnetic_field_components_gauss );

    float getHeading(float * magValue);
    float getTiltHeading(float * magValue, float * accelValue);
    
    void debug ( Stream& out );
    
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> needed only for disambiguation
		
    int SA0_bridge_value;
    byte read ( int device_address, byte register_address );
    void write ( byte data, byte device_address, byte register_address );
    
	private:
	
		Print* _printer;
		byte _acc_scale_bits;
	  const float _acc_scales [ 4 ] = { 1.0, 2.0, 0.0, 3.9 };
		float _acceleration_vector_components_g [ 3 ];
		float _magnetic_field_components_gauss [ 3 ];
};

#endif
