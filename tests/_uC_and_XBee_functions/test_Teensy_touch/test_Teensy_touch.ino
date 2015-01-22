/*
  test_Teensy_touch
  Author: Charles B. Malloch, PhD
  Date: 2014-02-23
  
  This program reads the values from a touch sensor and uses them to make 
  a tone on a speaker to demonstrate the sensitivity of such a sensor.
  It also uses the touch sensor as a switch by determining whether the 
  value is high enough to be considered a touch.
  
*/

const int BAUDRATE = 115200;

const int pdTone = 10;
const int pdLED = 13;
const int pdTouch = 15;

int threshold;

void setup () {
  Serial.begin ( BAUDRATE );
  pinMode ( pdLED, OUTPUT );
  
  /*
    I don't know, and don't *want* to know ahead of time what the value returned 
    from the touch sensor will be. I figure it out by reading it a bunch of times
    and seeing how big it gets. Then I add a little as a cushion.
    
    If I wanted to be really sophisticated, I'd store the sum and the sum of the squares
    of the values, and use statistics to determine mean and standard deviation, and 
    then set the threshold to be 2 or 3 sigma above the mean.
    
    References: http://en.wikipedia.org/wiki/Variance and
                http://en.wikipedia.org/wiki/Algebraic_formula_for_the_variance
    
    The code for this would be:
    
      const int nLoops = 1000;
      unsigned long sum = 0, sum2 = 0;
      for ( int i = 0; i < nLoops; i++ ) {
        int v = touchRead ( pdTouch );
        sum += v;
        sum2 += v * v;
      }
      float mean = sum / nLoops;
      float variance = sum2 / nLoops - mean * mean;
      threshold = int ( mean + 2 * sqrt ( variance ) );
      Serial.print ( "mean: " ); Serial.println ( mean );
      Serial.print ( "variance: " ); Serial.println ( variance );
      Serial.print ( "std dev: " ); Serial.println ( sqrt ( variance ) );
      Serial.print ( "threshold: " ); Serial.println ( threshold );
      
    Math geek note: The above use of the mean and standard deviation ASSUMES THAT THE 
    RANDOM VARIABLE IS NORMALLY DISTRIBUTED. This is in general a reasonable assumption,
    but strictly speaking even in this case it's not quite true!

  */
  
  unsigned long peak = 0;
  for ( int i = 0; i < 1000; i++ ) {
    int v = touchRead ( pdTouch );
    if ( v >  peak ) peak = v;
  }
  threshold = peak + 10;    
    
}

void loop () {
  int t = touchRead ( pdTouch );
  Serial.println ( t );
  tone ( pdTone, t );
  // "t > threshold" evaluates to 1 if true, 0 if false
  // and 0 and 1 in turn correspond to LOW and HIGH.
  digitalWrite ( pdLED, t > threshold );
  delay ( 100 );
}