/* Read multiple ultrasonic rangefinders, multiplexed with one HCF4051 8-channel analog mux/demux
  Charles B. Malloch, PhD  2013-04-16
  v1.0
  
  Connections:
    Analog:
      0 to HCF4051 pin 3
    Digital:
       2 channel to read bit 0 (low order)  to HCF4051 pin 11
       3 channel to read bit 1 to HCF4051 pin 10
       4 channel to read bit 2 (high order) to HCF4051 pin 9
       5 initiate readings to propagate sequentially to all MaxSonars
       
     Inputs to HCF4051:
      pin 1 channel 4 input/output
      pin 2 channel 6 input/output
      pin 3 common input/output
      pin 4 channel 7 input/output
      pin 5 channel 5 input/output
      pin 6 inhibit inputs (pull low)
      pin 7 Vee supply voltage (pull to GND)
      pin 8 Vss negative supply voltage (GND)
      pin 12 channel 3 input/output
      pin 13 channel 0 input/output
      pin 14 channel 1 input/output
      pin 15 channel 2 input/output
      pin 16 Vdd positive supply voltage (+5VDC)
*/

#define BAUDRATE 115200

#define paDistance     0

#define pdChannelBit0  2
#define pdChannelBit1  3
#define pdChannelBit2  4
#define pdInitiate     5

byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )
short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                        { 0,  2, -1 },
                                        { 0,  3, -1 },
                                        { 0,  4, -1 },
                                        { 0,  5, -1 },
                                        { 0, 13, -1 }
                                      };


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
      digitalWrite ( pinDefs [i] [1], 1 );  // enable internal pullups
    } else {
      // output pin
      pinMode ( pinDefs [i] [1], OUTPUT );
      digitalWrite ( pinDefs [i] [1], 0 );
    }
  }
  
  digitalWrite (pdInitiate, 1);
  delay (1);
  digitalWrite (pdInitiate, 0);
  delay (100);
}

void loop() {
  unsigned short i, pa, barLen;
  int  counts;
  long mv;
  for ( i = 0; i < 2; i++) {
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
    
    
  }
  Serial.println ();
  
  // initiate new readings
  digitalWrite (pdInitiate, 1);
  delay (1);
  digitalWrite (pdInitiate, 0);
  // delay (50);

  // size the delay by the length of time it will take to create it
  delay (250 * 1);
}

void printBar (unsigned short barLen) {
  barLen = constrain (barLen, 0, lineLen);
  for (int i = 0; i < barLen; i++) strBuf[i] = '-';
  strBuf[barLen] = '*';
  for (int i = barLen + 1; i <= lineLen; i++) strBuf[i] = ' ';
  strBuf[bufLen - 2] = '|';
  strBuf[bufLen - 1] = '\0';
  Serial.println (strBuf);
}