
#define VERSION "0.0.1"
#define VERDATE "2011-12-26"
#define PROGMONIKER "TCO_Slave_Simulator"

/*
  temperature controller slave simulator

  pretend to read 1 thermistor
       
  Protocol:
    Slave has register structure that it shares via I2C
    Slave waits until master notices it, blinking furiously for help ;)
    Master occasionally polls for slaves, notices them, sends them configuration data
    Slaves speak only when spoken to

  To Do:
    Put in watchdog timer
    Synchronize clocks?
    
*/

#define VERBOSE 6
#define TESTING 1

// ===================
// Arduino connections
// ===================

// analog pins
// #define pinAThermistor 0
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
// #define pinDHeater 8

// ============
// housekeeping
// ============

#define BAUDRATE 19200

#define bufLen 120
#define fbufLen 14
char strBuf[bufLen + 1], strFBuf[fbufLen + 1];

// includes

#include <stdio.h>
// #include <fix_new.h>
#include <FormatFloat.h>
#include <Wire.h>

// #include <EWMA.h>
// #include <Thermosensor.h>
// #include <PWMActuator.h>
// #include <PIDController.h>

#include <EEPROM.h>

//   =============================================================
//     definition of registers structure and support subroutines
//   =============================================================

// ditch the original idea of keeping this data in EEPROM
//   EEPROM's rated for only 100000 write cycles

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
    
    // 3: heater state (zero = off, 1 = on)
    #define reg_state          3
    byte state;

    // 4: enabled by master
    #define reg_enabled        4
    byte enabled;
    
  } r;
  byte bytes[14];   // note bytes are in little-endian order!!!!
} registers;
int numRegisters = 5;
int registerLengths[] = {4, 4, 4, 1, 1};
int registerOffsets[] = {0, 4, 8, 12, 13};

void setRegister(int n, const void *bytes) {
  // expects that the bytes will be in little-endian order
  // uses as many as needed to set the given value
  if ((n >= 0) && (n < numRegisters)) {
    memcpy(&registers.bytes[registerOffsets[n]], bytes, registerLengths[n]);
  }
}

int currentRegister;
int requestRegisterDump = 0;
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

  snprintf (strBuf, bufLen, "  state: %d\n", registers.r.state);
  Serial.print (strBuf);
  snprintf (strBuf, bufLen, "  enabled: %d\n", registers.r.enabled);
  Serial.print (strBuf);
  Serial.println ();
  
  requestRegisterDump = 0;
}

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

// high temperature limit is 145degF -> 335.93degK
#define high_temperature_limit 335.9

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
    requestRegisterDump = 0;
  }
  
  // housekeeping
  
  if (( msNow > (tBlink + 1000)) ||  msNow < tBlink) {
    tBlink =  msNow;
    digitalWrite(pinDLED, 1 - digitalRead(pinDLED));
  }
  
  delay (100);
}  // end loop

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
    // Serial.print (" 0x"); Serial.print (c, HEX);
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
    registers.r.current_temp_K = registers.r.setpoint_K + (rand() % 100) * 0.1;
    // Serial.print ("New current temp (degK): (");
    // for (int i = 0; i < 4; i++) {
      // Serial.print ((byte) *(&registers.bytes[registerOffsets[reg_current_temp_K] + i]), HEX);
      // Serial.print (" ");
    // }
    // Serial.print (") : ");
    // formatFloat(strFBuf, fbufLen, registers.r.current_temp_K, 2);
    // Serial.println (strFBuf);
    // requestRegisterDump = 11;
  }
  if (I2CBufPtr > 1) {
    setRegister(I2CBuf[0], &I2CBuf[1]);
    // Serial.print ("Got 2: ");
    // for (int i = 0; i < I2CBufPtr; i++) {
      // Serial.print (" "); Serial.print (I2CBuf[i], HEX);
    // }
    // Serial.println ();
    requestRegisterDump = 12;
  }
}

void requestEvent() {
  if (currentRegister >= 0 && currentRegister < numRegisters) {
    Wire.send (&registers.bytes[registerOffsets[currentRegister]], registerLengths[currentRegister]);
    requestRegisterDump = 20;
  }
}
