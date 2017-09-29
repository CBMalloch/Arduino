// Adafruit Motor shield library
// copyright Adafruit Industries LLC, 2009
// this code is public domain, enjoy!

// with help from cbm

#ifndef _AFMotor_h_
#define _AFMotor_h_

#include <inttypes.h>
#include <avr/io.h>

// comment out this line to remove microstepping support
// for smaller library. Be sure to delete the old library objects!
#define MICROSTEPPING 1

#ifdef MICROSTEPPING
#define MICROSTEP 8
#endif

#define MOTOR12_64KHZ _BV(CS20)  // no prescale
#define MOTOR12_8KHZ _BV(CS21)   // divide by 8
#define MOTOR12_2KHZ _BV(CS21) | _BV(CS20) // divide by 32
#define MOTOR12_1KHZ _BV(CS22)  // divide by 64

#define MOTOR34_64KHZ _BV(CS00)  // no prescale
#define MOTOR34_8KHZ _BV(CS01)   // divide by 8
#define MOTOR34_1KHZ _BV(CS01) | _BV(CS00)  // divide by 64

#define MOTOR1_A 2
#define MOTOR1_B 3
#define MOTOR2_A 1
#define MOTOR2_B 4
#define MOTOR4_A 0
#define MOTOR4_B 6
#define MOTOR3_A 5
#define MOTOR3_B 7

#define FORWARD 1
#define BACKWARD 2
#define BRAKE 3
#define RELEASE 4

#define SINGLE 1
#define DOUBLE 2
#define INTERLEAVE 3


/*
#define LATCH 4
#define LATCH_DDR DDRB
#define LATCH_PORT PORTB

#define CLK_PORT PORTD
#define CLK_DDR DDRD
#define CLK 4

#define ENABLE_PORT PORTD
#define ENABLE_DDR DDRD
#define ENABLE 7

#define SER 0
#define SER_DDR DDRB
#define SER_PORT PORTB
*/

// Arduino pin names
#define MOTORLATCH 12
#define MOTORCLK 4
#define MOTORENABLE 7
#define MOTORDATA 8

#define MICROSTEPS3 16  // 8, 16 & 32 are popular
#define HALFSTEP MICROSTEPS3 / 2

class AFMotorController {
  public:
    AFMotorController(void);
    void enable(void);
    void latch_tx(void);
};

class cbm_3Stepper {
 public:
  cbm_3Stepper(uint16_t);
  void setSpeed(uint16_t);
  void release(void);
  void step(uint16_t steps, uint8_t dir,  uint8_t style = SINGLE);
  uint8_t onestep(uint8_t dir, uint8_t style);
	void bump(int *wstep, int *mstep, int msteps);
  void setLocation(uint8_t wstep, uint8_t mstep);

  uint16_t revsteps; // # steps per revolution
  uint32_t usperstep, steppingcounter;

  int wstep, mstep;         // whole step number (0 - 2), microstep number (0 - MICROSTEPS3-1)

  uint8_t cosineTable[MICROSTEPS3 + 1];
 private:
};

// uint8_t getlatchstategetlatchstate(void);

#endif
