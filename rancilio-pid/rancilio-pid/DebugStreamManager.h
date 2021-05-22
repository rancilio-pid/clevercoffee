#ifndef DebugStreamManager_h
#define DebugStreamManager_h

#include "userConfig.h"

#if (DEBUGMETHOD == 1)
#include "SerialDebug.h"
#endif

#if (DEBUGMETHOD == 2)
#include "RemoteDebug.h"

#define DEBUG_AUTO_FUNC_DISABLED true
#define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_DEBUG
#endif

class DebugStreamManager
{
  public:

  	DebugStreamManager() {
  		#if( DEBUGMETHOD == 1)
	    Serial.begin(115200);
  		#endif  
        #if( DEBUGMETHOD == 2)
	    Debug.begin(HOSTNAME);
        #endif  
   	}

   	void handle() {
   		#if (DEBUGMETHOD == 1)
   		debugHandle();
   		#endif
   		#if (DEBUGMETHOD == 2)
   		Debug.handle();
   		#endif
   	}

	void writeE(const char* fmt, ...);
	void writeW(const char* fmt, ...);
	void writeI(const char* fmt, ...);
	void writeD(const char* fmt, ...);
	void writeV(const char* fmt, ...);

  private:
  	#if (DEBUGMETHOD == 2)
	RemoteDebug Debug;
	#endif
};

#endif
