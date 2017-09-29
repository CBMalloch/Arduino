/*
  programming:
    Arduino as ISP      USBtinyISP     ATtiny85 target
                   6      10
          10       5       5      W          1         Reset
          11       4       1      V          5         MOSI
          12       1       9      O          6         MISO
          13       3       7      G          7         SCK
                   6   4,6,8,10   Bk         4         GND
                   2       2      R          8         +5VDC
                            
*/

const int pdLED   = 4;

void setup () {
  pinMode ( pdLED,   OUTPUT );
  
  digitalWrite ( pdLED,   0 );
}

void loop () {
  flash ( pdLED, 250 );
  delay ( 250 );
  flash ( pdLED, 250 );
  delay ( 500 );
  flash ( pdLED, 1 );
  delay ( 500 );
}

void flash ( int pin, int dur_ms ) {
  digitalWrite ( pin, 1 ); delay ( dur_ms ); digitalWrite ( pin, 0 );
}