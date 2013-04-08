#define VERSION "0.12.7"
#define VERDATE "2012-01-20"
#define PROGMONIKER "TCO_Master"

/*
  temperature controller master

  for each of n (currently 2) slaves,
  display setpoint and actual temperature on new Sparkfun 7-seg 4-digit display
  log actual temperatures onto data logger shield
     
  new setup: crock pot on high, thermistor 2 with its resistor, relay-based switched outlet,
    CHEAP circulator pump (which melts at 150 degF or so!)
    
  Change log:
    0.12.5 2012-01-02 cbm  ready for action, I think
    
  Plans:
    o convert to running the slaves; defer work on setpoint setting, but instead
      just do it mechanically for first testing
    o integrate the display
      use slave's address to identify each section
    o rethink and recode the setpoint setting; use a pot to drive up or down, I think
      allow disabling of individual slave
      
    v set up brightness for Nokia backlight - have assigned pin 3
    o 10 seconds after a setpoint change, store it into EEPROM
    o read setpoints from EEPROM on startup
    o  card detect is *grounded* when a card is in - so switch it to red LED with a pullup
    o  and make the red LED the green one for blinking OK

*/

#define VERBOSE 4
#define TESTING 0
#define HIGHEST_SLAVE 3

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

#define pinDLED         2
#define pinDSelect      4
// setpoint buttons
#define pinDSPDown      5
#define pinDSPUp        6

// LCD (SPI)
// reset
#define pinDRST_LCD     7
// chip select
#define pinDCS_LCD      8
// data / command selector
#define pinDDC_LCD      9
#define pinDLED_LCD     3

// logging: SD card (SPI)
#define pinDCS_SD      10

// SPI pins
#define pinDMOSI       11
// LCD doesn't use MISO, so no off-shield connection needed
#define pinDMISO       12
#define pinDCLK        13

#define BAUDRATE 19200
#define bufLen 80
#define fbufLen 7
char strBuf[bufLen], strFBuf[fbufLen];
char sp[fbufLen], temperature[fbufLen];

// includes

#include <stdio.h>
#include <fix_new.h>
#include <FormatFloat.h>
// #include <NewSoftSerial.h>
// #include <math.h>
#include <EEPROM.h>
#include <Wire.h>          // I2C
#include <RTClib.h>        // real-time clock
#include <SD.h>            // secure digital card for logging (SPI) 
#include <PCD8544.h>       // Nokia display (SPI) which can hold 6 rows of 14 characters
#include <Thermosensor.h>  // for definitions of temperature constants


// ===================
// logging
// ===================

#define loggingInterval  10000UL

// ============
// housekeeping
// ============

unsigned int slavesOnline;

// We need to keep a local copy of the registers of each slave
// so we can reload them if the slave comes back online

#define READ_ONLY_REGISTERS B00001001
byte numRegisters = 5;
byte registerLengths[] = {4, 4, 4, 1, 1};
#define REGISTERS_SIZE 14
byte registerOffsets[] = {0, 4, 8, 12, 13};
byte requestRegisterDump = 0;

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
    #define reg_dutyCyclePct   3
    byte dutyCyclePct;

    // 4: enabled by master
    #define reg_enabled        4
    byte enabled;
    
  } r;
  byte bytes[REGISTERS_SIZE];   // note bytes are in little-endian order!!!!
} registers[HIGHEST_SLAVE];
// note -- indexed by slave *** - 1 ***

