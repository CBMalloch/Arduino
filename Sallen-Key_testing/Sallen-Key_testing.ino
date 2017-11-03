#define PROGNAME "Sallen-Key_testing.ino"
#define VERSION "0.0.2"
#define VERDATE "2017-11-02"

/*
  Send PWM through a Sallen-Key filter (which uses an op-amp, 2 capacitors, and 2 
  resistors) to measure response time and ripple.
  See design tool at <http://sim.okawa-denshi.jp/en/OPstool.php>
  
  Providing R1 = 330kΩ, R2 = 680kΩ, C1 = 22nF, C2 = 10nF
  it calculates
    Cut-off frequency fc = 22.651520775095[Hz]
    Quality factor Q = 0.69566698475004
    Damping ratio ζ = 0.71873469772273  
    Overshoot (in absolute value)
      The 1st peak  gpk = 1.04 (t =0.031[sec])
      The 2nd peak  gpk = 1 (t =0.064[sec])
      The 3rd peak  gpk = 1 (t =0.095[sec])
  The scope shows a PWM ripple of 840mV. That is way too much ripple.
  
  Asking for fc = 50Hz with damping ratio psi = 1.0, we are given the suggested values:
    R1 = 33kΩ, R2 = 33kΩ, C1 = 0.1uF, C2 = 0.1uF
  providing
    Cut-off frequency fc = 48.228770633908[Hz]
    Quality factor Q = 0.5
    Damping ratio ζ = 1
  Looks like about 20ms rise time
  The scope shows a PWM ripple of 73mV. That is still too much ripple.
    
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
    
*/

#define VERBOSE 4

const unsigned long BAUDRATE = 115200;
const unsigned short ppSet   =  3;
const unsigned short ppLED   = 11;

const int nStepsPWM = 256;

void setup () {

  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 2000 );
    
  pinMode ( ppLED, OUTPUT );
  
  #if VERBOSE >= 2
  
    Serial.print ( "\n\n" );
    Serial.print ( PROGNAME );
    Serial.print ( " v");
    Serial.print ( VERSION );
    Serial.print ( " (" );
    Serial.print ( VERDATE );
    Serial.print ( ")\n\n" );
  
  #endif
  
}

void loop () {

  // PWM: 500Hz; 255 counts = 5V
  static int command_counts = 0;
      
  static unsigned long lastVoltageChangeAt_ms = 0UL;

  const unsigned long voltageChangeSettlingTime_ms = 500UL;
  
  if ( ( millis() - lastVoltageChangeAt_ms ) > voltageChangeSettlingTime_ms ) {
    switch ( command_counts ) {
      case 0:
        command_counts = 63;
        break;
      case 63:
        command_counts = 127;
        break;
      case 127:
        command_counts = 191;
        break;
      case 191:
        command_counts = 255;
        break;
      case 255:
        command_counts = 0;
        break;
      default:
        command_counts = 0;
        break;
    }
    analogWrite ( ppSet, command_counts );
    analogWrite ( ppLED, command_counts );
    lastVoltageChangeAt_ms = millis();
  }
      
}