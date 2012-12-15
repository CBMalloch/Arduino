/*
	I2C.cpp - library to facilitate I2C communications
	Created by Charles B. Malloch, PhD, May 7, 2009
	Released into the public domain
*/

#ifndef cbmI2C_h
#define cbmI2C_h

#define I2C_VERSION "1.001.000"

typedef unsigned char byte;

class cbmI2C {

	public:
		cbmI2C();
		
		void init();
		// static makes functions *class* functions rather than *object* functions
		static byte deviceAddress (byte deviceType, byte addressLines);

		static void TX (byte device, byte regadd, byte tx_data);
		static int  RX (byte device, byte regadd, int requestedbytecount, byte *buf);
	
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> needed only for disambiguation
		
	private:

};

extern cbmI2C I2C;

#endif
