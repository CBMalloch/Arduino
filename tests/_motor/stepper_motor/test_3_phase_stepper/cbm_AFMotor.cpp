// Adafruit Motor shield library
// copyright Adafruit Industries LLC, 2009
// this code is public domain, enjoy!


#include <avr/io.h>
#include "WProgram.h"
#include "cbm_AFMotor.h"

static uint8_t latch_state;

#define MOTORDEBUG 1

AFMotorController::AFMotorController(void) {
}

void AFMotorController::enable(void) {
  // setup the latch
  /*
  LATCH_DDR |= _BV(LATCH);
  ENABLE_DDR |= _BV(ENABLE);
  CLK_DDR |= _BV(CLK);
  SER_DDR |= _BV(SER);
  */
  pinMode(MOTORLATCH, OUTPUT);
  pinMode(MOTORENABLE, OUTPUT);
  pinMode(MOTORDATA, OUTPUT);
  pinMode(MOTORCLK, OUTPUT);

  latch_state = 0;

  latch_tx();  // "reset"

  //ENABLE_PORT &= ~_BV(ENABLE); // enable the chip outputs!
  digitalWrite(MOTORENABLE, LOW);
}

void AFMotorController::latch_tx(void) {
  uint8_t i;

  //LATCH_PORT &= ~_BV(LATCH);
  digitalWrite(MOTORLATCH, LOW);

  //SER_PORT &= ~_BV(SER);
  digitalWrite(MOTORDATA, LOW);

  for (i=0; i<8; i++) {
    //CLK_PORT &= ~_BV(CLK);
    digitalWrite(MOTORCLK, LOW);

    if (latch_state & _BV(7-i)) {
      //SER_PORT |= _BV(SER);
      digitalWrite(MOTORDATA, HIGH);
    } else {
      //SER_PORT &= ~_BV(SER);
      digitalWrite(MOTORDATA, LOW);
    }
    //CLK_PORT |= _BV(CLK);
    digitalWrite(MOTORCLK, HIGH);
  }
  //LATCH_PORT |= _BV(LATCH);
  digitalWrite(MOTORLATCH, HIGH);
}

static AFMotorController MC;


/******************************************
               MOTORS
******************************************/

/*

	timer 2A controls *both* P1 and P2, respectively, of stepper 1
	timer 2B controls *both* P4 and P5, respectively, of stepper 1
	
	timer 0A controls *both* P1 and P2, respectively, of stepper 2
	timer 0B controls *both* P4 and P5, respectively, of stepper 2
	
	            Arduino outputs        motor/stepper pins         "PWM"
timer          A          B            A          B          A          B 
  0            6          5           M4S2      M3S2        PWM4      PWM3
  1            9         10
  2           11          3           M1S1      M2S1        PWM1      PWM2
  
  so to control three phases, we need to use timers 0 and 2
    
*/

inline void initPWM1(uint8_t freq) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega328P__)
    // use PWM from timer2A on PB3 (Arduino pin #11)
    TCCR2A |= _BV(COM2A1) | _BV(WGM20) | _BV(WGM21); // fast PWM, turn on oc2a
    TCCR2B = freq & 0x7;
    OCR2A = 0;
#elif defined(__AVR_ATmega1280__) 
    // on arduino mega, pin 11 is now PB5 (OC1A)
    TCCR1A |= _BV(COM1A1) | _BV(WGM10); // fast PWM, turn on oc1a
    TCCR1B = (freq & 0x7) | _BV(WGM12);
    OCR1A = 0;
#endif
    pinMode(11, OUTPUT);
}
inline void setPWM1(uint8_t s) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega328P__)
    // use PWM from timer2A on PB3 (Arduino pin #11)
    OCR2A = s;
#elif defined(__AVR_ATmega1280__) 
    // on arduino mega, pin 11 is now PB5 (OC1A)
    OCR1A = s;
#endif
}

inline void initPWM2(uint8_t freq) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega328P__)
    // use PWM from timer2B (pin 3)
    TCCR2A |= _BV(COM2B1) | _BV(WGM20) | _BV(WGM21); // fast PWM, turn on oc2b
    TCCR2B = freq & 0x7;
    OCR2B = 0;
#elif defined(__AVR_ATmega1280__) 
    // on arduino mega, pin 3 is now PE5 (OC3C)
    TCCR3A |= _BV(COM1C1) | _BV(WGM10); // fast PWM, turn on oc3c
    TCCR3B = (freq & 0x7) | _BV(WGM12);
    OCR3C = 0;
#endif
    pinMode(3, OUTPUT);
}
inline void setPWM2(uint8_t s) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega328P__)
    // use PWM from timer2A on PB3 (Arduino pin #11)
    OCR2B = s;
#elif defined(__AVR_ATmega1280__) 
    // on arduino mega, pin 11 is now PB5 (OC1A)
    OCR3C = s;
#endif
}

