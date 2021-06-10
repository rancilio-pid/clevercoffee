/*
 * Header for SerialDebug
 *
 * Copyright (C) 2018  Joao Lopes https://github.com/JoaoLopesF/SerialDebug
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * This header file describes the public API for SerialDebug.
 *
 */

#ifndef SERIAL_DEBUG_H
#define SERIAL_DEBUG_H

#include <Arduino.h>

#include "utility/Boards.h"

//////
// SerialDebug options
// It is here too, due Arduino IDE not working for project local defines
/////

// Disable all debug ? Good to release builds (production)
// as nothing of SerialDebug is compiled, zero overhead :-)
// For it just uncomment the DEBUG_DISABLED
//#define DEBUG_DISABLED true

// Disable SerialDebug debugger ? No more commands and features as functions and globals
// Uncomment this to disable it (SerialDebug will not do reads from Serial, good if already have this)
//#define DEBUG_DISABLE_DEBUGGER true

// Enable Flash variables support - F()
// Used internally in SerialDebug and in public API
// If is a low memory board, like AVR, all strings in SerialDebug is using flash memory
// If have RAM memory, this is more fast than flash
//#define DEBUG_USE_FLASH_F true

// For Espressif boards, default is not flash support for printf,
// due it have a lot of memory and Serial.printf is not compatible with it
// If you need more memory, can force it:
//#define DEBUG_USE_FLASH_F true

// Minimum mode - only to show messages of debug
// No commands from serial, no debugger, no printf
// Minimum usage of memory RAM and program
// Uncomment this, to use this mode
//#define DEBUG_MINIMUM true

#ifdef BOARD_LOW_MEMORY // For boards with low memory

	// Minimum mode, default for low memory - only to show messages of debug
	// Uncomment this, if not want to use this mode
	#define DEBUG_MINIMUM true

#endif

// For minimum mode - debugger always disabled

#ifdef DEBUG_MINIMUM
	#define DEBUG_DISABLE_DEBUGGER true
#endif

// TODO: see it

// Debug level - need stay here, to not cause errors in DEBUG_DISABLED empty macros

typedef enum {
	DEBUG_LEVEL_NONE,       // No debug output
	DEBUG_LEVEL_ERROR,      // Critical errors
	DEBUG_LEVEL_WARN,       // Error conditions but not critical
	DEBUG_LEVEL_INFO,       // Information messages
	DEBUG_LEVEL_DEBUG,      // Extra information - default level (if not changed)
	DEBUG_LEVEL_VERBOSE     // More information than the usual
} debug_level_t;

// Debug watch global variables - need stay here, to not cause errors in DEBUG_DISABLED empty macros

typedef enum {										// Type of operator
	DEBUG_WATCH_CHANGED,							// Changed value ?
	DEBUG_WATCH_EQUAL,								// Equal (==)
	DEBUG_WATCH_DIFF,								// Different (!=)
	DEBUG_WATCH_LESS,								// Less (<=)
	DEBUG_WATCH_GREAT,								// Greater (>)
	DEBUG_WATCH_LESS_EQ,							// Less or equal (<=)
	DEBUG_WATCH_GREAT_EQ							// Greater or equal (>=)
} debugEnumWatch_t;

// Disable debug - good for release (production)
// Put below define after the include SerialDebug in your project to disable all debug
// as nothing of SerialDebug is compiled, zero overhead :-)
//#define DEBUG_DISABLED true

#ifdef DEBUG_DISABLED

    // No debugs

	#define debugHandle()
	#define debugHandleInactive()

	#define debugA(fmt, ...)
    #define debugV(fmt, ...)
    #define debugD(fmt, ...)
    #define debugI(fmt, ...)
    #define debugW(fmt, ...)
    #define debugE(fmt, ...)

	#define debugSetLevel(...)
	#define debugSetProfiler(...)
	#define debugShowProfiler(...)
	#define debugSilence(...)

	#if defined ESP32 || defined ESP8266 // For Espressif boards

		#define debugIsrA(fmt, ...)
		#define debugIsrV(fmt, ...)
		#define debugIsrD(fmt, ...)
		#define debugIsrI(fmt, ...)
		#define debugIsrW(fmt, ...)
		#define debugIsrE(fmt, ...)

	#endif

	#define printA(x)
	#define printV(x)
	#define printD(x)
	#define printI(x)
	#define printW(x)
	#define printE(x)

	#define printlnA(x)
	#define printlnV(x)
	#define printlnD(x)
	#define printlnI(x)
	#define printlnW(x)
	#define printlnE(x)

	#define DEBUG_DISABLE_DEBUGGER true

#else // Debugs enabled

//////// Defines

// Internal printf ?

