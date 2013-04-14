#define VERSION "0.3.0"
#define VERDATE "2011-12-08"
#define PROGMONIKER "I2C_REG"

/* 
  Test using I2C to allow three (for example) Arduinos to communicate
  
  Master will have address 0
  Slave 1                  1
  Slave 2                  2
  Slave 3                  3
  
  Addresses will be two bits on digital pins 12 and 11 - 5V = 1, 12 is msb
  
  Notes:
    o Need to decide protocol for startup. Order of power-up should not matter. 
      A slave needs to register; the master should then provide the parameters 
      to the slave. 
    o Slave should save its parameters to EEPROM so as to be able to run with last 
      settings but without master if desired. NO - EEPROM slow, good for only 100K write cycles.
    o Slave registers with waiting for poll - master will poll potential slaves 
      every half second or so
      
  Protocol:
    Phase 1:
      Master sends message to slave; waits until slave repeats it back or timeout
    Phase 2: 
      Master sends occasional command to slave; slave confirms receipt
      Master occasionally requests data from a slave; slave provides
    Phase 3:
      Slave has register structure that it shares via I2C
      Slave waits until master notices it, blinking furiously for help ;)
      Master occasionally polls for slaves, notices them, sends them configuration data
      Slaves speak only when spoken to
      
  To Do:
    Develop protocol for querying registers in the slaves
      Set register number; read some bytes
    Develop protocol for verifying slave is awake
    Synchronize clocks?
  
*/

#include <stdio.h>
#include <FormatFloat.h>
#include <Wire.h>

#define BAUDRATE 19200
#define LOOPDELAYMS 100

#define bufLen 60
#define fbufLen 14
char strBuf[bufLen + 1], strFBuf[fbufLen + 1];

// ===================
// Arduino connections
// ===================

// analog pins

/*
  note that I2C (Wire.h) uses by default analog pins 4 and 5
  SCL is A5; use a 1.5K pullup
  SDA is A4; use a 1.5K pullup
  note that grounds need to be common, as well!
*/

// digital pins

#define pinLED 13
#define pinAddrmsb 12
#define pinAddrlsb 11

byte myAddr;
int currentRegister;
unsigned int slavesOnline;
int requestRegisterDump = 0;

#define I2CBufLen 20
unsigned char I2CBuf[I2CBufLen];
byte I2CBufPtr = 0;

unsigned long tLastBlinkCycle = 0;
unsigned long on_ms = 100;

// note that the registers for a real temperature control module should include
// the P, I, and D coefficients as well...

union registers_t {
  struct register_vars_t {
    // blink duty cycle
    float blink_duty_cycle;
    // blink period mSec
    int blink_period_ms;
    // LED state (zero = off, 1 = on)
    int state;
    // enabled by master
    int enabled;
  } r;
  byte bytes[10];   // note bytes are in little-endian order!!!!
} registers;
int numRegisters = 4;
int registerOffsets[] = {0, 4, 6, 8};
int registerLengths[] = {4, 2, 2, 2};

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
  formatFloat(strFBuf, fbufLen, registers.r.blink_duty_cycle, 2);
  snprintf (strBuf, bufLen, "  blink_duty_cycle: %s\n", strFBuf);
  Serial.print (strBuf);
  snprintf (strBuf, bufLen, "  blink_period_ms: %d\n", registers.r.blink_period_ms);
  Serial.print (strBuf);
  snprintf (strBuf, bufLen, "  state: %d\n", registers.r.state);
  Serial.print (strBuf);
  snprintf (strBuf, bufLen, "  enabled: %d\n", registers.r.enabled);
  Serial.print (strBuf);
  Serial.println ();
  
  requestRegisterDump = 0;
}

void setup() {
  pinMode(pinLED, OUTPUT);      // sets the digital pin as output
  Serial.begin(BAUDRATE);
	snprintf(strBuf, bufLen, "%s: I2C between Arduinos v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);
  
  pinMode(pinAddrmsb, INPUT); digitalWrite(pinAddrmsb, HIGH);
  pinMode(pinAddrlsb, INPUT); digitalWrite(pinAddrlsb, HIGH);
  myAddr = digitalRead(pinAddrmsb) * 2 + digitalRead(pinAddrlsb);
  
  Serial.print ("My address is: ");
  Serial.println(myAddr, HEX);
  
  Wire.begin(myAddr);        // join i2c bus (address optional for master)
    
  if (myAddr == 0) {
    // setup for master
    registers.r.blink_period_ms = 2000;
    registers.r.blink_duty_cycle = 0.5;
    slavesOnline = 0;
    Wire.onReceive(0);
  } else {
    // setup for slave(s)
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
      digitalWrite(pinLED, 1 - digitalRead(pinLED));
      delay (100);
    }
    
    dumpRegisters(1);
  }
  tLastBlinkCycle = 0;
  on_ms = 10;
} 

