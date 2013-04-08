
#define VERSION "0.9.3"
#define VERDATE "2012-01-02"
#define PROGMONIKER "TCO_Slave"

/*
  temperature controller slave

  read 1 thermistor
  given voltage divider 5V -- Rf -mm- Rt -- gnd, Rf is fixed resistor, mm is measurement point, 
  Rt is thermistor, we have counts / 1024 = 5V * (Rt / (Rf + Rt)). 
  Given the temperature span from, say, 25degC (77degF) to 40degC (104degF), 
  the span of mm would be s = (Rt+ / (Rf + Rt+) - (Rt- / (Rf + Rt-)).
  Given also that Rt+ is about 5K and Rt- is about 10K, we have (using resistance units of 1K)
  that s = -5Rf / (Rf**2 + 15Rf + 50), which is maximized at about 0.7K.
  So we choose  resistors of 6.8K nominal for Rf.
  We measure the actual resistances for more accuracy.
  
  We ought to use a voltage divider using a zener diode for a tighter Vref to obtain more resolution.

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
      
  More Notes:
    o Need to decide protocol for startup. Order of power-up should not matter. 
      A slave needs to register; the master should then provide the parameters 
      to the slave. 
    o Slave should save its parameters to EEPROM so as to be able to run with last 
      settings but without master if desired. NO - EEPROM slow, good for only 100K write cycles.
    o Slave registers with waiting for poll - master will poll potential slaves 
      every half second or so
      
  Protocol:
    Slave has register structure that it shares via I2C
    Slave waits until master notices it, blinking furiously for help ;)
    Master occasionally polls for slaves, notices them, sends them configuration data
    Slaves speak only when spoken to
    
    Slave changes state to 0x10 if it overtemps...
    
    Change Log:
      0.9.2 2012-01-02 cbm  put smoothed duty cycle into registers, replacing state
      0.9.3 2012-01-02 cbm  fixed sending of registers as a unit

  To Do:
    Put in watchdog timer
    Synchronize clocks?
    
*/

#define VERBOSE 6
#define TESTING 0

// ===================
// Arduino connections
// ===================

// analog pins
#define pinAThermistor 0
/*
  note that I2C (Wire.h) uses by default analog pins 4 and 5
  SCL is A5; use a 1.5K pullup
  SDA is A4; use a 1.5K pullup
  note that grounds need to be common, as well!
*/

// digital pins

#define pinDLED 13
// consideration of storing setpoint in EEPROM and operating autonomously
// #define pinDCarryOn 12
// need to do the storage when the setpoint changes...
#define pinDHeater 8

// ============
// housekeeping
// ============

#define BAUDRATE 19200

#define bufLen 120
#define fbufLen 14
char strBuf[bufLen + 1], strFBuf[fbufLen + 1];

// includes

#include <stdio.h>
#include <fix_new.h>
#include <FormatFloat.h>
#include <Wire.h>

#include <EWMA.h>
#include <Thermosensor.h>
#include <PWMActuator.h>
#include <PIDController.h>

#include <EEPROM.h>

//   =============================================================
//     definition of registers structure and support subroutines
//   =============================================================

// ditch the original idea of keeping this data in EEPROM
//   EEPROM's rated for only 100000 write cycles

byte numRegisters = 5;
byte registerLengths[] = {4, 4, 4, 1, 1};
#define REGISTERS_SIZE 14
byte registerOffsets[] = {0, 4, 8, 12, 13};
byte requestRegisterDump = 0;
byte currentRegister;

union registers_t {
  struct register_vars_t {

    // 0: current temperature
    #define reg_current_temp_K 0
    float current_temp_K;
    
    // 1: setpoint temperature
    #define reg_setpoint_K     1
    float setpoint_K;
    
    // 2: PWM period should be 2000 mSec for solid-state, 30000 mSec for mechanical relay
    #define reg_PWM_period     2
    unsigned long PWM_period;
    
    // 3: duty cycle pct (0-100), averaged over 2 minutes and reset when setpoint_K changes
    #define reg_dutyCyclePct         3
    byte dutyCyclePct;

    // 4: enabled by master
    #define reg_enabled        4
    byte enabled;
    
  } r;
  byte bytes[REGISTERS_SIZE];   // note bytes are in little-endian order!!!!
} registers;

void setRegister(int n, const void *bytes) {
  // expects that the bytes will be in little-endian order
  // uses as many as needed to set the given value
  if ((n >= 0) && (n < numRegisters)) {
    memcpy(&registers.bytes[registerOffsets[n]], bytes, registerLengths[n]);
  }
}

