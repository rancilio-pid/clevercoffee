#ifndef PeriodicTrigger_h
#define PeriodicTrigger_h

class PeriodicTrigger {

public:
	PeriodicTrigger(int millisec);
	bool check();
	bool reset();
	bool reset(int millisec);

private:
	int triggerInterval;
	unsigned long tref;
};
#endif