inline void initPWM3(uint8_t freq) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega328P__)
    // use PWM from timer0A / PD6 (pin 6)
    TCCR0A |= _BV(COM0A1) | _BV(WGM00) | _BV(WGM01); // fast PWM, turn on OC0A
    //TCCR0B = freq & 0x7;
    OCR0A = 0;
#elif defined(__AVR_ATmega1280__) 
    // on arduino mega, pin 6 is now PH3 (OC4A)
    TCCR4A |= _BV(COM1A1) | _BV(WGM10); // fast PWM, turn on oc4a
    TCCR4B = (freq & 0x7) | _BV(WGM12);
    //TCCR4B = 1 | _BV(WGM12);
    OCR4A = 0;
#endif
    pinMode(6, OUTPUT);
}
inline void setPWM3(uint8_t s) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega328P__)
    // use PWM from timer0A on PB3 (Arduino pin #6)
    OCR0A = s;
#elif defined(__AVR_ATmega1280__) 
    // on arduino mega, pin 6 is now PH3 (OC4A)
    OCR4A = s;
#endif
}

inline void initPWM4(uint8_t freq) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega328P__)
    // use PWM from timer0B / PD5 (pin 5)
    TCCR0A |= _BV(COM0B1) | _BV(WGM00) | _BV(WGM01); // fast PWM, turn on oc0a
    //TCCR0B = freq & 0x7;
    OCR0B = 0;
#elif defined(__AVR_ATmega1280__) 
    // on arduino mega, pin 5 is now PE3 (OC3A)
    TCCR3A |= _BV(COM1A1) | _BV(WGM10); // fast PWM, turn on oc3a
    TCCR3B = (freq & 0x7) | _BV(WGM12);
    //TCCR4B = 1 | _BV(WGM12);
    OCR3A = 0;
#endif
    pinMode(5, OUTPUT);
}
inline void setPWM4(uint8_t s) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega328P__)
    // use PWM from timer0A on PB3 (Arduino pin #6)
    OCR0B = s;
#elif defined(__AVR_ATmega1280__) 
    // on arduino mega, pin 6 is now PH3 (OC4A)
    OCR3A = s;
#endif
}

/******************************************
            3-PHASE STEPPERS
******************************************/

cbm_3Stepper::cbm_3Stepper(uint16_t steps) {
  MC.enable();

  revsteps = steps;
  wstep = 0;
	mstep = 0;

	latch_state &= ~_BV(MOTOR1_A) & ~_BV(MOTOR1_B) &
								 ~_BV(MOTOR2_A) & ~_BV(MOTOR2_B) &
								 ~_BV(MOTOR3_A) & ~_BV(MOTOR3_B) &
		 						 ~_BV(MOTOR4_A) & ~_BV(MOTOR4_B); // all motor pins to 0
	MC.latch_tx();
	
	// enable all three H bridges (motor 1, motor2, motor 4, pins 11, 3, and 6 respectively)
	pinMode(11, OUTPUT);
	pinMode( 3, OUTPUT);
	pinMode( 6, OUTPUT);
	digitalWrite(11, HIGH);
	digitalWrite( 3, HIGH);
	digitalWrite( 6, HIGH);

	// use PWM for microstepping support
	initPWM1(MOTOR12_64KHZ);
	initPWM2(MOTOR12_64KHZ);
	initPWM4(1);                                          // ???
	initPWM4(MOTOR34_64KHZ);      // was 1                                                // ???
	Serial.println("Generating cosine table:");
	for (int i = 0; i <= MICROSTEPS3; i++) {
		cosineTable[i] = cos(PI / 2.0 * i / MICROSTEPS3) * 255;
#ifdef MOTORDEBUGh
		Serial.print(i); Serial.print(": "); Serial.print(cosineTable[i]);
		Serial.println();
		delay(1);
#endif
	}
	
	setPWM1 (255);
	setPWM2 (  0);
	setPWM4 (  0);
}

void cbm_3Stepper::setSpeed(uint16_t rpm) {
  usperstep = 60000000 / (revsteps * rpm);
	steppingcounter = 0;
}

void cbm_3Stepper::release(void) {
	latch_state &= ~_BV(MOTOR1_A) & ~_BV(MOTOR1_B) &
								 ~_BV(MOTOR2_A) & ~_BV(MOTOR2_B) &
								 ~_BV(MOTOR3_A) & ~_BV(MOTOR3_B) &
		 						 ~_BV(MOTOR4_A) & ~_BV(MOTOR4_B); // all motor pins to 0
  MC.latch_tx();
}

