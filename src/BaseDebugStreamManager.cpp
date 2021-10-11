#include "BaseDebugStreamManager.h"
#include "DebugStreamManager.h"


#if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
BaseDebugStreamManager* BaseDebugStreamManager::instance = new DebugStreamManager();
#endif

BaseDebugStreamManager::BaseDebugStreamManager() 
{
	#if(DEBUGMETHOD == 1)
	Serial.begin(115200);
  	#endif  
    #if(DEBUGMETHOD == 2)
	Debug.begin(HOSTNAME);
    #endif  
}


#if (DEBUGMETHOD == 2)
void BaseDebugStreamManager::callprocessCmdRemoteDebug()
{
	return instance->processCmdRemoteDebug();
}
#endif

#if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
void BaseDebugStreamManager::setInstance(BaseDebugStreamManager* newOne)
{
	instance=newOne;
}

void BaseDebugStreamManager::callloghist()
{
	return instance->loghist();
}
#endif
