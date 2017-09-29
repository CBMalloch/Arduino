/*
  programming:
    Arduino as ISP      USBtinyISP       ATtiny85 target
          10                5 W                 1         Reset
          11                4 V                 5         MOSI
          12                1 O                 6         MISO
          13                3 G                 7         SCK
                            6 Bk                4         GND
                            2 R                 8         +5VDC
                            
*/

const int pdRed   = 4;
const int pdGreen = 0;
const int pdBlue  = 1;

void setup () {
  pinMode ( pdRed,   OUTPUT );
  pinMode ( pdGreen, OUTPUT );
  pinMode ( pdBlue,  OUTPUT );
  
  digitalWrite ( pdRed,   0 );
  digitalWrite ( pdGreen, 0 );
  digitalWrite ( pdBlue,  0 );
}

void loop () {
  static int b = 0;
  analogWrite ( pdRed, b );
  analogWrite ( pdGreen, b );
  analogWrite ( pdBlue, b );
  b = ( b >= 255 ) ? 0 : b + 1;
  delay ( 2 );
}