/*
  Test the response of an analog input with a high-impedance device v0.0.1
  Charles B. Malloch, Phd  2013-01-03
  
  LEDs produce *very* little current, making them high impedance devices. The analog read
  mechanism of the Arduino consists of a mux feeding a single sample-and-hold with a 4nF capacitor.
  In order to feed this capacitor in a reasonable amount of time, we need to give it charge from 
  a low-impedance source, like a voltage follower.
  
  In this program, we're just going to look at the charge/discharge curves.
*/

#define BAUDRATE 115200
#define lineLen 60
#define bufLen (lineLen + 3)
char strBuf[bufLen];

#define paLED A0
#define pa5V  A1
#define paGND A2

void setup () {
  Serial.begin ( BAUDRATE );
  
  pinMode ( paLED, INPUT );
  digitalWrite ( paLED, LOW );
  pinMode ( pa5V, INPUT );
  digitalWrite ( pa5V, LOW );
  pinMode ( paGND, INPUT );
  digitalWrite ( paGND, LOW );
  
  /*
    We'll try delays from 1ms to 2048ms in powers of 2
    at each delay, we'll connect to ground, read, connect to pin, read, 
    connect to 5V, read, connect to pin, read, all 100 times, and average 
    the readings of the pin
  */
  
 int reading;
 float sumRising, sumFalling;
 int i, iter;
 int nIterations = 100;
 
 long currentDelay, delays[] = { 1L, 2L, 4L, 8L, 16L, 32L, 64L, 128L, 256L, 512L, 1024L, 2048L };
 int nDelays = sizeof (delays) / sizeof (long);
 
  for (i = 0; i < nDelays; i++) {
    
    sumRising = 0.0;
    sumFalling = 0.0;
    currentDelay = delays[i];
    
    for (iter = 0; iter < nIterations; iter++) {
    
      reading = analogRead(paGND);
      delay (currentDelay);
      reading = analogRead(paLED);
      sumRising += float(reading);
      delay (currentDelay);
      
      reading = analogRead(pa5V);
      delay (currentDelay);
      reading = analogRead(paLED);
      sumFalling += float(reading);
      delay (currentDelay);

    }
    
    
    Serial.print ("At delay of "); 
    Serial.print (delays[i]); 
    Serial.print (" average values were ("); 
    Serial.print (sumRising / nIterations);
    Serial.print (", "); 
    Serial.print (sumFalling / nIterations);
    Serial.println (")");
    
    if (1 && abs ( sumRising - sumFalling ) < 2 * nIterations) {
      i = 99;
    }
  }  // each delay
}

void loop () {
  Serial.println (analogRead(paLED));
  delay (200);
}

/*

With the LED direct from GND to A0:
At delay of 1 average values were (279.16, 158.39)
At delay of 2 average values were (280.22, 158.43)
At delay of 4 average values were (280.12, 158.24)
At delay of 8 average values were (279.68, 158.43)
At delay of 16 average values were (279.67, 158.48)
At delay of 32 average values were (279.54, 158.65)
At delay of 64 average values were (280.08, 158.70)
At delay of 128 average values were (280.42, 158.88)
At delay of 256 average values were (280.25, 158.80)
At delay of 512 average values were (280.06, 158.93)
At delay of 1024 average values were (280.08, 158.82)

With a voltage follower circuit (NPN transistor w/ C +5V, E output and 1K to GND, B 1K to 5V, 1K to GND, and to LED +):
At delay of 1 average values were (765.86, 765.37)
At delay of 2 average values were (765.85, 765.28)
At delay of 4 average values were (765.84, 765.28)
At delay of 8 average values were (765.74, 765.15)
At delay of 16 average values were (765.55, 765.06)
At delay of 32 average values were (765.22, 765.00)
At delay of 64 average values were (765.02, 765.00)
At delay of 128 average values were (765.00, 765.00)
etc.

With a better voltage follower circuit (N-channel JFET transistor w/ D +5V, S output and 1K to GND, G to LED + parallel with 152 capacitor (1.5nF):
At delay of 1 average values were (395.71, 395.48)
--> this corresponds to 395/1024 * 5V = 1.92V (meas at 2.06), despite that we measure 1.21 across LED; 


*/