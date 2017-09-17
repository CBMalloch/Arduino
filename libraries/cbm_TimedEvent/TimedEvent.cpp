/*
	TimedEvent.cpp - library for timed events
	Created by Charles B. Malloch, PhD, May 12, 2009
	Released into the public domain
	
*/

extern "C" {
  #include <string.h>
}

#include <Arduino.h>
#include <wiring.h>
#include "TimedEvent.h"

TimedEvent::TimedEvent() {
}

TimedEvent::TimedEvent(char *name) {
	strncpy(this->name, name, 20);
	this->armed = 0;
}

void TimedEvent::set(unsigned long interval, void (*callback)(char *name)) {
	this->interval = interval;
	this->callback = callback;
	this->armed    = 0;
}

void TimedEvent::start() {
	this->msStart = millis();
	this->armed   = 1;
}

void TimedEvent::stop() {
	this->armed = 0;
}

void TimedEvent::tick() {
	if (this->armed != 0) {
		unsigned long now = millis();
		if (now >= (this->msStart + this->interval)) {
			this->msStart = now;
			this->armed = 0;
			this->callback(this->name);
		}
	}
}
