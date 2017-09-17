/*
  ANSI.h
  ANSI screen control v0.0.1
  Charles B. Malloch, PhD   2015-03-05
*/

#ifndef ANSI_h
#define ANSI_h

const char *version();
void clr2EOL ();
void cls ();
void curpos ( int row, int col );

#endif
