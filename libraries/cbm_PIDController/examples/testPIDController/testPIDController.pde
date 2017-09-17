#define VERSION "1.1.0"
#define VERDATE "2011-10-02"
#define PROGMONIKER "TPC"

/*

  Test PID controller
    
*/

#define VERBOSE 6

// ===================
// Arduino connections
// ===================

// analog pins
#define pinAThermistor 0

// digital pins

// setpoint buttons
#define pinSetpointDown 6
#define pinSetpointUp 7

#define pinHeater 8
#define pinLED 9
// logging: SD card attached to SPI bus
#define pinCS   10
#define pinMOSI 11
#define pinMISO 12
#define pinCLK  13


// ============
// housekeeping
// ============

#define baudRate 115200
#define bufLen 120
char strBuf[bufLen+1];

// includes

#include <stdio.h>
#include <fix_new.h>
#include <math.h>

#include <EWMA.h>
#include <Thermosensor.h>
#include <PWMActuator.h>
#include <PIDController.h>

unsigned long tBlink = 0;

// ===================
// control of setpoint
// ===================

#define tempUnits 'F'
// range is in temp units defined above
// Amy states range should be 4 degC to 50 degC - go 0 to 60 degC then
//  which is 32 to 140 degF, so for F go 20 to 140
#define tempRangeMin 20.0
#define tempRangeMax 140.0

float setpointDegreesF;
unsigned long setpointButtonStart, setpointButtonLastTick, setpointButtonDuration;
int setpointButtonStatus; // -1 down; 0 none; +1 up
#define nSetpointSteps 3
float setpointStepSizes[] = {0.1, 1.0, 10.0};
#define setpointButtonStepTimems 250
#define setpointRepsBeforeNextSizeStep 10
int setpointStepSizeCounter, setpointStepPointer;

// ============================================
// definition of temperature measurement device
// ============================================

// we are using a muRata NTC thermistor NTSD1XH103FPB

#define T0 (25.0 + zeroC)
#define R0 10000.0
#define B 3380.0

// =========================
// constants for PWM control
// =========================

// for SSR, use 2 seconds; for mechanical relay, use 30 seconds

#define kPWMPeriodms 30000UL

// ===============================
// constants of PID control tuning
// ===============================

// defaults 5.0, 100, and 20 respectively
// autotuned to 0.7, 966, and 241 respectively
// souf = 0.2 (overshoot suppression coefficient); ot = 2 (control period sec); filt = 0 (digital filtering strength);

// settings copied from autotuned JLD612 temperature controller
// settings 0089
// inty = P10.0; outy = 2 (control temperature); hy = 0.3; psb = 5.9 (temp sensor error correction); rd = 0 (heating); corf = 1 (Fahrenheit)
// settings 0036

// settings calculated from crockpot run

// coefficients in terms of degK, seconds
// float coeff_K_c = -0.055; // -0.04;
// float coeff_tau_I = 56.0; // 56.0;  // NOTE that tau_I occurs in the DENOMINATOR!
// float coeff_tau_D = 8.7; // 8.7;

// from crockpot values
float coeff_K_c = 0.04;
float coeff_tau_I = 3000.0;  // NOTE that tau_I occurs in the DENOMINATOR!
float coeff_tau_D = 500.0;  // too big a D really makes the valueD bounce badly


double resistors[] = {
                        // 6714.1  // 6710.7 ohms (used by original setup)
                        6797.5  // 6795.3 ohms (used by current wingshield setup)
                     };
       
// =====================
// definition of objects
// =====================

EWMA smoothX;
Thermistor *thermistor;
PWMActuator *heater;
PIDController *controller;

// timers

unsigned long msNow;

String formatFloat(double x, int precision = 2);

//************************************************************************************************
// 						                    Standard setup and loop routines
//************************************************************************************************

