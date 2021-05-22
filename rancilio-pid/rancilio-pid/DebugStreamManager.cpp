#include "DebugStreamManager.h"


#if (DEBUGMETHOD == 0)
void DebugStreamManager::writeE(const char* fmt, ...) {} 
void DebugStreamManager::writeW(const char* fmt, ...) {}
void DebugStreamManager::writeI(const char* fmt, ...) {}
void DebugStreamManager::writeD(const char* fmt, ...) {}
void DebugStreamManager::writeV(const char* fmt, ...) {}
#endif



/* from: https://en.cppreference.com/w/c/io/vfprintf

 	va_list args1;
  	va_start(args1, fmt);
  	va_list args2;
  	va_copy(args2, args1);
  	char buf[1+vsnprintf(NULL, 0, fmt, args1)];
  	va_end(args1);
  	vsnprintf(buf, sizeof buf, fmt, args2);
  	va_end(args2);
*/

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

	debugE("%s",buf);
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

	debugW("%s",buf);
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

	debugI("%s",buf);
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

	debugD("%s",buf);
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

	debugV("%s",buf);
}
#endif