#ifndef DebugStreamManager_h
#define DebugStreamManager_h

#include "BaseDebugStreamManager.h"
#include "Logbook.h"


class DebugStreamManager : public BaseDebugStreamManager
{
  public:
    void setup();

    void writeE(const char* fmt, ...);
    void writeW(const char* fmt, ...);
    void writeI(const char* fmt, ...);
    void writeD(const char* fmt, ...);
    void writeV(const char* fmt, ...);

  private:
    #if (DEBUGMETHOD == 2)
    void processCmdRemoteDebug();
    #endif

    #if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
    Logbook logbook;
    void loghist();
    #endif

};

#endif
