#define PROGNAME "powerSupplyTestLoad.ino"
#define VERSION "1.0.0"
#define VERDATE "2017-11-12"

/*
  Connect to a power supply and record its voltage output at a range of currents
  
  We control the current sink by changing the PWM average voltage going into an op-amp.
  The load resistor's value is known, and the op-amp is set up so that the PWM voltage 
  is matched by the voltage dropped by the load resistors.
  
  The PWM is sent through a Sallen-Key low-pass filter (which uses an op-amp, 2 capacitors, 
  and 2 resistors) to limit ripple while allowing good response time.
  See design tool at <http://sim.okawa-denshi.jp/en/OPstool.php>
  
  Asking for fc = 20Hz with damping ratio psi = 1.0, we are given the suggested values:
    R1 = 8.2kΩ, R2 = 8.2kΩ, C1 = 1uF, C2 = 1uF
  it calculates
    Cut-off frequency fc = 19.409139401451[Hz]
    Quality factor Q = 0.5
    Damping ratio ζ = 1
    500Hz -> -60dB
    From table of step response:
      Apparent 50ms rise time to 98%
      At 1.0ms, 0.8% change in value. 0.8% of 5V change is 40mV.
  The scope shows a PWM ripple of 13mV. Still more that I'd like to see...
  Scope shows 36ms rise time.
  Use these values. We are looking for DUT voltage changes on the order of 100mV, so 
  ripple is OK, sort of.  
  
  SEE THE CIRCUIT AT pro2:/Documents/Develop/eagle/Electronic_Load/electronic_load.sch
  
  We step the PWM value through its entire range and read back the current 
  provided to the current sink by the power supply under test.
  
  Program and device calibration
  
    Calibrate the current sunk vs the control voltage
  
    Verify the filtering by scoping the load resistor
  
  Voltage feedback calibration 
    ( to enable testing of power supplies whose output voltage exceeds the 5V capacity 
     of the Arduino's A2D )
    Connect the device to a known voltage source of 5VDC, with the control voltage GROUNDED
    to assure no load on the power supply.
    Run the program with the Mode pin grounded and adjust pot until counts is right
  
  To test the program, I set TESTING to 1 and loaded it.
  Start it with wire from pdMode ( pin 2 ) to GND, calibrated 5VDC source to adjust pot.
  Then remove the wire and it will set the first power level. Make meter measurements,
  cycle wire, and read program output. Repeat.
  
  To use the program, start it with wire from pdMode ( pin 2 ) to GND, 
  calibrated 5VDC source to adjust pot.
  Then remove the wire and it will run through its cycle.
*/

#include <Stats.h>
#include <FormatFloat.h>

#define VERBOSE 4
#define TESTING 0


const unsigned long BAUDRATE = 115200;
const unsigned short paVoltageInput = A0;
const unsigned short pdMode  =  2;
const unsigned short ppSet   =  3;
const unsigned short pdLED   = 13;

const int nStepsPWM = 256;
const float initialCurrent_a = 0.100;
const float finalCurrent_a   = 2.500;

const float rSense = 0.975;
const float a2dHeadroom_v = 0.5;
const float fCountsToVolts = ( 5.0 + a2dHeadroom_v ) / 1024.0;

#define bufLen 80
#define fbufLen 14
char strBuf[bufLen], strFBuf[fbufLen];

