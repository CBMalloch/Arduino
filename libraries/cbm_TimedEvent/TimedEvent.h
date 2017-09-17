/*
	TimedEvent.h - library for timed events
	Created by Charles B. Malloch, PhD, May 12, 2009
	Released into the public domain
	
*/

#ifndef TimedEvent_h
#define TimedEvent_h

#define TIMED_EVENT_VERSION "1.001.000"

typedef unsigned char byte;

class TimedEvent {
	public:
		TimedEvent();
		TimedEvent(char *name);
		void set(unsigned long interval, void (*callback)(char *name));
    void start();
    void stop();
    void tick();
	protected:
	private:
		unsigned long msStart, interval;
		char name[21];
		byte armed;
		void (*callback)(char *name);
};

#endif