#include <stdio.h>


// #define FFDEBUG 3
#ifdef FFDEBUG
  #include <Arduino.h>
#endif




void formatFloat(char buf[], int bufSize, double x, int precision) {
  /*
    puts into buf a left-justified string with 
      "-" or "" for sign and then the number with precision decimal places
    note that snprintf prints at most bufSize chars *including* the terminating null!
  */
  char theFormat[16];
  char sign[2];
  double rounder;
  unsigned long whole;
  unsigned long frac;
  rounder = 0.5;
  snprintf(theFormat, 16, "  ");
  snprintf (sign, 2, "-");
  if (x < 0) {
    x = -x;
  } else {
    snprintf (sign, 2, "");
  }
  for (int i = 0; i < precision; i++) {
    rounder /= 10.0;
  }
  #ifdef FFDEBUG  
    Serial.println();
    Serial.print ("rounder (x1e6)   :"); Serial.println(rounder * 1e6);
  #endif
  x = x + rounder;
  whole = long(x);
  #ifdef FFDEBUG
    Serial.print ("1) x (x10e6)     :"); Serial.println(x * 1e6);
    Serial.print ("   whole         :"); Serial.println(whole);
  #endif
  x -= float(whole);
  #ifdef FFDEBUG
    Serial.print ("2) x (x1e6)      :"); Serial.println(x * 1e6);
  #endif
  for (int i = 0; i < precision; i++) {
    x *= 10.0;
  }
  frac = long(x);
  #ifdef FFDEBUG
    Serial.print ("3) x             :"); Serial.println(x);
    Serial.print ("   frac          :"); Serial.println(frac);
  #endif 
  snprintf (theFormat, 16, "%%s%%ld.%%0%dlu", precision);  // format string is 10 or 11 chars
  // results in "%s%ld.%0<precision>lu"
  #ifdef FFDEBUG
    Serial.print ("frac   :"); Serial.println(frac);
    Serial.print ("format :"); Serial.println(theFormat);
  #endif
  snprintf(buf, bufSize, theFormat, sign, whole, frac);
}
