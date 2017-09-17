/*
	LSM303DLHC.cpp - library to use a LSM303DLHC tilt-compensated compass
	Created by Charles B. Malloch, PhD, June 6, 2013
	Released into the public domain
	
	Sensors on board the compass:
    3-axis accelerometer
    3-axis magnetometer
    thermometer
	
*/

#ifndef LSM303DLHC_h
#define LSM303DLHC_h

#define LSM303DLHC_VERSION "1.001.000"

#include <Arduino.h>
#include <Wire.h>

#define X_COORD 0
#define Y_COORD 1
#define Z_COORD 2

class LSM303DLHC {

	public:
		// LSM303DLHC         ();
		LSM303DLHC         ();
    void init          ( int scale );
    
    // void setAccelScale ( int scale );
        
    void getAccel(int * rawValues);
    void getMag ( int * rawValues );
    void getTemp ( int * pTemp );

    float getHeading(float * magValue);
    float getTiltHeading(float * magValue, float * accelValue);
    
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> needed only for disambiguation
		
    byte read ( int device_address, byte register_address );
    void write ( byte data, byte device_address, byte register_address );
    
	private:
		
		
};

#endif
