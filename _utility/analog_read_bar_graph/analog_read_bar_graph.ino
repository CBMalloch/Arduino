/* Analog read one or more inputs
  Charles B. Malloch, PhD  2012-11-29
  v1.3
*/

#define BAUDRATE 115200
#define lineLen 80
#define bufLen (lineLen + 3)
char strBuf[bufLen];

unsigned short nAnalogsToRead, paAnalogsToRead[] = {0, 1}; //, 3, 4, 5};

void setup() {
  Serial.begin (BAUDRATE);
  nAnalogsToRead = sizeof(paAnalogsToRead) / sizeof(unsigned short);
}

void loop() {
  unsigned short i, pa, reading, barLen;
  for ( i = 0; i < nAnalogsToRead; i++) {
    pa = paAnalogsToRead[i];
    snprintf (strBuf, bufLen, "%1d : ", pa);
    Serial.print (strBuf);
    reading = analogRead(pa);
    barLen = map (reading, 0, 1023, 0, lineLen);
    printBar (barLen);
  }
  Serial.println ();
  // size the delay by the length of time it will take to create it
  delay (50 * nAnalogsToRead);
}

void printBar (unsigned short barLen) {
  barLen = constrain (barLen, 0, lineLen);
  for (int i = 0; i < barLen; i++) strBuf[i] = '-';
  strBuf[barLen] = '*';
  for (int i = barLen + 1; i <= lineLen; i++) strBuf[i] = ' ';
  strBuf[bufLen - 2] = '|';
  strBuf[bufLen - 1] = '\0';
  Serial.println (strBuf);
}