void setup() {
  Serial.begin(baudRate);
	snprintf(strBuf, bufLen, "%s: Temperature Controller v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);

  pinMode(pinLED, OUTPUT);
  pinMode(pinHeater, OUTPUT);
  
  // setpoint
  pinMode(pinSetpointDown, INPUT);
  digitalWrite(pinSetpointDown, 1);
  pinMode(pinSetpointUp, INPUT);
  digitalWrite(pinSetpointUp, 1);
  setpointButtonStatus = 0;
  setpointDegreesF = 77.0;
  setpointStepSizeCounter = 0;

  thermistor = new Thermistor(pinAThermistor, resistors[0], R0, T0, B);
  heater = new PWMActuator(pinHeater, kPWMPeriodms, 0.25);
  controller = new PIDController(
                                  heater,
                                  coeff_K_c, coeff_tau_I, coeff_tau_D
                                );
  
  smoothX.init(smoothX.periods(1000)); 
   
}  // end setup

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void loop() {
  
  unsigned int counts;
   
  msNow = millis();

  // =============================
  // handle adjustment of setpoint
  // =============================
  
  if (! digitalRead(pinSetpointDown)) {
    setpointChange(-1);
  } else if (! digitalRead(pinSetpointUp)) {
    setpointChange(1);
  } else {
    if (setpointButtonStatus != 0 && VERBOSE) {
      snprintf (strBuf, bufLen, "At %lu: new setpoint: %d deciDeg\n",  
        msNow, 
        int(setpointDegreesF * 10.0));
      Serial.print (strBuf);
    }
    setpointButtonStatus = 0;
    setpointStepPointer = 0;
    setpointStepSizeCounter = 0;
    // all temperatures to controller must be in degK
    controller->setpoint((setpointDegreesF - 32.0) * 5 / 9 + 273.15);
  }

  // ===================
  // measure and control
  
  doControl ();
  doDisplay ();

  // housekeeping
  
  if (( msNow > (tBlink + 1000)) ||  msNow < tBlink) {
    tBlink =  msNow;
    digitalWrite(pinLED, 1 - digitalRead(pinLED));
  }
  
  delay (100);
}  // end loop

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->
// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void doControl () {

  float degK;
  
  degK = thermistor->getDegK();  // log is base e...
  // ah. for testing, we might want to ignore thermistor
  if (0) {
    degK = float(map(analogRead(pinAThermistor), 0, 1024, 
                     int(((float(tempRangeMin) - 32.0) * 5.0 / 9.0 + 273.15) * 100.0), 
                     int(((float(tempRangeMax) - 32.0) * 5.0 / 9.0 + 273.15) * 100.0))
                ) / 100.0;
  }
  
  smoothX.record(degK);
  controller->doControl(smoothX.value());
  heater->enable();
  
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void doDisplay () {
  
  String dataString;
  msNow = millis();
  static unsigned long tLastPrinting = 0UL;
  
  float controllerSetpointDegrees, currentError, errorIntegral, valueP, valueI, valueD, dutyCycle;
  float coeff_K_c, coeff_tau_I, coeff_tau_D;
  
  if (millis() > tLastPrinting + 2000UL) {
  
    controller->get_internal_variables(                          // NOTE: all temps in degK
                                        controllerSetpointDegrees, 
                                        currentError, errorIntegral, 
                                        valueP, valueI, valueD, 
                                        dutyCycle,
                                        coeff_K_c, coeff_tau_I, coeff_tau_D
                                      );
    snprintf (strBuf, bufLen, "At %lu: temp:avg/setpt: %d:%d/%d dDegF; PWM %d:%d pct; error: %d dDegF; e_integral: %d dDegF\n",  
      msNow, 
      int(thermistor->K2F(thermistor->getDegK()) * 10.0), int(thermistor->K2F(smoothX.value()) * 10.0),
      int(setpointDegreesF * 10.0),
      int(heater->dutyCycle() * 100.0), int(dutyCycle * 100.0),
      int(currentError * 10.0 * 1.8),
      int(errorIntegral * 10.0));
    Serial.print (strBuf);
    snprintf (strBuf, bufLen, "        valueP: %ld/100; valueI: %d/100; valueD: %d/100\n",  
      long(valueP * 100.0), int(valueI * 100.0), int(valueD * 100.0));
    Serial.print (strBuf);
    snprintf (strBuf, bufLen, "        coeff_K_c: %ld/100; coeff_tau_I: %d/10; coeff_tau_D: %d/10\n",  
      long(coeff_K_c * 100.0), int(coeff_tau_I * 10.0), int(coeff_tau_D * 10.0));
    Serial.print (strBuf);
    snprintf (strBuf, bufLen, "        heater: duty cycle pct: %d; duty cycle ms: %lu; PWM period ms: %lu\n",
      int(heater->dutyCycle() * 100.0), 
      heater->dutyCyclems(), 
      heater->PWMPeriodms() );
    Serial.print (strBuf);
    tLastPrinting = millis();
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->
// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void setpointChange (int dir) {
  // dir -1 for down
  if (setpointButtonStatus == 0) {
    setpointButtonStart = millis();
    setpointButtonStatus = dir;
    setpointButtonLastTick = millis() - setpointButtonStepTimems;
  }
  setpointButtonDuration = millis() - setpointButtonStart;
  if (millis() >= setpointButtonLastTick + setpointButtonStepTimems) {
    if (setpointStepSizeCounter >= setpointRepsBeforeNextSizeStep) {
      setpointStepPointer = constrain(setpointStepPointer + 1, 0, nSetpointSteps - 1);
      setpointStepSizeCounter = 0;
    }
    setpointStepSizeCounter++;
    setpointDegrees = constrain( setpointDegrees 
                                    + (dir * setpointStepSizes[setpointStepPointer]),
                                  tempRangeMin, tempRangeMax );
    // if (VERBOSE > 10) {
      // snprintf (strBuf, bufLen, "At %lu: Setpoint status: %d; start: %lu; duration: %lu; pointer: %d \n",  
        // msNow, 
        // setpointButtonStatus,
        // setpointButtonStart,
        // setpointButtonDuration,
        // setpointStepPointer);
      // Serial.print (strBuf);
    // }
    setpointButtonLastTick = millis();
    if (VERBOSE >= 5) {
      // snprintf (strBuf, bufLen, "At %lu: Setpoint adjusted to %d deciDeg\n",  
         // msNow, int(setpointDegrees * 10.0));
      snprintf (strBuf, bufLen, "%lu: Set to %d/10\n",  
         msNow, int(setpointDegrees * 10.0));
      Serial.print (strBuf);
    }
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

float K2F(float K) {
  // Serial.print("      ");
  // Serial.print(int(K));
  // Serial.print("  ->  ");
  // Serial.println(int((K - zeroC) * 1.8 + 32.0));
  return((K - zeroC) * 1.8 + 32.0);
}

float K2C(float K) {
  // Serial.print("      ");
  // Serial.print(int(K));
  // Serial.print("  ->  ");
  // Serial.println(int((K - zeroC) * 1.8 + 32.0));
  return(K - zeroC);
}

float tempInDesiredUnits (float degK) {
  float degrees;
  if (tempUnits == 'F') {
    degrees = K2F(degK);
  } else if (tempUnits == 'C') {
    degrees = K2C(degK);
  } else {
    degrees = degK;
  }
  return (degrees);
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

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void delayMicros (unsigned long n) {
  // Serial.print ("delayMicros\n");
  while (n > 10000L) {
    delay (10);  // milliseconds
    n -= 10000L;
    // Serial.print ("delaying 10 ms\n");
  }
  delayMicroseconds(n);
  // Serial.print ("delaying remainder\n");
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void multiFlash (int n, int ms, int dutyPct, int dDutyFlag) {
  // n flashes of duration ms milliseconds each, with duty cycle dutyPct
  // dDutyFlag -1 => start at given duty cycle, diminish to zero over the n flashes;
  //                    0 => all flashes alike
  //                    1 => start at zero duty cycle, increase to given duty cycle
  int i;
  unsigned long usOn, usOff, usStart;
  long dus = 0;
  
  // snprintf (buf, 128, "n: %5d; ms: %5d; dutyPct: %5d; dDutyFlag: %2d\n", n, ms, dutyPct, dDutyFlag);
  // Serial.print(buf);
  
  // 5 100 80 -1: msS = 80; dDF = 100 * 80 * -1 / 5 / 100 = -16 :: 0 80  1 64  2 48  3 32  4 16  5 0
  // 5 100 80 1: msS = 0; dDF = 100 * 80 * 1 / 5 / 100 = 16
  // 10 250 60 -1:
  //    usS = 250 * 60 * 10 = 150 000
  //    dus = 250 * 60 * -1 * 10 / 10 = -15 000
  if (dDutyFlag == 1) {
    usStart = 0;
  } else {
    usStart = long(ms) * long(dutyPct) * 10L;
  }
  if (dDutyFlag) {
    dus = long(ms) * long(dutyPct) * 10L * long(dDutyFlag) / long(n);
  }
  usOn = usStart;
  for (i = 0; i < n; i++) {
    usOff = long(ms) * 1000L - usOn;
    // snprintf (buf, 128, "%5d:   us on: %7lu;    us off: %7lu;      dus: %7ld\n", i, usOn, usOff, dus);
    // Serial.print(buf);
    if (i > 0 || dDutyFlag != 1) {
      digitalWrite(pinLED, HIGH);
      delayMicros (usOn);
    }
    digitalWrite(pinLED, LOW);
    delayMicros (usOff);
    usOn += dus;
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void die (int n) {
  int i;
  while (1) {
    multiFlash (500, 1, 60, -1);    // dim for 400 ms
    delay (1000);
    multiFlash (n, 600, 60, 0);     // indicate number
    delay (1000);
    multiFlash (500, 1, 60, 1);     // brighten for 400 ms
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

/*
unsigned long minutes2ms (int m) {
  return (long(m) * 60UL * 1000UL);
}
*/

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

/*
int availableMemory() {
 int size = 8192;
 byte *buf;
 while ((buf = (byte *) malloc(--size)) == NULL);
 free(buf);
 return size;
} 
*/