void cbm_3Stepper::step(uint16_t steps, uint8_t dir,  uint8_t style) {
  uint32_t uspers = usperstep;
  uint8_t ret = 0;

  if (style == INTERLEAVE) {
    uspers /= 2;
  }
#ifdef MICROSTEPPING
 else if (style == MICROSTEP) {
    uspers /= MICROSTEPS3;
    steps *= MICROSTEPS3;
#ifdef MOTORDEBUG
    Serial.print("steps = "); Serial.println(steps, DEC);
#endif
  }
#endif

  while (steps--) {
    ret = onestep(dir, style);
    // rather than use delaymicros(), which blocks while it waits,
    // we use delay and then delay an additional ms when the
    // microseconds add up to beyond 1 ms
    delay(uspers/1000); // in ms
    steppingcounter += (uspers % 1000);
    if (steppingcounter >= 1000) {
      delay(1);
      steppingcounter -= 1000;
    }
  }
#ifdef MICROSTEPPING
  if (style == MICROSTEP) {
  	// do all the microsteps for one step right here
    //Serial.print("last ret = "); Serial.println(ret, DEC);
    while ((ret != 0) && (ret != MICROSTEPS3)) {
      ret = onestep(dir, style);
      delay(uspers/1000); // in ms
      steppingcounter += (uspers % 1000);
      if (steppingcounter >= 1000) {
				delay(1);
				steppingcounter -= 1000;
      } 
    }
  }
#endif

}

uint8_t cbm_3Stepper::onestep(uint8_t dir, uint8_t style) {
  uint8_t a, b, c;
	// note _BV(x) is defined as 1 << x
	a = _BV(MOTOR1_A);
	b = _BV(MOTOR2_A);
	c = _BV(MOTOR4_A);

  //Serial.print("step "); Serial.print(step, DEC); Serial.print("\t");
  // next determine what sort of stepping procedure we're up to
  if (style == SINGLE) {
		if (dir == FORWARD)
			wstep = (wstep + 1) % 3;
		else
			wstep = (wstep + 2) % 3;
		mstep = 0; // by def one coil on at a time
 
  } else if (style == DOUBLE) {
		if (dir == FORWARD)
			wstep = (wstep + 1) % 3;
		else
			wstep = (wstep + 2) % 3;
		mstep = HALFSTEP; // by def two coils equally on at a time
		
  } else if (style == INTERLEAVE) {
	 if (dir == FORWARD)
		 bump (&wstep, &mstep, HALFSTEP);
	 else 
		 bump (&wstep, &mstep, -HALFSTEP);
		 
  }  else if (style == MICROSTEP) {
	 if (dir == FORWARD)
		 bump (&wstep, &mstep, 1);
	 else 
		 bump (&wstep, &mstep, -1);
	}


  //Serial.print(" -> step = "); Serial.print(step/MICROSTEPS3, DEC); Serial.print("\t");

  setLocation(wstep, mstep);

  // release all
  // latch_state &= ~a & ~b & ~c & ~d; // all motor pins to 0

  //Serial.println(step, DEC);

  latch_state |= a | b | c ; // energize all three coils
   
  MC.latch_tx();
  return mstep;
}

void cbm_3Stepper::bump(int *pwstep, int *pmstep, int msteps) {
#ifdef MOTORDEBUGh
	Serial.print("Bump: (");
	Serial.print(*pwstep, DEC);
	Serial.print(" : ");
	Serial.print(*pmstep, DEC);
	Serial.print(") + ");
	Serial.print(msteps, DEC);
#endif
	*pmstep += msteps;
	if (*pmstep < 0) {
		*pwstep -= 1;
		*pmstep += MICROSTEPS3;
	}
	if (*pmstep >= MICROSTEPS3) {
		*pwstep += 1;
		*pmstep -= MICROSTEPS3;
	}
	if (*pwstep < 0) {
		*pwstep += 3;
	}
	if (*pwstep >= 3) {
		*pwstep -= 3;
	}
#ifdef MOTORDEBUGh
	Serial.print("= (");
	Serial.print(*pwstep, DEC);
	Serial.print(" : ");
	Serial.print(*pmstep, DEC);
	Serial.println(")");
#endif
}

void cbm_3Stepper::setLocation(uint8_t wstep, uint8_t mstep) {
	/*
	at (0,0), coil a only; at (1,0), coil b only; at (2,0), coil c only
	at (0,2), coil a mostly, coil b a little
	*/
	
#ifdef MOTORDEBUG
	Serial.print("setLocation (");
	Serial.print(wstep, DEC);
	Serial.print(", ");
	Serial.print(mstep, DEC);
	Serial.print("): <");
	Serial.print(mstep, DEC);
	Serial.print(", ");
	Serial.print(MICROSTEPS3 - mstep, DEC);
	Serial.print("> ");
	Serial.print(cosineTable[mstep], DEC);
	Serial.print(", ");
	Serial.print(cosineTable[MICROSTEPS3 - mstep], DEC);
	Serial.print("\n");
#endif
	switch (wstep) {
	case 0:
		setPWM1 (cosineTable[mstep]);
		setPWM2 (cosineTable[MICROSTEPS3 - mstep]);
		setPWM4 (0);
		break;
	case 1:
		setPWM2 (cosineTable[mstep]);
		setPWM4 (cosineTable[MICROSTEPS3 - mstep]);
		setPWM1 (0);
		break;
	case 2:
		setPWM4 (cosineTable[mstep]);
		setPWM1 (cosineTable[MICROSTEPS3 - mstep]);
		setPWM2 (0);
		break;
	}
}
