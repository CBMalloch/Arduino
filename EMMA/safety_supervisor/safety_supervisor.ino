#define VERSION "1.1."
#define VERDATE "2013-04-19"
#define PROGMONIKER "SS"

/* Read multiple ultrasonic rangefinders, multiplexed with one HCF4051 8-channel analog mux/demux
  Charles B. Malloch, PhD  2013-04-16
  
  Connections:
    Analog:
      0 to HCF4051 pin 3
    Digital:
       2 channel to read bit 0 (low order)  to HCF4051 pin 11
       3 channel to read bit 1 to HCF4051 pin 10
       4 channel to read bit 2 (high order) to HCF4051 pin 9
       5 initiate readings to propagate sequentially to all MaxSonars
       8 ESTOP wire (yellow)
       
     Connections to each MaxSonar:
      pin 1 BW  - orange - to GND - sets trigger mode to sequential firing in series
      pin 2 PW  -          not connected
      pin 3 AN  - yellow - analog output to HCF4051
      pin 4 RX  - green  - trigger from TX of previous sensor in series or from Arduino if first in series
      pin 5 TX  - blue   - trigger to RX of next sensor in series
      pin 6 +5  - red    - 5VDC power
      pin 7 GND - black  - ground
       
     Inputs to HCF4051:
      pin  1 channel 4 input/output
      pin  2 channel 6 input/output
      pin  3 common input/output
      pin  4 channel 7 input/output
      pin  5 channel 5 input/output
      pin  6 inhibit inputs (pull low)
      pin  7 Vee supply voltage (pull to GND)
      pin  8 Vss negative supply voltage (GND)
      pin  9 address bit 2 (high-order)
      pin 10 address bit 1
      pin 11 address bit 0 (low-order)
      pin 12 channel 3 input/output
      pin 13 channel 0 input/output
      pin 14 channel 1 input/output
      pin 15 channel 2 input/output
      pin 16 Vdd positive supply voltage (+5VDC)
*/

#define BAUDRATE 115200

#define ESTOP_DURATION_MS 30000LU
#define LOOP_PERIOD_MS 250LU

#define NSENSORS 8
// sensor 1 basically disabled for resetability
int min_distances_in [ NSENSORS ] = { 24, 6, 12, 12, 36, 36, 12, 12 };

#define paDistance 0

#define pdChannelBit0  2
#define pdChannelBit1  3
#define pdChannelBit2  4
#define pdInitiate     5

#define pdESTOP        8

#define pdSTATUSLED    9

byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                        { 0, pdChannelBit0, -1 },
                                        { 0, pdChannelBit1, -1 },
                                        { 0, pdChannelBit2, -1 },
                                        { 0, pdInitiate,    -1 },
                                        { 1, pdESTOP,       -1 },  // configured as input for open drain
                                        { 0, pdSTATUSLED,   -1 }
                                      };

int bufPtr;
#define lineLen 80
#define bufLen (lineLen + 3)
char strBuf[bufLen];

void setup() {
  Serial.begin (BAUDRATE);
  
  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( int i = 0; i < nPinDefs; i++ ) {
    if ( pinDefs [i] [0] == 1 ) {
      // input pin
      pinMode ( pinDefs [i] [1], INPUT );
      digitalWrite ( pinDefs [i] [1], 1 );  // enable internal 50K pullup
    } else {
      // output pin
      pinMode ( pinDefs [i] [1], OUTPUT );
      digitalWrite ( pinDefs [i] [1], 0 );
    }
  }
  
  bufPtr = 0;  
  snprintf ( strBuf, bufLen, "%s: EMMA Safety Supervisor v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
  Serial.print ( strBuf );
  
  digitalWrite (pdInitiate, 1);
  delay (1);
  digitalWrite (pdInitiate, 0);
  delay (100);
}

void loop() {
  unsigned short i, pa, barLen;
  int  counts;
  long mv;
  static unsigned long last_ESTOP_at_ms = 0;
  static unsigned long loop_began_at_ms = millis();
  
  for ( i = 0; i < NSENSORS; i++) {
    // choose sensor i
    digitalWrite (pdChannelBit0, i & 0x0001);
    digitalWrite (pdChannelBit1, i & 0x0002);
    digitalWrite (pdChannelBit2, i & 0x0004);
    
    delay ( 1 );
  
    snprintf (strBuf, bufLen, "%1d : ", i);
    Serial.print (strBuf);
    counts = analogRead(paDistance);
    mv = 5000L * counts / 1024L;
    // Serial.print (i); Serial.print ("    |    "); Serial.print (counts); Serial.print ("     :     "); Serial.println (mv);
    // 10mV / in => 1 mv / 1/10 in
    
    barLen = map (mv, 0, 1000, 0, lineLen);
    printBar (barLen);
    
    // delay (1000);
    
    // mv is distance in tenths of an inch
    if ( mv < ( min_distances_in [ i ] * 10 ) ) {
      // initiate E-STOP
      /*
        the ESTOP pin is normally configured as an *input* pin and has been 
        written with a logic HIGH. To assert the active-low ESTOP, we reverse
        these settings.
      */
      
      digitalWrite ( pdESTOP, 0 );
      pinMode ( pdESTOP, OUTPUT );
      digitalWrite ( pdSTATUSLED, 1 );
      
      last_ESTOP_at_ms = millis ();
      
      Serial.print   ( "ESTOP asserted in response to sonar " );
      Serial.print   ( i );
      Serial.print   ( " at distance of " );
      Serial.print   ( mv );
      Serial.println ( " tenths of an inch" );
    }
    
    if ( ( millis () - last_ESTOP_at_ms ) > ESTOP_DURATION_MS ) {
      // ESTOP timed out; lift it
      digitalWrite ( pdESTOP, 1 );
      pinMode ( pdESTOP, INPUT );
      digitalWrite ( pdSTATUSLED, 0 );
      
      Serial.println ( "ESTOP de-asserted after timeout interval" );
      last_ESTOP_at_ms = 0L;
    } else if ( last_ESTOP_at_ms && ( i == ( NSENSORS - 1 ) ) ) {
      Serial.println ( "ESTOP still asserted" );
    }
    
    
  }
  Serial.println ();
  
  // initiate new readings
  digitalWrite (pdInitiate, 1);
  delay (1);
  digitalWrite (pdInitiate, 0);
  // delay (50);

  // wait before beginning next loop
  long theDelay_ms = LOOP_PERIOD_MS - ( millis() - loop_began_at_ms );
  if ( theDelay_ms > 0 ) delay ( theDelay_ms );
  loop_began_at_ms = millis();
  
}

void printBar ( unsigned short barLen ) {
  barLen = constrain (barLen, 0, lineLen);
  for (int i = 0; i < barLen; i++) strBuf[i] = '-';
  strBuf[barLen] = '*';
  for (int i = barLen + 1; i <= lineLen; i++) strBuf[i] = ' ';
  strBuf[bufLen - 2] = '|';
  strBuf[bufLen - 1] = '\0';
  Serial.println (strBuf);
}
