// tests the PWMActuator library

/* 
*/

#include <PWMActuator.h>

#define VERBOSE 2

#define cbmUseSERIAL 0
// #undef cbmUseSERIAL
#ifdef cbmUseSERIAL
#define baudRate 115200
#define bufLen 120
char strBuf[bufLen];

#endif

//************************************************************************************************
// 						                               Setup
//************************************************************************************************

#define pinLED 13
#define pinHeater 8

int mode = -1;
unsigned long tickTime;

PWMActuator myPWMActuator;

//************************************************************************************************
// 						                    Standard setup and loop routines
//************************************************************************************************

void setup()
{
  pinMode(pinLED, OUTPUT);
    
#ifdef cbmUseSERIAL
  Serial.begin(baudRate);       // start serial for output
  Serial.println ("Ready");
#endif
    
  myPWMActuator = PWMActuator("PWM1", pinLED, 500L, 0.5);
  myPWMActuator.enable();
  
  tickTime = 0UL;
 
}

/*
int availableMemory() {
 int size = 8192;
 byte *buf;
 while ((buf = (byte *) malloc(--size)) == NULL);
 free(buf);
 return size;
} 
*/

void loop() {

    /*
    snprintf (strBuf, bufLen, "free memory %u\n", availableMemory());
    Serial.print (strBuf);
    for (tickTime = 0UL; tickTime < 10UL; tickTime++) {
    snprintf (strBuf, bufLen, 
              "millis = %lu; tickTime = %lu; sum = %lu\n",
              millis, tickTime, tickTime + 10000UL);
    Serial.print (strBuf);
    
    }
    /*
    for (1;1;1) { delay (10);}
    */
    
  if (millis() > tickTime + 10000UL 
      || tickTime == 0) {
    mode++;
    if (mode > 3) {
      mode = 0;
    }
    if (mode == 0) {
      myPWMActuator.dutyCycle(0.2);
      myPWMActuator.PWMPeriodms(1000L);
    } else if (mode == 1) {
      myPWMActuator.dutyCycle(0.8);
      myPWMActuator.PWMPeriodms(1000L);
    } else if (mode == 2) {
      myPWMActuator.dutyCycle(0.2);
      myPWMActuator.PWMPeriodms(2000L);
    } else if (mode == 3) {
      myPWMActuator.dutyCycle(0.8);
      myPWMActuator.PWMPeriodms(2000L);
    }
      
#ifdef cbmUseSERIAL
    unsigned long period = myPWMActuator.PWMPeriodms();
    snprintf (strBuf, bufLen, 
              "Mode = %1d; duty cycle = %3d (%6lu ms / %7lu ms period)\n",
              mode,
              int(myPWMActuator.dutyCycle() * 100.0 + 0.5),
              myPWMActuator.dutyCyclems(),
              period);
    Serial.print (strBuf);
    snprintf (strBuf, bufLen, 
              "duty cycle:\nlong lu = %6lu\nlong ld = %6ld\nuint = %6u\nobject %6lu\n",
              long(myPWMActuator.dutyCycle() * period), 
              long(myPWMActuator.dutyCycle() * period), 
              int(myPWMActuator.dutyCycle() * float(period)),
              myPWMActuator.dutyCyclems() );
    Serial.print (strBuf);
#endif
    tickTime = millis();
  }
  
  myPWMActuator.doControl();
  delay(20);
      
}
