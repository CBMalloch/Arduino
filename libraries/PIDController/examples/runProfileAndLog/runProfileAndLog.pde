#define VERSION "0.8.2"
#define VERDATE "2011-10-18"
#define PROGMONIKER "TCO"

/*
  temperature controller
  read 1 thermistor
  display thermistor temperature on new Sparkfun 7-seg 4-digit display
  for now (or soon):
   log results onto data logger shield --> now frozen as hot_tub_temp_logger
     
  given voltage divider 5V -- Rf -mm- Rt -- gnd, Rf is fixed resistor, mm is measurement point, Rt is thermistor,
  we have counts / 1024 = 5V * (Rt / (Rf + Rt)). Given the temperature span from, say, 25degC (77degF)
  to 40degC (104degF), the span of mm would be s = (Rt+ / (Rf + Rt+) - (Rt- / (Rf + Rt-)).
  Given also that Rt+ is about 5K and Rt- is about 10K, we have (using resistance units of 1K)
  that s = -5Rf / (Rf**2 + 15Rf + 50), which is maximized at about 0.7K.
  So we choose  resistors of 6.8K nominal for Rf.
  We measure the actual resistances for more accuracy.
  
  Notes:
    having set up a JLD612 PID temperature controller with a water bath, 
    one Norpro 559 300W CH103 immersion heater, and the 2M waterproof PT100 RTD thermocouple,
    we ran the autotune and got the following coefficients:
      P 0.7 proportional band % (default 5)
      I 966 integration time sec (default 100)
      D 241 differentiation time sec (default 20)
      Souf 0.2 overshoot suppression coefficient (damping factor) (default 0.2)
      ot 2 control period (default 2; use 2 for SSR, use maybe 5-15 for mechanical relay)
     one coefficient is not related to autotune:
      filt 0 digital filtering (for readout; probably sets an EWMA factor) (default 0)
    
    new setup: crock pot on high, thermistor 2 with its resistor, relay-based switched outlet,
      CHEAP circulator pump (which melts at 150 degF or so!)
      
    
*/

#define VERBOSE 6
#define TESTING 0

// ===================
// Arduino connections
// ===================

// analog pins
#define pinAThermistor 0

// digital pins

// note that I2C (Wire.h) uses by default analog pins 4 and 5

// LCD display
// #define pinLCDDisplayReset 4
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
#define fbufLen 14
char strFBuf[fbufLen+1];

// includes

#include <stdio.h>
#include <fix_new.h>
#include <FormatFloat.h>
// #include <NewSoftSerial.h>
#include <math.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

#include <EWMA.h>
#include <Thermosensor.h>
#include <PWMActuator.h>
#include <PIDController.h>

// ===================
// logging
// ===================

#define loggingInterval  10000UL

unsigned long tBlink = 0;

// ===================
// control of setpoint
// ===================

#define tempUnits 'F'
// range is in temp units defined above
// Amy states range should be 4 degC to 50 degC
//  which is 32 to 122 degF, so for F go 20 to 140
#define tempRangeMin 20.0
#define tempRangeMax 140.0

#define high_temperature_limit ((145.0 + 32.0) * 5.0 / 9.0 + 273.15)

float setpointDegrees;
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
double resistors[] = {
                        // 6714.1  // 6710.7 ohms (used by original setup)
                        6797.5  // 6795.3 ohms (used by current wingshield setup)
                     };

// =========================
// constants for PWM control
// =========================

// for SSR, use 2 seconds; for mechanical relay, use 30 seconds
#define kPWMPeriodms 30000UL

// ===============================
// constants of PID control tuning
// ===============================


/* settings copied from autotuned JLD612 temperature controller
  settings 0089
    inty = P10.0; outy = 2 (control temperature); 
    hy = 0.3; 
    psb = 5.9 (temp sensor error correction); 
    rd = 0 (heating); 
    corf = 1 (Fahrenheit)
  settings 0036
    defaults P, I, D: 5.0, 100, and 20 respectively
    autotuned to 0.7, 966, and 241 respectively
    souf = 0.2 (overshoot suppression coefficient); 
    ot = 2 (control period sec); 
    filt = 0 (digital filtering strength);
*/

// settings calculated from crockpot run

// coefficients in terms of degK, seconds
// these are in terms of *minutes*
// float coeff_K_c = -0.055; // -0.04;
// float coeff_tau_I = 56.0; // 56.0;  // NOTE that tau_I occurs in the DENOMINATOR!
// float coeff_tau_D = 8.7; // 8.7;

