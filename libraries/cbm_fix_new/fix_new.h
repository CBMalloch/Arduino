
/* I was having problems with new. The web pages at 
http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1218306702
and
http://arduinoetcetera.blogspot.com/2010/04/implementing-new-and-delete-c.html
gave me this code, which fixes the problem...
*/

// BEGIN FIX  **************************************************************

#ifndef fix_new_h
#define fix_new_h

#define FIX_NEW_VERSION "1.000.000"

#include <stdlib.h>

/*
__extension__ typedef int __guard __attribute__((mode (__DI__)));

int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
void __cxa_guard_abort (__guard *) {};

*/

void * operator new(size_t size);
void operator delete(void * ptr);

void * operator new(size_t size)
{
  return malloc(size);
}

void operator delete(void * ptr)
{
  free(ptr);
}

#endif

// END FIX **************************************************************
