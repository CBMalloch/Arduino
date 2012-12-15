#include <stdio.h>
#include <FormatFloat.h>

#define BAUDRATE 115200

#define bufLen 80
#define fbufLen 14
char strBuf[bufLen], strFBuf[fbufLen];

double valD;
float valF;

void setup() {
  Serial.begin(BAUDRATE);
  int i;
  for (i = 0; i < 14; i++) {
    valF = 137.67429;
    formatFloat(strFBuf, fbufLen, valF, i);
    snprintf (strBuf, bufLen, "%6d %6d -> %s\n", i, 800, strFBuf);
    Serial.print(strBuf);
  }
  for (i = 0; i < 14; i++) {
    valD = 276.67429;
    formatFloat(strFBuf, fbufLen, valD, i);
    snprintf (strBuf, bufLen, "%6d %6d -> %s\n", i, 800, strFBuf);
    Serial.print(strBuf);
  }
}

void loop() {
}

#define FFDEBUG
