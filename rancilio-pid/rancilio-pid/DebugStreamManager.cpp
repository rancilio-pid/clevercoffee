#include "DebugStreamManager.h"

void DebugStreamManager::setup()
{
    #if(DEBUGMETHOD == 1)
	setInstance(this);
	if (debugAddFunctionVoid("loghist", &callloghist) >= 0) {
    	debugSetLastFunctionDescription("loghist - show logbook entries");
    }
    #endif

    #if(DEBUGMETHOD == 2)
	setInstance(this);
	String helpCmd = "loghist - print log history\n";
	// helpCmd.concat("bench2 - Benchmark 2");

	Debug.setHelpProjectsCmds(helpCmd);
	Debug.setCallBackProjectCmds(&callprocessCmdRemoteDebug);
    #endif  
}


#if (DEBUGMETHOD == 0)
void DebugStreamManager::writeE(const char* fmt, ...) {} 
void DebugStreamManager::writeW(const char* fmt, ...) {}
void DebugStreamManager::writeI(const char* fmt, ...) {}
void DebugStreamManager::writeD(const char* fmt, ...) {}
void DebugStreamManager::writeV(const char* fmt, ...) {}
#endif


#if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
void DebugStreamManager::writeE(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[1+vsnprintf(NULL, 0, fmt, args)];
	va_end(args);
	va_start(args,fmt);
	vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);

	char time[15];
	snprintf(time, 15, "t: %10.3f ",(double)(millis()%(long unsigned int)1e14)/1000);

	char str[1+sizeof(time)+sizeof(buf)];
	strcpy(str,time);
	strcat(str,buf);

	debugE("%s",str);

	// if (Debug.isActive(Debug.INFO)) {
		logbook.append(str);
	// }
}

void DebugStreamManager::writeW(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[1+vsnprintf(NULL, 0, fmt, args)];
	va_end(args);
	va_start(args,fmt);
	vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);

	char time[15];
	snprintf(time, 15, "t: %10.3f ",(double)(millis()%(long unsigned int)1e14)/1000);

	char str[1+sizeof(time)+sizeof(buf)];
	strcpy(str,time);
	strcat(str,buf);

	debugW("%s",str);

	// if (Debug.isActive(Debug.INFO)) {
		logbook.append(str);
	// }
}

void DebugStreamManager::writeI(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[1+vsnprintf(NULL, 0, fmt, args)];
	va_end(args);
	va_start(args,fmt);
	vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);

	char time[15];
	snprintf(time, 15, "t: %10.3f ",(double)(millis()%(long unsigned int)1e14)/1000);

	char str[1+sizeof(time)+sizeof(buf)];
	strcpy(str,time);
	strcat(str,buf);

	debugI("%s",str);

    #if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
	// if (Debug.isActive(Debug.INFO)) {
		logbook.append(str);
	// }
    #endif
}

void DebugStreamManager::writeD(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[1+vsnprintf(NULL, 0, fmt, args)];
	va_end(args);
	va_start(args,fmt);
	vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);

	char time[15];
	snprintf(time, 15, "t: %10.3f ",(double)(millis()%(long unsigned int)1e14)/1000);

	char str[1+sizeof(time)+sizeof(buf)];
	strcpy(str,time);
	strcat(str,buf);

	debugD("%s",str);

	// if (Debug.isActive(Debug.INFO)) {
		logbook.append(str);
	// }
}

void DebugStreamManager::writeV(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[1+vsnprintf(NULL, 0, fmt, args)];
	va_end(args);
	va_start(args,fmt);
	vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);

	char time[15];
	snprintf(time, 15, "t: %10.3f ",(double)(millis()%(long unsigned int)1e14)/1000);

	char str[1+sizeof(time)+sizeof(buf)];
	strcpy(str,time);
	strcat(str,buf);
	debugV("%s",str);

    #if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
	// if (Debug.isActive(Debug.VERBOSE)) {
		// logbook.append(str);
	// }
    #endif
}
#endif


#if (DEBUGMETHOD == 2)
void DebugStreamManager::processCmdRemoteDebug() 
{
	String lastCmd = Debug.getLastCommand();

	if (lastCmd == "loghist") 
	{
		loghist();
	}
}
#endif


#if (DEBUGMETHOD == 1 || DEBUGMETHOD == 2)
void DebugStreamManager::loghist() 
{
	debugA("");
	debugA(" *** logbook has %i lines ***",logbook.len());
	debugA("");
	debugA("     --- START loghist START ---");
	debugA("");

	for (int l=1; l <= logbook.len(); l++) 
	{

		debugA("%5i: %s",l,logbook.getline(l).c_str());
	}

	debugA("");
	debugA("     ---  END  loghist  END  ---");
	debugA("");
}
#endif

/*
	sources:
  	https://en.cppreference.com/w/c/io/vfprintf
	https://stackoverflow.com/questions/19679849/reference-to-member-function
	https://cpp4arduino.com/2020/02/07/how-to-format-strings-without-the-string-class.html
*/