#ifndef PRINTHEX_h
#define PRINTHEX_h

// #include <HardwareSerial.h>
#include <Arduino.h>

#undef PRINTHEX_VERBOSE
#define PRINTHEX_VERSION "0.002.000"

// class PrintHex {

	// public:
    // PrintHex ( HardwareSerial& print );
    // PrintHex ();
    
    void convertHex ( char* destBuf, int lenDestBuf, unsigned long value, int nBytes = 2 );
    void printHex ( unsigned long value, int nBytes = 2, Stream& out = Serial );
    void hexDump ( byte * theBuf, int theLen, int lineLen = 0x20, Stream& out = Serial );
    void serialHexDump ( byte theByte, int lineLen = 0x20, Stream& out = Serial );
    
  // private:
    // HardwareSerial& printer;
    
// };

#endif