#if (defined ESP8266 || defined ESP32) && !(defined DEBUG_USE_FLASH_F)  // For Espressif boards, have Serial.printf native (but not for Flash F)
											 							// TODO: see if another boards have it, Arduinos AVR not have it)
	#define DEBUG_USE_NATIVE_PRINTF true	// Enable native
	#define DEBUG_NOT_USE_FLASH_F true    	// Disable all printf with __FlashStringHelper - If you need, just comment it
#endif

// Size for commands

#ifndef DEBUG_MINIMUM

#define DEBUG_MAX_SIZE_COMMANDS 10			// Maximum size of commands - can be changed
#define DEBUG_MAX_SIZE_CMD_OPTIONS 64		// Maximum size of commands options - can be changed

// Max size for compare char arrays or string

#define DEBUG_MAX_CMP_STRING 48

// Timeout for debugBreak - put 0 if not want timeout

#ifndef DEBUG_BREAK_TIMEOUT
	#define DEBUG_BREAK_TIMEOUT 5000
#endif

// Timeout for watch triggered - put 0 if not want timeout

#ifndef DEBUG_BREAK_WATCH
	#define DEBUG_BREAK_WATCH 10000
#endif

// Minimum time to process handle event

#define DEBUG_MIN_TIME_EVENT 850

#endif // DEBUG_MINIMUM

//////// Prototypes - public

void debugHandleInactive();
void debugSetLevel(uint8_t level);
void debugSilence(boolean activate, boolean showMessage, boolean fromBreak = false);
void debugPrintInfo(const char level, const char* function);

#ifndef DEBUG_MINIMUM

void debugHandleEvent(boolean calledByHandleEvent);
String debugBreak();
String debugBreak(const __FlashStringHelper * str, uint32_t timeout = DEBUG_BREAK_TIMEOUT, boolean byWatch = false);
String debugBreak(const char* str, uint32_t timeout = DEBUG_BREAK_TIMEOUT, boolean byWatch = false);

void debugSetProfiler(boolean active);

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
	String debugBreak(String& str, uint32_t timeout = DEBUG_BREAK_TIMEOUT);
	void debugShowProfiler(boolean activate, uint16_t minTime, boolean showMessage);
#endif

#endif // DEBUG_MINIMUM

// Debugger