void setup () {

  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 2000 );
    
  pinMode ( pdMode, INPUT_PULLUP );
  pinMode ( pdLED, OUTPUT );
  
  #if VERBOSE >= 2
  
    Serial.print ( "\n\n" );
    Serial.print ( PROGNAME );
    Serial.print ( " v");
    Serial.print ( VERSION );
    Serial.print ( " (" );
    Serial.print ( VERDATE );
    Serial.print ( ")\n\n" );
  
  #endif
  
  int modeCalibrate = 1 - digitalRead ( pdMode );
  analogWrite ( ppSet, 0 );
  while ( modeCalibrate ) {
    // calibrate pot by setting it to a known 5V
  
    const int lookingForCounts = 5.0 / fCountsToVolts;
    int lastCounts = -5;
    // const int hysteresis = 5;
    // unsigned long countsLastChangedAt_ms = 0UL;
    // const unsigned long okTimeout_ms = 4000UL;
    
    // while ( ( millis() - countsLastChangedAt_ms ) < okTimeout_ms ) {
      int counts = analogRead ( paVoltageInput );
      static unsigned long lastPrintedAt_ms = 0UL;
      const unsigned long printInterval_ms = 500UL;
      if ( ( millis() - lastPrintedAt_ms ) > printInterval_ms ) {
        Serial.print ( "counts: " ); Serial.print ( counts );
        Serial.print ( "; looking for " ); Serial.print ( lookingForCounts );
        Serial.println ();
        lastPrintedAt_ms = millis();
      }
    //   if ( abs ( counts - lastCounts ) > hysteresis ) {
    //     lastCounts = counts;
    //     countsLastChangedAt_ms = millis();
    //   }
    // }
    modeCalibrate = 1 - digitalRead ( pdMode );
    static unsigned long lastThrobAt_ms = 0UL;
    const unsigned long throbberInterval_ms = 100UL;
    
    if ( ( millis() - lastThrobAt_ms ) > throbberInterval_ms ) {
      digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
      lastThrobAt_ms = millis();
    }
  }
  Serial.println ( "Done calibrating..." );
}

int volts2counts ( float command_v ) {
  // nStepsPWM is 256
  return ( round ( command_v / 5.0 * ( nStepsPWM - 1 ) ) );
}