void dumpRegisters (byte registerSetNumber) {
  Serial.print ("\n\n");
  Serial.println ("Bytes:");
  for (byte i = 0; i < REGISTERS_SIZE; i++) {
    Serial.print (" "); Serial.print (registers[registerSetNumber].bytes[i], HEX);
  }
  Serial.println ();
  
  Serial.println ("Values:");
  formatFloat(strFBuf, fbufLen, registers[registerSetNumber].r.current_temp_K, 2);
  snprintf (strBuf, bufLen, "  current_temp_K: %s\n", strFBuf);
  Serial.print (strBuf);
  formatFloat(strFBuf, fbufLen, registers[registerSetNumber].r.setpoint_K, 2);
  snprintf (strBuf, bufLen, "  setpoint_K: %s\n", strFBuf);
  Serial.print (strBuf);
  Serial.println ();
  
  snprintf (strBuf, bufLen, "  PWM_period: %lu\n", registers[registerSetNumber].r.PWM_period);
  Serial.print (strBuf);
  Serial.println ();

  snprintf (strBuf, bufLen, "  duty cycle pct: %d\n", registers[registerSetNumber].r.dutyCyclePct);
  Serial.print (strBuf);
  snprintf (strBuf, bufLen, "  enabled: %d\n", registers[registerSetNumber].r.enabled);
  Serial.print (strBuf);
  Serial.println ();
  requestRegisterDump = 0;
}

// ===================
// control of setpoint
// ===================

#define tempUnits 'C'
// range is in temp units defined above
// Amy states range should be 4 degC to 50 degC
//  which is 32 to 122 degF, so for F go 20 to 140
// Note that range max must be less than high_temperature_limit 
//   defined in slaves as 145 degF, or 335.9 degK (62.8 degC)
//   to protect the circulating pump from melting
#define tempRangeMin  4.0
#define tempRangeMax 66.0

unsigned long setpointButtonStart, setpointButtonLastTick, setpointButtonDuration;
byte selectedItem;
int setpointButtonStatus; // -1 down; 0 none; +1 up

#define nSetpointSteps 3
float setpointStepSizes[] = {0.1, 1.0, 10.0};
#define setpointButtonStepTimems 500
#define setpointRepsBeforeNextSizeStep 5
int setpointStepSizeCounter, setpointStepPointer;
unsigned long setpointChangedAt = 0;

       
// =====================
// definition of objects
// =====================

// NewSoftSerial mySerial(nssRX, nssTX);
RTC_DS1307 RTC;
static PCD8544 nokia = PCD8544(pinDCLK, pinDMOSI, pinDDC_LCD, pinDRST_LCD, pinDCS_LCD);

// timers

DateTime unixNow;
unsigned long msNow;

//************************************************************************************************
// 						                    Standard setup and loop routines
//************************************************************************************************