void dumpRegisters (int n) {
  Serial.print ("\n\n"); Serial.print (n); Serial.print ("\n");
  Serial.println ("Registers as bytes:");
  int sum = 0;
  for (int i = 0; i < numRegisters; i++) {
    sum += registerLengths[i];
  }
  for (int i = 0; i < sum; i++) {
    Serial.print (" "); Serial.print (registers.bytes[i], HEX);
  }
  Serial.println ();
  
  Serial.println ("Registers as variables:");
  formatFloat(strFBuf, fbufLen, registers.r.current_temp_K, 2);
  snprintf (strBuf, bufLen, "  current_temp_K: %s\n", strFBuf);
  Serial.print (strBuf);
  formatFloat(strFBuf, fbufLen, registers.r.setpoint_K, 2);
  snprintf (strBuf, bufLen, "  setpoint_K: %s\n", strFBuf);
  Serial.print (strBuf);
  Serial.println ();
  
  snprintf (strBuf, bufLen, "  PWM_period: %lu\n", registers.r.PWM_period);
  Serial.print (strBuf);
  Serial.println ();

  snprintf (strBuf, bufLen, "  duty cycle pct: %d\n", registers.r.dutyCyclePct);
  Serial.print (strBuf);
  snprintf (strBuf, bufLen, "  enabled: %d\n", registers.r.enabled);
  Serial.print (strBuf);
  Serial.println ();
  
  requestRegisterDump = 0;
}

float oldSetpoint_K = -100;

#define I2CBufLen 20
unsigned char I2CBuf[I2CBufLen];
byte I2CBufPtr = 0;

// these values will come from EEPROM
byte myAddr;
// voltage divider used to turn thermistor resistance to voltage
// this is the carefully-measured value of the fixed resistor
  // 6714.1  // 6710.7 ohms (used by original setup)
  // 6797.5  // 6795.3 ohms (used by current wingshield setup)
float reference_resistance;

// =============================

unsigned long tBlink = 0;

// constants of PID control tuning

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

  settings calculated from crockpot run
    coefficients in terms of degK, seconds
      these are in terms of *minutes*
    // float coeff_K_c = -0.055; // -0.04;
    // float coeff_tau_I = 56.0; // 56.0;  // NOTE that tau_I occurs in the DENOMINATOR!
    // float coeff_tau_D = 8.7; // 8.7;

  from crockpot values
    I: 50 * 60 = 3000; D: 8 * 60 = 480
*/

#define coeff_K_c        0.3
#define coeff_tau_I   1000.0
#define coeff_tau_D    100.0

// we are using a muRata NTC thermistor NTSD1XH103FPB
#define T0 (25.0 + FP_H2O_K)
#define R0 10000.0
#define B 3380.0

// high temperature limit is 155degF ~> 68degC -> 341.15degK
#define high_temperature_limit_K 341.15

// =====================
// definition of objects
// =====================

EWMA smoothX, dutyCycle;
Thermistor *thermistor;
PWMActuator *heater;
PIDController *controller;

// timers

unsigned long msNow;

//************************************************************************************************
// 						                       setup and loop routines
//************************************************************************************************

