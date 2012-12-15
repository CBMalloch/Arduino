/*
	Thermosensor.h - Bridge pattern library for thermal sensors
    RTD (thermistor) devices
    LM334 temperature-sensitive current sources
    DS75 two-wire digital thermometers
    
	Created by Charles B. Malloch, PhD, September 15, 2011
	to be Released into the public domain
	
*/

extern "C" {
  #include <string.h>
}

#include <math.h>
#include <wiring.h>
#include "Thermosensor.h"

// Thermosensor::Thermosensor() {
// }

/*

Note - if you get the error "undefined reference to `vtable blah blah", 
you have probably tried to create a Thermosensor. Try creating more specifically
the type of thermosensor you want (e.g. Thermistor) and the error will 
probably go away.

*/

float K2F(float degK) {
	return (1.8 * degK - FP_H2O_R + 32);  // degR, then degF
}

float F2K(float degF) {
	return ((degF - 32 + FP_H2O_R) / 1.8);
}

float C2K(float degC) {
	return (degC + FP_H2O_K);
}

float K2C(float degK) {
	return (degK - FP_H2O_K);
}

float Thermosensor::getDegK () {};

// <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <-> <->

Thermistor::Thermistor() {
}

Thermistor::Thermistor (
                          int pin,
                          float resistor_value,
                          float R0,
                          float T0degK,
                          float beta
                       ) :
                       _pin(pin),
                       _resistor_value(resistor_value),
                       _R0(R0),
                       _T0degK(T0degK),
                       _beta(beta) {
  this->_r_infty = this->_R0 * exp(-this->_beta / (this->_T0degK));
}

float Thermistor::getDegK () {
  int counts = analogRead(this->_pin);
  return (getDegK(counts));
}

float Thermistor::getDegK (int counts) {
  // from old calculation
  // degK = T0 * B / (T0 * log(thermistorResistance / R0) + B);  // log is base e...
  float R = (this->_resistor_value * float(counts)) / (1024.0 - counts);
  return (this->_beta / log(R / this->_r_infty));  // note log is NATURAL log
}