void setup() {
  Serial.begin(BAUDRATE);
  
	snprintf(strBuf, bufLen, "%s: Temperature Controller Master v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);

  pinMode(pinDLED, OUTPUT);
  
  // set the data rate for the NewSoftSerial port
  // mySerial.begin(9600);
  
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("no RTC");
    if (! VERBOSE) {
      die(1);
    }
  }
  
  // setpoint
  pinMode(pinDSelect, INPUT);
  digitalWrite(pinDSelect, 1);
  pinMode(pinDSPDown, INPUT);
  digitalWrite(pinDSPDown, 1);
  pinMode(pinDSPUp, INPUT);
  digitalWrite(pinDSPUp, 1);
  selectedItem = 0;
  setpointButtonStatus = 0;
  setpointStepSizeCounter = 0;

  // SD card setup for logging
  // Note: have modified the SD module to use quarter
  // (clock) speed for compatibility with Nokia display
  pinMode(pinDCS_SD, OUTPUT);
  // see if the card is present and can be initialized:
  if (! SD.begin(pinDCS_SD)) {
    Serial.println("no SD");
    if (! VERBOSE) {
      die(2);
    }
  }
  
  // Note: have modified the PCD8544 module to use *hardware* SPI
  // for compatibility with SD module
  // But that requires also SD module *must* be initialized first
  
  // Nokia backlight LCD is controlled with PWM
  pinMode(pinDLED_LCD, OUTPUT);
  analogWrite(pinDLED_LCD, 240);
  
  nokia.begin();
  nokia.clear();

  nokia.setCursor(0, 0);
  nokia.print ("TCO_M v");
  nokia.setCursor(7*6, 0);
  nokia.print (VERSION);
  nokia.setCursor(0, 1);
  nokia.print("    SET   ACT ");

  // Serial.println("Arduino ready");
  Serial.println("unix\t=A1/60/60/24+DATE(1970,1,1)\ts\tSP\ttemp\tDC%");
  
  // handle slaves
  slavesOnline = 0;
  // set default values for registers
  for (byte currentSlave = 1; currentSlave <= HIGHEST_SLAVE; currentSlave++) {
    // 0: current temperature
    // current_temp_K should be read-only!
  
    // setpoint temperature
    registers[currentSlave - 1].r.setpoint_K = readSetpointFromEEPROM (currentSlave);  // C2K(4.0); // 
   
    // PWM period should be 2000 mSec for solid-state, 30000 mSec for mechanical relay (such as PowerSwitch Tail II)
    registers[currentSlave - 1].r.PWM_period = 30000;
    
    // duty cycle percent, averaged over 2 minutes and reset when setpoint_K changes
    registers[currentSlave - 1].r.dutyCyclePct = 0;

    // enabled by master
    registers[currentSlave - 1].r.enabled = 1;
  
  }
  
  // Serial.print ("Registers at ");
  // Serial.println ((unsigned long) &registers, HEX);
  // delay(1000);
  
}  // end setup

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void loop() {
  
  #define slavePollInterval_ms 1000
  #define displayInterval_ms 2000
  // typical response is 13 ms (38x12, 57x13, 13x14); ran for a while at 500, logging anything over 20, got nothing
  #define slaveAliveResponseTimeLimit 20
  
  static unsigned long lastBlinkAt = 0, lastDisplayAt = 0, lastPollAt = 0;
   
  msNow = millis();

  // =============================
  // handle adjustment of setpoint
  // =============================
  
  checkSetpointChange();
  
  // every so often, verify that the slaves are responsive
  
  if ((msNow > (lastPollAt + slavePollInterval_ms)) || (msNow < lastPollAt)) {
    int enabled = 0;

    // Serial.print ("poll: slavesOnline: 0x"); Serial.println (slavesOnline, HEX);
    // note slaves are numbered 1 up, not starting at 0!!
    for (int currentSlave = 1; currentSlave <= HIGHEST_SLAVE; currentSlave++) {
      // set up to read register 4 (on the slave), which is the "enabled" value
      Wire.beginTransmission(currentSlave); Wire.send((byte) reg_enabled); Wire.endTransmission();
      Wire.requestFrom(currentSlave, int(registerLengths[reg_enabled]));
      
      unsigned long tStart;
      
      tStart = millis();
      while (!Wire.available() && millis() < tStart + slaveAliveResponseTimeLimit) {
        digitalWrite(pinDLED, 1 - digitalRead(pinDLED));
      }      

      if (Wire.available() >= registerLengths[reg_enabled]) {
        enabled = Wire.receive();
        // unsigned long itTook = millis() - tStart;
        // if (itTook > 20) {
          // Serial.print ("SRT (ms > 20): "); Serial.println (itTook);
        // }
        
        if (enabled && (slavesOnline & (0x0001 << currentSlave))) {
          // slave was already listed online -- good.
        } else {
          // slave just came online
          // Serial.print ("Enabled: "); Serial.println (enabled, HEX);
          slaveNewlyOnline (currentSlave);
        }
      } else {
        // device is missing (no Wire.available()
        if (slavesOnline & (0x0001 << currentSlave)) {
          // but it had been on line
          // Serial.print ("1 slavesOnline: 0x"); Serial.println (slavesOnline, HEX);
          slavesOnline &= ~ (0x0001 << currentSlave);
          // Serial.print ("2 slavesOnline: 0x"); Serial.println (slavesOnline, HEX);
          snprintf (strBuf, bufLen, "%d down\n", currentSlave);
          Serial.print (strBuf);
        } else {
          // and was not expected to be on line -- OK
        }
      }
    }  // poll of slave currentSlave
    
    checkSetpointChange();
      
  }  // done periodic poll of slaves
  
  // ===================
  //   display status
  
  if ((msNow > (lastDisplayAt + displayInterval_ms)) || (msNow < lastDisplayAt)) {
    doDisplay ();
    lastDisplayAt = msNow;
  }
  
  checkSetpointChange();

  
  // housekeeping
  
  if ((msNow > (lastBlinkAt + 1000)) ||  (msNow < lastBlinkAt)) {
    digitalWrite(pinDLED, 1 - digitalRead(pinDLED));
    lastBlinkAt =  msNow;
  }
  
  delay (100);
}  // end loop

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void slaveNewlyOnline (byte currentSlave) {
  // mark slave online
  slavesOnline |= (0x0001 << currentSlave);
  // Serial.print ("0 slavesOnline: 0x"); Serial.println (slavesOnline, HEX);
  snprintf (strBuf, bufLen, "%d up\n", currentSlave);
  Serial.print (strBuf);
  
  // initialize the new slave's registers
    
  // Serial.print ("ps pwmp: "); Serial.println(registers[currentSlave - 1].r.PWM_period);
  for (byte n = 0; n < numRegisters; n++) {
    // don't send current temperature or duty cycle
    if (! (READ_ONLY_REGISTERS & (0x01 << n))) {
      Wire.beginTransmission(currentSlave); 
      Wire.send(n); 
      Wire.send(&registers[currentSlave - 1].bytes[registerOffsets[n]], registerLengths[n]); 
      Wire.endTransmission();
    }
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void doDisplay () {
  
  DateTime unixNow = RTC.now();
  msNow = millis();
  
  static unsigned long lastLoggingAt = 0UL;
  byte logDuringThisInvocation = ((msNow > lastLoggingAt + loggingInterval) || (msNow < lastLoggingAt));
  static unsigned long lastPrintingAt = 0UL;
  byte printDuringThisInvocation = (msNow > (lastPrintingAt + 5000UL) || (msNow < lastLoggingAt));
  
  char selectMarker;
  
  /*
    selectedItem will be 0, 1, 2, or 3; 0 for nothing; 1-3 for slave 1-3 respectively
    
    display map:
      |--------------|
       commentcomment
           SET   ACT
       o1 123.4 123.4
       o2 123.4 123.4
       o3 123.4 123.4
       o4 123.4 123.4
      |--------------|
  */
  
  // Nokia display is 6 rows of 14 6x8 bit characters
  for (int currentSlave = 1; currentSlave <= HIGHEST_SLAVE; currentSlave++) {
    selectMarker = ' ';
    if (currentSlave == selectedItem) {
      selectMarker = 'o';
    }
    
    nokia.setCursor (0, currentSlave + 1);
    nokia.clearLine ();
    nokia.print(selectMarker);
    // note that setCursor is to a pixel, so * 6 for character position
    nokia.setCursor (1*6, currentSlave + 1);
    nokia.print(currentSlave);
    
    if (slavesOnline & (0x0001 << currentSlave)) {
      updateRegistersCopy (currentSlave);
        
      formatFloat(sp, fbufLen, K2C(registers[currentSlave - 1].r.setpoint_K), 1);
      formatFloat(temperature, fbufLen, K2C(registers[currentSlave - 1].r.current_temp_K), 2);
 
      byte len;
      len = constrain (strlen(sp), 1, 5);
      nokia.setCursor ((3 + (5 - len)) * 6, currentSlave + 1);
      nokia.print(sp);
      len = constrain (strlen(temperature) - 1, 1, 5);
      nokia.setCursor ((9 + (5 - len)) * 6, currentSlave + 1);
      char x = temperature[6];
      temperature[len] = '\0';
      nokia.print(temperature);
      temperature[len] = x;
                  
      // print

      #if VERBOSE >= 10 
        if (printDuringThisInvocation) {
          lastPrintingAt = msNow;
          snprintf (strBuf, bufLen, "At %lu ms: temp/setpoint %s/%s degF/duty cycle %d%%\n",
            millis(),
            temperature, sp, registers[currentSlave - 1].r.dutyCyclePct);
          Serial.print (strBuf);
         }
      #endif

      // log to SD card
     
      if (logDuringThisInvocation) {
        lastLoggingAt = msNow;
                 
         // unixNow.unixtime is seconds since epoch of 1970-01-01
        // =A1/60/60/24+DATE(1970,1,1) for Excel date
        snprintf (strBuf, bufLen, 
                  "%lu\t\t%d\t%s\t%s\t%d\n",
                  unixNow.unixtime(),
                  currentSlave,
                  sp,
                  temperature,
                  registers[currentSlave - 1].r.dutyCyclePct
                 );

        Serial.print (strBuf);
        
        // open the file. note that only one file can be open at a time,
        // so you have to close this one before opening another.
        
        File dataFile = SD.open("TCMaster.txt", FILE_WRITE);
        // if the file is available, write to it:
        if (dataFile) {
          dataFile.print(strBuf);
          dataFile.close();
        } else {
          Serial.println("open err TCMaster.txt");
        } 
      }
      
    } else {
      nokia.setCursor (6 * 6, currentSlave + 1);
      nokia.print("offline");
    }  // it's off line
  } // each slave
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->
// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void updateRegistersCopy (int currentSlave) {
  if (slavesOnline & (0x0001 << currentSlave)) {
    // Serial.print ("AA ");
    // Serial.print (currentSlave);
    // dumpRegisters(currentSlave - 1);
  
    // update the registers from the slave
    Wire.beginTransmission(currentSlave); Wire.send((byte) 0xff); Wire.endTransmission();
    
    byte * pVal = &registers[currentSlave - 1].bytes[0];
    Wire.requestFrom(currentSlave, REGISTERS_SIZE);
    for (int j = 0; Wire.available(); j++) {
      byte m = Wire.receive();
      *(pVal++) = m;
    }
    // Serial.println ("bytes rec: ");
    // pVal = &registers[currentSlave - 1].bytes[0];
    // for (int j = 0; j < REGISTERS_SIZE; j++) {
      // Serial.print (j);
      // Serial.print (":");
      // Serial.print (*(pVal++), HEX);
      // Serial.print (" ");
    // }

    // Serial.print ("\n\nAB");
    // dumpRegisters(currentSlave - 1);
  }
}
    
// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->
// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void checkSetpointChange () {
  static byte selectButtonPressed = 0;
  // don't select again unless the button has been released
  //   (at which time this sub will complete and reset selectButtonPressed)
  if (! digitalRead(pinDSelect)) {
    if (! selectButtonPressed) {
      // selection taking place
      // find the next online slave; if none, make selectedItem zero (no selection)
      byte found = 0;
      for (byte i = selectedItem + 1; (! found) && (i <= HIGHEST_SLAVE); i++) {
        if (slavesOnline & (0x0001 << i)) {
          selectedItem = i;
          found = 1;
        }
      }
      if (! found) {
        selectedItem = 0;
      }
      selectButtonPressed = 1;
      // Serial.print ("Sel: ");
      // Serial.println ((int) selectedItem);
    }
    return;
  } else if (! digitalRead(pinDSPDown)) {
    setpointChange(-1);
  } else if (! digitalRead(pinDSPUp)) {
    setpointChange(1);
  } else {
    // must have just let up on a button
    if (selectedItem > 0 && setpointButtonStatus != 0 && VERBOSE) {
      formatFloat (strFBuf, fbufLen, K2C(registers[selectedItem - 1].r.setpoint_K), 1);
      snprintf (strBuf, bufLen, "At %lu: %d new SP: %s degC\n",  
        msNow, selectedItem, strFBuf);
      Serial.print (strBuf);
    }
    setpointButtonStatus = 0;
    setpointStepPointer = 0;
    setpointStepSizeCounter = 0;
  }
  selectButtonPressed = 0;
  if (setpointChangedAt > 0 && (millis() > (setpointChangedAt + 30000L))) {
    // write the new value to EEPROM (just once!)
    writeSetpointToEEPROM (selectedItem, registers[selectedItem - 1].r.setpoint_K);
    setpointChangedAt = 0;
  }
}

void setpointChange (int dir) {
  if (! selectedItem) {
    return;
  }
  // dir -1 for down
  if (setpointButtonStatus == 0) {
    setpointButtonStart = millis();
    setpointButtonStatus = dir;
    setpointButtonLastTick = millis() - setpointButtonStepTimems;
  }
  setpointButtonDuration = millis() - setpointButtonStart;
  if (millis() >= setpointButtonLastTick + setpointButtonStepTimems) {
    // setpoint change taking place
    if (setpointStepSizeCounter >= setpointRepsBeforeNextSizeStep) {
      setpointStepPointer = constrain(setpointStepPointer + 1, 0, nSetpointSteps - 1);
      setpointStepSizeCounter = 0;
    }
    setpointStepSizeCounter++;
    registers[selectedItem - 1].r.setpoint_K = constrain( registers[selectedItem - 1].r.setpoint_K
                                                            + (dir * setpointStepSizes[setpointStepPointer]),
                                                          C2K(tempRangeMin), C2K(tempRangeMax) );

    // all temperatures to controller must be in degK
    // controller->setpoint(C2K(registers[selectedItem - 1].r.setpoint_K));
    Wire.beginTransmission(selectedItem); 
    Wire.send(reg_setpoint_K); 
    Wire.send(&registers[selectedItem - 1].bytes[registerOffsets[reg_setpoint_K]], 
              registerLengths[reg_setpoint_K]); 
    Wire.endTransmission();

    formatFloat(sp, fbufLen, K2C(registers[selectedItem - 1].r.setpoint_K), 1);
    byte len;
    len = constrain (strlen(sp), 1, 5);
    nokia.setCursor ((3 + (5 - len)) * 6, selectedItem + 1);
    nokia.print(sp);

    #if VERBOSE >= 10 
      snprintf (strBuf, bufLen, "At %lu: Setpoint status: %d; start: %lu; duration: %lu; pointer: %d \n",  
        msNow, 
        setpointButtonStatus,
        setpointButtonStart,
        setpointButtonDuration,
        setpointStepPointer);
      Serial.print (strBuf);
    #endif
    setpointButtonLastTick = millis();
    #if VERBOSE >= 5 
      // snprintf (strBuf, bufLen, "At %lu: Setpoint adjusted to %d deciDeg\n",  
         // msNow, int(registers[selectedItem - 1].r.setpoint_K * 10.0));
      snprintf (strBuf, bufLen, "%lu: %d set to %d/10\n",  
         msNow, selectedItem, int(K2C(registers[selectedItem - 1].r.setpoint_K) * 10.0));
      Serial.print (strBuf);
    #endif
    setpointChangedAt = millis();
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void writeSetpointToEEPROM (byte theSlave, float theValue) {
  snprintf (strBuf, bufLen, "SP to EEPROM for %d\n", theSlave);
  Serial.print (strBuf);
  byte loc = (theSlave - 1) * 4;
  byte* p = (byte*)(void*) &theValue;
  for (int i = 0; i < sizeof(theValue); i++) {
	  EEPROM.write(loc++, *p++);
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

float readSetpointFromEEPROM (byte theSlave) {
  byte loc = (theSlave - 1) * 4;
  float value = 0.0;
  byte* p = (byte*)(void*) &value;
  for (int i = 0; i < sizeof(value); i++) {
    *p++ = EEPROM.read(loc++);
  }
  return value;
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

void multiFlash (int n, int ms, int dutyPct = 50, int dDutyFlag = 0) {
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
    multiFlash (6, 100);            // flash marker
    delay (1000);
    multiFlash (n, 600, 60, 0);     // indicate number
    delay (1000);
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->
