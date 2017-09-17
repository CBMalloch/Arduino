#undef int()
#include <stdio.h>
#include <math.h>
#include <EWMA.h>
#include <FormatFloat.h>

EWMA myEWMA;

void setup() {
  Serial.begin(19200);       // start serial for output
  
  /*  quote from EWMA.h
  -ln(0.5) =. 0.693147, so
  
    factor = -ln(0.5)/nPeriods
    so for 10 periods, factor = 0.0693147; for 100, 0.00693147
  */
  
  // 2011-10-17 cbm 
  myEWMA.init(myEWMA.periods(40));  // 0.693147 / 40.0);
  
  #define bufLen 80
  #define fbufLen 14
  char strBuf[bufLen], strFBuf[fbufLen];
  double val;
  int frac;
  double results[2];
  
  for (int i = 0; i < 10; i++) {
    val = myEWMA.record(800.0);
    formatFloat(strFBuf, fbufLen, val, 2);
    snprintf (strBuf, bufLen, "%6d %6d -> %s\n", i, 800, strFBuf);
    Serial.print(strBuf);
    
    /*
    myEWMA.results(results);
    sprintf (strBuf, "                        %6d  /  %s\n",
             int(results[0]), formatF(results[1]));
    Serial.print(strBuf);
    */
    
    delay(50);
  }
  for (int i = 0; i < 200; i++) {
    val = myEWMA.record(0.0);
    formatFloat(strFBuf, fbufLen, val, 2);
    snprintf (strBuf, bufLen, "%6d %6d -> %s\n", i, 800, strFBuf);
    Serial.print(strBuf);
    
    /*
    myEWMA.results(results);
    sprintf (strBuf, "                        %6d  /  %s\n",
             int(results[0]), formatF(results[1]));
    Serial.print(strBuf);
    */

    delay(500);
  }
  
}

void loop() {
  int pin = analogRead(3);
  
}
