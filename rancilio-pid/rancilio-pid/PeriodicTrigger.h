#ifndef PeriodicTrigger_h
#define PeriodicTrigger_h

class PeriodicTrigger {

public:
	PeriodicTrigger(int millisec);
	bool check();

private:
	int triggerInterval;
	unsigned long tref;
};
#endif