void loop () {

  float control_v;
  float loadCurrent_a;
  float measuredVoltage_v, voltageVariability_v;
  
  const int finalCurrentCommand_counts = round ( nStepsPWM * finalCurrent_a / 5.0 );
  
  static unsigned long lastVoltageChangeAt_ms = millis();
  #if TESTING == 0
    // want 256 steps in less than 30 sec, so at least 10 steps / sec
    const unsigned long voltageChangeSettlingTime_ms = 50UL;
    const int nSteps = nStepsPWM;
  #else
    const unsigned long voltageChangeSettlingTime_ms = 0UL; // 10000UL;
    const int nSteps = 8;
  #endif
  
  const float deltaCurrent_a   = ( finalCurrent_a - initialCurrent_a ) / ( nSteps - 1 );

  float command_a = initialCurrent_a;
  float command_v = command_a * rSense;
  // PWM: 500Hz; 255 counts = 5V;
  int command_counts = volts2counts ( command_v );
  analogWrite ( ppSet, command_counts );
      
  while ( command_counts <= finalCurrentCommand_counts ) {
    
    // in testing and calibration, we might set voltageChangeSettlingTime_ms to zero and 
    // wait for a button press before proceeding to the next commanded current
    // otherwise, we proceed after voltageChangeSettlingTime_ms
    
    if ( voltageChangeSettlingTime_ms
           ? ( ( millis() - lastVoltageChangeAt_ms ) > voltageChangeSettlingTime_ms ) 
           : ! digitalRead ( pdMode ) ) {
      
      // if we're using the button, wait here for button release & debounce
      if ( ! voltageChangeSettlingTime_ms ) {
        unsigned long lastButtonAt_ms = millis();
        while ( ( millis() - lastButtonAt_ms ) < 50UL ) { 
          if ( ! digitalRead ( pdMode ) ) lastButtonAt_ms = millis();
        }
      }
    
      // Sample. Take a many readings to smooth the results
      int samplingPeriod_us = 4000UL;
      // int nReadingsDesired = 10;
      // unsigned long lastSampleAt_us = 0UL;
      // const unsigned long sampleInterval_us = samplingPeriod_us / nReadingsDesired;
      static Stats samples = Stats();
      samples.reset();
      unsigned long firstSampleAt_us = micros();
      // while ( samples.num() < nReadingsDesired ) {
      while ( ( micros() - firstSampleAt_us ) < samplingPeriod_us ) {
        // if ( ( micros() - lastSampleAt_us ) > sampleInterval_us ) {
          samples.record ( analogRead ( paVoltageInput ) );
        //   lastSampleAt_us = micros();
        // }
      }
      // unsigned long actualSamplingPeriod_us = micros() - firstSampleAt_us;  // s.b. samplingPeriod_us
      // Serial.print ( samplingPeriod_us ); Serial.print ( "\t" );
      // Serial.print ( actualSamplingPeriod_us ); Serial.print ( "\t" );
      // Serial.print ( actualSamplingPeriod_us ); Serial.print ( "\t" );      // 100 samples take 30200us, despite intent for it to take 4000us.
      // take 10 samples instead; that takes 5220us
      // we actually get 28 samples when going by time instead of number of samples
      // 28's pretty good, spread over 4ms or 2 cycles
      // Serial.print ( samples.num() ); Serial.print ( "\t" );
   
      measuredVoltage_v = samples.mean() * fCountsToVolts;
      voltageVariability_v = samples.stDev() * fCountsToVolts;
      
      // strncpy ( strBuf, "", bufLen );
      snprintf ( strBuf, bufLen, "%d\t", command_counts );
      formatFloat(strFBuf, fbufLen, command_a, 3);             // command amps
      strncat ( strBuf, strFBuf, bufLen );
      strncat ( strBuf, "\t", bufLen );
      formatFloat(strFBuf, fbufLen, measuredVoltage_v, 3);     // measured voltage
      strncat ( strBuf, strFBuf, bufLen );
      strncat ( strBuf, "\t", bufLen );
      formatFloat(strFBuf, fbufLen, voltageVariability_v, 6);  // stDev voltage
      strncat ( strBuf, strFBuf, bufLen );
      Serial.println ( strBuf );
      
      // go to next setting
      command_a += deltaCurrent_a;
      command_v = command_a * rSense;
      command_counts = volts2counts ( command_v );
      if ( command_counts <= finalCurrentCommand_counts ) {
        analogWrite ( ppSet, command_counts );
        lastVoltageChangeAt_ms = millis();
      }
    }
    
    static unsigned long lastThrobAt_ms = 0UL;
    const unsigned long throbberInterval_ms = 200UL;
    if ( ( millis() - lastThrobAt_ms ) > throbberInterval_ms ) {
      digitalWrite ( pdLED, 1 - digitalRead ( pdLED ) );
      lastThrobAt_ms = millis();
    }
    
    delay ( 2 );
    
    
    
    
    /*
      Results during testing:
      
        MOD-powerSupplyTestLoad.ino v0.2.0 (2017-10-26)

        Done calibrating...
        0.100	4.947
        0.100	5.006
        0.109	5.006
        0.119	5.006
        0.128	4.936
        0.138	5.006
        0.147	5.006
        0.156	5.006
        0.166	4.909
        0.175	5.006
        0.185	5.006
        0.194	5.006
        0.204	4.904
        0.213	5.006
        0.222	5.006
        0.232	5.006
        0.241	4.872
        0.251	5.011
        0.260	5.011
        0.269	5.006
        0.279	4.866
        0.288	5.006
        0.298	5.011
        0.307	5.006
        0.316	4.925
        0.326	5.011
        0.335	4.990
        0.345	4.990
        0.354	4.195
        0.364	4.952
        0.373	4.931
        0.382	4.947
        0.392	4.748
        0.401	4.866
        0.411	4.898
        0.420	4.888
        0.429	4.603
        0.439	4.796
        0.448	4.839
        0.458	4.743
        0.467	4.705
        0.476	4.737
        0.486	4.796
        0.495	4.501
        0.505	4.619
        0.514	4.678
        0.524	4.753
        0.533	4.490
        0.542	4.555
        0.552	4.651
        0.561	4.721
        0.571	3.722
        0.580	4.877
        0.589	4.598
        0.599	4.684
        0.608	3.636
        0.618	4.904
        0.627	4.549
        0.636	4.646
        0.646	3.432
        0.655	4.630
        0.665	4.555
        0.674	4.625
        0.684	3.292
        0.693	4.802
        0.702	4.506
        0.712	4.603
        0.721	3.019
        0.731	4.855
        0.740	4.485
        0.749	4.582
        0.759	2.917
        0.768	4.415
        0.778	4.453
        0.787	4.549
        0.796	2.696
        0.806	4.383
        0.815	4.431
        0.825	4.539
        0.834	2.476
        0.844	4.334
        0.853	4.431
        0.862	4.426
        0.872	2.342
        0.881	4.297
        0.891	4.404
        0.900	4.340
        0.909	2.342
        0.919	4.259
        0.928	4.383
        0.938	4.179
        0.947	2.256
        0.956	4.222
        0.966	4.372
        0.975	3.432
        0.985	1.971
        0.994	4.206
        1.004	4.367
        1.013	3.239
        1.022	3.330
        1.032	4.173
        1.041	4.340
        1.051	2.949
        1.060	2.691
        1.069	4.136
        1.079	4.324
        1.088	2.879
        1.098	3.776
        1.107	4.109
        1.116	4.308
        1.126	2.745
        1.135	2.686
        1.145	4.082
        1.154	4.313
        1.164	2.583
        1.173	3.137
        1.182	4.141
        1.192	4.308
        1.201	2.347
        1.211	4.093
        1.220	4.146
        1.229	4.109
        1.239	2.229
        1.248	4.034
        1.258	4.152
        1.267	3.164
        1.276	2.240
        1.286	4.189
        1.295	4.146
        1.305	3.545
        1.314	2.100
        1.324	3.985
        1.333	4.157
        1.342	2.911
        1.352	2.025
        1.361	3.953
        1.371	4.168
        1.380	2.616
        1.389	2.691
        1.399	3.921
        1.408	4.179
        1.418	2.385
        1.427	2.567
        1.436	3.985
        1.446	4.146
        1.455	2.288
        1.465	2.734
        1.474	3.991
        1.484	3.808
        1.493	2.191
        1.502	3.593
        1.512	4.012
        1.521	3.180
        1.531	2.100
        1.540	3.862
        1.549	4.007
        1.559	3.029
        1.568	2.046
        1.578	3.862
        1.587	4.007
        1.596	2.761
        1.606	2.009
        1.615	3.840
        1.625	4.012
        1.634	2.583
        1.644	1.977
        1.653	3.819
        1.662	4.023
        1.672	2.460
        1.681	1.950
        1.691	3.808
        1.700	4.028
        1.709	2.385
        1.719	1.928
        1.728	3.781
        1.738	4.028
        1.747	2.272
        1.756	1.864
        1.766	3.776
        1.775	3.604
        1.785	2.165
        1.794	3.040
        1.804	3.765
        1.813	4.334
        1.822	2.127
        1.832	1.901
        1.841	3.856
        1.851	2.959
        1.860	2.046
        1.869	2.691
        1.879	3.883
        1.888	2.637
        1.898	1.977
        1.907	3.663
        1.916	3.894
        1.926	2.455
        1.935	1.944
        1.945	3.733
        1.954	3.937
        1.964	2.234
        1.973	1.896
        1.982	3.711
        1.992	3.754
        2.001	2.165
        2.011	1.885
        2.020	3.695
        2.029	3.507
        2.039	2.138
        2.048	1.885
        2.058	3.690
        2.067	3.223
        2.076	2.100
        2.086	1.875
        2.095	3.674
        2.105	3.104
        2.114	2.046
        2.124	1.950
        2.133	3.760
        2.142	2.675
        2.152	1.993
        2.161	1.687
        2.171	3.776
        2.180	2.492
        2.189	1.950
        2.199	2.997
        2.208	3.760
        2.218	2.487
        2.227	1.934
        2.236	2.546
        2.246	3.787
        2.255	2.315
        2.265	1.917
        2.274	3.561
        2.284	3.797
        2.293	2.245
        2.302	1.896
        2.312	3.953
        2.321	3.658
        2.331	2.132
        2.340	1.875
        2.349	3.620
        2.359	4.222
        2.368	2.084
        2.378	1.864
        2.387	3.609
        2.396	2.729
        2.406	1.993
        2.415	1.842
        2.425	3.588
        2.434	2.471
        2.444	1.939
        2.453	1.708
        2.462	3.690
        2.472	2.299
        2.481	1.907
        2.491	2.234
        Done...
        
      The commanded current and the calculated current are identical. My mistake for 
      thinking they would be different - calculated the same way from the same data!
      
      The third value, though, is the measured voltage. This variation is most likely
      from the ripple in the PWM. Need to scope this value as we step slowly.
      
    */
    
    
    
  
  }
    
  analogWrite ( ppSet, 0 );
  Serial.println ( "Done..." );
  while ( 1 ) delay ( 20 );

}