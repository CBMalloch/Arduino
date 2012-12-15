/*
	PWMActuator.h - library for PWM actuation of slow devices (such as heaters)
    where "slow" defines the PWM period of greater than 100 ms (arbitrarily)
    practically speaking, slower than the built-in PWM mechanism of the Atmel chip
      can handle
	Created by Charles B. Malloch, PhD, September 15, 2011
	Released into the public domain
	
*/

#ifndef PWMActuator_h
#define PWMActuator_h

#define PWM_ACTUATOR_VERSION "1.000.000"

typedef unsigned char byte;

class PWMActuator {
	public:
		PWMActuator();
		PWMActuator(byte outputPin, 
                unsigned long PWMPeriodms = 30000UL, 
                float dutyCycle = 0.5);
    void enable();
    void enable(bool enabled);
    bool enabled();
    unsigned long PWMPeriodms ();
    unsigned long PWMPeriodms (unsigned long  PWMPeriodms);
    float dutyCycle ();
		float dutyCycle (float newDutyCycle);
    long dutyCyclems ();
    
    void doControl();
	protected:
	private:
		byte _outputPin;
		bool _enabled;
    // _override_current_cycle is set by a change in _enabled.
    //    if it is set, we can interrupt the current PWM cycle with a change in duty cycle
    //    otherwise a change will be delayed until the start of the next cycle
    bool _override_current_cycle;
    unsigned long _PWMPeriodms;
    float _DutyCycle, _newDutyCycle;
    unsigned long _DutyCyclems, _newDutyCyclems;
		unsigned long _DutyCycleBeganAt;
};

#endif