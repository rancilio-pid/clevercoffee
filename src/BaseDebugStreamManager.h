#ifndef BaseDebugStreamManager_h
#define BaseDebugStreamManager_h

#include "userConfig.h"

#if (DEBUGMETHOD == 1)
#include "SerialDebug.h"
#endif

#if (DEBUGMETHOD == 2)
#include "RemoteDebug.h"

#define DEBUG_AUTO_FUNC_DISABLED true
#define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_DEBUG
#endif

class BaseDebugStreamManager
{
  public:
    static BaseDebugStreamManager* instance;

    #if (DEBUGMETHOD == 2)
    RemoteDebug Debug;
    #endif

    void handle() {
      #if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
      debugHandle();
      #endif
      // #if (DEBUGMETHOD == 2)
      // Debug.handle();
      // #endif
    }

    BaseDebugStreamManager();

    #if (DEBUGMETHOD == 2)
    virtual void processCmdRemoteDebug() = 0;
    static  void callprocessCmdRemoteDebug();
    #endif

    #if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
    static  void setInstance(BaseDebugStreamManager* newOne);
    
    virtual void loghist() = 0;
    static  void callloghist();
    #endif
    
};

#endif