void loop() {
  static unsigned long lastBlink = 0;
  if (myAddr == 0) {
    doMasterStuff();
  } else {
    doSlaveStuff();
  }
  
  // blink LED
  
  if (millis() > lastBlink + registers.r.blink_period_ms) {
    lastBlink = millis();
  }
  if (millis() <= lastBlink + int(registers.r.blink_period_ms * registers.r.blink_duty_cycle)) {
    digitalWrite(pinLED, 1);
  } else {
    digitalWrite(pinLED, 0);
  }
  
  delay (LOOPDELAYMS);
}

void doSlaveStuff () {
  // don't do anything without being told; just blink
  // slaves are handled via interrupts
  if (requestRegisterDump) {
    dumpRegisters(1);
  }
}

void doMasterStuff () {
  static unsigned int lastPoll = 0;
  // every so often, verify that the slaves are responsive
  #define slavePollInterval_ms 500
  
  union {
    unsigned char b;
    int i;
    float f;
    unsigned long lu;
    uint8_t a;
  } sendBuf;
  
  if ((millis() > lastPoll + slavePollInterval_ms) || (millis() < lastPoll)) {
    int enabled = 0;

    // note slaves are numbered 1 to 3, not 0 to 2!!
    for (int currentSlave = 1; currentSlave < 4; currentSlave++) {
      // set the register to read
      // register 3 (on the slave) is the "enabled" value
      Wire.beginTransmission(currentSlave); Wire.send((byte) 0x03); Wire.endTransmission();
      Wire.requestFrom(currentSlave, 2);  // enabled is an int, 2 bytes
      
      unsigned long tStart;
      #define masterTimeout 250
      
      tStart = millis();
      while (!Wire.available() && millis() < tStart + masterTimeout) {
        digitalWrite(pinLED, 1 - digitalRead(pinLED));
      }      

      if (Wire.available() >= 2) {
        enabled = (Wire.receive() << 8) | Wire.receive();
        if (enabled && (slavesOnline & (0x01 << currentSlave))) {
          // slave was already listed online -- good.
        } else {
          // slave just came online
          slavesOnline |= (0x01 << currentSlave);
          // Serial.print ("0 slavesOnline: 0x"); Serial.println (slavesOnline, HEX);
          snprintf (strBuf, bufLen, "Slave %d just came on line\n", currentSlave);
          Serial.print (strBuf);
          // initialize slave's registers
          // blink_duty_cycle
          sendBuf.f = (currentSlave) * 0.25;
          Wire.beginTransmission(currentSlave); Wire.send((byte) 0x00); 
            Wire.send(&sendBuf.a, 4); Wire.endTransmission();
          // blink period mSec
          sendBuf.i = 1000;
          Wire.beginTransmission(currentSlave); Wire.send((byte) 0x01); 
            Wire.send(&sendBuf.a, 2); Wire.endTransmission();
          // LED state
          sendBuf.i = 0;
          Wire.beginTransmission(currentSlave); Wire.send((byte) 0x02); 
            Wire.send(&sendBuf.a, 2); Wire.endTransmission();          
          // enabled
          sendBuf.i = 1;
          Wire.beginTransmission(currentSlave); Wire.send((byte) 0x03); 
            Wire.send(&sendBuf.a, 2); Wire.endTransmission();
        }
      } else {
        // device is missing (no Wire.available()
        if (slavesOnline & (0x01 << currentSlave)) {
          // but it had been on line
          // Serial.print ("1 slavesOnline: 0x"); Serial.println (slavesOnline, HEX);
          slavesOnline &= ~ (0x0001 << currentSlave);
          // Serial.print ("2 slavesOnline: 0x"); Serial.println (slavesOnline, HEX);
          snprintf (strBuf, bufLen, "Wah! Slave %d just disappeared!\n", currentSlave);
          Serial.print (strBuf);
        } else {
          // and was not expected to be on line -- OK
        }
      }
    }  // poll of slave currentSlave
  }  // done periodic poll of slaves

}

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
    requestRegisterDump = 1;
    // Serial.print ("Got 2: ");
    // for (int i = 0; i < I2CBufPtr; i++) {
      // Serial.print (" "); Serial.print (I2CBuf[i], HEX);
    // }
    // Serial.println ();
  }
}

void requestEvent() {
  if (currentRegister >= 0 && currentRegister < numRegisters) {
    Wire.send (&registers.bytes[registerOffsets[currentRegister]], registerLengths[currentRegister]);
  }
}
