/*
	Thermosensor.h - Bridge pattern library for thermal sensors
    RTD (thermistor) devices
    LM334 temperature-sensitive current sources
    DS75 two-wire digital thermometers
    
	Created by Charles B. Malloch, PhD, September 15, 2011
	to be Released into the public domain
  
  0.001.001 2011-12-07 cbm changed the names of constants zeroC and zeroF 
                           to FP_H2O_K and FP_H2O_R respectively
  0.002.000 2011-12-13 cbm moved conversions outside any class
                           corrected FP_H2O_R
	
*/

#ifndef Thermosensor_h
#define Thermosensor_h

#define THERMOSENSOR_VERSION "0.001.001"

#define aref_voltage 5.0
#define fullScaleCounts 1023
#define FP_H2O_K 273.15
// 459.67 degR = 0 degF
#define FP_H2O_R (459.67 + 32)

typedef unsigned char byte;

float K2F(float degK);
float K2C(float degK);
float F2K(float degK);
float C2K(float degK);

class Thermosensor {
  public:
    // Thermosensor();
    // virtual ~Thermosensor();
    virtual float getDegK();
  protected:
  private:
    // Thermosensor _device;
};

class Thermistor: public Thermosensor {
/*
from the Wikipedia page "Thermistor"
\frac{1}{T}=\frac{1}{T_0} + \frac{1}{B}\ln \left(\frac{R}{R_0}\right)

Where the temperatures are in kelvins and R0 is the resistance at temperature T0 (usually 25 #C = 298.15 K). Solving for R yields:

    R=R_0e^{B(\frac{1}{T} - \frac{1}{T_0})}

or, alternatively,

    R=r_\infty e^{B/T}

where r_\infty=R_0 e^{-{B/T_0}}. This can be solved for the temperature:

    T={B\over { {\ln{(R / r_\infty)}}}}
    
    NTSD1XH103FPB:
      R_25 = 10k \pm 1\%
      \beta = 3380 \pm 0.5\% (in degK, good for 25-50degC)
    
    NTSD1XH104FPB s.b. 100K at 25degC
    
    wiring:
      5VDC ------
                |
                device
                |
                \
                /
                \ dropping resistor
                / value 2/3 that of device nominal?
                |
      ground ----
              
*/

  public:
    Thermistor();
    Thermistor (
                  int pin,
                  float resistor_value,
                  float R0,
                  float T0degK,
                  float beta
               );
    float getDegK();
    float getDegK(int counts);
                     
  protected:
  private:
    int _pin;
    float _resistor_value;
    float _beta;
    float _R0, _T0degK;
    float _r_infty;
};

class LM_334_Current_Source {
  public:
  protected:
  private:
    
};

class DS75_Two_Wire {
  public:
  protected:
  private:
    
};

class Thermocouple_Type_H {
  public:
  protected:
  private:
};

#endif