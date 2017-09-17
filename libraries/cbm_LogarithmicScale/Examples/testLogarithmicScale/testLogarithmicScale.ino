#include <LogarithmicScale.h>

#define BAUDRATE 115200

LogarithmicScale lsPot;

void setup() {
	Serial.begin(BAUDRATE);
	
  lsPot.init(220.0, 880.0, 0, 24);
  
  for (int i = 0; i <= 24; i++) {
    Serial.print (i);
    Serial.print (" -> ");
    Serial.print (lsPot.value(i));
    Serial.println ();
  }
}

void loop() {
}
