/*
	PIDController.h - 
  
    
	Created by Charles B. Malloch, PhD, September 28, 2011
	to be Released into the public domain
	
	Change History:
		2011-10-17 v1.001000 cbm corrected sign of error
	
*/

extern "C" {
  #include <string.h>
}

#include <math.h>
#include <wiring.h>
#include "PIDController.h"

#define epsilon 1e-6

PIDController::PIDController() {}

PIDController::PIDController(  
                              PWMActuator *heater,
                              float coeff_K_c,
                              float coeff_tau_I,
                              float coeff_tau_D
                            ) :
                            _heater(heater),
                            _coeff_K_c(coeff_K_c),
                            _coeff_tau_I(coeff_tau_I),
                            _coeff_tau_D(coeff_tau_D) {
  this->_heater->enable(0);
  this->_currentError = 0.0;
  this->_errorIntegral = 0.0;
  this->_tLastLoop = 0;
}

// PIDController::~PIDController() {}

float PIDController::setpoint() {
  return (this->_setpoint);
}

float PIDController::setpoint(float newSetpoint) {
  this->_setpoint = newSetpoint;
  return (this->_setpoint);
}

void PIDController::doControl(float currentValue) {

  // NOTE: all temperatures should be in degK
  
  unsigned long msNow;
  float deltaTsec;
  
  msNow = millis();
  deltaTsec = float(msNow - this->_tLastLoop) / 1000.0;
  this->_tLastLoop = msNow;

  // calculate duty cycle
  
  float previousError = this->_currentError;
  this->_currentError = this->_setpoint - currentValue;
  this->_errorIntegral += this->_currentError * deltaTsec;
   
  this->_valueP = this->_coeff_K_c * this->_currentError;
  // allow integral windup only when proportional is satisfied
  this->_valueI = this->_coeff_K_c / this->_coeff_tau_I * this->_errorIntegral;
  this->_valueD = this->_coeff_K_c * this->_coeff_tau_D 
              * (this->_currentError - previousError) / deltaTsec;

  float dutyCycle;
  dutyCycle = this->_valueP + this->_valueD + this->_valueI;
  this->_dutyCycle = constrain(dutyCycle, 0.0, 1.0);
  if ((abs(this->_dutyCycle - dutyCycle) > epsilon)
      // and control error is the same sign as the control signal
      && (this->_currentError * (dutyCycle - 0.5) >= 0.0)) {
    // undo the current piece of integral windup
    this->_errorIntegral -= this->_currentError * deltaTsec;
  }

	this->_heater->dutyCycle (this->_dutyCycle);
  this->_heater->doControl ();

}

// for debugging only
void PIDController::get_internal_variables(  
                                            float& setpointDegrees,
                                            float& currentError, float& errorIntegral,
                                            float& valueP, float& valueI, float& valueD, 
                                            float& dutyCycle,
                                            float& coeff_K_c, float& coeff_tau_I, float& coeff_tau_D
                                          ) {
  setpointDegrees = this->_setpoint;  
  currentError = this->_currentError;  
  errorIntegral = this->_errorIntegral;  
  valueP = this->_valueP;
  valueI = this->_valueI;
  valueD = this->_valueD;
  dutyCycle = this->_dutyCycle;
  coeff_K_c = this->_coeff_K_c;  
  coeff_tau_I = this->_coeff_tau_I;  
  coeff_tau_D = this->_coeff_tau_D;  
}

float PIDController::get_dutyCycle () {
  return (this->_dutyCycle);
}
