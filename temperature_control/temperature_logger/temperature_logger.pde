#define VERSION "0.2.0"
#define VERDATE "2011-07-15"
#define PROGMONIKER "TLO"

/*
  temperature logger
  read 3 thermistors
  display thermistor temperatures on new Sparkfun 7-seg 4-digit display
  for now:
   log results onto data logger shield --> now frozen as temperature_logger
  later:
    POE
    send a UDP packet with the status every 10 seconds
    
  given voltage divider 5V -- Rf -mm- Rt -- gnd, Rf is fixed resistor, mm is measurement point, Rt is thermistor,
  we have counts / 1024 = 5V * (Rt / (Rf + Rt)). Given the temperature span from, say, 25degC (77degF)
  to 40degC (104degF), the span of mm would be s = (Rt+ / (Rf + Rt+) - (Rt- / (Rf + Rt-)).
  Given also that Rt+ is about 5K and Rt- is about 10K, we have (using resistance units of 1K)
  that s = -5Rf / (Rf**2 + 15Rf + 50), which is maximized at about 0.7K.
  So we choose  resistors of 6.8K nominal for Rf.
  We measure the actual resistances for more accuracy.
*/

#include <stdio.h>
#include <NewSoftSerial.h>
#include <math.h>
#include <SD.h>
// note that I2C (Wire.h) uses by default analog pins 4 and 5
#include <Wire.h>
#include "RTClib.h"

#include "EWMA.h"

// newSoftSerial
#define nssRX 2
#define nssTX 3

#define LEDreset 4

/* logging:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11 on Arduino Uno/Duemilanove/Diecimila
 ** MISO - pin 12 on Arduino Uno/Duemilanove/Diecimila
 ** CLK - pin 13 on Arduino Uno/Duemilanove/Diecimila
 ** CS - depends on your SD card shield or module
*/

#define pinCS   10
#define pinMOSI 11
#define pinMISO 12
#define pinCLK  13


#define aref_voltage 5.0
#define fullScaleCounts 1023
#define zeroC 273.15
#define T0 (25.0 + zeroC)
#define R0 10000.0
#define B 3380.0

#define numberDuration  400
#define updateInterval 2000
unsigned long tLastUpdate = 0;
int displayItem = 0;  // 0 means display the number; 1 means display the temperature
int nUpdate = 2;

#define recordingInterval 30000L
unsigned long tLastRecording = 0;

// double resistors[] = {6772.3, 6764.3, 6795.3};

double resistors[] = {6774.8  // 6772.3
                     ,6765.9  // 6764.3
                     ,6797.5  // 6795.3
                     };


NewSoftSerial mySerial(nssRX, nssTX);
EWMA smoothX[3];

RTC_DS1307 RTC;

String formatFloat(double x, int precision = 2);
char strBuf[61];

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void setup() {
  Serial.begin(19200);
	snprintf(strBuf, 60, "%s: Temperature Logger v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);

  // set the data rate for the NewSoftSerial port
  mySerial.begin(9600);
  
  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    Serial.println("RTC not running!");
  }

  
  pinMode(pinCS, OUTPUT);     // for SD card - chip select
  // see if the card is present and can be initialized:
  if (!SD.begin(pinCS)) {
    Serial.println("SD failed");
    // don't do anything more:
    return;
  }

  // reset the display
  pinMode(LEDreset, OUTPUT);
  digitalWrite(LEDreset, LOW);
  delay(10);
  digitalWrite(LEDreset, HIGH);
  // 55 is too short; 60 works reliably
  delay(60);
  
  for (int i = 0; i < 3; i++) {
    smoothX[i].init(0.0693147 * 0.1);  // 0.0693147 smooths over 10 periods; so this over 100 periods
  }
  
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void loop() {
  String dataString;

  unsigned int reading;
  float degF, thermistorResistance;
  
  DateTime now = RTC.now();
  
  for (int n = 0; n < 3; n++) {
    reading = analogRead(n);
    thermistorResistance = resistors[n] * reading / (fullScaleCounts - reading);
    degF = K2F(T0 * B / (T0 * log(thermistorResistance / R0) + B));  // log is base e...
    smoothX[n].record(degF);
  }
  
  if (displayItem == 0) {  // display the appropriate item number
    if ((millis() > tLastUpdate + (updateInterval - numberDuration))
        || (millis() < tLastUpdate)) {
      tLastUpdate = millis();
      displayItem = 1;
      
      // update display
      nUpdate++;
      if (nUpdate > 2) { nUpdate = 0; }
        // remove decimal point
        mySerial.print(char(0x77)); mySerial.print(char(0x00)); // delay(60);
        send((unsigned long) (0x78787830L + nUpdate));  // three blanks and a zero + nUpdate
    }

  } else {
    // display item is 1; display the appropriate temperature
    if ((millis() > tLastUpdate + (numberDuration)) || (millis() < tLastUpdate)) {
      tLastUpdate = millis();
      displayItem = 0;
    
      // decimal point
      mySerial.print(char(0x77)); mySerial.print(char(0x04)); // delay(60);
      int digits = int(smoothX[nUpdate].value() * 10.0 + 0.5);
      send(i2s(digits));
               
      // now.unixtime is seconds since epoch of 1970-01-01
      dataString = String(now.unixtime()) + '\t';
      dataString += formatFloat(smoothX[0].value()) + '\t';
      dataString += formatFloat(smoothX[1].value()) + '\t';
      dataString += formatFloat(smoothX[2].value()) + '\n';

      Serial.print (dataString);
      
    }
  }
  
  if ((millis() > (tLastRecording + recordingInterval)) || (millis() < tLastRecording)) {
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    
    File dataFile = SD.open("crockpot.txt", FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.print(dataString);
      dataFile.close();
    } else {
      Serial.println("error opening crockpot.txt");
    }
  }
  
  delay (10);

}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void send(unsigned int x) {
  for (int i = 0; i < 4; i++) {
    uint8_t c = x >> ((3 - i) * 4) & 0x0f;
    if (c < 0x0a) {
      mySerial.print(char(0x30 + c));
    } else {
      mySerial.print(char(0x41 + (c - 10)));
    }
  }
}

void send(unsigned long x) {
  for (int i = 0; i < 4; i++) {
    uint8_t c = x >> ((3 - i) * 8) & 0xff;
    mySerial.print(char(c));
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

unsigned int i2s (int k) {
  unsigned int s = 0;
  for (int i = 0; i < 4; i++) {
    uint8_t c = k % 10; k /= 10;
    s += c << (i * 4);
  }
  return(s);
}
  
// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

float K2F(float K) {
  // Serial.print("      ");
  // Serial.print(int(K));
  // Serial.print("  ->  ");
  // Serial.println(int((K - zeroC) * 1.8 + 32.0));
  return((K - zeroC) * 1.8 + 32.0);
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

String formatFloat(double x, int precision) {
  char result[16];
  char theFormat[16];
  char sign[2];
  double rounder;
  unsigned long whole;
  int frac;
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
  x = x + rounder;
  whole = long(x);
  x -= float(whole);
  for (int i = 0; i < precision; i++) {
    x *= 10.0;
  }
  frac = int(x);
  snprintf (theFormat, 16, "%%s%%ld.%%0%dd", precision);
  
  // Serial.print ("frac   :"); Serial.println(frac);
  // Serial.print ("format :"); Serial.println(theFormat);
  
  snprintf(result, 16, theFormat, sign, whole, frac);
  // Serial.print("result            :"); Serial.println(result);
  return String(result);
}
