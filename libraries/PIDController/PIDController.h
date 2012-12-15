/*
	PIDController.h - 
  
    
	Created by Charles B. Malloch, PhD, September 28, 2011
	to be Released into the public domain
	
	2.001.000  2012-01-02 cbm  added getter for duty cycle
  
*/

#ifndef PIDController_h
#define PIDController_h

#define PIDCONTROLLER_VERSION "2.001.000"

#include <Thermosensor.h>
#include <PWMActuator.h>

typedef unsigned char byte;

class PIDController {
  public:
    PIDController();
    PIDController(  
                    PWMActuator *heater,
                    float coeff_K_c,
                    float coeff_tau_I,
                    float coeff_tau_D
                 );
    // virtual ~PIDController();
    
    // NOTE: all temperatures are in deg K
    
    float setpoint();
    float setpoint(float newSetpoint);
    
    void doControl(float currentValue);
    
    // for debugging: _coeff_K_c, _coeff_tau_I, _coeff_tau_D, _currentError;
    void get_internal_variables( 
                                float& setpointDegrees,
                                float& currentError, float& errorIntegral, 
                                float& valueP, float& valueI, float& valueD, 
                                float& dutyCycle,
                                float& coeff_K_c, float& coeff_tau_I, float& coeff_tau_D
                               );
    float get_dutyCycle ();
    
  protected:
  
  private:
    PWMActuator *_heater;
    float _coeff_K_c;
    float _coeff_tau_I;
    float _coeff_tau_D;
    float _currentError;
    unsigned long _tLastLoop;
    float _errorIntegral;
    float _setpoint;
    float _dutyCycle;
    float _valueP, _valueI, _valueD;
};

#endif