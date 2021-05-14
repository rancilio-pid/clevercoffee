#ifndef periodicTrigger_h
#define periodicTrigger_h

#include <Arduino.h>

class periodicTrigger {

public:
	periodicTrigger(int millisec);
	bool check();

private:
	int triggerInterval;
	unsigned long tref;
};

#endif