// from crockpot values

// I: 50 * 60 = 3000; D: 8 * 60 = 480
float coeff_K_c = 0.3;
float coeff_tau_I = 1000.0;  // NOTE that tau_I occurs in the DENOMINATOR!
float coeff_tau_D = 100.0;

       
// =====================
// definition of objects
// =====================

// NewSoftSerial mySerial(nssRX, nssTX);
RTC_DS1307 RTC;
EWMA smoothX;
Thermistor *thermistor;
PWMActuator *heater;
PIDController *controller;

// timers

DateTime unixNow;
unsigned long msNow;

int step = -1;
unsigned long tickTime = 0UL;
int plannedDurationMinutes;
unsigned long abortedAt = 0UL;
bool logEntryRequired;

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
  
  // set the data rate for the NewSoftSerial port
  // mySerial.begin(9600);
  
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC not running!");
    if (! VERBOSE) {
      die(1);
    }
  }
  
  // setpoint
  pinMode(pinSetpointDown, INPUT);
  digitalWrite(pinSetpointDown, 1);
  pinMode(pinSetpointUp, INPUT);
  digitalWrite(pinSetpointUp, 1);
  setpointButtonStatus = 0;
  setpointDegrees = 100.0;
  setpointStepSizeCounter = 0;

  // SD card setup for logging
  pinMode(pinCS, OUTPUT);
  // see if the card is present and can be initialized:
  if (! SD.begin(pinCS)) {
    Serial.println("SD failed");
    if (! VERBOSE) {
      die(2);
    }
  }

  // reset the display
  // pinMode(pinLCDDisplayReset, OUTPUT);
  // digitalWrite(pinLCDDisplayReset, LOW);
  // delay(10);
  // digitalWrite(pinLCDDisplayReset, HIGH);
  // // 55 is too short; 60 works reliably
  // delay(60);
  
  thermistor = new Thermistor(pinAThermistor, resistors[0], R0, T0, B);
  heater = new PWMActuator(pinHeater, kPWMPeriodms, 0.25);
  controller = new PIDController(
                                  heater,
                                  coeff_K_c, coeff_tau_I, coeff_tau_D
                                );  
  
  // a loop occurs on the order of 100 ms; we want to smooth temperature with a time constant of 10 sec
  smoothX.init(smoothX.periods(100));  // smooth over 100 periods
  
  // Serial.println("Arduino ready");
  Serial.println("unix\t=A1/60/60/24+DATE(1970,1,1)\tsetpoint\ttemp\tduty cycle");
  
}  // end setup

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void loop() {
  
  unsigned int counts;
   
  msNow = millis();

  // control

  if ((msNow > tickTime)
      || (tickTime == 0) ) {
    step++;
    logEntryRequired = true;
    Serial.print ("New step: ");
    Serial.println (step);
    
    multiFlash (step, 500, 20, 0);
    
    if        (step == 0) {
      // record initial temperature 
      beginStep(110.0, 60);
    } else if (step == 1) {
      beginStep(100.0, 360);
    } else if (step == 2) {
      beginStep(125.0, 120);
    } else if (step == 3) {
      // stop heating and record trajectory
      beginStep(-459.67, 480);
    } else {
      Serial.println ("Done.");
      // stop
      die(10);
    }
      
    if (TESTING) { tickTime = msNow + 10000UL; }  // 10 seconds from now for testing
      
    formatFloat(strFBuf, fbufLen, thermistor->K2F(controller->setpoint()), 2);
    snprintf (strBuf, bufLen, 
              "At %04d-%02d-%02d %02d:%02d:%02d step %d: %d min at %s degF\n",
              unixNow.year(), unixNow.month(), unixNow.day(), unixNow.hour(), unixNow.minute(), unixNow.second(), 
              step, 
              plannedDurationMinutes,
              strFBuf );
    Serial.print (strBuf);
#if TESTING > 0
    snprintf (strBuf, bufLen, 
              "  step will end at %lu millis (duration %lu ms)\n",
              tickTime,
              long(tickTime - msNow) );
    Serial.print (strBuf);
#endif
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

void beginStep (float setPointDegF, int durationMin) {
  controller->setpoint(thermistor->F2K(setPointDegF));
  tickTime += long(durationMin) * 60UL * 1000UL;
  plannedDurationMinutes = durationMin;
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void doControl () {

  float degK;
  
  degK = thermistor->getDegK();  // log is base e...

  smoothX.record(degK);
  
  if (smoothX.value() > high_temperature_limit) {
    if (abortedAt == 0UL) {
      // force cool-down recording step
      Serial.print ("Aborting run - over temp\n");
      abortedAt = millis();
      step = 99;
      beginStep(0.0, 600);
    }
  }
  
  controller->doControl(smoothX.value());
  heater->enable();
  
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void doDisplay () {
  
  #define fbufLen 12
  String dataString;
  DateTime unixNow = RTC.now();
  msNow = millis();
  
  static unsigned long tLastPrinting = 0UL;
  static unsigned long tLastUpdate = 0UL;
  
  char dc[fbufLen], sp[fbufLen], temperature[fbufLen];
  formatFloat(dc, fbufLen - 1, heater->dutyCycle(), 2);
  formatFloat(sp, fbufLen - 1, thermistor->K2F(controller->setpoint()), 1);
  formatFloat(temperature, fbufLen, thermistor->K2F(smoothX.value()), 2);

  // print
  
  #if VERBOSE >= 10 
    if (millis() > tLastPrinting + 5000UL) {
      snprintf (strBuf, bufLen, "At %lu ms: temp/setpoint %s/%s degF; duty cycle %s\n",
        millis(),
        temperature, sp, dc);
      Serial.print (strBuf);
    
      float controllerSetpointDegrees, currentError, errorIntegral, valueP, valueI, valueD, dutyCycle;
      float coeff_K_c, coeff_tau_I, coeff_tau_D;
      char strP[fbufLen], strI[fbufLen], strEI[fbufLen], strD[fbufLen];
      char strT[fbufLen];
      controller->get_internal_variables(                          // NOTE: all temps in degK
                                          controllerSetpointDegrees, 
                                          currentError, errorIntegral, 
                                          valueP, valueI, valueD, 
                                          dutyCycle,
                                          coeff_K_c, coeff_tau_I, coeff_tau_D
                                        );
      formatFloat(strP, fbufLen - 1, valueP, 4);
      formatFloat(strI, fbufLen - 1, valueI, 4);
      formatFloat(strEI, fbufLen - 1, errorIntegral, 0);
      formatFloat(strD, fbufLen - 1, valueD, 4);
      
      formatFloat(strT, fbufLen - 1, thermistor->K2F(smoothX.value()), 4);
      snprintf (strBuf, bufLen, "   PID %s | %s (%s) | %s\n",  
        msNow, 
        strT, strP, strI, strEI, strD);
      Serial.print (strBuf);
      
      snprintf (strBuf, bufLen, "        heater: duty cycle pct: %d; duty cycle ms: %lu; PWM period ms: %lu\n",
        int(heater->dutyCycle() * 100.0), 
        heater->dutyCyclems(), 
        heater->PWMPeriodms() );
      Serial.print (strBuf);
    
      tLastPrinting = millis();
    }
  #endif

  // log to SD card
 
  if ((msNow > tLastUpdate + loggingInterval) 
      || (msNow < tLastUpdate)
      || logEntryRequired) {
    tLastUpdate = msNow;
                            
    // unixNow.unixtime is seconds since epoch of 1970-01-01
    // =A1/60/60/24+DATE(1970,1,1) for Excel date
    snprintf (strBuf, bufLen, 
              "%lu\t\t%s\t%s\t%s\n",
              unixNow.unixtime(),
              sp,
              temperature,
              dc);

    Serial.print (strBuf);
    
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    
    File dataFile = SD.open("PIDLog.txt", FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.print(strBuf);
      dataFile.close();
    } else {
      Serial.println("error opening PIDLog.txt");
    } 
  }
  logEntryRequired = false;
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->
// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

unsigned int i2s (int k) {
  // convert an int into characters for 4-digit display
  unsigned int s = 0;
  for (int i = 0; i < 4; i++) {
    uint8_t c = k % 10; k /= 10;
    s += c << (i * 4);
  }
  return(s);
}
  
// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

float tempInDesiredUnits (float degK) {
  float degrees;
  if (tempUnits == 'F') {
    degrees = thermistor->K2F(degK);
  } else if (tempUnits == 'C') {
    degrees = thermistor->K2C(degK);
  } else {
    degrees = degK;
  }
  return (degrees);
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
    multiFlash (50, 10, 60, -1);    // dim for 400 ms
    delay (1000);
    multiFlash (n, 600, 60, 0);     // indicate number
    delay (1000);
    multiFlash (50, 10, 60, 1);     // brighten for 400 ms
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

