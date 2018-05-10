#ifndef FormatFloat_h
#define FormatFloat_h

// can define this after the include statement in the main program for debugging
// but that doesn't work!
#define FFDEBUG
// it also doesn't work here...
// but it does work directly in the .cpp file.



/*
  There is a known compiler bug that now causes this code to throw this error:
    Unable to find a register to spill in class 'NO_REGS'
  But there is a much better substitute anyways:
    char * dtostrf(
      double __val,
      signed char __width,
      unsigned char __prec,
      char * __s)
    <http://www.microchip.com/webdoc/AVRLibcReferenceManual/group__avr__stdlib_1ga060c998e77fb5fc0d3168b3ce8771d42.html>
  Note: this is not safe regarding buffer overrun. __width is described as
    the *minimum* width...

*/

#define FORMAT_FLOAT_VERSION 1.000001
// v1.000001 made precision argument optional

void formatFloat(char buf[], int bufSize, double x, int precision = 2);

#endif
