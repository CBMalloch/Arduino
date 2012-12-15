// test for TimedEvent class

#include <stdio.h>
#include <TimedEvent.h>

#define LEDPin 13
byte LEDStatus = 0;

char buffer[80];

TimedEvent banana = (TimedEvent)("grape");
TimedEvent evtLEDBlink = (TimedEvent)("LED");

void setup() {
	Serial.begin(115200);
	
	banana.set(2500L, *cb);
	banana.start();
	
	evtLEDBlink.set(1000L, *LEDToggle);
	evtLEDBlink.start();
}

void loop() {
	// Serial.print("0");
	banana.tick();
	// Serial.print("1");
	evtLEDBlink.tick();
	// Serial.print("2");
	// Serial.println();
	delay (2);
	
}

void cb(char *name) {
	banana.start();
	Serial.print(millis());
	Serial.print("   ");
	Serial.println(name);
}

void LEDToggle(char *name) {
	evtLEDBlink.start();
	// sprintf (buffer, "%10lu LEDT %s\n", millis(), name); Serial.print (buffer);
  LEDStatus = 1 - LEDStatus;
  if (LEDStatus) {
    digitalWrite(LEDPin, HIGH);   // sets the LED on
  } else {
    digitalWrite(LEDPin, LOW);
  }
}
