/*
  Program: knock_sensor.ino
  Author:  Charles B. Malloch, PhD  2014-02-27
  Purpose: Demonstrate alternation of listening and sounding a piezo device
*/

const int pdBlinkyLED = 13;
const int paKnock = A8;
const int pdSqueak = 22;

const int BAUDRATE = 115200;

int threshold;

void setup () {

  pinMode ( pdBlinkyLED, OUTPUT );
  
  Serial.begin ( BAUDRATE );
  digitalWrite ( pdBlinkyLED, 1 );            // indicate that we're waiting for Serial
  while ( !Serial && millis() < 5000UL ) {};  // wait for serial port to open, but give up soon
  digitalWrite ( pdBlinkyLED, 0 );
  delay ( 250 );
  
  pinMode ( pdSqueak, INPUT);


  const long nLoops = 100L;
  unsigned long sum = 0, sum2 = 0;
  
  for ( int i = 0; i < nLoops; i++ ) {
    int v;
    v = maxSoundLevel ( paKnock );
    // Serial.print ( i ); Serial.print ( " " ); Serial.println ( v );
    sum += v;
    sum2 += v * v;
    digitalWrite ( pdBlinkyLED, 1 - digitalRead ( pdBlinkyLED ) );            // indicate that we're calibrating
    
  }
  float mean = sum / nLoops;
  float variance = sum2 / nLoops - mean * mean;
  threshold = int ( mean + 2 * sqrt ( variance ) + 1.99 );      // KLUDGE ! is this still necessary?
  Serial.print ( "nLoops: " ); Serial.println ( nLoops );
  Serial.print ( "sum: " ); Serial.println ( sum );
  Serial.print ( "sum2: " ); Serial.println ( sum2 );
  Serial.print ( "mean: " ); Serial.println ( mean );
  Serial.print ( "variance: " ); Serial.println ( variance );
  Serial.print ( "std dev: " ); Serial.println ( sqrt ( variance ) );
  Serial.print ( "threshold: " ); Serial.println ( threshold );
  
  digitalWrite ( pdBlinkyLED, 0 );
  
}

int maxSoundLevel ( int aPin ) {

  unsigned long timeStamp_ms;
  unsigned long aMoment_ms = 50;
  int vol, maxVol = 0;
  
  // listen for a moment
  timeStamp_ms = millis();
  while ( ( millis() - timeStamp_ms ) < aMoment_ms ) {
    vol = analogRead ( aPin );
    if ( vol > maxVol ) {
      maxVol = vol;
    }
  }
  return ( maxVol );
}

void loop () {
  static int numKnocks = 0;
  int maxVol = maxSoundLevel ( paKnock );
  
  if ( maxVol > threshold ) { 
    if ( numKnocks < 5 ) {
      numKnocks++;
    } else { 
      numKnocks = 1;
    }
    digitalWrite ( pdBlinkyLED, 1 );
    delay ( 100 );                    // delay after knock
    for ( int i = 0; i < numKnocks; i++ ) {
      tone ( pdSqueak, 440, 100 );
      delay ( 150 );                  // delay to make the next sound distinct
    }
    delay ( 100 );                    // delay to let speaker settle down after sounding
    pinMode ( pdSqueak, INPUT);
    digitalWrite ( pdBlinkyLED, 0 );
  }
    
  // Serial.println ( maxVol );
}