/*
	PWMActuator.cpp -  - library for PWM actuation of slow devices (such as heaters)
    where "slow" defines the PWM frequency as less than 100 ms (arbitrarily)
    practically speaking, slower than the built-in PWM mechanism of the Atmel chip
      can handle
	Created by Charles B. Malloch, PhD, September 15, 2011
	Released into the public domain
	
*/

extern "C" {
  #include <string.h>
}

#include <wiring.h>
#include "PWMActuator.h"

PWMActuator::PWMActuator() {
}

PWMActuator::PWMActuator( byte outputPin, 
                          unsigned long PWMPeriodms, 
                          float dutyCycle) :
                        _outputPin(outputPin),
                        _PWMPeriodms(PWMPeriodms),
                        _DutyCycle(dutyCycle) {
	this->_enabled = 0;
  this->_DutyCycleBeganAt = millis();
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

unsigned long PWMActuator::PWMPeriodms() {
	return (this->_PWMPeriodms);
}

unsigned long PWMActuator::PWMPeriodms (unsigned long newPWMPeriodms) {
	this->_PWMPeriodms = newPWMPeriodms;
  this->_DutyCyclems = long(float(this->_DutyCycle) * float(this->_PWMPeriodms) + 0.5);
	return (this->_PWMPeriodms);
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

float PWMActuator::dutyCycle() {
  if (this->_enabled) {
    return (this->_DutyCycle);
  } else {
    return (- this->_DutyCycle);
  }
}

float PWMActuator::dutyCycle(float newDutyCycle) {
	this->_newDutyCycle = newDutyCycle;
  this->_newDutyCyclems = long(float(this->_newDutyCycle) * float(this->_PWMPeriodms) + 0.5);
  if (this->_enabled) {
    return (this->_DutyCycle);
  } else {
    return (- this->_DutyCycle);
  }
}

long PWMActuator::dutyCyclems() {
  if (this->_enabled) {
    return (this->_DutyCyclems);
  } else {
    return (- this->_DutyCyclems);
  }
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

bool PWMActuator::enabled() {
  return (this->_enabled);
}

void PWMActuator::enable() {
	this->enable(true);
}

void PWMActuator::enable(bool enabled) {
  if (this->_enabled == enabled) {
    return;
  }
  this->_override_current_cycle = 1;
	this->_enabled = enabled;
  this->doControl();
}

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

void PWMActuator::doControl() {
  unsigned long now;
  now = millis();
	if (! this->_enabled) {
    digitalWrite(this->_outputPin, 0);
    return;
  }
  if (this->_override_current_cycle) {
    this->_DutyCycle = this->_newDutyCycle;
    this->_DutyCyclems = this->_newDutyCyclems;
  }
  if (now > (this->_DutyCycleBeganAt + this->_PWMPeriodms)) {
    // begin a new PWM period
    this->_DutyCycleBeganAt = now;
    this->_override_current_cycle = 0;
    this->_DutyCycle = this->_newDutyCycle;
    this->_DutyCyclems = this->_newDutyCyclems;
  }
  if (now < (this->_DutyCycleBeganAt + this->_DutyCyclems)) {
    // ON
    digitalWrite(this->_outputPin, 1);
  } else {
    // OFF (and fail-soft)
    digitalWrite(this->_outputPin, 0);
  }
}

