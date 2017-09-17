#ifndef FormatFloat_h
#define FormatFloat_h

// can define this after the include statement in the main program for debugging
// this doesn't work!
#undef FFDEBUG

#define FORMAT_FLOAT_VERSION 1.000001
// v1.000001 made precision argument optional

void formatFloat(char buf[], int bufSize, double x, int precision = 2);

#endif
