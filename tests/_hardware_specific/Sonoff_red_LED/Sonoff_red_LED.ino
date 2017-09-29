/*
  Test Sonoff red LED connection to ESP8266 pin 16 GPIO4
  
  SUCCESS!
*/

#define BAUDRATE 115200

const int pd_RED     =  4;
const int pd_GREEN   = 13;

void setup() {
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis() < 5000 );

  pinMode ( pd_RED,   OUTPUT );
  pinMode ( pd_GREEN, OUTPUT );

  Serial.println ( "Ready to rock and roll" );
}

void loop () {
  
  digitalWrite ( pd_RED, ( millis() % 1000 ) < 500 );
  digitalWrite ( pd_GREEN, ( ( millis() + 250 ) % 1000 ) < 500 );
  
}