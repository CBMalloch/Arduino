/*
  Charles B. Malloch, PhD  2012-12-29
  v0.2.0
  
  Test running a three-phase stepper motor (from a diskette drive) powered via a L293 dual H-bridge chip
  Can run only so fast without feedback. Testing feedback now.
*/

#define BAUDRATE 115200

#define paPot 2
#define paReflector 1

#define motorA 2
#define motorB 3
#define motorC 4
#define motorD 5

#define pdLED 13
#define BLINK_INTERVAL 500

#define MIN_STEPS_PER_SECOND 1.0
#define MAX_STEPS_PER_SECOND 2400.0

#include <LogarithmicScale.h>

/*
  L293NE (no internal clamp diodes, RoHS)
    pins 1, 9 (enable pins) strapped high
    pin 8 (Vcc2 motor power) to Arduino Vin
    pins 4, 5, 12, 13 (GND) to Arduino GND
    pins 2, 7, 10, 15 (1A, 2A, 3A, 4A motor control) to Arduino pins D2, D3, D4, D5
    pins 3, 6, 11, 14 (1Y, 2Y, 3Y, 4Y motor power) to motor ROYG
  
    sensor setup:
      power to LED and photodiode
      photodiode to yellow sense wire and through br-g-bl-red (15K) resistor to ground
      LED through 330R resistor to ground
*/

unsigned short motorPins[] = { motorA, motorB, motorC, motorD };

// looks like the fourth wire is a ground?

unsigned short sequence[][4] = {
                                { 1, 0, 0, 0 },
                                { 0, 1, 0, 0 },
                                { 0, 0, 1, 0 },
                               };
/*
unsigned short sequence[][4] = {
                                { 1, 0, 0, 0 },
                                { 1, 1, 0, 0 },
                                { 0, 1, 0, 0 },
                                { 0, 1, 1, 0 },
                                { 0, 0, 1, 0 },
                                { 0, 0, 1, 1 },
                                { 0, 0, 0, 1 },
                                { 1, 0, 0, 1 },
                               };
 */
 
/*

  17.89 strobe stops 2400 steps per second of motor, so 2400 / 17.89 steps per revolution = 134
  2.72 strobe stops 33.5 steps per second. 33.5 / 2.72 = 12.3 steps per revolution 
  8.82 125.00 14.1
  17..61 2436.83 = 138
*/

int reflectorValue = 0;
                               
int potValue = 0;
int nSteps;
int dir = 1;

LogarithmicScale lsPot;

void setup() {
	Serial.begin(BAUDRATE);
	
  for (int i = 0; i < 4; i++) {
    pinMode (motorPins[i], OUTPUT);
    digitalWrite (motorPins[i], LOW);
  }
  
  pinMode(pdLED, OUTPUT);
	
  nSteps = sizeof(sequence) / sizeof(unsigned short) / 4;
	Serial.println (nSteps);
  
  lsPot.init(MIN_STEPS_PER_SECOND, MAX_STEPS_PER_SECOND, 0, 511);  // steps per second
  
}

void loop() {
  
  float stepsPerSecond;
  unsigned long usPerStep;
  long usRemaining;
  long msRemaining;
  static unsigned long now, loopIterationBegunAt_us, lastBlinkAt_ms = 0;
  static short currentStep = 0;
  
  now = millis();
  loopIterationBegunAt_us = micros();

  potValue = analogRead(paPot) - 512;
  // Serial.print ("Pot: ");
  // Serial.print (potValue);
  // Serial.print ("   ");
  
  reflectorValue = analogRead(paReflector);
  Serial.print ("Refl: ");
  Serial.print (reflectorValue);
  Serial.print ("   ");
  
	if (potValue >= 1) {
		dir = 1;
	} else if (potValue < 0) {
		potValue = -potValue;
		dir = -1;
	} else {
		potValue = 1;
		dir = 1;
	}
  
  // make one step
  currentStep = ( ( currentStep + nSteps ) + dir ) % nSteps;
  // Serial.print ("Step ");
  // Serial.print (currentStep);
  for (int i = 0; i < 4; i++) {
    digitalWrite (motorPins[i], sequence[currentStep][i]);
    // Serial.print ("  ");
    // Serial.print (sequence[currentStep][i]);
  }
  
  // Serial.println ();
  
  
  
  if ((now - lastBlinkAt_ms) > BLINK_INTERVAL) {
    digitalWrite (pdLED, 1 - digitalRead(pdLED));
    lastBlinkAt_ms = now;
  }
  
  // delay to get proper loop rate
  stepsPerSecond = lsPot.value(potValue);
  usPerStep = 1000000.0 / stepsPerSecond;
  usRemaining = usPerStep - (micros() - loopIterationBegunAt_us);
  Serial.print ("                                          ");
  Serial.print (stepsPerSecond);
  Serial.print ("  ");
  // Serial.print (usPerStep);
  // Serial.print ("  ");
  Serial.print (usRemaining);
  Serial.println ();
  if (usRemaining >= 16384) {
    delay (usRemaining / 1000);
  } else if (usRemaining > 0) {
    delayMicroseconds (usRemaining);
  } else {
    Serial.print ("Warning! Cannot step that fast. Loop time = ");
    Serial.print (micros() - loopIterationBegunAt_us);
    Serial.println ();
  }
}

