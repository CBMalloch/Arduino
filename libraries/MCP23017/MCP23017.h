/*
	MCP23017.h - library for using a MCP23017 serial port extender
	Created by Charles B. Malloch, PhD, May 7, 2009
	Released into the public domain
*/

#ifndef MCP23017_h
#define MCP23017_h

#define MCP32017_VERSION "1.001.000"

#include <I2C.h>

class MCP23017 {
	public:
		MCP23017();
		MCP23017(int addressLines);
		
		void TX (byte regadd, byte tx_data);
		int  RX (byte regadd, int requestedbytecount, byte *buf);
		
		#define		IOCON		  0x0A		// MCP23017 Config Reg.
		
		#define		IODIRA		0x00		// MCP23017 address of I/O direction
		#define		IODIRB		0x10		// MCP23017 1=input
		
		#define		IPOLA			0x01		// MCP23017 address of I/O Polarity
		#define		IPOLB			0x11		// MCP23017   1 = Inverted
		
		#define		GPIOA			0x09		// MCP23017 address of GP Value
		#define		GPIOB			0x19		// MCP23017 address of GP Value
		
		#define		GPINTENA	0x02		// MCP23017 IOC Enable
		#define		GPINTENB	0x12		// MCP23017 IOC Enable
		
		#define		INTCONA		0x04		// MCP23017 Interrupt Cont 
		#define		INTCONB		0x14		// MCP23017   1 = compare to DEFVAL(A or B) 0 = change
		
		#define		DEFVALA		0x03		// MCP23017 IOC Default value
		#define		DEFVALB		0x13		// MCP23017   if INTCONA set then INT. if diff. 
		
		#define		GPPUA			0x06		// MCP23017 Weak Pull-Ups
		#define		GPPUB			0x16		// MCP23017   1 = Pulled HIgh via internal 100k
		
	
	protected:
		// anything that needs to be available only to:
		//    the class itself
		//    friend functions and classes
		//    inheritors
		// this-> needed only for disambiguation

		// specific to I/O expander
		void initialize (int addressLines);
		int _address;		
		
	private:

};

#endif
