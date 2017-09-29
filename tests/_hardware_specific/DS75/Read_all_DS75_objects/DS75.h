/*
	cbmDS75.cpp - library to communicate to I2C DS75 thermometers
	Created by Charles B. Malloch, PhD, January 15, 2010
	Released into the public domain
*/

#ifndef cbmDS75_h
#define cbmDS75_h

#define DS75_VERSION "0.001.000"

typedef unsigned char byte;

class cbmDS75 {

	public:
		cbmDS75();
		void init();
		void init(byte address, byte resolution, bool enabled);
		
		// static makes functions *class* functions rather than *object* functions
		static byte generateDeviceAddress (byte addressLines);

		void read (char *buffer);
		
		byte addressLines;
		byte deviceAddress;
		byte resolution;
		bool enabled;
	
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> needed only for disambiguation
		
	private:
		#define TRegPtr 0x00
		#define CRegPtr 0x01
		#define CFaultTolerance 0
		#define CPolarity 0
		#define CThermostatMode 0
		#define CShutdown 0
		int configValue;
		int conversionTime;
		static char buffer[40];
};

#endif
