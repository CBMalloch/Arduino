/*

Test the running time of an Arduino running on a capacitor to bridge the time 
between turning one battery off and the other on.

Oscillate pin 11 and look at it vs. power to measure the amount of time that 
the Arduino continues to run once power has been cut.


New mission:
  Connections on PB-4R board:
    PB-4R pin     Arduino pin
      Input side:
        2             2
        3             GND
        4             3
        5             GND
        6             4
        7             GND
        8             5
        9             GND
      
      Output side:
        2             motor power
        3             battery A +
        4             motor power
        5             battery B +
        6             logic power
        7             battery A +
        8             logic power
        9             battery B +
        
  So to run motor A and logic B, want relay 1 (pins 2,3) and 4 (pins 8,9) ON,
  the others off (Arduino pins 2 and 5 HIGH, 3 and 4 LOW)
  ...       motor B and logic A,      relay 2 (pins 4,5) and 3 (pins 6,7) ON,
  the others off (Arduino pins 3 and 4 HIGH, 2 and 5 LOW)

  Arduino pin 7 will be an input, selecting MALB (motor A, logic B) when HIGH,
  MBLA when LOW.

Left to do:
  Assemble into a single board
  Use momentary switch to put power originally - expect it to latch
  Mount on EMMA

*/

#define pdMA 2
#define pdMB 3
#define pdLA 4
#define pdLB 5

#define pdSENSE 7

void setup () {

  // relay pins
  pinMode (2, OUTPUT); digitalWrite (2, LOW);
  pinMode (3, OUTPUT); digitalWrite (3, LOW);
  pinMode (4, OUTPUT); digitalWrite (4, LOW);
  pinMode (5, OUTPUT); digitalWrite (5, LOW);
  
  pinMode (pdSENSE, INPUT); digitalWrite (7, HIGH);
  
  // pinMode (11, OUTPUT);
  // analogWrite (11, 128);
  
  
}

#define DELAY_ms 10

void loop () {
  int mode = digitalRead ( pdSENSE );
  
  if ( mode == 1 ) {
    // mode is 1 -> MBLA
    // shut stuff off first, then wait, then turn on in correct configuration
    // shut logic off last, turn it on first
    digitalWrite ( MA, LOW );
    digitalWrite ( LB, LOW );
    delay ( DELAY_ms );
    digitalWrite ( LA, HIGH );
    digitalWrite ( MB, HIGH );
  } else {
    // mode is 0 -> MALB
    digitalWrite ( MB, LOW );
    digitalWrite ( LA, LOW );
    delay ( DELAY_ms );
    digitalWrite ( LB, HIGH );
    digitalWrite ( MA, HIGH );
  }
}