void setup() {
  Serial.begin(BAUDRATE);
	snprintf(strBuf, bufLen, "%s: Temperature Controller Slave v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);

  pinMode(pinDLED, OUTPUT);
  pinMode(pinDHeater, OUTPUT);
  
  // read my address and my reference resistance from EEPROM
  myAddr = EEPROM.read(0);
  if (myAddr > 32) {
    // means EEPROM is not set up yet -- problem!
    die(8);
  }
  for (int i = 0; i < 4; i++) {
    *((byte*) &reference_resistance + i) = EEPROM.read(1 + i);
    snprintf (strBuf, bufLen, "Read byte %d: %02x\n", i, (byte) *((byte*) &reference_resistance + i));
    Serial.print (strBuf);
  }

  Serial.print ("My address is: ");
  Serial.println(myAddr, HEX);
  Serial.print ("My reference resistance is: ");
  formatFloat (strFBuf, fbufLen, reference_resistance, 2);
  Serial.println(strFBuf);
  
  Wire.begin(myAddr);        // join i2c bus (address optional for master)
    
  // wait for master to notice us
  // master noticing us is recognized by setting of registers, 
  //   which is done in interrupt service routines
  registers.r.enabled = 0;
  
  dumpRegisters(0);
  
  Wire.onReceive(receiveEvent);
  // for request system - but request requires number of bytes, blocks
  Wire.onRequest(requestEvent);
  while (! registers.r.enabled) {
    // blink furiously
    digitalWrite(pinDLED, 1 - digitalRead(pinDLED));
    delay (100);
  }
  
  dumpRegisters(1);
  
  thermistor = new Thermistor(pinAThermistor, reference_resistance, R0, T0, B);
  heater = new PWMActuator(pinDHeater, registers.r.PWM_period, 0.25);
  controller = new PIDController(
                                  heater,
                                  coeff_K_c, coeff_tau_I, coeff_tau_D
                                );  
  
  // a loop occurs on the order of 100 ms; we want to smooth temperature with a time constant of 10 sec
  smoothX.init(smoothX.periods(100));  // smooth over 100 periods
  dutyCycle.init(dutyCycle.periods(1200));  // smooth over 2 minutes
    
}  // end setup

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void loop() {
  
  unsigned int counts;

  msNow = millis();

  while (! registers.r.enabled) {
    // blink furiously
    digitalWrite(pinDLED, 1 - digitalRead(pinDLED));
    delay (100);
  }
  
  if (requestRegisterDump) {
    dumpRegisters(requestRegisterDump);
  }
  
 /////////// TODO: if anything changes in the registers, 
 //// make sure that the appropriate changes are made in the objects that might be affected....


  // ===============================
  // handle adjustment of parameters
  // ===============================
  
  controller->setpoint(registers.r.setpoint_K);
  
  // ===================
  // measure and control
  // ===================
  
  doControl ();
  
  // housekeeping
  
  if (( msNow > (tBlink + 1000)) ||  msNow < tBlink) {
    tBlink =  msNow;
    digitalWrite(pinDLED, 1 - digitalRead(pinDLED));
  }
  
  delay (100);
}  // end loop

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void doControl () {

  float degK;
  
  degK = thermistor->getDegK();  // log is base e...

  smoothX.record(degK);
  registers.r.current_temp_K = smoothX.value();
  
  // Serial.print ("Temperature (degK): ");
  // formatFloat (strFBuf, fbufLen, degK, 2);
  // Serial.println(strFBuf);
  
  if (registers.r.current_temp_K > high_temperature_limit_K) {
    heater->enable(0);
    Serial.println ("High temperature limit exceeded. Shutting down...");
    die(5);
  }

  controller->doControl(registers.r.current_temp_K);
  if (abs(registers.r.setpoint_K - oldSetpoint_K) > 1e-3) {
    dutyCycle.reset();
    oldSetpoint_K = registers.r.setpoint_K;
  }
  dutyCycle.record(controller->get_dutyCycle());
  registers.r.dutyCyclePct = (short) (dutyCycle.value() * 100.0 + 0.5);
  heater->enable();
  
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
      digitalWrite(pinDLED, HIGH);
      delayMicros (usOn);
    }
    digitalWrite(pinDLED, LOW);
    delayMicros (usOff);
    usOn += dus;
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void die (int n) {
  int i;
  registers.r.enabled = 0x10;
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

void receiveEvent(int howMany) {
  char c;
  I2CBufPtr = 0;
  // Serial.print ("Got 0: ");
  while (Wire.available()) {
    c = Wire.receive(); // receive byte as a character
    I2CBuf[I2CBufPtr++] = c;
    // Serial.print (" "); Serial.print (c, HEX);
  }
  // Serial.println ();
  // presumably have received a complete message
  // should never enter routine until wire.beginTransmission from other end;
  // Wire.available should never drop until wire.endTransmission from other end
  if (I2CBufPtr > 0) {
    currentRegister = I2CBuf[0];
    // Serial.print ("Got 1: ");
    // Serial.print (" "); Serial.print (I2CBuf[0], HEX);
    // Serial.println ();
  }
  if (I2CBufPtr > 1) {
    setRegister(I2CBuf[0], &I2CBuf[1]);
    requestRegisterDump = 12;
    // Serial.print ("Got 2: ");
    // for (int i = 0; i < I2CBufPtr; i++) {
      // Serial.print (" "); Serial.print (I2CBuf[i], HEX);
    // }
    // Serial.println ();
  }
}

void requestEvent() {
  byte num;
  uint8_t *start;
  // Serial.print(">>>"); Serial.println (currentRegister, HEX);
  // currentRegister is byte (unsigned char), so can't be negative!
  if ((currentRegister < numRegisters) || (currentRegister == 0xff)) {
    if (currentRegister == 0xff) {
      num = REGISTERS_SIZE;
      start = &registers.bytes[0];
    } else {
      num = registerLengths[currentRegister];
      start = &registers.bytes[registerOffsets[currentRegister]];
    }
    Wire.send (start, num);
  }
  // requestRegisterDump = 20;
}
