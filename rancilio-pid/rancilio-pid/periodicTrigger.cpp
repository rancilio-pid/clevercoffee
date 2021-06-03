#include "periodicTrigger.h"

periodicTrigger::periodicTrigger(int ms) 
{
 	triggerInterval = ms;
 	tref = millis();
}

bool periodicTrigger::check() 
{ 
	if ((millis()-tref) > triggerInterval) {
		tref += triggerInterval;
		return true;
	}
	else
		return false;
}
