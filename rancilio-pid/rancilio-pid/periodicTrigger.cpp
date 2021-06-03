#include "PeriodicTrigger.h"

#include <Arduino.h>


PeriodicTrigger::PeriodicTrigger(int ms) {
 	triggerInterval = ms;
 	tref = millis();
}

bool PeriodicTrigger::check() 
{ 
	if ((millis()-tref) > triggerInterval) {
		tref += triggerInterval;
		return true;
	}
	else
		return false;
}

bool PeriodicTrigger::reset()
{
	tref = millis();
} 

bool PeriodicTrigger::reset(int ms)
{
 	triggerInterval = ms;
	tref = millis();
} 