#ifndef DEBUG_DISABLE_DEBUGGER	// Only if for process commands and SerialDebug debugger

	// For functions // TODO: make more types

	int8_t debugAddFunctionVoid(const char* name, void (*callback)());
	int8_t debugAddFunctionStr(const char* name, void (*callback)(String));
	int8_t debugAddFunctionChar(const char* name, void (*callback)(char));
	int8_t debugAddFunctionInt(const char* name, void (*callback)(int));

	void debugSetLastFunctionDescription(const char *description);

	// For globals

	#ifndef BOARD_LOW_MEMORY // Not for low memory boards

		int8_t debugAddGlobalBoolean (const char* name, boolean* pointer);
		int8_t debugAddGlobalChar (const char* name, char* pointer);
		int8_t debugAddGlobalByte (const char* name, byte* pointer);
		int8_t debugAddGlobalInt (const char* name, int* pointer);
		int8_t debugAddGlobalUInt (const char* name, unsigned int* pointer);
		int8_t debugAddGlobalLong (const char* name, long* pointer);
		int8_t debugAddGlobalULong (const char* name, unsigned long* pointer);
		int8_t debugAddGlobalFloat (const char* name, float* pointer);
		int8_t debugAddGlobalDouble (const char* name, double* pointer);
		int8_t debugAddGlobalInt8_t (const char* name, int8_t* pointer);
		int8_t debugAddGlobalInt16_t (const char* name, int16_t* pointer);
		int8_t debugAddGlobalInt32_t (const char* name, int32_t* pointer);
	//		#ifdef ESP32
		//int8_t debugAddGlobalInt64_t (const char* name, int64_t* pointer);
	//		#endif
		int8_t debugAddGlobalUInt8_t (const char* name, uint8_t* pointer);
		int8_t debugAddGlobalUInt16_t (const char* name, uint16_t* pointer);
		int8_t debugAddGlobalUInt32_t (const char* name, uint32_t* pointer);
	//		#ifdef ESP32
		//int8_t debugAddGlobalUInt64_t (const char* name, uint64_t* pointer);
	//		#endif
		int8_t debugAddGlobalCharArray (const char* name, char* pointer);
		int8_t debugAddGlobalCharArray (const char* name, char* pointer, uint8_t showLength);
		int8_t debugAddGlobalString (const char* name, String* pointer);
		int8_t debugAddGlobalString (const char* name, String* pointer, uint8_t showLength);

		void debugSetLastGlobalDescription(const char *description);

		// For flash F

		int8_t debugAddFunctionVoid(const __FlashStringHelper* name, void (*callback)());
		int8_t debugAddFunctionStr(const __FlashStringHelper* name, void (*callback)(String));
		int8_t debugAddFunctionChar(const __FlashStringHelper* name, void (*callback)(char));
		int8_t debugAddFunctionInt(const __FlashStringHelper* name, void (*callback)(int));

		void debugSetLastFunctionDescription(const __FlashStringHelper *description);

		int8_t debugAddGlobalBoolean (const __FlashStringHelper* name, boolean* pointer);
		int8_t debugAddGlobalChar (const __FlashStringHelper* name, char* pointer);
		int8_t debugAddGlobalByte (const __FlashStringHelper* name, byte* pointer);
		int8_t debugAddGlobalInt (const __FlashStringHelper* name, int* pointer);
		int8_t debugAddGlobalUInt (const __FlashStringHelper* name, unsigned int* pointer);
		int8_t debugAddGlobalLong (const __FlashStringHelper* name, long* pointer);
		int8_t debugAddGlobalULong (const __FlashStringHelper* name, unsigned long* pointer);
		int8_t debugAddGlobalFloat (const __FlashStringHelper* name, float* pointer);
		int8_t debugAddGlobalDouble (const __FlashStringHelper* name, double* pointer);
		int8_t debugAddGlobalInt8_t (const __FlashStringHelper* name, int8_t* pointer);
		int8_t debugAddGlobalInt16_t (const __FlashStringHelper* name, int16_t* pointer);
		int8_t debugAddGlobalInt32_t (const __FlashStringHelper* name, int32_t* pointer);
	//			#ifdef ESP32
		//int8_t debugAddGlobalInt64_t (const __FlashStringHelper* name, int64_t* pointer);
	//			#endif
		int8_t debugAddGlobalUInt8_t (const __FlashStringHelper* name, uint8_t* pointer);
		int8_t debugAddGlobalUInt16_t (const __FlashStringHelper* name, uint16_t* pointer);
		int8_t debugAddGlobalUInt32_t (const __FlashStringHelper* name, uint32_t* pointer);
	//			#ifdef ESP32
		//int8_t debugAddGlobalUInt64_t (const __FlashStringHelper* name, uint64_t* pointer);
	//			#endif
		int8_t debugAddGlobalCharArray (const __FlashStringHelper* name, char* pointer);
		int8_t debugAddGlobalCharArray (const __FlashStringHelper* name, char* pointer, uint8_t showLength);
		int8_t debugAddGlobalString (const __FlashStringHelper* name, String* pointer);
		int8_t debugAddGlobalString (const __FlashStringHelper* name, String* pointer, uint8_t showLength);

		void debugSetLastGlobalDescription(const __FlashStringHelper *description);

		// Watches

		int8_t debugAddWatchBoolean (uint8_t globalNum, uint8_t operation, boolean value, boolean allwaysStop = false);
		int8_t debugAddWatchChar (uint8_t globalNum, uint8_t operation, char value, boolean allwaysStop = false);
		int8_t debugAddWatchByte (uint8_t globalNum, uint8_t operation, byte value, boolean allwaysStop = false);
		int8_t debugAddWatchInt (uint8_t globalNum, uint8_t operation, int value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt (uint8_t globalNum, uint8_t operation, unsigned int value, boolean allwaysStop = false);
		int8_t debugAddWatchLong (uint8_t globalNum, uint8_t operation, long value, boolean allwaysStop = false);
		int8_t debugAddWatchULong (uint8_t globalNum, uint8_t operation, unsigned long value, boolean allwaysStop = false);
		int8_t debugAddWatchFloat (uint8_t globalNum, uint8_t operation, float value, boolean allwaysStop = false);
		int8_t debugAddWatchDouble (uint8_t globalNum, uint8_t operation, double value, boolean allwaysStop = false);
		int8_t debugAddWatchInt8_t (uint8_t globalNum, uint8_t operation, int8_t value, boolean allwaysStop = false);
		int8_t debugAddWatchInt16_t (uint8_t globalNum, uint8_t operation, int16_t value, boolean allwaysStop = false);
		int8_t debugAddWatchInt32_t (uint8_t globalNum, uint8_t operation, int32_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt8_t (uint8_t globalNum, uint8_t operation, uint8_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt16_t (uint8_t globalNum, uint8_t operation, uint16_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt32_t (uint8_t globalNum, uint8_t operation, uint32_t value, boolean allwaysStop = false);

		int8_t debugAddWatchCharArray (uint8_t globalNum, uint8_t operation, const char* value, boolean allwaysStop = false);
		int8_t debugAddWatchString (uint8_t globalNum, uint8_t operation, String value, boolean allwaysStop = false);

		int8_t debugAddWatchBoolean  (const char* globalName, uint8_t operation, boolean value, boolean allwaysStop = false);
		int8_t debugAddWatchChar  (const char* globalName, uint8_t operation, char value, boolean allwaysStop = false);
		int8_t debugAddWatchByte  (const char* globalName, uint8_t operation, byte value, boolean allwaysStop = false);
		int8_t debugAddWatchInt  (const char* globalName, uint8_t operation, int value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt  (const char* globalName, uint8_t operation, unsigned int value, boolean allwaysStop = false);
		int8_t debugAddWatchLong  (const char* globalName, uint8_t operation, long value, boolean allwaysStop = false);
		int8_t debugAddWatchULong  (const char* globalName, uint8_t operation, unsigned long value, boolean allwaysStop = false);
		int8_t debugAddWatchFloat  (const char* globalName, uint8_t operation, float value, boolean allwaysStop = false);
		int8_t debugAddWatchDouble  (const char* globalName, uint8_t operation, double value, boolean allwaysStop = false);
		int8_t debugAddWatchInt8_t  (const char* globalName, uint8_t operation, int8_t value, boolean allwaysStop = false);
		int8_t debugAddWatchInt16_t  (const char* globalName, uint8_t operation, int16_t value, boolean allwaysStop = false);
		int8_t debugAddWatchInt32_t  (const char* globalName, uint8_t operation, int32_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt8_t  (const char* globalName, uint8_t operation, uint8_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt16_t  (const char* globalName, uint8_t operation, uint16_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt32_t  (const char* globalName, uint8_t operation, uint32_t value, boolean allwaysStop = false);

		int8_t debugAddWatchCharArray (const char* globalName, uint8_t operation, const char* value, boolean allwaysStop = false);
		int8_t debugAddWatchString (const char* globalName, uint8_t operation, String value, boolean allwaysStop = false);

		int8_t debugAddWatchCross(uint8_t globalNum, uint8_t operation, uint8_t anotherGlobalNum, boolean allwaysStop = false);
		int8_t debugAddWatchCross(const char* globalName, uint8_t operation, const char* anotherGlobalName, boolean allwaysStop = false);

		// For Flash F

		int8_t debugAddWatchBoolean (const __FlashStringHelper* globalName, uint8_t operation, boolean value, boolean allwaysStop = false);
		int8_t debugAddWatchChar (const __FlashStringHelper* globalName, uint8_t operation, char value, boolean allwaysStop = false);
		int8_t debugAddWatchByte (const __FlashStringHelper* globalName, uint8_t operation, byte value, boolean allwaysStop = false);
		int8_t debugAddWatchInt (const __FlashStringHelper* globalName, uint8_t operation, int value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt (const __FlashStringHelper* globalName, uint8_t operation, unsigned int value, boolean allwaysStop = false);
		int8_t debugAddWatchLong (const __FlashStringHelper* globalName, uint8_t operation, long value, boolean allwaysStop = false);
		int8_t debugAddWatchULong (const __FlashStringHelper* globalName, uint8_t operation, unsigned long value, boolean allwaysStop = false);
		int8_t debugAddWatchFloat (const __FlashStringHelper* globalName, uint8_t operation, float value, boolean allwaysStop = false);
		int8_t debugAddWatchDouble (const __FlashStringHelper* globalName, uint8_t operation, double value, boolean allwaysStop = false);
		int8_t debugAddWatchInt8_t (const __FlashStringHelper* globalName, uint8_t operation, int8_t value, boolean allwaysStop = false);
		int8_t debugAddWatchInt16_t (const __FlashStringHelper* globalName, uint8_t operation, int16_t value, boolean allwaysStop = false);
		int8_t debugAddWatchInt32_t (const __FlashStringHelper* globalName, uint8_t operation, int32_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt8_t (const __FlashStringHelper* globalName, uint8_t operation, uint8_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt16_t (const __FlashStringHelper* globalName, uint8_t operation, uint16_t value, boolean allwaysStop = false);
		int8_t debugAddWatchUInt32_t (const __FlashStringHelper* globalName, uint8_t operation, uint32_t value, boolean allwaysStop = false);

		int8_t debugAddWatchCharArray (const __FlashStringHelper* globalName, uint8_t operation, const char* value, boolean allwaysStop = false);
		int8_t debugAddWatchString (const __FlashStringHelper* globalName, uint8_t operation, String value, boolean allwaysStop = false);

		int8_t debugAddWatchCross(const __FlashStringHelper* globalName, uint8_t operation, const __FlashStringHelper* anotherGlobalName, boolean allwaysStop = false);

	#else // Low memory boards -> reduced number of functions

		// For flash F

		int8_t debugAddFunctionVoid(const __FlashStringHelper* name, void (*callback)());
		int8_t debugAddFunctionStr(const __FlashStringHelper* name, void (*callback)(String));
		int8_t debugAddFunctionChar(const __FlashStringHelper* name, void (*callback)(char));
		int8_t debugAddFunctionInt(const __FlashStringHelper* name, void (*callback)(int));

		#define debugSetLastFunctionDescription(str) // Not compile this

		int8_t debugAddGlobalBoolean (const __FlashStringHelper* name, boolean* pointer);
		int8_t debugAddGlobalChar (const __FlashStringHelper* name, char* pointer);
		int8_t debugAddGlobalInt (const __FlashStringHelper* name, int* pointer);
		int8_t debugAddGlobalULong (const __FlashStringHelper* name, unsigned long* pointer);
		int8_t debugAddGlobalString (const __FlashStringHelper* name, String* pointer);
		int8_t debugAddGlobalString (const __FlashStringHelper* name, String* pointer, uint8_t showLength);

		#define debugSetLastGlobalDescription(str) // Not compile this

		// No watches

	#endif // Low memory

	// Handle debugger

	void debugHandleDebugger (boolean calledByHandleEvent);

#endif //DEBUG_DISABLE_DEBUGGER

#ifndef DEBUG_MINIMUM

// For printf support, if not have a native

#ifndef DEBUG_USE_NATIVE_PRINTF
	void debugPrintf(boolean newline, const char level, const char* function, const char* format, ...);
	void debugPrintf(boolean newline, const char level, const char* function, const __FlashStringHelper *format, ...) ;
#endif

#endif // DEBUG_MINIMUM

//////// External variables (need to use macros)

extern boolean _debugActive;		   			// Debug is only active after receive first data from Serial
extern uint8_t _debugLevel; 					// Current level of debug (init as disabled)
extern bool _debugSilence;						// Silent mode ?

extern bool _debugShowProfiler;					// Show profiler time ?
extern uint16_t _debugMinTimeShowProfiler;		// Minimum time to show profiler
extern unsigned long _debugLastTime; 			// Last time show a debug

extern boolean _debugPrintIsNewline;			// Used in print macros

#ifndef DEBUG_DISABLE_DEBUGGER
extern uint8_t _debugFunctionsAdded;			// Number of functions added
extern uint8_t _debugGlobalsAdded;				// Number of globals added
extern uint8_t _debugWatchesAdded;				// Number of watches added
extern boolean _debugWatchesEnabled;			// Watches is enabled (only after add any)?
extern boolean _debugDebuggerEnabled;			// Simple Software Debugger enabled ?
#endif

//////// Defines and macros

#define DEBUG_LEVELS_SIZE 6						// Number of levels

#ifndef DEBUG_INITIAL_LEVEL
#define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_DEBUG  	// Initial level - used to set when debug is activate (on fist data receive)
#endif

// Macro to handle the SerialDebug (better performance)

#ifndef DEBUG_MINIMUM

#define debugHandle() {  \
	if (!_debugActive) { \
		debugHandleInactive(); \
	} \
	if (_debugActive || Serial.available() > 0) { \
		debugHandleEvent(true);\
	} \
}

#else

#define debugHandle() {  \
	if (!_debugActive) { \
		debugHandleInactive(); \
	} \
}

#endif // DEBUG_MINIMUM

// Macros for debugs
// Note: not used F() for formats, due it is small and Flash can be slow

#ifndef DEBUG_DISABLE_DEBUGGER
	#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		#define DEBUG_HANDLE_DEBUGGER() \
			if (_debugDebuggerEnabled && _debugGlobalsAdded > 0) { \
				debugHandleDebugger(false); \
			}
	#else
		#define DEBUG_HANDLE_DEBUGGER()
	#endif
#else
	#define DEBUG_HANDLE_DEBUGGER()
#endif

#ifndef DEBUG_USE_FLASH_F // Only if not using flash

#ifdef DEBUG_USE_NATIVE_PRINTF // Only for native Serial.printf, as in Expressif boards

#ifdef ESP32  // Only for ESP32 - not if using flash

	// ESP32 debug core id? // Puts C0 or C1 in each debug, to show that core this debug is running

	#define DEBUG_CORE true   // debug show core id ? (comment to disable it)

	// Show core in debug ?

	#ifdef DEBUG_CORE

		#ifndef DEBUG_AUTO_FUNC_DISABLED

			// Normal debug

			#define _debug(level, fmt, ...) { \
				DEBUG_HANDLE_DEBUGGER() \
				if (_debugShowProfiler) { \
					Serial.printf("(%c p:^%04lu %s C%d) " fmt "\r\n", \
							level, (millis() - _debugLastTime), \
							__func__, xPortGetCoreID(), \
							##__VA_ARGS__); \
					_debugLastTime = millis(); \
				} else {\
					Serial.printf("(%c %lu %s C%d) " fmt "\r\n", \
							level, millis(), \
							__func__, xPortGetCoreID(), \
							##__VA_ARGS__); \
				} \
			}

			// ISR debug (use only for 115200 bauds - default of Espressif boards in Arduino)
			// Due Serial.print is not good to ISR, instead is used ets_printf
			// Note this can cause delays and crash, due use only necessary to debug and remove after this

			#define _debugIsr(level, fmt, ...) { \
				if (_debugShowISR == 'Y') { \
					if (_debugShowProfiler) { \
						ets_printf("ISR(%c p:^%04lu %s C%d) " fmt "\r\n", \
								level, (millis() - _debugLastTime), \
								__func__, xPortGetCoreID(), \
								##__VA_ARGS__); \
						_debugLastTime = millis(); \
					} else {\
						ets_printf("ISR(%c %lu %s C%d) " fmt "\r\n", \
								level, millis(), \
								__func__, xPortGetCoreID(), \
								##__VA_ARGS__); \
					} \
				} \
			}

		#else

			// Normal debug

			#define _debug(level, fmt, ...) { \
				DEBUG_HANDLE_DEBUGGER() \
				if (_debugShowProfiler) { \
					Serial.printf("(%c p:^%04lu C%d) " fmt "\r\n", \
						level, (millis() - _debugLastTime), \
						xPortGetCoreID(), \
						##__VA_ARGS__); \
					_debugLastTime = millis(); \
				} else {\
					Serial.printf("(%c %lu C%d) " fmt "\r\n", \
						level, millis(), \
						xPortGetCoreID(), \
						##__VA_ARGS__); \
				} \
			}

			// ISR debug (use only for 115200 bauds - default of Espressif boards in Arduino)
			// Due Serial.print is not good to ISR, instead is used ets_printf
			// Note this can cause delays and crash, due use only necessary to debug and remove after this

			#define _debugIsr(level, fmt, ...) { \
				if (_debugShowISR == 'Y') { \
					if (_debugShowProfiler) { \
						ets_printf("ISR(%c p:^%04lu C%d) " fmt "\r\n", \
								level, (millis() - _debugLastTime), \
								xPortGetCoreID(), \
								##__VA_ARGS__); \
						_debugLastTime = millis(); \
					} else {\
						ets_printf("ISR(%c %lu C%d) " fmt "\r\n", \
							level, millis(), \
							xPortGetCoreID(), \
							##__VA_ARGS__); \
					} \
				} \
			}

		#endif

	#endif

#endif // ESP32

#ifdef ESP8266 // Only for Esp8266

	// ISR debug (use only for 115200 bauds - default of Espressif boards in Arduino)
	// Due Serial.print is not good to ISR, instead use ets_printf
	// Note this can cause delays and crash, due use only necessary to debug and remove after this

	#ifndef DEBUG_AUTO_FUNC_DISABLED

		#define _debugIsr(level, fmt, ...) { \
			if (_debugShowISR == 'Y') { \
				if (_debugShowProfiler) { \
					ets_printf("ISR(%c p:^%04lu %s) " fmt "\r\n", \
						level, (millis() - _debugLastTime), \
						__func__, ##__VA_ARGS__); \
					_debugLastTime = millis(); \
				} else {\
					ets_printf("ISR(%c %lu %s) " fmt "\r\n", \
						level, millis(), \
						__func__, ##__VA_ARGS__); \
				} \
			} \
		}

	#else

		#define _debugIsr(level, fmt, ...) { \
			if (_debugShowISR == 'Y') { \
				if (_debugShowProfiler) { \
					ets_printf("ISR(%c p:^%04lu) " fmt "\r\n", \
						level, (millis() - _debugLastTime), \
						##__VA_ARGS__); \
					_debugLastTime = millis(); \
				} else {\
					ets_printf("ISR(%c %lu) " fmt "\r\n", \
						level, millis(), \
						##__VA_ARGS__); \
				} \
			} \
		}

	#endif

#endif // ESP8266

#ifndef _debug // Not defined yet

	// Normal debug

	#ifndef DEBUG_AUTO_FUNC_DISABLED

		#define _debug(level, fmt, ...) { \
			DEBUG_HANDLE_DEBUGGER() \
			if (_debugShowProfiler) { \
				Serial.printf("(%c p:^%04lu %s) " fmt "\r\n", \
					level, (millis() - _debugLastTime), \
					__func__, \
					##__VA_ARGS__); \
				_debugLastTime = millis(); \
			} else {\
				Serial.printf("(%c %lu %s) " fmt "\r\n", \
					level, millis(), \
					__func__, \
					##__VA_ARGS__); \
			} \
		}

	#else

		#define _debug(level, fmt, ...) { \
			DEBUG_HANDLE_DEBUGGER() \
			if (_debugShowProfiler) { \
				Serial.printf("(%c p:^%04lu) " fmt "\r\n", \
					level, (millis() - _debugLastTime), \
					##__VA_ARGS__); \
				_debugLastTime = millis(); \
			} else {\
				Serial.printf("(%c %lu) " fmt "\r\n", \
					level, millis(), \
					##__VA_ARGS__); \
			} \
		}

	#endif

#endif // _debug

#endif // DEBUG_USE_NATIVE_PRINTF

#endif // DEBUG_USE_FLASH_F

// FPSTR -

#ifndef FPSTR // Thanks a lot Espressif -> https://arduino-esp8266.readthedocs.io/en/latest/PROGMEM.html
	#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#endif

// If it not defined yet, can be:
// - Not Espressif boards not have printf :-(,
// - Or Espressif boards to use F() - flash strings - in printf, just enable DEBUG_USE_FLASH_F in your project)

#ifndef DEBUG_MINIMUM

#ifndef _debug // Not defined yet

	// Normal debug

#ifdef BOARD_ENOUGH_MEMORY // Only for enough memory boards - modern with more CPU power ...

	#ifndef DEBUG_AUTO_FUNC_DISABLED

		#define _debug(level, fmt, ...) { \
			DEBUG_HANDLE_DEBUGGER() \
			debugPrintf(true, level, __func__, fmt, ##__VA_ARGS__); \
		}

	#else

		#define _debug(level, fmt, ...) { \
			DEBUG_HANDLE_DEBUGGER() \
			debugPrintf(true, level, __func__, fmt, ##__VA_ARGS__); \
		}

	#endif

#else // Not process handle debugger on each call

	#ifndef DEBUG_AUTO_FUNC_DISABLED

		#define _debug(level, fmt, ...)  	debugPrintf(true, level, __func__, fmt, ##__VA_ARGS__)

	#else

		#define _debug(level, fmt, ...) 	debugPrintf(true, level, 0, fmt, ##__VA_ARGS__)

	#endif

#endif // BOARD_ENOUGH_MEMORY

#endif // _debug

// Always

#define debugA(fmt, ...) if (!_debugSilence)									 		_debug('A', fmt, ##__VA_ARGS__)

// With level

#define debugV(fmt, ...) if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_VERBOSE) 		_debug('V', fmt, ##__VA_ARGS__)
#define debugD(fmt, ...) if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_DEBUG) 		_debug('D', fmt, ##__VA_ARGS__)
#define debugI(fmt, ...) if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_INFO) 		_debug('I', fmt, ##__VA_ARGS__)
#define debugW(fmt, ...) if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_WARN) 		_debug('W', fmt, ##__VA_ARGS__)

// For errors (always showed)

#define debugE(fmt, ...) if (!_debugSilence) 											_debug('E', fmt, ##__VA_ARGS__)

// For ISR (only for Espressif boards)

#ifdef _debugIsr

	// Always

	#define debugIsrA(fmt, ...) if (!_debugSilence) 					  					_debugIsr('A', fmt, ##__VA_ARGS__)

	// With level

	#define debugIsrV(fmt, ...) if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_VERBOSE) 	_debugIsr('V', fmt, ##__VA_ARGS__)
	#define debugIsrD(fmt, ...) if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_DEBUG) 	_debugIsr('D', fmt, ##__VA_ARGS__)
	#define debugIsrI(fmt, ...) if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_INFO) 		_debugIsr('I', fmt, ##__VA_ARGS__)
	#define debugIsrW(fmt, ...) if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_WARN) 		_debugIsr('W', fmt, ##__VA_ARGS__)

	// For errors (always showed)

	#define debugIsrE(fmt, ...) if (!_debugSilence) 										_debugIsr('E', fmt, ##__VA_ARGS__)

#endif // _debugIsr

#endif // DEBUG_MINIMUM

// New macros for debug - print macros (to not use printf and to easy to migrate)

#ifndef DEBUG_AUTO_FUNC_DISABLED

	#define printLevel(level, x, ...) { \
		if (_debugPrintIsNewline) { \
			debugPrintInfo(level, __func__); \
			_debugPrintIsNewline = false; \
		} \
		Serial.print(x, ##__VA_ARGS__); \
	}

	#define printlnLevel(level, x, ...) { \
		if (_debugPrintIsNewline) { \
			debugPrintInfo(level, __func__); \
		} \
		Serial.println(x, ##__VA_ARGS__); \
		_debugPrintIsNewline = true; \
	}


#else

	#define printLevel(level, x, ...) { \
		if (_debugPrintIsNewline) { \
			debugPrintInfo(level, 0); \
			_debugPrintIsNewline = false; \
		} \
		Serial.print(x, ##__VA_ARGS__); \
	}

	#define printlnLevel(level, x, ...) { \
		if (_debugPrintIsNewline) { \
			debugPrintInfo(level, 0); \
		} \
		Serial.println(x, ##__VA_ARGS__); \
		_debugPrintIsNewline = true; \
	}

#endif // DEBUG_AUTO_FUNC_DISABLED

// Always

#define printA(x, ...) 		if (!_debugSilence)									 		printLevel('A', x, ##__VA_ARGS__)
#define printlnA(x, ...) 	if (!_debugSilence)									 		printlnLevel('A', x, ##__VA_ARGS__)

// With level

#define printV(x, ...) 		if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_VERBOSE) 	printLevel('V', x, ##__VA_ARGS__)
#define printD(x, ...) 		if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_DEBUG) 	printLevel('D', x, ##__VA_ARGS__)
#define printI(x, ...) 		if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_INFO) 		printLevel('I', x, ##__VA_ARGS__)
#define printW(x, ...) 		if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_WARN) 		printLevel('W', x, ##__VA_ARGS__)

#define printlnV(x, ...) 	if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_VERBOSE) 	printlnLevel('V', x, ##__VA_ARGS__)
#define printlnD(x, ...) 	if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_DEBUG) 	printlnLevel('D', x, ##__VA_ARGS__)
#define printlnI(x, ...) 	if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_INFO) 		printlnLevel('I', x, ##__VA_ARGS__)
#define printlnW(x, ...) 	if (!_debugSilence && _debugLevel >= DEBUG_LEVEL_WARN) 		printlnLevel('W', x, ##__VA_ARGS__)

// For errors (always showed)

#define printE(x, ...) 		if (!_debugSilence) 										printLevel('E', x, ##__VA_ARGS__)
#define printlnE(x, ...) 	if (!_debugSilence) 										printlnLevel('E', x, ##__VA_ARGS__)

#endif // DEBUG_DISABLED

// Debugger disabled ?

#ifdef DEBUG_DISABLE_DEBUGGER

	#define debugAddGlobalBoolean (...)
	#define debugAddGlobalChar (...)
	#define debugAddGlobalByte (...)
	#define debugAddGlobalInt (...)
	#define debugAddGlobalUInt (...)
	#define debugAddGlobalLong (...)
	#define debugAddGlobalULong (...)
	#define debugAddGlobalFloat (...)
	#define debugAddGlobalDouble (...)
	#define debugAddGlobalInt8_t (...)
	#define debugAddGlobalInt16_t (...)
	#define debugAddGlobalInt32_t (...)
	//#ifdef ESP32
	//#define debugAddGlobalInt64_t (...)
	//#endif
	#define debugAddGlobalUInt8_t (...)
	#define debugAddGlobalUInt16_t (...)
	#define debugAddGlobalUInt32_t (...)
	//#ifdef ESP32
	//#define debugAddGlobalUInt64_t (...)
	//#endif
	#define debugAddGlobalCharArray (...)
	#define debugAddGlobalString (...)

	#define debugSetLastGlobalDescription(...)

	#define debugAddWatchBoolean (...)
	#define debugAddWatchChar (...)
	#define debugAddWatchByte (...)
	#define debugAddWatchInt (...)
	#define debugAddWatchUInt (...)
	#define debugAddWatchLong (...)
	#define debugAddWatchULong (...)
	#define debugAddWatchFloat (...)
	#define debugAddWatchDouble (...)
	#define debugAddWatchInt8_t (...)
	#define debugAddWatchInt16_t (...)
	#define debugAddWatchInt32_t (...)
	#define debugAddWatchUInt8_t (...)
	#define debugAddWatchUInt16_t (...)
	#define debugAddWatchUInt32_t (...)

	#define debugAddWatchCharArray (...)
	#define debugAddWatchString (...)

	#define debugAddWatchCross(...)

	#define debugHandleDebugger(...)

#endif // DEBUG_DISABLE_DEBUGGER

#endif /* SERIAL_DEBUG_H_ */

//////// End
