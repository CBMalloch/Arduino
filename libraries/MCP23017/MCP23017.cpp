/*
	MCP23017.h - library for using a MCP23017 serial port extender
	Created by Charles B. Malloch, PhD, May 7, 2009
	Released into the public domain
	
	Delays are found only in the initialization routine, in which there are 10,
	for a total delay of 500 ms
	
*/

#include <Arduino.h>
#include <wiring.h>
#include "MCP23017.h"

#define dtMCP23017 0x04             // device type: I/O expander with serial interface
#define MCP23017_Delay 60

//************************************************************************************************
// 						                       I/O Expander subroutines 
//************************************************************************************************

MCP23017::MCP23017 () {
}

MCP23017::MCP23017 (int addressLines) {
	I2C.init();
	initialize (addressLines);
}

void MCP23017::initialize (int addressLines) {
  _address = I2C.deviceAddress(dtMCP23017, addressLines);
  
  /*
    RESET VALUES ARE ALL ZEROES!!!!!
    That means we've started up in IOCON.BANK = 0 mode
    So we first need to set bank = 1:
      BANK            = 1
      MIRROR          = 0
      SEQOP           = 1 (1 = do NOT increment)
      DISSLW          = 0 (0 = slew rate enabled)
      HAEN            = 1 (1 = enable address pins (ALWAYS enabled on MCP23017))
      ODR             = 0 (0 = active driver outputs)
      INTPOL          = 0 (0 = active low if not open-drain)
      <unimplemented> = 0
    IOCON = B10101000
  */
  
  TX(IOCON, 0xa8);  // B10101000
  delay(MCP23017_Delay);
  
  // ------------------------------
  // Group A (outputs to LCD)
  // ------------------------------
  
  //  I/O direction
  TX(IODIRA, 0x00);  // B00000000
  delay(MCP23017_Delay);
  
  //   polarity - don't invert
  TX(IPOLA, 0x00);  // B00000000
  delay(MCP23017_Delay);
  
  //   make output all zeros
  TX(GPIOA, 0x00);  // B00000000
  delay(MCP23017_Delay);
  
  // ------------------------------
  // Group B (inputs from switches and command outputs to LCD)
  // ------------------------------
 
  //  5 switch bits set for input; command outputs
  TX(IODIRB, 0xf8);  // B11111000
  delay(MCP23017_Delay);
  
  //   all switch bits set to invert (switch on brings input low)
  TX(IPOLB, 0xf8);  // B11111000
  delay(MCP23017_Delay);
  
  //  all pullups on
  TX(GPPUB, 0xf8);  // B11111000
  delay(MCP23017_Delay);

  //  all default values zero
  TX(DEFVALB, 0x00);  // B00000000
  delay(MCP23017_Delay);

  //  set to *use* default values for interrupt
  TX(INTCONB, 0x00);  // B00000000
  delay(MCP23017_Delay);
  
  // IOC
  TX(GPINTENB, 0x00);  // B00000000
  delay(MCP23017_Delay);

}

void MCP23017::TX (byte regadd, byte tx_data)
{
	I2C.TX (_address, regadd, tx_data);
}

int  MCP23017::RX (byte regadd, int requestedbytecount, byte *buf)
{
	return I2C.RX(_address, regadd, requestedbytecount, buf);
}


