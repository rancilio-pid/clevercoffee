/*****************************************
 * Library   : SerialDebug - Improved serial debugging to Arduino, with simple software debugger
 * Programmer: Joao Lopes
 * Comments  : Tips on gcc logging seen at http://www.valvers.com/programming/c/logging-with-gcc/
 *             For remote debugging -> RemodeDebug: https://github.com/JoaoLopesF/SerialDebug
 * 			   Based on RemoteDebug library and ESP-IDF logging debug levels
 * 			   Note: This lybrary not use tasks, when for ESP32, due avoid serial output mixed
 * Versions  :
 * ------ 	---------- 		-------------------------
 * 0.9.82	2018-11-25		corrected bug on debugHandleEvent
 * 0.9.81	2018-11-16		print macros now support second arg, e.g.: printlnA(10, HEX);
 * 							thanks to @wjwieland to open a issue about this.
 * 0.9.80	2018-11-15		Few adjustments in header files
 * 0.9.79	2018-11-07		Update examples
 * 0.9.78	2018-10-28		Update examples
 * 0.9.77	2018-10-26		Adjustments for minimum mode
 * 							#include stdarg, to avoid error in Arduino IDE 1.8.5 and Linux - thanks to @wd5gnr
 * 0.9.76	2018-10-26		#includes for Arduino.h corrected to work in Linux (case sensitive F.S.) - thanks @wd5gnr
 * 0.9.75	2018-10-25		Few adjustments
 * 0.9.74	2018-10-25		Adjustments to SerialDebugApp show debugger info in App
 *                          Now low memory boards have debugger disabled by default, but enabled commands (debug level, help ...)
 *                          Create an mode minimum to low memory boards - only debug output enabled to save memory
 * 0.9.73	2018-10-24		Adjustments to SerialDebugApp show debugger info in App
 * 0.9.72	2018-10-21		Corrected bug on basic example
 * 							Few adjustments
 * 0.9.71	2018-10-19		Just for new release, due problems on library.proprierties
 * 0.9.7	2018-10-18		Checking if debugger is enabled
 * 0.9.6 	2018-10-09		New debug format output
 * 0.9.5	2018-10-07		New print macros
 *							Optimization on debugPrintf logic
 * 0.9.4    2018-10-04		Now debugger starts disabled
 * 0.9.3	2018-10-01   	Few adjustments
 * 0.9.2	2018-08-28    	Few adjustments
 * 0.9.1	2018-08-28   	Few adjustments
 * 0.9.0  	2018-08-26		First beta
 *****************************************/

/*
 * Source for SerialDebug
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
 * This file contains the code for SerialDebug library.
 *
 */

/*
 * TODO list:
 * - see all warnings
 * - more optimizations to speed
 * - more optimizations to reduce memory and program for low memory boards (UNO, etc.)
 * - more types
 * - gpio commands
 */

/*
 * TODO known issues:
 * - Error on use debug* macros with F()
 */

// Disable debug - good for release (production)
// Put below define after the include SerialDebug in your project to disable all debug
// as nothing of SerialDebug is compiled, zero overhead :-)
//#define DEBUG_DISABLED true

#ifndef DEBUG_DISABLED

/////// Includes

#include <Arduino.h>

#ifndef _STDARG_H // To avoid error in Arduino IDE 1.8.5 and Linux - thanks to @wd5gnr
	#include <stdarg.h>
#endif

// Utilities

#include "utility/Util.h"
#include "utility/Fields.h"
#include "utility/Boards.h"

// This module

#include "SerialDebug.h"

// Debugger ?

#ifndef DEBUG_DISABLE_DEBUGGER

#ifdef BOARD_LOW_MEMORY
// Warning to low memory MCUs
#warning "Debugger on low memory MCU is still not yet full optimized - use with caution"
#endif

// Vector (to reduce memory and no fixed limit)

// Arduino arch have C++ std vector ?

#if defined ESP8266 || defined ESP32 || defined _GLIBCXX_VECTOR

	#define VECTOR_STD true

	// C++ std vector

	#include <vector>

	using namespace std;

#else // This Arduino arch is not compatible with std::vector

	// Using a lightweight Arduino_Vector library: https://github.com/zacsketches/Arduino_Vector/
	// Adapted and otimized by JoaoLopesF

	#include <utility/Vector.h>

#endif

#endif // DEBUG_DISABLE_DEBUGGER

// ESP8266 SDK

#if defined ESP8266
	extern "C" {
		bool system_update_cpu_freq(uint8_t freq);
	}
#endif

// Version -- Note to JoaoLopesF -> not forgot change it in github repo and versoes.txt (for app)
//                               -> Testing of low, medium e enough memory boards

#define DEBUG_VERSION F("0.9.82")                   // Version of this library

// Low memory board ?

#if defined BOARD_LOW_MEMORY && !(defined DEBUG_NOT_USE_FLASH_F)
	#define DEBUG_USE_FLASH_F true
#endif

/////// Variables - public

boolean _debugActive = false;		        		// Debug is only active after receive first data from Serial
uint8_t _debugLevel = DEBUG_LEVEL_NONE;         	// Current level of debug (init as disabled)

bool _debugSilence = false;							// Silent mode ?

bool _debugShowProfiler = true;						// Show profiler time ?
uint16_t _debugMinTimeShowProfiler = 0;				// Minimum time to show profiler
unsigned long _debugLastTime = millis(); 			// Last time show a debug

#if defined ESP32 || defined ESP8266 				// For Espressif boards
char _debugShowISR = ' ';							// Can show ISR (only if in 115200 bps)
#endif

#ifndef DEBUG_DISABLE_DEBUGGER 						// Only if debugger is enabled

uint8_t _debugFunctionsAdded = 0;					// Number of functions added

uint8_t _debugGlobalsAdded = 0;						// Number of globals added

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

uint8_t _debugWatchesAdded = 0;						// Number of watches added
boolean _debugWatchesEnabled = false;				// Watches is enabled (only after add any)?

#endif

boolean _debugDebuggerEnabled = false;				// Simple Software Debugger enabled ?

#endif // DEBUG_DISABLE_DEBUGGER

boolean _debugPrintIsNewline = true;				// Used in print macros

/////// Variables - private

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

	// Type of global or function arg

	typedef enum {

		DEBUG_TYPE_BOOLEAN,									// Basic types
		DEBUG_TYPE_CHAR,
		DEBUG_TYPE_INT,
		DEBUG_TYPE_U_LONG,

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

		DEBUG_TYPE_BYTE,
		DEBUG_TYPE_U_INT,
		DEBUG_TYPE_LONG,

		DEBUG_TYPE_FLOAT,
		DEBUG_TYPE_DOUBLE,

		DEBUG_TYPE_INT8_T,									// Integers size _t
		DEBUG_TYPE_INT16_T,
		DEBUG_TYPE_INT32_T,
//#ifdef ESP32
//		DEBUG_TYPE_INT64_T,
//#endif
		DEBUG_TYPE_UINT8_T,									// Unsigned integers size _t
		DEBUG_TYPE_UINT16_T,
		DEBUG_TYPE_UINT32_T,
//#ifdef ESP32
//		DEBUG_TYPE_UINT64_T
//#endif

#endif

		DEBUG_TYPE_CHAR_ARRAY,								// Strings
		DEBUG_TYPE_STRING,

		DEBUG_TYPE_FUNCTION_VOID,							// For function void

		DEBUG_TYPE_UNDEFINED								// Not defined

	} debugEnumTypes_t;

	// Debug functions

	struct debugFunction_t {
		const char* name = 0;							// Name
		const __FlashStringHelper *nameF = 0;			// Name (in flash)
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		const char* description = 0;					// Description
		const __FlashStringHelper *descriptionF = 0;	// Description (in flash)
#endif
		void (*callback)() = 0;							// Callbacks
		uint8_t argType = 0;							// Type of argument
	};

	#ifdef VECTOR_STD  // Arduino arch have C++ std vector

		vector<debugFunction_t> _debugFunctions;		// Vector array of functions

	#else // Using a Arduino_Vector library: https://github.com/zacsketches/Arduino_Vector/

		Vector<debugFunction_t> _debugFunctions;		// Vector array of functions

	#endif

	// Debug global variables

	struct debugGlobal_t {
		const char* name = 0;							// Name
		const __FlashStringHelper *nameF = 0;			// Name (in flash)
		uint8_t type = 0;								// Type of variable (see enum below)
		void *pointer = 0;								// Generic pointer
		uint8_t showLength = 0;							// To show only a part (strings)
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		const char* description = 0;					// Description
		const __FlashStringHelper *descriptionF = 0;	// Description (in flash)
#endif
		uint8_t typeOld = 0;							// Type of old value variable (used to strings)
		void *pointerOld = 0;							// Generic pointer for old value
		boolean changed = false;						// Value change (between 2 debug handle call)
		boolean updateOldValue = false;					// Update old value ? (in debug handle call)
	};

	#ifdef VECTOR_STD  // Arduino arch have C++ std vector

		vector<debugGlobal_t> _debugGlobals;		// Vector array of globals

	#else // Using a Arduino_Vector library: https://github.com/zacsketches/Arduino_Vector/

		Vector<debugGlobal_t> _debugGlobals;		// Vector array of globals

	#endif

	typedef enum {
			DEBUG_SHOW_GLOBAL, 							// For globals
			DEBUG_SHOW_GLOBAL_WATCH,					// For watches
			DEBUG_SHOW_GLOBAL_APP_CONN					// For SerialDebugApp connection
	} debugEnumShowGlobais_t;

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

	// Watches

	struct debugWatch_t {
		uint8_t globalNum;								// Global id
		uint8_t operation;								// Operation
		uint8_t typeValue = 0;							// Type of old value variable (used to strings)
		void *pointerValue = 0;							// Generic pointer to value (to do operation)
		boolean watchCross = false;						// Is a cross watch ?
		uint8_t globalNumCross = 0;						// To watch cross - compare two globals
		boolean enabled = true;							// Enabled ?
		boolean triggered = false;						// Triggered ? (operation -> true)
		boolean alwaysStop = false;						// Always stop, even _debugWatchStop = false
	};

	#ifdef VECTOR_STD  // Arduino arch have C++ std vector

		vector<debugWatch_t> _debugWatches;		// Vector array of watches

	#else // Using a Arduino_Vector library: https://github.com/zacsketches/Arduino_Vector/

		Vector<debugWatch_t> _debugWatches;		// Vector array of watches

	#endif

	static boolean _debugWatchStop = true;				// Causes a stop in debug for any positive watches ?

	static int8_t addWatch(Fields& fields);

#endif

#endif // DEBUG_DISABLE_DEBUGGER

#ifndef DEBUG_MINIMUM

	// Connection with SerialDebugApp ?

	static boolean _debugSerialApp = false;

	// String helper, used to store name extracted from Flash and receive commands
	// Unified this 2 usages, to reduce memory

	static String _debugString = "";

	// Last command

	static String _debugLastCommand = "";

	// Repeat last command (in each debugHandler)

	static boolean _debugRepeatCommand = false;

	// To show help (uses PROGMEM)
	// Note: Using PROGMEM in large string (even for Espressif boards)

#ifndef DEBUG_DISABLE_DEBUGGER

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

   static const char debugHelp[] PROGMEM = \
"\
*\r\n\
* Commands:\r\n\
*   ? or help -> display these help of commands\r\n\
*   m -> show free memory\r\n\
*   n -> set debug level to none\r\n\
*   v -> set debug level to verbose\r\n\
*   d -> set debug level to debug\r\n\
*   i -> set debug level to info\r\n\
*   w -> set debug level to warning\r\n\
*   e -> set debug level to errors\r\n\
*   s -> silence (Not to show anything else, good for analysis)\r\n\
*   p -> profiler:\r\n\
*      p      -> show time between actual and last message (in millis)\r\n\
*      p min  -> show only if time is this minimal\r\n\
*   r -> repeat last command (in each debugHandle)\r\n\
*      r ? -> to show more help \r\n\
*   reset -> reset the Arduino board\r\n\
*\r\n\
*   f -> call the function\r\n\
*      f ?  -> to show more help \r\n\
*   dbg [on|off] -> enable/disable the simple software debugger\r\n\
*\r\n\
*   Only if debugger is enabled: \r\n\
*      g -> see/change global variables\r\n\
*         g ?  -> to show more help \r\n\
*      wa -> see/change watches for global variables\r\n\
*         wa ?  -> to show more help \r\n\
*\r\n\
*   Not yet implemented:\r\n\
*      gpio -> see/control gpio\r\n\
*";

#else // Low memory board

   static const char debugHelp[] PROGMEM = \
"\
*\r\n\
* Commands:\r\n\
*   ? or help -> display these help of commands\r\n\
*   m -> show free memory\r\n\
*   n -> set debug level to none\r\n\
*   v -> set debug level to verbose\r\n\
*   d -> set debug level to debug\r\n\
*   i -> set debug level to info\r\n\
*   w -> set debug level to warning\r\n\
*   e -> set debug level to errors\r\n\
*   s -> silence (Not to show anything else, good for analysis)\r\n\
*   r -> repeat last command (in each debugHandle)\r\n\
*      r ? -> to show more help \r\n\
*   reset -> reset the Arduino board\r\n\
*\r\n\
*   f -> call the function\r\n\
*      f ?  -> to show more help \r\n\
*   dbg [on|off] -> enable/disable the simple software debugger\r\n\
*\r\n\
*   Only if debugger is enabled: \r\n\
*      g -> see/change global variables\r\n\
*         g ?  -> to show more help \r\n\
*\r\n\
*   Not yet implemented:\r\n\
*      gpio -> see/control gpio\r\n\
*";

#endif

#else // Debugger disabled

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

   static const char debugHelp[] PROGMEM = \
"\
*\r\n\
* Commands:\r\n\
*   ? or help -> display these help of commands\r\n\
*   m -> show free memory\r\n\
*   n -> set debug level to none\r\n\
*   v -> set debug level to verbose\r\n\
*   d -> set debug level to debug\r\n\
*   i -> set debug level to info\r\n\
*   w -> set debug level to warning\r\n\
*   e -> set debug level to errors\r\n\
*   s -> silence (Not to show anything else, good for analysis)\r\n\
*   p -> profiler:\r\n\
*      p      -> show time between actual and last message (in millis)\r\n\
*      p min  -> show only if time is this minimal\r\n\
*   r -> repeat last command (in each debugHandle)\r\n\
*      r ? -> to show more help \r\n\
*   reset -> reset the Arduino board\r\n\
*\r\n\
*   Not yet implemented:\r\n\
*      gpio -> see/control gpio\r\n\
*";

#else // Low memory board

   static const char debugHelp[] PROGMEM = \
"\
*\r\n\
* Commands:\r\n\
*   ? or help -> display these help of commands\r\n\
*   m -> show free memory\r\n\
*   n -> set debug level to none\r\n\
*   v -> set debug level to verbose\r\n\
*   d -> set debug level to debug\r\n\
*   i -> set debug level to info\r\n\
*   w -> set debug level to warning\r\n\
*   e -> set debug level to errors\r\n\
*   s -> silence (Not to show anything else, good for analysis)\r\n\
*   r -> repeat last command (in each debugHandle)\r\n\
*      r ? -> to show more help \r\n\
*   reset -> reset the Arduino board\r\n\
*\r\n\
*   Not yet implemented:\r\n\
*      gpio -> see/control gpio\r\n\
*";

#endif // BOARD_LOW_MEMORY

#endif // DEBUG_DISABLE_DEBUGGER

   /////// Prototypes - private

	// Note: only public functions start with debug...

	static void processCommand(String& command, boolean repeating = false, boolean showError = true);
	static void showHelp();

#ifndef DEBUG_DISABLE_DEBUGGER

	// For functions

	static int8_t addFunction(const char* name, uint8_t argType);
	static int8_t addFunction(const __FlashStringHelper *name, uint8_t argType);

	static void processFunctions(String& options);
	static int8_t showFunctions(String& options, boolean one, boolean debugSerialApp = false);
	static void callFunction(String& options);

	// For globais

	static int8_t addGlobal(const char* name,  void* pointer, uint8_t type, uint8_t showLength);
	static int8_t addGlobal(const __FlashStringHelper* name, void* pointer, uint8_t type, uint8_t showLength);

	static void processGlobals(String& options);
	static int8_t showGlobals(String& options, boolean one, boolean debugSerialApp = false);
	static boolean showGlobal(uint8_t globalNum, debugEnumShowGlobais_t mode, boolean getLastNameF);
	static void changeGlobal(Fields& fields);

	static boolean findGlobal (const char* globalName, uint8_t* globalNum, boolean sumOne = true);
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
	static boolean verifyGlobalType(uint8_t globalNum, uint8_t type);
#endif

	static void removeQuotation(String& string, boolean single);

	// For void* pointerValue values

	static void getStrValue(uint8_t type, void* pointer, uint8_t showLength, boolean showSize,  String& response, String& responseType);
	static void updateValue(uint8_t typeFrom, void* pointerFrom, uint8_t typeTo, void** pointerTo);
	static boolean apllyOperation(uint8_t type1, void* pointer1, uint8_t operation, uint8_t type2, void* pointer2);

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

	// For watches

	static int8_t addWatch(uint8_t globalNum, uint8_t operation, boolean allwaysStop);

	static void processWatches(String& options);
	static void processWatchesAction(Fields& fields);
	static int8_t showWatches(String& options, boolean debugSerialApp = false);
	static boolean showWatch(uint8_t watchNum, boolean debugSerialApp = false);
	static int8_t addWatchCross(Fields& fields);
	static boolean changeWatch(Fields& fields);
	static boolean getWatchOperation(String str, uint8_t* operation);

#endif

#endif // DEBUG_DISABLE_DEBUGGER

static int freeMemory();

#ifdef ARDUINO_ARCH_AVR

	// Only for AVR boards

	static void(* avrResetArduino) (void) = 0;	// Based on https://www.instructables.com/id/two-ways-to-reset-arduino-in-software/

#endif

#endif // DEBUG_MINIMUM

static void printSerialDebug();

/////// Prototypes - private

#ifndef DEBUG_MINIMUM
static void debugSerialAppConnection();
#endif // DEBUG_MINIMUM

/////// Defines (private)

#ifndef DEBUG_USE_FLASH_F // Not using flash to string and functions (good for board with memory  - more faster)
	#ifdef BOARD_ENOUGH_MEMORY
		#define DEBUG_NOT_USE_FLASH_F true
	#endif
#endif
#ifdef DEBUG_NOT_USE_FLASH_F // Not using Flash F, only string in RAM (more fast)
	#undef F
 	#define F(str) str
#endif

// Internal printf (no time, auto func, etc, just print it)

#ifndef DEBUG_MINIMUM

#ifdef DEBUG_USE_NATIVE_PRINTF // For Espressif boards, have Serial.printf

	#define PRINTF(fmt, ...)   Serial.printf(fmt, ##__VA_ARGS__)
	#define PRINTFLN(fmt, ...) Serial.printf(fmt "\r\n", ##__VA_ARGS__)

#else // Use debugPrintf

	#define PRINTF(fmt, ...)   debugPrintf(false, ' ', " ", fmt, ##__VA_ARGS__)
	#define PRINTFLN(fmt, ...) debugPrintf(true, ' ', " ", fmt, ##__VA_ARGS__)

#endif

#endif // DEBUG_MINIMUM

// Debug internal, all must commented on release

#define D(fmt, ...)
//#define D(fmt, ...) PRINTF("$dbg: " fmt "\r\n", ##__VA_ARGS__)

/////// Methods 

// Note: SerialDebug not is a C++ object class, as RemoteDebug
//       due all show debug is precompiler macros, 
//       for more performance.

////// Handles

// SerialDebug handle for debug deactivate
// Only activate it where receive first data from serial
// to show periodic messages and activate the debugs at first data is arrivved
// And only is call by debugHandler macro if debugs still deactive

void debugHandleInactive() {

#ifndef DEBUG_DISABLE_DEBUGGER
    static uint8_t execCount = 6; // Count debugHandleInactive calls
#endif

#if defined ESP8266 || defined ESP32 // Only for Espressif boards

    // Debug for ISR enable ? (only if serial bps = 1115200, due it use default of ESP SDK)

    if (_debugShowISR == ' ') { // not verified yet

    	_debugShowISR = ((Serial.baudRate() == 115200) ? 'Y': 'N');
    }

#endif

#ifndef DEBUG_MINIMUM
    // SerialApp connected ?

    if (_debugSerialApp && execCount == 6) {
    	Serial.println(F("$app:I"));
    }
#endif // DEBUG_MINIMUM

    // Test if first data is arrived, to enable debugs

    if (Serial.available() > 0) {

        _debugActive = true;

        printSerialDebug();
        Serial.println(F("Debug now is active."));

        // Verify
        // Show helps
        // Uncomment this if You want the help here
        //showHelp();

#ifdef DEBUG_MINIMUM

        // For minimum - checks if is app

        String data = "";
        while (Serial.available() > 0) {
        	char character = Serial.read();
        	if (character == 10 or character == 13) { // new line
        		break;
        	} else {
        		data.concat(character);
        	}
        }

        if (data == "$app") { // App connected - send status

        	// Send info

        	char features = 'D';
        	char dbgEnabled = 'D';
        	char dbgMinimum = 'Y';

        	Serial.print("$app:V:");
        	Serial.print(DEBUG_VERSION);
        	Serial.print(':');
        	Serial.print(BOARD);
        	Serial.print(':');
        	Serial.print(features);
        	Serial.print(':');
        	Serial.print('?');
        	Serial.print(':');
        	Serial.print(dbgEnabled);
        	Serial.print(':');
        	Serial.println(dbgMinimum);

        	// Print message

        	printSerialDebug();
        	Serial.print(F("Conection with app - SerialDebug library version "));
        	Serial.println(DEBUG_VERSION);

        }
#else
        // SerialApp connected ?

        if (_debugSerialApp) {
        	Serial.println(F("$app:A"));
        }
#endif
        // Go to initial level

        if (_debugLevel == DEBUG_LEVEL_NONE) {
        	debugSetLevel (DEBUG_INITIAL_LEVEL);
        }

        return;
    }

#ifndef DEBUG_DISABLE_DEBUGGER

    // Show message initial (only after n executions, due where board is booting, some messages can be lost

	if (execCount > 0) { // To count executions. You can change it

		if (execCount % 2 == 0) { // For each 2 seconds
			printSerialDebug();
			Serial.println(F("Please press ? or another command and enter to activate debugs"));
		}

    	execCount--;

#ifndef BOARD_LOW_MEMORY // Not for low memory boards to save Flash memory
		if (execCount == 0) {
			Serial.println(F("* Please verify if is in Newline mode in monitor serial"));
		}
#endif

    }
#else
		printSerialDebug();
		Serial.println(F("Please press any key and enter to activate debugs"));
#endif

}

// Set actual levef of debug

void debugSetLevel(uint8_t level) {

	printSerialDebug();

	if (level < DEBUG_LEVELS_SIZE) {
		_debugLevel = level;
		Serial.print(F("Level set to "));
		switch (_debugLevel) {
		case DEBUG_LEVEL_NONE:
			Serial.println(F("None"));
			break;
		case DEBUG_LEVEL_ERROR:
			Serial.println(F("Error"));
			break;
		case DEBUG_LEVEL_WARN:
			Serial.println(F("Warning"));
			break;	\
		case DEBUG_LEVEL_INFO:
			Serial.println(F("Information"));
			break;	\
		case DEBUG_LEVEL_DEBUG:
			Serial.println(F("Debug"));
			break;
		case DEBUG_LEVEL_VERBOSE:
			Serial.println(F("Verbose"));
			break;
		}
	}

#ifndef DEBUG_MINIMUM
	if (_debugSerialApp) { // For DebugSerialApp connection ?

		// Send status

		PRINTFLN(F("$app:L:%u"), _debugLevel);
	}
#endif

	// Out of silent mode

	if (_debugSilence) {

		debugSilence(false, false);
	}
}

// Silence

void debugSilence(boolean activate, boolean showMessage, boolean fromBreak) {

	_debugSilence = activate;

	//D("silence %d", _debugSilence);

	if (_debugSilence && showMessage) {

		printSerialDebug();
		Serial.println();
		Serial.println(F("* Debug now is in silent mode"));
		Serial.println(F("* Press enter or another command to return show debugs"));

	} else if (!_debugSilence && showMessage) {

		printSerialDebug();
		Serial.println(F("Debug now exit from silent mode"));
	}

#ifndef DEBUG_MINIMUM
	if (_debugSerialApp && !fromBreak) { // For DebugSerialApp connection ?

		// Send status

		PRINTFLN(F("$app:S:%c"), ((_debugSilence)? '1':'0'));
	}
#endif // DEBUG_MINIMUM

}

// Show debug info - in begin of line
// Using String to concat, in tests, optimize only about 2%, due it not used

void debugPrintInfo(const char level, const char* function) {

	// Show level

	Serial.print('(');
	Serial.print(level);
	Serial.print(' ');

	// Show time / profiler

	uint32_t time = 0;

	if (_debugShowProfiler) {

		time = (millis() - _debugLastTime);

		Serial.print("p:^");

		if (time < 10) { // Simple formatter, to not use printf
			Serial.print("000");
		} else if (time < 100) {
			Serial.print("00");
		} else if (time < 1000) {
			Serial.print('0');
		}
		Serial.print(time);

	} else {

		Serial.print(millis());
	}

	_debugLastTime = millis();

	// Show function

#ifndef DEBUG_AUTO_FUNC_DISABLED

	if (function) { // Auto function

		Serial.print(' ');
		Serial.print(function);

	}
#endif

	// Show Core Id ?

#ifdef DEBUG_CORE

	Serial.print(' ');
	Serial.print('C');
	Serial.print(xPortGetCoreID());

#endif

	Serial.print(')');
	Serial.print(' ');

}

// Print SerialDebug begin for messages
// Note: It is done to reduce Program memory consupmition - for low memory boards support

void printSerialDebug() {

	Serial.print(F("* SerialDebug: "));
}

// Not for minimum mode - to save memory

#ifndef DEBUG_MINIMUM

// SerialDebug handle, to process data receipts, handles and events
// Note: In ESP32 not used task for it, to avoid mixed serial outputs

void debugHandleEvent(boolean calledByHandleEvent) {

	//D("handle");

	static uint32_t lastTime = millis(); 		// Time of last receipt
	static char last = ' ';						// Last char received

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
	static String command;						// Buffer for command receipt
#else
	static String& command = _debugString;		// Buffer for command receipt
	command = "";
#endif

	uint32_t diffTime = (millis() - lastTime);	// Diff of time

	// Process serial data

	while (Serial.available()) {

		// Ignore repeatings

		_debugRepeatCommand = false;

	    // Get the new char:

	    char character = (char)Serial.read();

	    //D("char=%c %d", character, character);

	    // Clear buffer if is a long time of last receipt

	    if (command.length() > 0 && diffTime > 2000) {
	    	command = "";
	    }

		// Newline (CR or LF) - once one time if (\r\n)

		if (isCRLF(character) == true) {

			if (isCRLF(last) == false) {

				if (_debugSilence && command != "s") { // Exit from silence mode

					debugSilence(false, true);

				}

				// Process the command

				if (command.length() > 0) { // Exist command ?

					processCommand(command);

				}
			}

			command = ""; // Init it for next command

		} else if (isPrintable(character)) { // Only valid

			// Concat

			command.concat(character);

		}

		// Last char

		last = character;
	}

	// Test time, if is low delay in loop, can execute only in this time

	if (diffTime >= DEBUG_MIN_TIME_EVENT) { // For this minimum time

//		D("diffTime = %u", diffTime);

		// Repeating commands ?

		if (_debugRepeatCommand && _debugLastCommand.length() > 0) {

			processCommand(_debugLastCommand, true);
		}

#ifndef DEBUG_DISABLE_DEBUGGER

		// Handle the debugger (globals and watches)

		if (_debugDebuggerEnabled && _debugGlobalsAdded > 0) {

	#ifndef BOARD_LOW_MEMORY // Not for low memory boards

			debugHandleDebugger(calledByHandleEvent);

	#else
			if (calledByHandleEvent) {
				debugHandleDebugger(calledByHandleEvent);
			}
	#endif
		}
#endif // DEBUG_DISABLE_DEBUGGER

		// Save time

	    lastTime = millis();

	}
}

// Profiler

void debugSetProfiler(boolean activate) {

	_debugShowProfiler = activate;

}

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

// Profiler

void debugShowProfiler(boolean activate, uint16_t minTime, boolean showMessage) {

	_debugShowProfiler = activate;

	if (activate) {

		_debugMinTimeShowProfiler = minTime;

		if (showMessage) {

			if (minTime == 0) {

	    		printSerialDebug();
				Serial.println(F("Show profiler: On (without minimal time)\r\n*"));

			} else {

	    		printSerialDebug();
				PRINTFLN(F("Show profiler: On (with minimal time: %u\r\n*"), _debugMinTimeShowProfiler);

			}
		}

	} else {

		_debugMinTimeShowProfiler = 0;

		if (showMessage) {

    		printSerialDebug();
			Serial.println(F("Show profiler: Off\r\n*"));
		}

	}
}
#endif

// Debug printf (used for not Espressif boards (that have it) or to use flash strings
// Based on Arduino Espressif Print.printf (thanks a lot)

#ifndef DEBUG_USE_NATIVE_PRINTF // Not for native Serial.printf

void debugPrintf(boolean newline, const char level, const char* function, const char* format, ...) {

	// Buffer

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
	const size_t bufSize = 64; 	// Size of buffer
#else
	const size_t bufSize = 32; 	// Size of buffer
#endif
	char buffer[bufSize];		// Buffer

	// Process the initial format of SerialDebug

	if (level != ' ') { // ' ' is used internally in SerialDebug to do printf, if Arduino no have, or for flash F support

//#ifndef DEBUG_AUTO_FUNC_DISABLED
//		if (function) { // Auto function
//
//			if (_debugShowProfiler) {
//				snprintf(buffer, bufSize,
//						"(%c p:^%04lu) (%s) ",
//						level, (millis() - _debugLastTime),
//						function);
//				_debugLastTime = millis();
//			} else {
//				snprintf(buffer, bufSize,
//						"(%c) (%lu) (%s) ",
//						level, millis(),
//						function);
//			}
//
//		}
//
//#else // No auto function
//
//		if (_debugShowProfiler) {
//			snprintf(buffer, bufSize,
//					"(%c p:^%04lu)",
//					level, (millis() - _debugLastTime));
//			_debugLastTime = millis();
//		} else {
//			snprintf(buffer, bufSize,
//					"(%c) (%lu)",
//					level, millis());
//		}
//#endif
//
//		// Send it to serial
//
//		Serial.print(buffer);

		// Now using a any prints, to avoid one printf

		debugPrintInfo(level, function);

	}

	// Process the var arg to process the custom format

	va_list arg;
	va_list copy;
	char* temp = buffer;

	va_start(arg, format);
	va_copy(copy, arg);
	size_t len = vsnprintf(NULL, 0, format, arg);
	va_end(copy);
	if(len >= bufSize){
	    temp = new char[len+1];
	    if(temp == NULL) {
	    	va_end(arg);
	        return;
	    }
	}
	len = vsnprintf(temp, len+1, format, arg);

	// Send it to serial

//	Serial.write((uint8_t*)temp, len);
	Serial.write((const char*)temp);

	// Newline ?

	if (newline) {
		Serial.println(); // TODO append \r\n in buffer to avoid this command
	}

	// Clean

	va_end(arg);
	if(len > bufSize){
	    delete[] temp;
	}

	return;
}

void debugPrintf(boolean newline, const char level, const char* function, const __FlashStringHelper *format, ...) {

	// For Flash string variables

	String strFormat = format;

	// Buffer

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
	const size_t bufSize = 64; 	// Size of buffer
#else
	const size_t bufSize = 32; 	// Size of buffer
#endif
	char buffer[bufSize];		// Buffer

	// Process the initial format of SerialDebug

	if (level != ' ') { // ' ' is used internally in SerialDebug to do printf, if Arduino no have, or for flash F support

//#ifndef DEBUG_AUTO_FUNC_DISABLED
//		if (function) { // Auto function
//
//			if (_debugShowProfiler) {
//				snprintf(buffer, bufSize,
//						"(%c p:^%04lu) (%s) ",
//						level, (millis() - _debugLastTime),
//						function);
//				_debugLastTime = millis();
//			} else {
//				snprintf(buffer, bufSize,
//						"(%c) (%lu) (%s) ",
//						level, millis(),
//						function);
//			}
//
//		}
//
//#else // No auto function
//
//		if (_debugShowProfiler) {
//			snprintf(buffer, bufSize,
//					"(%c p:^%04lu)",
//					level, (millis() - _debugLastTime));
//			_debugLastTime = millis();
//		} else {
//			snprintf(buffer, bufSize,
//					"(%c) (%lu)",
//					level, millis());
//		}
//#endif
//
//		// Send it to serial
//
//		Serial.print(buffer);

		// Now using a any prints, to avoid one printf

		debugPrintInfo(level, function);
	}


	// Process the var arg to process the custom format

	va_list arg;
	va_list copy;
	char* temp = buffer;

	va_start(arg, format);
	va_copy(copy, arg);
	size_t len = vsnprintf(NULL, 0, strFormat.c_str(), arg);
	va_end(copy);
	if(len >= bufSize){
	    temp = new char[len+1];
	    if(temp == NULL) {
	    	va_end(arg);
	        return;
	    }
	}
	len = vsnprintf(temp, len+1, strFormat.c_str(), arg);

	// Send it to serial

//	Serial.write((uint8_t*)temp, len);
	Serial.write((const char*)temp);

	// Newline ?

	if (newline) {
		Serial.println(); // TODO append \r\n in buffer to avoid this command
	}

	// Clean

	va_end(arg);
	if(len > bufSize){
	    delete[] temp;
	}

	return;

}

#endif // DEBUG_USE_NATIVE_PRINTF

// debugBreak - show a message and wait for response

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
String debugBreak(String str, uint32_t timeout) {
	return debugBreak(str.c_str(), timeout);
}
#endif

String debugBreak() {

	return debugBreak("", DEBUG_BREAK_TIMEOUT, false);

}
String debugBreak(const __FlashStringHelper *ifsh, uint32_t timeout, boolean byWatch) {

	return debugBreak(String(ifsh).c_str(), timeout, byWatch);
}

String debugBreak(const char* str, uint32_t timeout, boolean byWatch) {

	//D("debugBreak - timeout %u", timeout);

	// Show a message

	char appBreakType = '1';

	if (strlen(str) > 0) { // String is informed ?

		PRINTFLN(F("* %s"), str);

	} else if (timeout == DEBUG_BREAK_TIMEOUT) { // Is called by debugBreak() and not for watches ?

		Serial.println(F("* Press any key or command, and enter to continue"));
		appBreakType = 'C';
	}

	if (_debugSerialApp) { // For DebugSerialApp connection ?

		PRINTFLN(F("$app:B:%c:%c"), appBreakType, ((byWatch)?'W':' '));
	}

	// Response buffer

	String response = "";

	// Wait for response // Note: the Arduino will wait for it, to continue runs

	uint32_t lastTime = millis(); 			// Time of last receipt
	char last = ' ';						// Last char received

	// Enter in silence (if is not yet)

	boolean oldSilence = _debugSilence;		// In silence mode ?

	if (!_debugSilence) {
		debugSilence(true, false, true);
	}

	// Ignore buffer

	while (Serial.available()) {
		Serial.read();
	}

	// Process serial data (until timeout, if informed)

	while (timeout == 0 ||
			((millis() - lastTime) <= timeout)){

		if (Serial.available()) {

		    // Get the new char:

		    char character = (char)Serial.read();

		    // Clear buffer if is a long time of last receipt

		    if (response.length() > 0 && (millis() - lastTime) > 2000) {
		    	response = "";
		    }
		    lastTime = millis(); // Save it

			// Newline (CR or LF) - once one time if (\r\n)

			if (isCRLF(character) == true) {

				if (isCRLF(last) == false) { // New line -> return the command

					//D("break");

					break;
				}

			} else if (isPrintable(character)) { // Only valid

				// Concat

				response.concat(character);
			}

			// Last char

			last = character;
		}

		delay(10); // Give a time
	}

	if (_debugSerialApp) { // For DebugSerialApp connection ?

		Serial.println(F("$app:B:0"));

	}

	// Is in silence ? (restore it)

	if (_debugSilence && !oldSilence) {
		debugSilence(false, false, true);
	}

	//D("response -> %s", response.c_str());

	// Response (tolower always)
	// response.toLowerCase();

	return response;
}

// Process command receipt by serial (Arduino monitor, etc.)

static void processCommand(String& command, boolean repeating, boolean showError) {

	// Reduce information on error to support low memory boards
	// Use a variable to reduce memory

#if DEBUG_USE_FLASH_F

	__FlashStringHelper* errorSintax = F("Invalid command. Use ? to show help");

#else

	const char* errorSintax = "Invalid command. Use ? to show help";

#endif

	//D(F("Command: %s last: %s"), command.c_str(), _debugLastCommand.c_str());

	// Disable repeating

	if (!repeating && _debugRepeatCommand) {
		_debugRepeatCommand = false;
	}

	// Can repeat ?

	boolean canRepeat = false;

	// Extract options

	String options = "";
	int16_t pos = command.indexOf(' ');
	if (pos > 0) {
		options = command.substring(pos + 1);
		command = command.substring(0, pos);
	}

	// Invalid

	if (command.length() > DEBUG_MAX_SIZE_COMMANDS) {
		printSerialDebug();
		Serial.println(errorSintax);
		return;
	}

	if (options.length() > DEBUG_MAX_SIZE_CMD_OPTIONS) {
		printSerialDebug();
		Serial.println(errorSintax);
		return;
	}

	// Show command

	if (command != "$app") {
		printSerialDebug();
		PRINTFLN(F("Command recv.: %s opt: %s"), command.c_str(), options.c_str());
	}

	// Verify if pass value between '"' - to avoid split the value in fields logic

	boolean inMark = false;
	char conv = 31;

	for (uint8_t i=0; i<options.length(); i++) {

		char c = options.charAt(i);

		if (c == '"') {

			inMark = !inMark;

		} else {

			if (c == ' ' && inMark) {
				options.setCharAt(i, conv);
			}
		}
	}

	//D("opt -> %s", options.c_str());

	// Process the command

	if (command == "h" || command == "help" || command == "?") {

		// Show help

		showHelp();

		// Do a break

		String response = debugBreak("", DEBUG_BREAK_TIMEOUT, true);

		if (response.length() > 0) { // Process command

			processCommand(response, false);
		}

	} else if (command == "v") {

		// Debug level

		debugSetLevel(DEBUG_LEVEL_VERBOSE);

	} else if (command == "d") {

		// Debug level

		debugSetLevel(DEBUG_LEVEL_DEBUG);

	} else if (command == "i") {

		// Debug level

		debugSetLevel(DEBUG_LEVEL_INFO);

	} else if (command == "w") {

		// Debug level

		debugSetLevel(DEBUG_LEVEL_WARN);

	} else if (command == "e") {

		// Debug level

		debugSetLevel(DEBUG_LEVEL_ERROR);

	} else if (command == "n") {

		// Debug level

		debugSetLevel(DEBUG_LEVEL_NONE);

	} else if (command == "s") {

		// Silence

		debugSilence (!_debugSilence, true);

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

	} else if (command == "p") {

		// Show profiler with minimal time

		if (options.length() == 0) {

			// Show profiler

			debugShowProfiler(!_debugShowProfiler, 0, true);

		} else {

			// With minimal time

			if (strIsNum(options)) {

				int32_t aux = options.toInt();

				if (aux > 0) { // Valid number

					// Show profiler

					debugShowProfiler(true, aux, true);

				}

			} else {

				printSerialDebug();
				Serial.println(F("Invalid number in argument"));
			}
		}

#endif // Not low memory board

	} else if (command == "dbg") {

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

		// Enable/disable the Simple Software Debugger

		boolean saveWatchStop = _debugWatchStop;

		if (_debugSerialApp) { // For DebugSerialApp connection ?

			_debugWatchStop = false; // disable it for now

		}
#endif // Not low memory board

		// Globals or functions added ?

		if (_debugGlobalsAdded == 0 && _debugFunctionsAdded == 0) {
			printSerialDebug();
			Serial.println(F("No globals or functions added"));
			return;
		}

		// Process

		if (options == "on") {
			_debugDebuggerEnabled = true;
		} else if (options == "off") {
			_debugDebuggerEnabled = false;
		} else {
			_debugDebuggerEnabled = !_debugDebuggerEnabled; // invert it
		}

		if (_debugSerialApp) { // For DebugSerialApp connection ?

			// Debugger enabled ?

			if (_debugDebuggerEnabled) {

				// Process handle to update globals

				debugHandleDebugger(true);

				// Send debugger elements

				printSerialDebug();
				PRINTFLN(F("Sending debugger objects ..."));

				// Send info

				PRINTFLN(F("$app:D:")); // To clean arrays

				String all="";

				if (_debugFunctionsAdded > 0) {
					showFunctions(all, false, true);
				}

				if (_debugGlobalsAdded > 0) {
					showGlobals(all, false, true);
				}

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

				if (_debugWatchesAdded > 0) {
					showWatches(all, true);
				}

#endif // BOARD_LOW_MEMORY

				printSerialDebug();
				PRINTFLN(F("End of sending."));

			}

			// Send status

			PRINTFLN(F("$app:D:%u"), _debugDebuggerEnabled);

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

			// Restore stop

			_debugWatchStop = saveWatchStop;
#endif // Not low memory board


		}

		printSerialDebug();
		PRINTFLN(F("Simple software debugger: %s"), (_debugDebuggerEnabled) ? "On":"Off");

#else
		printSerialDebug();
		Serial.println(F("Debugger is not enabled in your project"));

#endif
	} else if (command == "f") {

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

		// Process

		processFunctions(options);

		canRepeat = true;
#else
		printSerialDebug();
		Serial.println(F("Debug functions is not enabled in your project"));

#endif
	} else if (command == "g") {

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

		// Process

		if (_debugDebuggerEnabled) {

			processGlobals(options);

			canRepeat = true;

		} else {

			printSerialDebug();
			Serial.println(F("Debugger is not enabled, please command dbg to enable this"));
		}

#else
		printSerialDebug();
		Serial.println(F("Debug functions is not enabled in your project"));

#endif

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

	} else if (command == "wa") {

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

		// Process

		if (_debugDebuggerEnabled) {

			processWatches(options);

		} else {

			printSerialDebug();
			Serial.println(F("Debugger is not enabled, please command dbg to enable this"));
		}

#else
		printSerialDebug();
		Serial.println(F("Debug functions is not enabled in your project"));

#endif

#endif // Not low memory board

	} else if (command == "m") {

		int free = freeMemory();

		if (free != -1) {

			PRINTFLN(F("* Free Heap RAM: %d"), free);

			canRepeat = true;

			if (_debugSerialApp) { // For DebugSerialApp connection ?

				PRINTFLN(F("$app:M:%d:"), free);
			}

		} else {

			printSerialDebug();
			Serial.println(F("This option is not implemented for this board"));
		}

	} else if (command == "reset" ) {

#ifdef ARDUINO_ARCH_AVR

		printSerialDebug();
		Serial.println(F("Resetting ..."));

		delay(1000);

		// Reset

		avrResetArduino();

#elif defined ESP8266 || defined ESP32 // Only for Espressif boards

		printSerialDebug();

	#if defined ESP8266
		Serial.println("Resetting the ESP8266 ...");
	#elif defined ESP32
		Serial.println("Resetting the ESP32 ...");
	#endif

		delay(1000);

		// Reset

		ESP.restart();

#else
		printSerialDebug();
		Serial.println(F("No reset for this board"));

#endif // ARDUINO_ARCH_AVR

#if defined ESP8266 // Only to ESP8266

	} else if (command == "cpu80") {

		// Change ESP8266 CPU para 80 MHz

		system_update_cpu_freq(80);
		printSerialDebug();
		Serial.println(F("CPU ESP8266 changed to: 80 MHz"));

	} else if (command == "cpu160") {

		// Change ESP8266 CPU para 160 MHz

		system_update_cpu_freq(160);
		printSerialDebug();
		Serial.println(F("CPU ESP8266 changed to: 160 MHz"));

#endif

	} else if (command == "r") {

		// Repeat last command

		if (options == "?") { // Help

			printSerialDebug();
			Serial.println(F("Valid last commands to repeat: f|g|m"));

		} else if (_debugLastCommand.length() == 0) {

			printSerialDebug();
			Serial.println(F("Last command not set or unsupported. use r ? to show help"));

		} else { // Start repeats

			printSerialDebug();
			PRINTFLN(F("Start repeating command: %s - press any command or enter to stop"), _debugLastCommand.c_str());
			_debugRepeatCommand = true;
		}

	} else if (command == "$app") {

		// Connection with SerialDebugApp

		D("$app -> %s", options.c_str());

		debugSerialAppConnection();

	} else {

		// Command invalid

		if (showError) {
			printSerialDebug();
			Serial.println(errorSintax);
		}
	}

	// Can repeat ?

	//D("canrepeat=%d", canRepeat);

	if (!repeating) { // Not if repeating commands

		if (canRepeat) {

			// Save it

			_debugLastCommand = command;
			_debugLastCommand.concat(' ');
			_debugLastCommand.concat(options);

		} else if (!_debugRepeatCommand) {

			// Clear it

			_debugLastCommand = "";
		}
	}
}

// Show debug help

void showHelp() {

	Serial.println('*');
	printSerialDebug();
	Serial.println();
	Serial.print(F("* Version: "));
	Serial.print(DEBUG_VERSION);
	Serial.println();

	Serial.print(F("* Arduino board: "));
	Serial.print(BOARD);
	Serial.println();

	int free = freeMemory();

	if (free != -1) {

		PRINTFLN(F("* Free Heap RAM: %d"), free);
	}

#if defined ESP8266 || defined ESP32
	PRINTFLN(F("* ESP SDK version: %s"), ESP.getSdkVersion());
#endif

	// Using PROGMEM in large strings (even for Espressif boards)

	Serial.println(FPSTR(debugHelp));

#ifdef ESP8266 // Esp8266 only (ESP32 not to easy to change it)

	Serial.println(F("*    cpu80  -> ESP8266 CPU a 80MHz"));
	Serial.println(F("*    cpu160 -> ESP8266 CPU a 160MHz"));

#endif

}

// Connection with SerialDebugApp

static void debugSerialAppConnection() {

	// Connected

	_debugSerialApp = true;

	// Send version, board, debugger disabled and  if is low or enough memory board

	char features;
	char dbgEnabled;

	// Features

#if defined DEBUG_DISABLE_DEBUGGER
	features = 'D'; // Disabled
#elif defined BOARD_LOW_MEMORY
	features = 'L'; // Low
#elif defined BOARD_ENOUGH_MEMORY
	features = 'E'; // Enough
#else
	features = 'M'; // Medium
#endif

	// Debugger is enabled ?

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

	dbgEnabled = 'E';

	// Send debugger elements

	if (_debugDebuggerEnabled) {

		printSerialDebug();
		PRINTFLN(F("Sending debugger objects ..."));

		// Send info

		PRINTFLN(F("$app:D:")); // To clean arrays

		String all="";

		if (_debugFunctionsAdded > 0) {
			showFunctions(all, false, true);
		}

		if (_debugGlobalsAdded > 0) {
			showGlobals(all, false, true);
		}

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

		if (_debugWatchesAdded > 0) {
			showWatches(all, true);
		}

#endif // BOARD_LOW_MEMORY

		printSerialDebug();
		PRINTFLN(F("End of sending."));

	}

#else

	dbgEnabled = 'D';

#endif // DEBUG_DISABLE_DEBUGGER

	// Send info

	String version = String(DEBUG_VERSION);
	String board = String(BOARD);

	PRINTFLN(F("$app:V:%s:%s:%c:%d:%c:N"), version.c_str(), board.c_str(), features, freeMemory(), dbgEnabled);

	// Send status

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

	// Send status of debugger

	PRINTFLN(F("$app:D:%u"), _debugDebuggerEnabled);
#endif

	// Status of debug level

	PRINTFLN(F("$app:L:%u"), _debugLevel);

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

	// Status of nonstop

	PRINTFLN(F("$app:W:s:%c"), ((_debugWatchStop)?'1':'0'));

#endif // BOARD_LOW_MEMORY

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled
	// Status of debugger

	PRINTFLN(F("$app:D:%u"), _debugDebuggerEnabled);
#endif

	// Print message

	printSerialDebug();
	PRINTFLN(F("Conection with app - SerialDebug library version %s"), version.c_str());

	// Out of silent mode

	if (_debugSilence) {

		debugSilence(false, false);
	}
}

//////// Simple software debugger

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

// Handle for debugger

void debugHandleDebugger (boolean calledByHandleEvent) {

	// Time measure - give commented

	uint32_t time = micros();

	//D("debugH %d", _debugWatchesEnabled);

	// Inactive ?

	if (!_debugActive || !_debugDebuggerEnabled) {
		return;
	}

	// Process globals

	for (uint8_t g=0; g < _debugGlobalsAdded; g++) {

#ifdef BOARD_ENOUGH_MEMORY // Modern and faster board ?

		boolean process = _debugSerialApp; // Always process for app

#else
		boolean process = _debugSerialApp && calledByHandleEvent; //false;
#endif
		debugGlobal_t *global = &_debugGlobals[g];

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

		if (!process) {

			// Process watches of type when changed

			if (_debugWatchesAdded > 0 && _debugWatchesEnabled) {

				// Verify if exist any watch for this global
				// Not for strings - due is low to process any time

				if (calledByHandleEvent ||
					(global->type != DEBUG_TYPE_CHAR_ARRAY &&
					global->type != DEBUG_TYPE_STRING)) {

					for (uint8_t w=0; w < _debugWatchesAdded; w++) {

						if (_debugWatches[w].globalNum == g &&
							_debugWatches[w].enabled) {

							process = true; // Process it

							break;
						}
					}
				}
			}
		}
#endif

		// Process ?

		//D("debugH w %d enab %d proc %d ", _debugWatchesAdded,  _debugWatchesEnabled, process);

		if (process) {

			// Value changed ?

			boolean changed = apllyOperation(global->type, global->pointer, \
												DEBUG_WATCH_DIFF, \
												global->typeOld, global->pointerOld);

			//D("global %u %s type %u type old %u chg %d %d", g, global->name, global->type, global->typeOld, changed, global->changed);

			if (changed) { // Changed ?

				//D("global changed");

				if (global->pointerOld) { // Only if it has content

					global->changed = true;

					if (_debugSerialApp) { // SerialDebugApp connected ?

						// Get value

						String value = "";
						String type = "";

						getStrValue(global->type, global->pointer, global->showLength, false, value, type);

						// Send it

						if (_debugSerialApp) {
							PRINTFLN(F("$app:C:%u:%s"), (g + 1), value.c_str());
						}
					}
				}

				// Mark to update old value

				global->updateOldValue = true;

			} else if (!changed) { // && calledByHandleEvent) { // only if called by handle event

				if (global->changed) {

					global->changed = false;

					//D("global %u !chg", g);

				}

			}
		}
	}

	// Break ?

	boolean hasBreak = false;

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

	// Process watches

	if (_debugWatchesAdded > 0 && _debugWatchesEnabled) {

		uint8_t totTriggered = 0;
		boolean alwaysStop = false;

		for (uint8_t w=0; w < _debugWatchesAdded; w++) {

			debugWatch_t *watch = &_debugWatches[w];
			debugGlobal_t *global = &_debugGlobals[watch->globalNum];

			// Only verify watch, if have change in global or in globals for watch cross
			// Only if enabled

			if (watch->enabled && (global->updateOldValue || watch->watchCross || watch->operation == DEBUG_WATCH_CHANGED)) {

				boolean triggered = false;

				// When changed type

				if (watch->operation == DEBUG_WATCH_CHANGED) {

					// Otimized: global is checked before

					triggered = global->changed;

				} else if (watch->watchCross) { // Is cross type ?

					debugGlobal_t *globalCross = &_debugGlobals[watch->globalNumCross];

					if (global->updateOldValue || globalCross->updateOldValue) {

						triggered = apllyOperation(global->type, global->pointer, \
													watch->operation, \
													globalCross->type, globalCross->pointer);
					}

				} else { // Anothers

					triggered = apllyOperation(global->type, global->pointer, \
												watch->operation, \
												global->type, watch->pointerValue);
				}

				// Has triggered  ?

				if (!watch->triggered && triggered) {

					watch->triggered = true;

					totTriggered++;

					if (watch->alwaysStop && !alwaysStop) { // Always stop ?
						alwaysStop = true;
					}

					printSerialDebug();
					PRINTFLN(F("Watch %u is triggered:"), (w + 1));

					showWatch(w, false);

					if (_debugSerialApp) { // App ?
						PRINTFLN(F("$app:T:%u:1"), (w + 1));
					}

				} else if (watch->triggered && !triggered) { // Unmark it

					watch->triggered = false;

					if (_debugSerialApp) { // App ?
						PRINTFLN(F("$app:T:%u:0"), (w + 1));
					}

				}
			}
		}

		// Has triggered any watch ?

		if (totTriggered > 0 && (_debugWatchStop || alwaysStop)) { // Stop on watches ?

			printSerialDebug();
			PRINTFLN(F("%u watch(es) has triggered."), totTriggered);
			Serial.println(F("* Press enter to continue"));
			Serial.println(F("* or another command as: reset(to reset) or ns(to not stop again)"));

			// Do a break

			String response = debugBreak("", DEBUG_BREAK_WATCH, true);

			if (response.length() > 0) { // Process command

				if (response == "ns") { // Non stop
					response = "wa ns";
				}
				processCommand(response, false, false);
			}

			hasBreak = true;
		}
	}

#endif

	// Update old value for globals (after watch, to show old value correct

	for (uint8_t g=0; g < _debugGlobalsAdded; g++) {

		debugGlobal_t *global = &_debugGlobals[g];

		if (global->updateOldValue) {

			// Copy value of globals variable to old

			updateValue(global->type, global->pointer, global->typeOld, &(global->pointerOld));

			// Unmark it

			global->updateOldValue = false;
		}
	}

	// Debug (give commented)

	if (!hasBreak) { // Not for watch triggered, have a delay for waiting response
		time = (micros() - time);
		if (time >= 250) {
			D("handle dbg call=%d elap=%u us", calledByHandleEvent, time);
		}
	}
}

// Add a function

int8_t debugAddFunctionVoid(const char* name, void (*callback)()) {

	int8_t pos = addFunction(name, DEBUG_TYPE_FUNCTION_VOID);

	if (pos != -1) {
		_debugFunctions[pos].callback = callback;
	}
	return pos;
}

int8_t debugAddFunctionStr(const char* name, void (*callback)(String)) {

	int8_t pos = addFunction(name, DEBUG_TYPE_STRING);

	if (pos != -1) {
		_debugFunctions[pos].callback = (void(*) (void)) callback;
	}
	return pos;
}

int8_t debugAddFunctionChar(const char* name, void (*callback)(char)) {

	int8_t pos = addFunction(name, DEBUG_TYPE_CHAR);

	if (pos != -1) {
		_debugFunctions[pos].callback = (void(*) (void)) callback;
	}
	return pos;
}

int8_t debugAddFunctionInt(const char* name, void (*callback)(int)) {

	int8_t pos = addFunction(name, DEBUG_TYPE_INT);

	if (pos != -1) {
		_debugFunctions[pos].callback = (void(*) (void))callback;
	}
	return pos;
}

// For Flash F()

int8_t debugAddFunctionVoid(const __FlashStringHelper* name, void (*callback)()) {

	int8_t pos = addFunction(name, DEBUG_TYPE_FUNCTION_VOID);

	if (pos != -1) {
		_debugFunctions[pos].callback = callback;
	}
	return pos;
}

int8_t debugAddFunctionStr(const __FlashStringHelper* name, void (*callback)(String)) {

	int8_t pos = addFunction(name, DEBUG_TYPE_STRING);

	if (pos != -1) {
		_debugFunctions[pos].callback = (void(*) (void)) callback;
	}
	return pos;
}

int8_t debugAddFunctionChar(const __FlashStringHelper* name, void (*callback)(char)) {

	int8_t pos = addFunction(name, DEBUG_TYPE_CHAR);

	if (pos != -1) {
		_debugFunctions[pos].callback = (void(*) (void)) callback;
	}
	return pos;
}

int8_t debugAddFunctionInt(const __FlashStringHelper* name, void (*callback)(int)) {

	int8_t pos = addFunction(name, DEBUG_TYPE_INT);

	if (pos != -1) {
		_debugFunctions[pos].callback = (void(*) (void)) callback;
	}
	return pos;
}

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
// Add a function description for last added

void debugSetLastFunctionDescription(const char *description) {

	if (_debugFunctionsAdded > 0) {
		_debugFunctions[_debugFunctionsAdded - 1].description = description;
	}
}

void debugSetLastFunctionDescription(const __FlashStringHelper *description) {

	if (_debugFunctionsAdded > 0) {
		_debugFunctions[_debugFunctionsAdded - 1].descriptionF = description;
	}
}
#endif

// Add a global variable

// Basic types

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

int8_t debugAddGlobalBoolean (const char* name, boolean* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_BOOLEAN, 0);
}
int8_t debugAddGlobalChar (const char* name, char* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_CHAR, 0);
}
int8_t debugAddGlobalByte (const char* name, byte* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_BYTE, 0);
}
int8_t debugAddGlobalInt (const char* name, int* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_INT, 0);
}
int8_t debugAddGlobalUInt (const char* name, unsigned int* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_U_INT, 0);
}
int8_t debugAddGlobalLong (const char* name, long* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_LONG, 0);
}
int8_t debugAddGlobalULong (const char* name, unsigned long* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_U_LONG, 0);
}
int8_t debugAddGlobalFloat (const char* name, float* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_FLOAT, 0);
}
int8_t debugAddGlobalDouble (const char* name, double* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_DOUBLE, 0);
}

// Integer C size t

int8_t debugAddGlobalInt8_t (const char* name, int8_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT8_T, 0);
}
int8_t debugAddGlobalInt16_t (const char* name, int16_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT16_T, 0);
}
int8_t debugAddGlobalInt32_t (const char* name, int32_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT32_T, 0);
}
//#ifdef ESP32
//int8_t debugAddGlobalInt64_t (const char* name, int64_t* pointer) {
//
//	return debugAddGlobal(name, pointer, DEBUG_TYPE_UINT64_T);
//}
//#endif
int8_t debugAddGlobalUInt8_t (const char* name, uint8_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT8_T, 0);
}
int8_t debugAddGlobalUInt16_t (const char* name, uint16_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT16_T, 0);
}
int8_t debugAddGlobalUInt32_t (const char* name, uint32_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT32_T, 0);
}
//#ifdef ESP32
//int8_t debugAddGlobalUInt64_t (const char* name, uint64_t* pointer) {
//
//	debugAddGlobal(name, pointer, DEBUG_TYPE_UINT64_T);
//}
//#endif

// Strings

int8_t debugAddGlobalCharArray (const char* name, char* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_CHAR_ARRAY, 0);
}
int8_t debugAddGlobalCharArray (const char* name, char* pointer, uint8_t showLength) {

	return addGlobal(name, pointer, DEBUG_TYPE_CHAR_ARRAY, showLength);
}

int8_t debugAddGlobalString (const char* name, String* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_STRING, 0);
}
int8_t debugAddGlobalString (const char* name, String* pointer, uint8_t showLength) {

	return addGlobal(name, pointer, DEBUG_TYPE_STRING, showLength);
}
#endif// Not low memory board

// For Flash F()

// Basic types

int8_t debugAddGlobalBoolean (const __FlashStringHelper* name, boolean* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_BOOLEAN, 0);
}
int8_t debugAddGlobalChar (const __FlashStringHelper* name, char* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_CHAR, 0);
}
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
int8_t debugAddGlobalByte (const __FlashStringHelper* name, byte* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_BYTE, 0);
}
#endif
int8_t debugAddGlobalInt (const __FlashStringHelper* name, int* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_INT, 0);
}
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
int8_t debugAddGlobalUInt (const __FlashStringHelper* name, unsigned int* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_U_INT, 0);
}
int8_t debugAddGlobalLong (const __FlashStringHelper* name, long* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_LONG, 0);
}
#endif
int8_t debugAddGlobalULong (const __FlashStringHelper* name, unsigned long* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_U_LONG, 0);
}
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
int8_t debugAddGlobalFloat (const __FlashStringHelper* name, float* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_FLOAT, 0);
}
int8_t debugAddGlobalDouble (const __FlashStringHelper* name, double* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_DOUBLE, 0);
}

// Integer C size t

int8_t debugAddGlobalInt8_t (const __FlashStringHelper* name, int8_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT8_T, 0);
}
int8_t debugAddGlobalInt16_t (const __FlashStringHelper* name, int16_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT16_T, 0);
}
int8_t debugAddGlobalInt32_t (const __FlashStringHelper* name, int32_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT32_T, 0);
}
//#ifdef ESP32
//int8_t debugAddGlobalInt64_t (const char* name, int64_t* pointer) {
//
//	return debugAddGlobal(name, pointer, DEBUG_TYPE_UINT64_T);
//}
//#endif
int8_t debugAddGlobalUInt8_t (const __FlashStringHelper* name, uint8_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT8_T, 0);
}
int8_t debugAddGlobalUInt16_t (const __FlashStringHelper* name, uint16_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT16_T, 0);
}
int8_t debugAddGlobalUInt32_t (const __FlashStringHelper* name, uint32_t* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_UINT32_T, 0);
}
//#ifdef ESP32
//int8_t debugAddGlobalUInt64_t (const char* name, uint64_t* pointer) {
//
//	debugAddGlobal(name, pointer, DEBUG_TYPE_UINT64_T);
//}
//#endif

// Strings

int8_t debugAddGlobalCharArray (const __FlashStringHelper* name, char* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_CHAR_ARRAY, 0);
}
int8_t debugAddGlobalCharArray (const __FlashStringHelper* name, char* pointer, uint8_t showLength) {

	return addGlobal(name, pointer, DEBUG_TYPE_CHAR_ARRAY, showLength);
}
#endif // Not low memory board

int8_t debugAddGlobalString (const __FlashStringHelper* name, String* pointer) {

	return addGlobal(name, pointer, DEBUG_TYPE_STRING, 0);
}
int8_t debugAddGlobalString (const __FlashStringHelper* name, String* pointer, uint8_t showLength) {

	return addGlobal(name, pointer, DEBUG_TYPE_STRING, showLength);
}

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
// Add a description for global in last added

void debugSetLastGlobalDescription(const char *description) {

	_debugGlobals[_debugGlobalsAdded - 1].description = description;
}

void debugSetLastGlobalDescription(const __FlashStringHelper *description) {

	_debugGlobals[_debugGlobalsAdded - 1].descriptionF = description;
}
#endif // Not low memory board

//// Not allowed
//
//int8_t debugAddGlobalCharArray (const char* name, const char* pointer) {
//}
//int8_t debugAddGlobalCharArray (const char* name, const char* pointer, uint8_t showLength) {
//}

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

// Add a watch

int8_t debugAddWatchBoolean (const char* globalName, uint8_t operation, boolean value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchBoolean(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}

int8_t debugAddWatchBoolean (uint8_t globalNum, uint8_t operation, boolean value, boolean allwaysStop) {

	// Check Operation

	if (operation == DEBUG_WATCH_LESS_EQ || operation == DEBUG_WATCH_GREAT_EQ) {
		printSerialDebug();
		Serial.println(F("Operation not allowed for boolean"));
		return -1;
	}

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_BOOLEAN)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {

		// Alloc memory for pointerValue and copy value
		size_t size = sizeof(boolean);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchChar (const char* globalName, uint8_t operation, char value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchChar(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchChar (uint8_t globalNum, uint8_t operation, char value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_CHAR)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(char);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchByte (const char* globalName, uint8_t operation, byte value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchByte(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}

int8_t debugAddWatchByte (uint8_t globalNum, uint8_t operation, byte value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_BYTE)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(byte);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchInt (const char* globalName, uint8_t operation, int value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchInt(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchInt (uint8_t globalNum, uint8_t operation, int value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_INT)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(int);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchUInt (const char* globalName, uint8_t operation, unsigned int value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchUInt(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchUInt (uint8_t globalNum, uint8_t operation, unsigned int value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_U_INT)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(unsigned int);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchLong (const char* globalName, uint8_t operation, long value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchLong(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchLong (uint8_t globalNum, uint8_t operation, long value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_LONG)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(long);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchULong (const char* globalName, uint8_t operation, unsigned long value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchULong(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchULong (uint8_t globalNum, uint8_t operation, unsigned long value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_U_LONG)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(unsigned long);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchFloat (const char* globalName, uint8_t operation, float value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchFloat(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchFloat (uint8_t globalNum, uint8_t operation, float value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_FLOAT)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(float);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchDouble (const char* globalName, uint8_t operation, double value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchDouble(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchDouble (uint8_t globalNum, uint8_t operation, double value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_DOUBLE)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(double);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchInt8_t (const char* globalName, uint8_t operation, int8_t value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchInt8_t(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchInt8_t (uint8_t globalNum, uint8_t operation, int8_t value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_INT8_T)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(int8_t);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchInt16_t (const char* globalName, uint8_t operation, int16_t value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchInt16_t(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchInt16_t (uint8_t globalNum, uint8_t operation, int16_t value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_INT16_T)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(int16_t);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchInt32_t (const char* globalName, uint8_t operation, int32_t value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchInt32_t(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchInt32_t (uint8_t globalNum, uint8_t operation, int32_t value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_INT32_T)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(int32_t);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

//#ifdef ESP32
//int8_t debugAddWatchInt64_t (uint8_t globalNum, uint8_t operation, int64_t value);
//#endif

int8_t debugAddWatchUInt8_t (const char* globalName, uint8_t operation, uint8_t value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		//D("debugAddWatchUInt8_t: global=%u oper=%u value=%u", globalNum, operation, value);
		return debugAddWatchUInt8_t(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchUInt8_t (uint8_t globalNum, uint8_t operation, uint8_t value, boolean allwaysStop) {

	// Verify global type

    //D("debugAddWatch: globalNum=%u oper=%u value=%u", globalNum. operation, value);

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_UINT8_T)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	//D("debugAddWatchUInt8_t: ret =%u ", ret);

	if (ret != -1) {
		size_t size = sizeof(uint8_t);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);

		// Test
//		String value="";
//		String type="";
//		getStrValue(DEBUG_TYPE_UINT8_T, _debugWatches[ret].pointer, 0, value, type);
//		D("debugAddWatchUInt8_t: size=%u value=%s", size, value.c_str());

	}

	return ret;
}

int8_t debugAddWatchUInt16_t (const char* globalName, uint8_t operation, uint16_t value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchUInt16_t(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchUInt16_t (uint8_t globalNum, uint8_t operation, uint16_t value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_UINT16_T)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(uint16_t);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchUInt32_t (const char* globalName, uint8_t operation, uint32_t value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchUInt32_t(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchUInt32_t (uint8_t globalNum, uint8_t operation, uint32_t value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_UINT32_T)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = sizeof(uint32_t);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

//#ifdef ESP32
//int8_t debugAddWatchUInt64_t (uint8_t globalNum, uint8_t operation, uint64_t value);
//#endif

int8_t debugAddWatchCharArray (const char* globalName, uint8_t operation, const char* value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchCharArray(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}

int8_t debugAddWatchCharArray (uint8_t globalNum, uint8_t operation, const char* value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_CHAR_ARRAY)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		size_t size = strlen(value);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy( _debugWatches[ret].pointerValue, &value, size);
	}

	return ret;
}

int8_t debugAddWatchString (const char* globalName, uint8_t operation, String value, boolean allwaysStop) {

	uint8_t globalNum;

	if (findGlobal(globalName, &globalNum)) {
		return debugAddWatchString(globalNum, operation, value, allwaysStop);
	} else {
		return -1;
	}
}
int8_t debugAddWatchString (uint8_t globalNum, uint8_t operation, String value, boolean allwaysStop) {

	// Verify global type

	if (!verifyGlobalType(globalNum, DEBUG_TYPE_STRING)) {
		return -1;
	}

	// Add watch

	int8_t ret = addWatch(globalNum, operation, allwaysStop);

	if (ret != -1) {
		_debugWatches[ret].typeValue = DEBUG_TYPE_CHAR_ARRAY; // Store value as char array
		const char* content = value.c_str(); // by char array
		size_t size = strlen(content);
		_debugWatches[ret].pointerValue = malloc (size);
		memcpy(&content, _debugWatches[ret].pointerValue, size);
	}

	return ret;
}

// For watches cross - between 2 globals

int8_t debugAddWatchCross(const char* globalName, uint8_t operation, const char* anotherGlobalName, boolean allwaysStop) {

	uint8_t globalNum;
	uint8_t anotherGlobalNum;

	if (!findGlobal(globalName, &globalNum)) {
		return -1;
	}
	if (!findGlobal(anotherGlobalName, &anotherGlobalNum)) {
		return -1;
	}

	// Add

	return debugAddWatchCross(globalNum, operation, anotherGlobalNum, allwaysStop);
}
int8_t debugAddWatchCross(uint8_t globalNum, uint8_t operation, uint8_t anotherGlobalNum, boolean allwaysStop) {

	int8_t ret = -1;

	// Validate

	if (globalNum > _debugGlobalsAdded) {

		printSerialDebug();
		PRINTFLN(F("First global number must between 1 and %u"), _debugGlobalsAdded);
		return -1;
	}

	if (anotherGlobalNum > _debugGlobalsAdded) {

		printSerialDebug();
		PRINTFLN(F("Second global number must between 1 and %u"), _debugGlobalsAdded);
		return -1;
	}

	if (globalNum == anotherGlobalNum) {

		printSerialDebug();
		Serial.println(F("Globals numbers (first and second) can not be equals"));
		return -1;
	}

	if (operation == DEBUG_WATCH_CHANGED) {

		printSerialDebug();
		Serial.println(F("Changed type opretation is not allowed for cross watch"));
		return -1;
	}

	// Adjust the numbers

	globalNum--;
	anotherGlobalNum--;

	// Add this

	debugWatch_t watch;

	watch.globalNum = globalNum;
	watch.operation = operation;
	watch.watchCross = true;
	watch.globalNumCross = anotherGlobalNum;
	watch.alwaysStop = allwaysStop;
	watch.typeValue = DEBUG_TYPE_UNDEFINED;

	_debugWatches.push_back(watch);

	ret = _debugWatchesAdded;

	// Count it

	if (ret != -1) {

		_debugWatchesAdded++;
	}

	// Return index of this

	return ret;

}

// For Flash F

int8_t debugAddWatchBoolean (const __FlashStringHelper* globalName, uint8_t operation, boolean value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchBoolean(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchChar (const __FlashStringHelper* globalName, uint8_t operation, char value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchChar(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchByte (const __FlashStringHelper* globalName, uint8_t operation, byte value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchByte(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchInt (const __FlashStringHelper* globalName, uint8_t operation, int value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchInt(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchUInt (const __FlashStringHelper* globalName, uint8_t operation, unsigned int value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchUInt(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchLong (const __FlashStringHelper* globalName, uint8_t operation, long value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchLong(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchULong (const __FlashStringHelper* globalName, uint8_t operation, unsigned long value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchULong(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchFloat (const __FlashStringHelper* globalName, uint8_t operation, float value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchFloat(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchDouble (const __FlashStringHelper* globalName, uint8_t operation, double value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchDouble(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchInt8_t (const __FlashStringHelper* globalName, uint8_t operation, int8_t value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchInt8_t(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchInt16_t (const __FlashStringHelper* globalName, uint8_t operation, int16_t value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchInt16_t(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchInt32_t (const __FlashStringHelper* globalName, uint8_t operation, int32_t value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchInt32_t(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchUInt8_t (const __FlashStringHelper* globalName, uint8_t operation, uint8_t value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchUInt8_t(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchUInt16_t (const __FlashStringHelper* globalName, uint8_t operation, uint16_t value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchUInt16_t(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchUInt32_t (const __FlashStringHelper* globalName, uint8_t operation, uint32_t value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchUInt32_t(name.c_str(), operation, value, allwaysStop);
}

int8_t debugAddWatchCharArray (const __FlashStringHelper* globalName, uint8_t operation, const char* value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchCharArray(name.c_str(), operation, value, allwaysStop);
}
int8_t debugAddWatchString (const __FlashStringHelper* globalName, uint8_t operation, String value, boolean allwaysStop) {

	String name = String(globalName);
	return debugAddWatchString(name.c_str(), operation, value, allwaysStop);
}

int8_t debugAddWatchCross(const __FlashStringHelper* globalName, uint8_t operation, const __FlashStringHelper* anotherGlobalName, boolean allwaysStop) {

	String name = String(globalName);
	String anotherName = String(anotherGlobalName);
	return debugAddWatchCross(name.c_str(), operation, anotherName.c_str(), allwaysStop);
}

#endif // Not low memory board


#endif // DEBUG_DISABLE_DEBUGGER

/////// Private code (Note: this not starts with debug)

// Private function used for all types of functions

#ifndef DEBUG_DISABLE_DEBUGGER // Only if debugger is enabled

// Only for debugger enabled

// Add function

static int8_t addFunction(const char* name, uint8_t argType) {

	int8_t ret = -1;

	// Add this

	debugFunction_t function;

	function.name = name;
	function.argType = argType;

	_debugFunctions.push_back(function);

	ret = _debugFunctionsAdded;

	// Count it

	if (ret != -1) {

		_debugFunctionsAdded++;
	}

	// Return index of this

	return ret;
}

static int8_t addFunction(const __FlashStringHelper* name, uint8_t argType) {

	int8_t ret = -1;

	debugFunction_t function;

	// Add this

	function.nameF = name;
	function.argType = argType;

	_debugFunctions.push_back(function);

	ret = _debugFunctionsAdded;

	// Count it

	if (ret != -1) {

		_debugFunctionsAdded++;
	}

	// Return index of this

	return ret;
}

// Add Global variable

static int8_t addGlobal(const char* name, void* pointer, uint8_t type, uint8_t showLength) {

	int8_t ret = -1;

	debugGlobal_t global;

	// Add this

	global.name = name;
	global.pointer = pointer;
	global.type = type;
	global.showLength = showLength;

	if (type == DEBUG_TYPE_STRING) {
		global.typeOld = DEBUG_TYPE_CHAR_ARRAY; // Store old value as char array
	} else {
		global.typeOld = type; // Same type
	}

	_debugGlobals.push_back(global);

	ret = _debugGlobalsAdded;

	// Count it

	if (ret != -1) {

		_debugGlobalsAdded++;
	}

	// Return index of this

	return ret;
}

static int8_t addGlobal(const __FlashStringHelper* name, void* pointer, uint8_t type, uint8_t showLength) {

	int8_t ret = -1;

	debugGlobal_t global;

	// Add this

	global.nameF = name;
	global.pointer = pointer;
	global.type = type;
	global.showLength = showLength;

	if (type == DEBUG_TYPE_STRING) {
		global.typeOld = DEBUG_TYPE_CHAR_ARRAY; // Store old value as char array
	} else {
		global.typeOld = type; // Same type
	}

	_debugGlobals.push_back(global);

	ret = _debugGlobalsAdded;

	// Count it

	if (ret != -1) {

		_debugGlobalsAdded++;
	}

	// Return index of this

	return ret;
}

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

// For watches

static int8_t addWatch(uint8_t globalNum, uint8_t operation, boolean allwaysStop) {

	int8_t ret = -1;

	// Validate

	if (globalNum == 0 || globalNum > _debugGlobalsAdded) {

		printSerialDebug();
		PRINTFLN(F("Global number must between 1 and %u"), _debugGlobalsAdded);
		return -1;
	}

	// Adjust the number

	globalNum--;

	// Add this

	debugWatch_t watch;

	watch.globalNum = globalNum;
	watch.operation = operation;
	watch.alwaysStop = allwaysStop;
	watch.typeValue = _debugGlobals[globalNum].type;

	_debugWatches.push_back(watch);

	//D("debugAddWatch n=%u glob=%u oper=%u", _debugWatchesAdded, globalNum, operation);

	ret = _debugWatchesAdded;

	// Count it

	_debugWatchesAdded++;

	// Enabled

	_debugWatchesEnabled = true;

	// Return index of this

	return ret;

}

// Process watches commands

static void processWatches(String& options) {

#if DEBUG_USE_FLASH_F

	__FlashStringHelper* errorSintax = F("Invalid sintax for watches. use w ? to show help");

#else

	const char* errorSintax = "Invalid sintax for watches. use w ? to show help";

#endif

	// Get fields of command options

	Fields fields(options, ' ', true);

    D("options = %s fields.size() = %u ", options.c_str(), fields.size());

	String firstOption = (fields.size() >= 1)? fields.getString(1) : "";
	firstOption.toLowerCase();

	//D("first = %s", firstOption.c_str());

	if (fields.size() > 6) {

		printSerialDebug();
		Serial.println(errorSintax);

	} else if (_debugWatchesAdded > 0 || (firstOption == "a" || firstOption == "")) {

		if (firstOption.length() == 0 || firstOption == "?") {

			// Just show globals and help

			firstOption = "";
			showWatches(firstOption);

		} else if (fields.size() == 1 && fields.isNum(1)) {

			// Search by number

			uint8_t watchNum = fields.getInt(1);

			showWatch(watchNum);

		} else {

			// Process commands

			if (firstOption == "a") {

				// Add watch

				if (fields.size() == 3 || fields.size() == 4) {

					addWatch(fields);

				} else {

					printSerialDebug();
					Serial.println(errorSintax);
				}

			} else if (firstOption == "ac") {

				// Add cross watch

				if (fields.size() == 4) {

					addWatchCross(fields);

				} else {

					printSerialDebug();
					Serial.println(errorSintax);
				}

			} else if (firstOption == "u") {

				// Update watch

				if (fields.size() >= 4) {

					changeWatch(fields);

				} else {

					printSerialDebug();
					Serial.println(errorSintax);
				}
			} else if (firstOption == "d" || firstOption == "e" || firstOption == "r") {

				// Process watches action

				if (fields.size() == 2) {

					processWatchesAction(fields);

				} else {

					printSerialDebug();
					Serial.println(errorSintax);
				}

			} else if (firstOption == "ns") {

				// Set to nonstop for watches

				_debugWatchStop = false;

				printSerialDebug();
				Serial.println("Watches set to non stop");

				if (_debugSerialApp) { // App ?
					PRINTFLN(F("$app:W:s:%c"), ((_debugWatchStop)?'1':'0'));
				}

			} else if (firstOption == "s") {

				// Set to stop for watches

				_debugWatchStop = true;

				printSerialDebug();
				Serial.println("Watches set to stop");

				if (_debugSerialApp) { // ? App
					PRINTFLN(F("$app:W:s:%c"), ((_debugWatchStop)?'1':'0'));
				}

			} else {

				printSerialDebug();
				Serial.println(errorSintax);
			}
		}

	} else {

		printSerialDebug();
		Serial.println(F("Watches not added yet"));
	}

	// Clear the fields

	fields.clear();

}

// Process watches action commands

static void processWatchesAction(Fields& fields) {

	String firstOption = fields.getString(1);
	String secondOption = fields.getString(2);

	uint8_t watchNum = 0;
	boolean all = false;

	// Process second option

	if (secondOption == "a" || secondOption == "all") {

		all = true;

	} else if (fields.isNum(2)) {

		watchNum = secondOption.toInt();

		if (watchNum == 0 || watchNum > _debugWatchesAdded) {

			printSerialDebug();
			Serial.println(F("Invalid watch num\r\n*"));
			return;
		}

		watchNum--; // Index starts in 0

	} else {

		printSerialDebug();
		Serial.println(F("Watch num must be numeric\r\n*"));
		return;

	}

	// Remove ?

	if (firstOption == "r") {

		if (all) {

			// Remove all

			_debugWatches.clear();
			_debugWatchesAdded = 0;

			printSerialDebug();
			Serial.println(F("All watches has removed"));

		} else {

			// Remove item

#ifdef VECTOR_STD
			_debugWatches.erase(_debugWatches.begin() + watchNum);
#else
			_debugWatches.erase(watchNum);
#endif

			_debugWatchesAdded--;

			printSerialDebug();
			Serial.println(F("Watch has removed"));

		}

		return;
	}

	// Process watches

	_debugWatchesEnabled = false; // Have any enabled ?

	for (uint8_t i=0; i < _debugWatchesAdded; i++) {

		// Process action

		if (firstOption == "d") {

			// Disable watch

			if (all || i == watchNum) {

				if (_debugWatches[i].enabled) {

					_debugWatches[i].enabled = false;

					printSerialDebug();
					PRINTFLN(F("Watch %u has disabled"), (i + 1));
				}
			}

		} else if (firstOption == "e") {

			// Enable watch

			if (all || i == watchNum) {

				if (!_debugWatches[i].enabled) {

					_debugWatches[i].enabled = true;

					printSerialDebug();
					PRINTFLN(F("Watch %u has enabled"), (i + 1));

				}
			}
		}

		// Have any enabled ?

		if (_debugWatches[i].enabled) {
			_debugWatchesEnabled = true;
		}
	}
}

// Add a watch

static int8_t addWatch(Fields& fields) {

	int8_t ret = -1; // Return
	uint8_t pos = 2; // Field possition

	// Global number

	uint8_t globalNum;

	if (fields.isNum(pos)) { // By num

		globalNum = fields.getInt(pos);

		if (globalNum == 0 || globalNum > _debugGlobalsAdded) {

			printSerialDebug();
			Serial.println(F("Invalid index for global in watch\r\n*"));
			return false;
		}

		globalNum--; // Globals index start in 0

	} else { // By name

		 if (!(findGlobal(fields.getString(pos).c_str(), &globalNum, false))) {
			 return false;
		 }
	}

	//D("globalnum=%u", globalNum);

	// Verify operation

	pos++;

	uint8_t operation = 0;

	if (!(getWatchOperation(fields.getString(pos), &operation))) {

		printSerialDebug();
		Serial.println(F("Invalid operation\r\n*"));
		return false;

	}

	// Verify value

	String value = "";

	if (operation != DEBUG_WATCH_CHANGED) { // Not for changed type

		pos++;

		switch (_debugGlobals[globalNum].type) {
			case DEBUG_TYPE_INT:
			case DEBUG_TYPE_U_INT:
			case DEBUG_TYPE_LONG:
			case DEBUG_TYPE_U_LONG:
			case DEBUG_TYPE_FLOAT:
			case DEBUG_TYPE_DOUBLE:
			case DEBUG_TYPE_INT8_T:
			case DEBUG_TYPE_INT16_T:
			case DEBUG_TYPE_INT32_T:
			case DEBUG_TYPE_UINT8_T:
			case DEBUG_TYPE_UINT16_T:
			case DEBUG_TYPE_UINT32_T:

				// Is number ?

				if (!(fields.isNum(pos))) {

					printSerialDebug();
					Serial.println(F("The value must be numeric\r\n*"));
					return false;

				}
				break;
		}

		// Get value

		value = fields.getString(pos);

		// Return the spaces

		char conv = 31;
		value.replace(conv, ' ');

	}

	// Verify allways stop

	pos++;

	boolean allwaysStop = false;

	if (fields.size() == pos) {

		allwaysStop = (fields.getString(pos) == "as");

	}

	// Add watch (these funcions work with index of global start by 1)

	if (operation == DEBUG_WATCH_CHANGED) { // Changed type

		ret = addWatch((globalNum + 1), DEBUG_WATCH_CHANGED, allwaysStop);

	} else {

		// From type of global

		switch (_debugGlobals[globalNum].type) {
			case DEBUG_TYPE_BOOLEAN:
				{
					boolean conv = (value == "1" || value == "t" || value == "true");
					ret = debugAddWatchBoolean((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_INT:
				{
					int conv = value.toInt();
					ret = debugAddWatchInt((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_U_INT:
				{
					unsigned int conv = value.toInt();
					ret = debugAddWatchUInt((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_LONG:
				{
					long conv = value.toInt(); // TODO see if works with large values ?????
					ret = debugAddWatchLong((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_U_LONG:
				{
					unsigned long conv = value.toInt(); // TODO see if works with large values ?????
					ret = debugAddWatchULong((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_FLOAT:
				{
					float conv = value.toFloat();
					ret = debugAddWatchFloat((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_DOUBLE:
				{
					double conv = value.toFloat(); // TODO see if works with large values ?????
					ret = debugAddWatchULong((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_INT8_T:
				{
					int8_t conv = value.toInt();
					ret = debugAddWatchInt8_t((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_INT16_T:
				{
					int16_t conv = value.toInt();
					ret = debugAddWatchInt16_t((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_INT32_T:
				{
					int32_t conv = value.toInt();
					ret = debugAddWatchInt32_t((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_UINT8_T:
				{
					uint8_t conv = value.toInt();
					ret = debugAddWatchUInt8_t((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_UINT16_T:
				{
					uint16_t conv = value.toInt();
					ret = debugAddWatchUInt16_t((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_UINT32_T:
				{
					uint32_t conv = value.toInt(); // TODO see if works with large values ?????
					ret = debugAddWatchUInt32_t((globalNum + 1), operation, conv, allwaysStop);
				}
				break;
			case DEBUG_TYPE_CHAR_ARRAY:
				{
					ret = debugAddWatchCharArray(globalNum, operation, value.c_str(), allwaysStop);
				}
				break;
			case DEBUG_TYPE_STRING:
				{
					ret = debugAddWatchString(globalNum, operation, value, allwaysStop);
				}
				break;
		}
	}

	// Return

	if (ret != -1) {
		printSerialDebug();
		Serial.println(F("Watch added with sucess:"));
		showWatch(ret);
		Serial.println('*');
	}

	return ret;
}

// Add a cross watch

static int8_t addWatchCross(Fields& fields) {

	int8_t ret = -1; // Return
	uint8_t pos = 2; // First field

	// Global number 1

	uint8_t globalNum1;

	if (fields.isNum(pos)) { // By num

		globalNum1 = fields.getInt(pos);

		if (globalNum1 == 0 || globalNum1 > _debugGlobalsAdded) {

			printSerialDebug();
			Serial.println(F("Invalid index for global1\r\n*"));
			return false;
		}

		globalNum1--; // Globals index start in 0

	} else { // By name

		 if (!(findGlobal(fields.getString(pos).c_str(), &globalNum1, false))) {
			 return false;
		 }
	}

	//D("globalnum1=%u", globalNum1);

	// Verify operation

	pos++;

	uint8_t operation = 0;

	if (!(getWatchOperation(fields.getString(pos), &operation))) {

		printSerialDebug();
		Serial.println(F("Invalid operation\r\n*"));
		return false;

	}

	if (operation == DEBUG_WATCH_CHANGED) { // Not for changed type

		printSerialDebug();
		Serial.println(F("Invalid operation - not can be of change type\r\n*"));
		return false;

	}

	// Global number 2

	pos++;

	uint8_t globalNum2;

	if (fields.isNum(pos)) { // By num

		globalNum2 = fields.getInt(pos);

		if (globalNum2 == 0 || globalNum2 > _debugGlobalsAdded) {

			printSerialDebug();
			Serial.println(F("Invalid index for global2\r\n*"));
			return false;
		}

		globalNum2--; // Globals index start in 0

	} else { // By name

		 if (!(findGlobal(fields.getString(pos).c_str(), &globalNum2, false))) {
			 return -1;
		 }
	}

	//D("globalnum2=%u", globalNum1);

	if (globalNum2 == globalNum1) {

		printSerialDebug();
		Serial.println(F("Global2 must be different than global1\r\n*"));
		return false;
	}

	// Verify allways stop

	pos++;

	boolean allwaysStop = false;

	if (fields.size() == pos) {

		allwaysStop = (fields.getString(pos) == "as");

	}

	// Add watch cross (these funcions work with index of global start by 1)

	ret = debugAddWatchCross((globalNum1 + 1), operation, (globalNum1 + 2), allwaysStop);

	// Return

	if (ret != -1) {
		printSerialDebug();
		Serial.println(F("Watch added with sucess:"));
		showWatch(ret);
	}

	return ret;
}

// Show list of watches for globals available
// Used to search and show all (return total showed)
// Or to search and show one (return number showed)

static int8_t showWatches(String& options, boolean debugSerialApp) {

	// Show all ?

	boolean showAll = false;
	int8_t byNumber = -1;

	// Searching ?

	if (options.length() == 0) {

		showAll = true;

	} else {

		// Is integer ?

		if (strIsNum(options)) {

			uint8_t num = options.toInt();

			//D("byNumber %s = %d", options.c_str(), num);

			if (num > 0) { // Find by number, else by exact name

				byNumber = num;
				showAll = false;

			} else {

				printSerialDebug();
				Serial.println(F("Option not is a number valid (>0)"));
				return -1;

			}

		} else {

			printSerialDebug();
			Serial.println(F("Option not is a number"));
			return -1;
		}

	}

	// Show global(s)

	if (!debugSerialApp) { // Not for SerialDebugApp connection

		printSerialDebug();
		if (showAll) {
			PRINTFLN(F("Showing all watches for global variables (%u):"), _debugWatchesAdded);
		} else {
			Serial.println(F("Searching and showing watch for global variable:"));
		}
	}

	int8_t showed = 0;

	// Process

	for (uint8_t i=0; i < _debugWatchesAdded; i++) {

		String name = "";
		boolean show = showAll;

		// Show ?

		if (byNumber != -1) {
			show = ((i + 1) == byNumber);
		}

		if (show) {

			// Get global num

			uint8_t globalNum = _debugWatches[i].globalNum;

			if (globalNum > _debugGlobalsAdded) {

				printSerialDebug();
				Serial.println(F("Invalid index for global variable in watch\r\n*"));
				return -1;
			}

			// Show

			if (showWatch(i, debugSerialApp)) {

				showed++;

			}
		}
	}

	// Help

	if (!debugSerialApp) { // Not for SerialDebugApp connection

		boolean doBreak = false;

		if (showed > 0) {

			Serial.println('*');
			printSerialDebug();
			Serial.println();
			Serial.println(F("* To show: wa [num]"));
			Serial.println(F("* To add: wa a {global [name|number]} [==|!=|<|>|<=|>=|change] [value] [as]"));
			Serial.println(F("* note: as -> set watch to stop always"));
			Serial.println(F("* To add cross (2 globals): wa ac {global [name|number]} [=|!=|<|>|<=|>=] {global [name|number]} [as]"));
			Serial.println(F("* To change: wa u {global [name|number]} [==|!=|<|>|<=|>=|change] [value] [as]"));
			Serial.println(F("* To change cross (not yet implemented)"));
			Serial.println(F("* To disable: wa d [num|all]"));
			Serial.println(F("* To enable: wa e [num|all]"));
			Serial.println(F("* To nonstop on watches: wa ns"));
			Serial.println(F("* To stop on watches: wa s"));

			doBreak = true;

		} else {

			printSerialDebug();
			Serial.println(F("Watch not found."));

			doBreak = true;
		}

		// Do break ?

		if (doBreak) {
			String response = debugBreak();
			if (response.length() > 0) {
				processCommand(response, false, false);
			}
		}
	}

	// Return

	return showed;

}

// Show watch

static boolean showWatch(uint8_t watchNum, boolean debugSerialApp) {

	if (watchNum >= _debugWatchesAdded) {

		printSerialDebug();
		Serial.println(F("Invalid index for watch \r\n*"));
		return false;
	}

	debugWatch_t* watch = &_debugWatches[watchNum];

	// Show

	if (debugSerialApp) { // For DebugSerialApp connection ?

		// Operation

		String oper = "";

		switch (watch->operation) {
			case DEBUG_WATCH_CHANGED:
				oper = "chg";
				break;
			case DEBUG_WATCH_EQUAL:
				oper = "==";
				break;
			case DEBUG_WATCH_DIFF:
				oper = "!=";
				break;
			case DEBUG_WATCH_LESS:
				oper = "<";
				break;
			case DEBUG_WATCH_GREAT:
				oper = ">";
				break;
			case DEBUG_WATCH_LESS_EQ:
				oper = "<=";
				break;
			case DEBUG_WATCH_GREAT_EQ:
				oper = ">=";
				break;
		}

		// Is cross (between 2 globals)?

		if (watch->watchCross) {

			PRINTFLN(F("$app:W:a:c:%u:%u:%s:%u:%c:%c:%c"),
					(watchNum + 1),
					(watch->globalNum + 1),
					oper.c_str(),
					(watch->globalNumCross + 1),
					(watch->enabled)?'1':'0',
					(watch->alwaysStop)?'1':'0',
					(watch->triggered)?'1':'0');

		} else { // Normal

			// Global

			debugGlobal_t* global = &_debugGlobals[watch->globalNum];

			// Value

			String value = "";
			String type = "";

			if (watch->operation != DEBUG_WATCH_CHANGED) {

				getStrValue(global->type, watch->pointerValue, global->showLength, true, value, type);
			}

			PRINTFLN(F("$app:W:a:n:%u:%u:%s:%s:%c:%c:%c"),
					(watchNum + 1),
					(watch->globalNum + 1),
					oper.c_str(),
					value.c_str(),
					(watch->enabled)?'1':'0',
					(watch->alwaysStop)?'1':'0',
					(watch->triggered)?'1':'0');

		}

	} else { // For monitor serial

		PRINTF(F("* %02u {global "), (watchNum + 1));

		if (showGlobal((watch->globalNum + 1), DEBUG_SHOW_GLOBAL_WATCH, false)) {

			// Operation

			String oper = "";

			switch (watch->operation) {
				case DEBUG_WATCH_CHANGED:
					oper = "change";
					break;
				case DEBUG_WATCH_EQUAL:
					oper = "==";
					break;
				case DEBUG_WATCH_DIFF:
					oper = "!=";
					break;
				case DEBUG_WATCH_LESS:
					oper = "<";
					break;
				case DEBUG_WATCH_GREAT:
					oper = ">";
					break;
				case DEBUG_WATCH_LESS_EQ:
					oper = "<=";
					break;
				case DEBUG_WATCH_GREAT_EQ:
					oper = ">=";
					break;
			}

			// Is cross (between 2 globals)?

			if (watch->watchCross) {

				PRINTF(F("} %s {global "), oper.c_str());

				if (showGlobal((watch->globalNumCross + 1),  DEBUG_SHOW_GLOBAL_WATCH, false)) {

					PRINTFLN(F("} (%s) %s"), \
							((watch->enabled)?"enabled":"disabled"), \
							((watch->alwaysStop)?"(allwaysStop)":""));

				}

			} else { // Simple watch

				// Global

				if (watch->globalNum > _debugGlobalsAdded) {

					printSerialDebug();
					Serial.println(F("Invalid index for global in watch\r\n*"));
					return false;
				}

				debugGlobal_t* global = &_debugGlobals[watch->globalNum];

				// Value

				String value = "";
				String type = "";

				if (watch->operation != DEBUG_WATCH_CHANGED) {

					getStrValue(global->type, watch->pointerValue, global->showLength, true, value, type);

					PRINTFLN(F("} %s %s (%s) %s"), oper.c_str(), value.c_str(), \
							((watch->enabled)?"enabled":"disabled"), \
							((watch->alwaysStop)?"(allwaysStop)":""));

				} else { // When changed

					String oldValue = "";
					String oldType = "";

					getStrValue(global->type, global->pointerOld, global->showLength, true, oldValue, oldType);

					PRINTFLN(F("} %s (old value: %s) (%s) %s"), oper.c_str(), oldValue.c_str(), \
							((watch->enabled)?"enabled":"disabled"), \
							((watch->alwaysStop)?"(allwaysStop)":""));
				}
			}

			return true;
		}
	}

	return false;

}

// Change watches of global variables

static boolean changeWatch(Fields& fields) {

	uint8_t pos = 2; // Field possition

	// Watch number

	uint8_t watchNum;

	if (fields.isNum(pos)) { // By num

		watchNum = fields.getInt(pos);

		if (watchNum == 0 || watchNum > _debugWatchesAdded) {

			printSerialDebug();
			Serial.println(F("Invalid index for watch\r\n*"));
			return false;
		}

		watchNum--; // Watch index start in 0

	} else {

		printSerialDebug();
		Serial.println(F("Invalid watch number\r\n*"));

		return false;
	}

	// Global number

	pos++;

	uint8_t globalNum;

	if (fields.isNum(pos)) { // By num

		globalNum = fields.getInt(pos);

		if (globalNum == 0 || globalNum > _debugGlobalsAdded) {

			printSerialDebug();
			Serial.println(F("Invalid index for global in watch\r\n*"));
			return false;
		}

		globalNum--; // Globals index start in 0

	} else { // By name

		 if (!(findGlobal(fields.getString(pos).c_str(), &globalNum, false))) {
			 return false;
		 }
	}

	//D("globalnum=%u", globalNum);

	// Verify operation

	pos++;

	uint8_t operation = 0;

	if (!(getWatchOperation(fields.getString(pos), &operation))) {

		printSerialDebug();
		Serial.println(F("Invalid operation\r\n*"));
		return false;

	}

	// Verify value

	String value = "";

	if (operation != DEBUG_WATCH_CHANGED) { // Not for changed type

		pos++;

		switch (_debugGlobals[globalNum].type) {
			case DEBUG_TYPE_INT:
			case DEBUG_TYPE_U_INT:
			case DEBUG_TYPE_LONG:
			case DEBUG_TYPE_U_LONG:
			case DEBUG_TYPE_FLOAT:
			case DEBUG_TYPE_DOUBLE:
			case DEBUG_TYPE_INT8_T:
			case DEBUG_TYPE_INT16_T:
			case DEBUG_TYPE_INT32_T:
			case DEBUG_TYPE_UINT8_T:
			case DEBUG_TYPE_UINT16_T:
			case DEBUG_TYPE_UINT32_T:

				// Is number ?

				if (!(fields.isNum(pos))) {

					printSerialDebug();
					Serial.println(F("The value must be numeric\r\n*"));
					return false;

				}
				break;
		}

		// Get value

		value = fields.getString(pos);

		// Return the spaces

		char conv = 31;
		value.replace(conv, ' ');

	}

	// Verify allways stop

	pos++;

	boolean allwaysStop = false;

	if (fields.size() == pos) {

		allwaysStop = (fields.getString(pos) == "as");

	}

	// Get watch and this global

	debugWatch_t *watch = &_debugWatches[watchNum];
	debugGlobal_t *global = &_debugGlobals[globalNum];

	// Update watch

	watch->globalNum = globalNum;
	watch->operation = operation;
	watch->alwaysStop = allwaysStop;
	watch->typeValue = _debugGlobals[globalNum].type;

	if (operation == DEBUG_WATCH_CHANGED) { // Changed type

		if (watch->pointerValue) { // Free it
			free (watch->pointerValue);
		}

	} else { // Anothers

		// From type of global

		switch (_debugGlobals[globalNum].type) {
			case DEBUG_TYPE_BOOLEAN:
				{
					boolean conv = (value == "1" || value == "t" || value == "true");
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_INT:
				{
					int conv = value.toInt();
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_U_INT:
				{
					unsigned int conv = value.toInt();
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_LONG:
				{
					long conv = value.toInt(); // TODO see if works with large values ?????
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_U_LONG:
				{
					unsigned long conv = value.toInt(); // TODO see if works with large values ?????
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_FLOAT:
				{
					float conv = value.toFloat();
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_DOUBLE:
				{
					double conv = value.toFloat(); // TODO see if works with large values ?????
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_INT8_T:
				{
					int8_t conv = value.toInt();
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_INT16_T:
				{
					int16_t conv = value.toInt();
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_INT32_T:
				{
					int32_t conv = value.toInt();
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_UINT8_T:
				{
					uint8_t conv = value.toInt();
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_UINT16_T:
				{
					uint16_t conv = value.toInt();
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_UINT32_T:
				{
					uint32_t conv = value.toInt(); // TODO see if works with large values ?????
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));
				}
				break;
			case DEBUG_TYPE_CHAR_ARRAY:
				{
					const char* conv = value.c_str();
					if (watch->pointerValue) { // Free it first
						free (watch->pointerValue);
					}
					updateValue(global->type, &conv, global->type, &(watch->pointerValue));

				}
				break;
			case DEBUG_TYPE_STRING:
				{
					const char* conv = value.c_str();
					if (watch->pointerValue) { // Free it first
						free (watch->pointerValue);
					}
					updateValue(global->type, &conv, DEBUG_TYPE_CHAR_ARRAY, &(watch->pointerValue));
				}
				break;
		}
	}

	// Return

	printSerialDebug();
	Serial.println(F("Watch updated with sucess:"));
	showWatch(watchNum);
	Serial.println('*');

	return true;
}

// Verify global type

static boolean verifyGlobalType(uint8_t globalNum, uint8_t type) {

	if (globalNum == 0 || globalNum > _debugGlobalsAdded) {

		printSerialDebug();
		Serial.println(F("Invalid index for global in watch\r\n*"));
		return false;
	}

	boolean ret = (_debugGlobals[globalNum - 1].type == type);

	if (!ret) {

		printSerialDebug();
		Serial.println(F("Invalid type for global in watch\r\n*"));
	}

	return ret;
}

// Get operation type for watch from string

boolean getWatchOperation(String str, uint8_t* operation) {

	int8_t oper = -1;

	if (str.length() == 0) {
		oper = DEBUG_WATCH_CHANGED;
	} else if (str == "==") {
		oper = DEBUG_WATCH_EQUAL;
	} else if (str == "!=") {
		oper = DEBUG_WATCH_DIFF;
	} else if (str == "<") {
		oper = DEBUG_WATCH_LESS;
	} else if (str == ">") {
		oper = DEBUG_WATCH_GREAT;
	} else if (str == "<=") {
		oper = DEBUG_WATCH_LESS_EQ;
	} else if (str == ">=") {
		oper = DEBUG_WATCH_GREAT_EQ;
	}

	if (oper != -1) {
		*operation = oper;
		return true;
	} else {
		return false;
	}
}

#endif // Not low memory board

// Aplly the operation between two *void pointers values

static boolean apllyOperation(uint8_t type1, void* pointer1, uint8_t operation, uint8_t type2, void* pointer2) {

	// Process types and values)

	//D("apllyOperation(type1=%u pointer1=%p, operation=%u, type2=%u, pointer2=%p",
	//		type1,pointer1, operation, type2, pointer2);

	if (pointer1 && pointer2) { // Only if 2 pointer has data

		switch (type1) {

			case DEBUG_TYPE_BOOLEAN:
				{
					boolean value1 = *(boolean*) pointer1;
					boolean value2 = *(boolean*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_CHAR:
				{
					char value1 = *(char*) pointer1;
					char value2 = *(char*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_INT:
				{
					int value1 = *(int*) pointer1;
					int value2 = *(int*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_U_LONG:
				{
					unsigned long value1 = *(unsigned long*) pointer1;
					unsigned long value2 = *(unsigned long*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

			case DEBUG_TYPE_BYTE:
				{
					byte value1 = *(byte*) pointer1;
					byte value2 = *(byte*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_U_INT:
				{
					unsigned int value1 = *(unsigned int*) pointer1;
					unsigned int value2 = *(unsigned int*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_LONG:
				{
					long value1 = *(long*) pointer1;
					long value2 = *(long*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_FLOAT:
				{
					float value1 = *(float*) pointer1;
					float value2 = *(float*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_DOUBLE:
				{
					double value1 = *(double*) pointer1;
					double value2 = *(double*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_INT8_T:
				{
					int8_t value1 = *(int8_t*) pointer1;
					int8_t value2 = *(int8_t*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_INT16_T:
				{
					int16_t value1 = *(int16_t*) pointer1;
					int16_t value2 = *(int16_t*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;
			case DEBUG_TYPE_INT32_T:
				{
					int32_t value1 = *(int32_t*) pointer1;
					int32_t value2 = *(int32_t*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_UINT8_T:
				{
					uint8_t value1 = *(uint8_t*) pointer1;
					uint8_t value2 = *(uint8_t*) pointer2;

					//D("aplly v1=%u v2=%u", value1, value2);

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_UINT16_T:
				{
					uint16_t value1 = *(uint16_t*) pointer1;
					uint16_t value2 = *(uint16_t*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_UINT32_T:
				{
					uint32_t value1 = *(uint32_t*) pointer1;
					uint32_t value2 = *(uint32_t*) pointer2;

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (value1 != value2);
							break;

						case DEBUG_WATCH_EQUAL:
							return (value1 == value2);
							break;

						case DEBUG_WATCH_LESS:
							return (value1 < value2);
							break;

						case DEBUG_WATCH_GREAT:
							return (value1 > value2);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (value1 <= value2);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (value1 >= value2);
							break;

						default:
							return false;
							break;
					}
				}
				break;

			case DEBUG_TYPE_CHAR_ARRAY:
				{
					const char* value1 = (const char*) pointer1;
					const char* value2 = (const char*) pointer2;

					int ret = strncmp(value1, value2, DEBUG_MAX_CMP_STRING); // Compare max n characters to avoid problems - TODO see another ways

					//D("aplly v1=%s v2=%s ret=%u", value1, value2, ret);

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (ret != 0);
							break;

						case DEBUG_WATCH_EQUAL:
							return (ret == 0);
							break;

						case DEBUG_WATCH_LESS:
							return (ret < 0);
							break;

						case DEBUG_WATCH_GREAT:
							return (ret > 0);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (ret <= 0);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (ret >= 0);
							break;

						default:
							return false;
							break;
					}
				}
				break;


#endif // LOW_MEMORY

			case DEBUG_TYPE_STRING:
				{
					// Allways compare char array, due pointer2 can be of this type

					const char* value1;
					const char* value2;

					String temp = *(String*) pointer1;
					value1 = temp.c_str();

					switch (type2) {
						case DEBUG_TYPE_STRING:
							temp = *(String*) pointer2;
							value2 = temp.c_str();
							break;
						case DEBUG_TYPE_CHAR_ARRAY: // Old values of global string is a char array
							value2 = (const char*) pointer2;
							break;
						default:
							return false;
							break;
					}

					// Compare

					int ret = strncmp(value1, value2, DEBUG_MAX_CMP_STRING); // Compare max n characters to avoid problems - TODO see another ways

					//D("apply v1=%s v2=%s p2=%p ret=%d", value1, value2, pointer2, ret);

					// Aplly operation

					switch (operation) {

						case DEBUG_WATCH_CHANGED:
						case DEBUG_WATCH_DIFF:
							return (ret != 0);
							break;

						case DEBUG_WATCH_EQUAL:
							return (ret == 0);
							break;

						case DEBUG_WATCH_LESS:
							return (ret < 0);
							break;

						case DEBUG_WATCH_GREAT:
							return (ret > 0);
							break;

						case DEBUG_WATCH_LESS_EQ:
							return (ret <= 0);
							break;

						case DEBUG_WATCH_GREAT_EQ:
							return (ret >= 0);
							break;

						default:
							return false;
							break;
					}

				}
				break;
		}

		return false;

	} else {

		if (!pointer2) { // No value -> it is changed
			return true;
		} else {
			return false;
		}
	}
}

// Find a global variable added by name

static boolean findGlobal (const char* globalName, uint8_t* globalNum, boolean sumOne) {

	String find = globalName;
	String name = "";

	for (uint8_t i=0; i < _debugGlobalsAdded; i++) {

		// Get name

		if (_debugGlobals[i].name) { // Memory

			name = _debugGlobals[i].name;

		} else if (_debugGlobals[i].nameF) { // For Flash F

			name = String(_debugGlobals[i].nameF);
		}

		if (name == find) {
			*globalNum = (sumOne)? (i + 1) : i;
			return true;
		}
	}

	// Not find

	printSerialDebug();
	PRINTFLN(F("Global mame not found: %s"), globalName);

	return false;
}

// Get a string value from a void pointerValue

static void getStrValue(uint8_t type, void* pointer, uint8_t showLength, boolean showSize, String& response, String& responseType) {

	response = "";

	if (responseType) responseType = "";

	if (!pointer) { // Not has value
		response = "?";
		return;
	}

	switch (type) {

		// Basic types

		case DEBUG_TYPE_BOOLEAN:
			response = ((*(boolean*)pointer) ? F("true") : F("false"));
			if (responseType) responseType = F("boolean");
			break;
		case DEBUG_TYPE_CHAR:
			response = '\'';
			response.concat(String(*(char*)pointer));
			response.concat('\'');
			if (responseType) responseType = F("char");
			break;
		case DEBUG_TYPE_INT:
			response = String(*(int*)pointer);
			if (responseType) responseType = F("int");
			break;
		case DEBUG_TYPE_U_LONG:
			response = String(*(unsigned long*)pointer);
			if (responseType) responseType = F("unsigned long");
			break;
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		case DEBUG_TYPE_BYTE:
			response = String(*(byte*)pointer);
			if (responseType) responseType = F("byte");
			break;
		case DEBUG_TYPE_U_INT:
			response = String(*(unsigned int*)pointer);
			if (responseType) responseType = F("unsigned int");
			break;
		case DEBUG_TYPE_LONG:
			response = String(*(long*)pointer);
			if (responseType) responseType = F("long");
			break;
		case DEBUG_TYPE_FLOAT:
			response = String(*(float*)pointer);
			if (responseType) responseType = F("float");
			break;
		case DEBUG_TYPE_DOUBLE:
			response = String(*(double*)pointer);
			if (responseType) responseType = F("double");
			break;

		// Integer C size _t

		case DEBUG_TYPE_INT8_T:
			response = String(*(int8_t*)pointer);
			if (responseType) responseType = F("int8_t");
			break;
		case DEBUG_TYPE_INT16_T:
			response = String(*(int16_t*)pointer);
			if (responseType) responseType = F("int16_t");
			break;
		case DEBUG_TYPE_INT32_T:
			response = String(*(int32_t*)pointer);
			if (responseType) responseType = F("int32_t");
			break;
//#ifdef ESP32
//		case DEBUG_TYPE_INT64_T:
//			response = String(*(int64_t*)pointer);
//			if (responseType) responseType = F("int64_t");
//			break;
//#endif
		// Unsigned integer C size _t

		case DEBUG_TYPE_UINT8_T:
			response = String(*(uint8_t*)pointer);
			if (responseType) responseType = F("uint8_t");
			break;
		case DEBUG_TYPE_UINT16_T:
			response = String(*(uint16_t*)pointer);
			if (responseType) responseType = F("uint16_t");
			break;
		case DEBUG_TYPE_UINT32_T:
			response = String(*(uint32_t*)pointer);
			if (responseType) responseType = F("uint32_t");
			break;
//#ifdef ESP32
//			case DEBUG_TYPE_UINT64_T:
//					response = String(*(uint64_t*)pointer);
//					if (responseType) responseType = "uint64_t";
//					break;
//#endif
		// Strings

		case DEBUG_TYPE_CHAR_ARRAY:
			{
				String show = String((char*)pointer);
				size_t size = show.length();

				if (showLength > 0 &&
					size > showLength) {
					show = show.substring(0, showLength);
					show.concat(F("..."));
				}
				response = '\"';
				response.concat(show);
				response.concat('\"');
				if (showSize) {
					response.concat(F(" (size:"));
					response.concat(size);
					response.concat(")");
				}
				if (responseType) responseType = F("char array");
			}
			break;
#endif// Not low memory board

		case DEBUG_TYPE_STRING:
			{
				String show = *(String*)pointer;
				size_t size = show.length();
				if (showLength > 0 &&
						size > showLength) {
					show = show.substring(0, showLength);
					show.concat(F("..."));
				}
				response = '\"';
				response.concat(show);
				response.concat('\"');
				if (showSize) {
					response.concat(F(" (size:"));
					response.concat(size);
					response.concat(")");
				}
				if (responseType) responseType = F("String");
			}
			break;
	}
}

// Update to value in pointer from another pointer

static void updateValue(uint8_t typeFrom, void* pointerFrom, uint8_t typeTo, void** pointerTo) {

	D("updateValue from type=%u pointer=%p to type=%u pointer=%p",
			typeFrom, pointerFrom, typeTo, *pointerTo);

	if (!pointerFrom) {
		printSerialDebug();
		Serial.println(F("source value empty"));
		return;
	}

	// Update

	switch (typeTo) {

		// Basic types

		case DEBUG_TYPE_BOOLEAN:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(boolean);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_CHAR:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(char);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_INT:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(int);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_U_LONG:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(unsigned long);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

		case DEBUG_TYPE_BYTE:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(byte);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_U_INT:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(unsigned int);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_LONG:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(long);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_FLOAT:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(float);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_DOUBLE:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(double);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		// Integer C size _t

		case DEBUG_TYPE_INT8_T:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(int8_t);
				//D("size=%u", size);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_INT16_T:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(int16_t);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

		case DEBUG_TYPE_INT32_T:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(int32_t);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;

//#ifdef ESP32
//		case DEBUG_TYPE_INT64_T:
//			break;
//#endif

		// Unsigned integer C size _t

		case DEBUG_TYPE_UINT8_T:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(uint8_t);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;
		case DEBUG_TYPE_UINT16_T:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(uint16_t);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;
		case DEBUG_TYPE_UINT32_T:
			{
				// Alloc memory for pointer (if need) and copy value
				size_t size = sizeof(uint32_t);
				if (!*pointerTo) {
					*pointerTo = malloc (size);
				}
				memcpy(*pointerTo, pointerFrom, size);
			}
			break;
//#ifdef ESP32
//		case DEBUG_TYPE_UINT64_T:
//			break;
//#endif

#endif // Not low memory board

		// Strings

		case DEBUG_TYPE_CHAR_ARRAY:
			{
				// Always free it before, due size can be changed // TODO optimize it
				if (*pointerTo) {
					free (*pointerTo);
				}
				const char* content;
				size_t size;
				// Value is string ?
				if (typeFrom == DEBUG_TYPE_STRING) {
					String temp = *(String*)pointerFrom;
					size = temp.length();
					//D("upd str temp %s size %d", temp.c_str(), size);
					// Alloc memory for pointerValue and copy value
					*pointerTo = malloc (size+1);
					memset(*pointerTo, '\0', (size+1));
					memcpy(*pointerTo, temp.c_str(), size);
				} else {
					content = (const char*)pointerFrom;
					size = strlen(content);
					// Alloc memory for pointerValue and copy value
					*pointerTo = malloc (size+1);
					memset(*pointerTo, '\0', (size+1));
					memcpy(*pointerTo, content, size);
				}
				//D("upd char* %s size %d", *pointerTo, size);
			}
			break;

		case DEBUG_TYPE_STRING:
			{
				// String not allowed to update
				printSerialDebug();
				Serial.println(F("String not allow updated"));
			}
			break;
	}

	//D("updateValue after from type=%u pointer=%p to type=%u pointer=%p",
	//		typeFrom, pointerFrom, typeTo, *pointerTo);

}


#ifndef DEBUG_DISABLE_DEBUGGER

// Process functions commands

static void processFunctions(String& options) {

	// Get fields of command options

	Fields fields(options, ' ', true);

	if (fields.size() > 2) {
		printSerialDebug();
		Serial.println(F("Invalid sintax for functions. use f ? to show help"));
		return;
	}

	String option = (fields.size() >= 1)? fields.getString(1) : "";

	//D("procf opts=%s flds=%d opt=%s", options.c_str(), fields.size(), option.c_str());

	if (_debugFunctionsAdded > 0) {

		if (option.length() == 0 || option == "?") {

			// Just show functions and help

			option = "";
			showFunctions(option,false);

		} else if (option.indexOf('*') >= 0) {

			// Search by name (case insensitive)

			showFunctions(options, false);

		} else {

			// Call a function

			callFunction(options);
		}

	} else {

		printSerialDebug();
		Serial.println(F("Functions not added to SerialDebug in your project"));
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		Serial.println(F("* See how do it in advanced example"));
#endif
	}
}

// Show list of functions available
// Used to search and show all (return total showed)
// Or to search and show one (return number of item showed)

static int8_t showFunctions(String& options, boolean one, boolean debugSerialApp) {

	// Show all ?

	boolean showAll = false;
	boolean byStartName = false;
	int8_t byNumber = -1;

	_debugString = "";

	// Searching ?

	if (options.length() == 0) {

		if (one) {
    		printSerialDebug();
			Serial.println(F("Option not informed"));
			return -1;
		}

		showAll = true;

	} else {

		// Search by start name (case insensitive)

		int8_t pos = options.indexOf('*');

		//D("pos %d", pos);

		if (pos > 0) {

			if (one) {

				printSerialDebug();
				Serial.println(F("* not allowed, use name or number"));
				return -1;
			}

			options = options.substring(0, pos);
			options.toLowerCase(); // Case insensitive

			byStartName = true;
			//D("byName %s", options.c_str());

		} else {

			// Is integer ?

			if (strIsNum(options)) {

				uint8_t num = options.toInt();

				//D("byNumber %s = %d", options.c_str(), num);

				if (num > 0) { // Find by number, else by exact name

					if (num > _debugFunctionsAdded) {

						printSerialDebug();
						PRINTFLN(F("Function number must between 1 and %u"), _debugFunctionsAdded);
						return -1;
					}

					byNumber = num;
				}
			}
		}

		showAll = false;
	}

	// Show function(s)

	if (!debugSerialApp) { // Not for SerialDebugApp connection

		if (showAll) {
			printSerialDebug();
			PRINTFLN(F("Showing all functions (%u):\r\n*"), _debugFunctionsAdded);
		} else if (!one) {
			printSerialDebug();
			Serial.println(F("Searching and showing functions:"));
		}
	}

	int8_t showed = (!one)? 0 : -1;

	for (uint8_t i=0; i < _debugFunctionsAdded; i++) {

		String type = "";
		String name = "";
		boolean show = showAll;

		// Get name

		if (_debugFunctions[i].name) { // Memory

			name = _debugFunctions[i].name;

		} else if (_debugFunctions[i].nameF) { // For Flash F

			name = String(_debugFunctions[i].nameF);
		}

		// Show ?

		if (!showAll) {

			if (byStartName) {
				String tolower = name;
				tolower.toLowerCase(); // Case insensitive
				show = (tolower.startsWith(options));
			} else if (byNumber > 0) {
				show = ((i + 1) == byNumber);
			} else {
				show = (name == options);
			}
		}

		//D("showFunction options = %s name: %s showAll=%d show=%d", options.c_str(), name.c_str(), showAll, show);

		if (show) {

			switch (_debugFunctions[i].argType) {
				case DEBUG_TYPE_FUNCTION_VOID:
					type = "";
					break;
				case DEBUG_TYPE_STRING:
					type = "String";
					break;
				case DEBUG_TYPE_CHAR:
					type = "char";
					break;
				case DEBUG_TYPE_INT:
					type = "int";
					break;
				default:
					break;
			}

			if (one) { // One search ?

				printSerialDebug();
				PRINTFLN(F("Function found: %02u %s(%s)"), (i + 1), name.c_str(), type.c_str());

				if (_debugFunctions[i].nameF) { // For Flash F - save the content in variable in memory, to not extract Flash again
					_debugString = name;
				}
				showed = i; // Return the index

				break;

			} else { // Description, not for one search

				if (debugSerialApp) { // For DebugSerialApp connection ?

					PRINTF(F("$app:F:%u:%s:%s:"), (i + 1), name.c_str(), type.c_str());

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
					if (_debugFunctions[i].description) { // Memory
						PRINTFLN(F(":%s"), _debugFunctions[i].description);
					} else if (_debugFunctions[i].descriptionF) { // For Flash F, multiples print
						Serial.print(':');
						Serial.println(_debugFunctions[i].descriptionF);
					} else {
						Serial.println();
					}
#else
					Serial.println();
#endif

				} else { // For monitor serial

					PRINTF(F("* %02u %s(%s)"), (i + 1), name.c_str(), type.c_str());

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
					if (_debugFunctions[i].description) { // Memory
						PRINTFLN(F(" // %s"), _debugFunctions[i].description);
					} else if (_debugFunctions[i].descriptionF) { // For Flash F, multiples print
						Serial.print(F(" // "));
						Serial.println(_debugFunctions[i].descriptionF);
					} else {
						Serial.println();
					}
#else
					Serial.println();
#endif
				}

				showed++;

			}
		}
	}

	// Help

	if (!debugSerialApp) {

		boolean doBreak = false;

		if (!one && showed > 0) {

			if (!_debugRepeatCommand && !debugSerialApp) { // Not repeating and not for SerialDebugApp connection

				printSerialDebug();
				Serial.println();
				Serial.println(F("* To show: f [name|num]"));
				Serial.println(F("* To search with start of name (case insensitive): f name*"));
				Serial.println(F("* To call it, just command: f [name|number] [arg]\r\n*"));

				doBreak = true;
			}

		} else if (!one || showed == -1) {

			printSerialDebug();
			Serial.println(F("Function not found."));

			doBreak = true;
		}

		// Do break

		if (doBreak) {
			String response = debugBreak();
			if (response.length() > 0) {
				processCommand(response, false, false);
			}
		}
	}

	// Return

	return showed;

}

// Call a function

static void callFunction(String& options) {

	// Extract options

	String funcId = "";
	String funcArg = "";

	int8_t pos = options.indexOf(' ');
	if (pos > 0) {
		funcId = options.substring(0, pos);
		funcArg = options.substring(pos + 1);
	} else {
		funcId = options;
	}

	//D("callFunction: id %s arg %s", funcId.c_str(), funcArg.c_str());

	// Return the spaces

	char conv = 31;
	funcArg.replace(conv, ' ');

	// Find and show a function (one)

	int8_t num = showFunctions(funcId, true);

	if (num == -1) { // Not found or error

		return;

	}

	//D("callFunction: num %u", num);

	// Call the function

	unsigned long timeBegin = 0;

	printSerialDebug();
	if (_debugFunctions[num].name) { // Memory
		PRINTF(F("Calling function %u -> %s("), (num + 1), _debugFunctions[num].name);
	} else if (_debugFunctions[num].nameF) { // Use a temporary var to not get flash again
		PRINTF(F("Calling function %u -> %s("), (num + 1), _debugString.c_str());
	}

	if (!_debugFunctions[num].callback) { // Callback not set ?

		Serial.println(F(") - no callback set for function"));
		return;

	}

	if (funcArg.length() == 0 &&
		!(_debugFunctions[num].argType == DEBUG_TYPE_FUNCTION_VOID ||
			_debugFunctions[num].argType == DEBUG_TYPE_STRING ||
			_debugFunctions[num].argType == DEBUG_TYPE_CHAR_ARRAY)) {  // For others can not empty

		Serial.println(F(") - argument not informed"));
		return;
	}

	// Exit from silence mode

	if (_debugSilence) {
		debugSilence(false, false);
	}

	// Process

	bool called = false;

	switch (_debugFunctions[num].argType) {

		case DEBUG_TYPE_FUNCTION_VOID:
			{
				// Void arg

				PRINTFLN(F(") ..."));
				delay(500);
				timeBegin = micros();

				_debugFunctions[num].callback();
				called = true;
			}
			break;

		case DEBUG_TYPE_STRING:
			{
				// String arq

				if (funcArg.indexOf('"') != -1) {
					PRINTFLN(F("%s) ..."), funcArg.c_str());
				} else {
					PRINTFLN(F("\"%s\") ..."), funcArg.c_str());
				}
				delay(500);

				removeQuotation(funcArg, false);

				timeBegin = micros();

				void (*callback)(String) = (void(*) (String))_debugFunctions[num].callback;

				callback(funcArg);
				called = true;
			}
			break;

		case DEBUG_TYPE_CHAR:
			{
				// Arg char

				PRINTFLN(F("%s) ..."), funcArg.c_str());
				delay(500);

				removeQuotation(funcArg, true);

				timeBegin = micros();

				void (*callback)(char) = (void(*) (char))_debugFunctions[num].callback;

				callback(funcArg[0]);

				called = true;
			}
			break;

		case DEBUG_TYPE_INT:
			{
				// Arg Int

				if (strIsNum(funcArg)) { // Is numeric ?

					int val = funcArg.toInt();

					PRINTFLN(F("%d) ..."), val);
					delay(500);
					timeBegin = micros();

					void (*callback)(int) = (void(*) (int))_debugFunctions[num].callback;

					callback(val);

					called = true;

				} else {

					PRINTFLN(F("%s) - invalid number in argument"), funcArg.c_str());
				}
			}
			break;
	}

	// Called ?

	if (called) {

		unsigned long elapsed = (micros() - timeBegin);

		printSerialDebug();
		PRINTFLN(F("End of execution. Elapsed: %lu ms (%lu us)"), (elapsed / 1000), elapsed);

	}

	// Clear buffer

	_debugString = "";

	// Do a break

	String response = debugBreak(F("* Press enter to continue"), DEBUG_BREAK_TIMEOUT, true);

	if (response.length() > 0) { // Process command

		processCommand(response, false, false);
	}

}

// Process global variables commands

static void processGlobals(String& options) {

	// Reduce information on error to support low memory boards
	// Use a variable to reduce memory

#if DEBUG_USE_FLASH_F

	__FlashStringHelper* errorSintax = F("Invalid sintax. use g ? to show help");

#else

	const char* errorSintax = "Invalid sintax. use g ? to show help";

#endif

	// Have functions added ?

	if (_debugGlobalsAdded > 0) {

		// Get fields of command options

		Fields fields(options, ' ', true);

		if (fields.size() > 4) {
			printSerialDebug();
			Serial.println(errorSintax);
			return;
		}

		String firstOption = (fields.size() >= 1)? fields.getString(1) : "";

		//D("first=%s options=%s", firstOption.c_str(), options.c_str());

		if (firstOption.length() == 0 || firstOption == "?") {

			// Just show globals and help

			showGlobals(firstOption, false);

		} else if (fields.size() >= 2) { // Process change command

			if (fields.getChar(2) == '=') {

				if (fields.size() == 2) {
					printSerialDebug();
					Serial.println(errorSintax);
					return;

				} else {

					// Change the variable

					changeGlobal (fields);

				}

			} else {

				printSerialDebug();
				Serial.println(errorSintax);
				return;

			}

		} else {

			// Search by name/number

			showGlobals(options, false);

		}

	} else {

		printSerialDebug();
		Serial.println();
		Serial.println(F("* Global variables not added to SerialDebug"));
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		Serial.println(F("* See how do it in advanced example"));
#endif
	}
}

// Show list of globals available
// Used to search and show all (return total showed)
// Or to search and show one (return number showed)

static int8_t showGlobals(String& options, boolean one, boolean debugSerialApp) {

	// Show all ?

	boolean showAll = false;
	boolean byStartName = false;
	int8_t byNumber = -1;

	// Searching ?

	if (options.length() == 0 || options == "?") {

		if (one) {
			printSerialDebug();
			Serial.println(F("Option not informed"));
			return -1;
		}

		showAll = true;

	} else {

		// Search by start name (case insensitive)

		int8_t pos = options.indexOf('*');

		//D("pos %d", pos);

		if (pos > 0) {

			if (one) {

				printSerialDebug();
				Serial.println(F("* not allowed, use name or number instead"));
				return -1;
			}

			options = options.substring(0, pos);
			options.toLowerCase(); // Case insensitive

			byStartName = true;

		} else {

			// Is integer ?

			if (strIsNum(options)) {

				uint8_t num = options.toInt();

				//D("byNumber %s = %d", options.c_str(), num);

				if (num > 0) { // Find by number, else by exact name

					byNumber = num;
				}
			}
		}

		showAll = false;
	}

	// Show global(s)

	if (!debugSerialApp) { // Not for SerialDebugApp connection
		printSerialDebug();
		if (showAll) {
			PRINTFLN(F("Showing all global variables (%u) and actual values:"), _debugGlobalsAdded);
		} else {
			Serial.println(F("Searching and showing global variables and actual values:"));
		}
	}

	int8_t showed = (!one)? 0 : -1;

	// Process

	for (uint8_t i=0; i < _debugGlobalsAdded; i++) {

		String name = "";
		boolean show = showAll;

		// Get name

		if (_debugGlobals[i].name) { // Memory

			name = _debugGlobals[i].name;

		} else if (_debugGlobals[i].nameF) { // For Flash F

			name = String(_debugGlobals[i].nameF);
			_debugString = name; // For Flash F - save the content in variable in memory, to not extract Flash again
		}

		// Show ?

		if (!showAll) {

			if (byStartName) {
				String tolower = name;
				tolower.toLowerCase(); // Case insensitive
				show = (tolower.startsWith(options));
			} else if (byNumber > 0) {
				show = ((i + 1) == byNumber);
			} else {
				show = (name == options);
			}
		}

//		D("showGlobais options = %s name: %s showAll=%d show=%d byStartName=%d byNumber=%d",
//				options.c_str(), name.c_str(), showAll, show,
//				byStartName, byNumber);

		if (show) {

			// Show

			if (debugSerialApp) { // For DebugSerialApp connection ?

				if (showGlobal((i + 1), DEBUG_SHOW_GLOBAL_APP_CONN, true)) {

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
					if (_debugGlobals[i].description) { // Memory
						PRINTFLN(F(":%s"), _debugGlobals[i].description);
					} else if (_debugGlobals[i].descriptionF) { // For Flash F, multiples print
						Serial.print(':');
						Serial.println(_debugGlobals[i].descriptionF);
					} else {
						Serial.println();
					}
#else
					Serial.println();
#endif
				}

			} else if (showGlobal((i + 1), DEBUG_SHOW_GLOBAL, true)) {

				if (one) { // One search ?

					showed = i; // Return the index
					break;

				} else { // Description, not for one search

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
					if (_debugGlobals[i].description) { // Memory
						PRINTFLN(F(" // %s"), _debugGlobals[i].description);
					} else if (_debugGlobals[i].descriptionF) { // For Flash F, multiples print
						Serial.print(F(" // "));
						Serial.println(_debugGlobals[i].descriptionF);
					} else {
						Serial.println();
					}
#else
					Serial.println();
#endif

					showed++;

				}
			}
		}
	}

	// Help

	if (!debugSerialApp) { // Not for SerialDebugApp connection

		boolean doBreak = false;

		if (!one && showed > 0) {

			if (!_debugRepeatCommand) { // Not repeating

				printSerialDebug();
				Serial.println();
				Serial.println(F("* To show: g [name|num]"));
				Serial.println(F("* To search by start of name: g name*"));
				Serial.println(F("* To change global variable, g [name|number] = value [y]\r\n*"));

				doBreak = true;
			}

		} else if (!one || showed == -1) {

			printSerialDebug();
			Serial.println(F("Global variable not found."));

			doBreak = true;
		}

		// Do break ?

		if (doBreak) {
			String response = debugBreak();
			if (response.length() > 0) {
				processCommand(response, false, false);
			}
		}

	}

	// Clear buffer

	if (!one) {
		_debugString = "";
	}

	// Return

	return showed;

}

// Show one global variable

static boolean showGlobal(uint8_t globalNum, debugEnumShowGlobais_t mode, boolean getLastNameF) {

	// Validate

	if (globalNum == 0 || globalNum > _debugGlobalsAdded) {

		printSerialDebug();
		Serial.println(F("Invalid index for global variable\r\n*"));
		return false;
	}

	// Global

	debugGlobal_t* global = &_debugGlobals[globalNum - 1];

	// Get name

	String name = "";

	if (global->name) { // Memory

		name = global->name;

	} else if (global->nameF) { // For Flash F

		if (getLastNameF) {

			name = _debugString; // Yet get from flash

		} else {

			name = String(global->nameF);
			_debugString = name; // For Flash F - save the content in variable in memory, to not extract Flash again
		}
	}

	// Get value and type

	String value = "";
	String type = "";

	// Get value


	getStrValue(global->type, global->pointer, \
			((mode != DEBUG_SHOW_GLOBAL_APP_CONN)?global->showLength:0), (mode != DEBUG_SHOW_GLOBAL_APP_CONN), \
			value, type);
/*
 * TODO: see it
//	getStrValue(global->type, global->pointer, \
//			((!_debugSerialApp)?global->showLength:0), (!_debugSerialApp), \
//			value, type);
*/
	if (value.length() > 0) {

		// Show

		switch (mode) {
			case DEBUG_SHOW_GLOBAL:
				PRINTF(F("* %02u %s(%s) = %s"), globalNum, name.c_str(), type.c_str(), value.c_str());
				break;
			case DEBUG_SHOW_GLOBAL_WATCH:
				PRINTF(F("%02u: %s (%s) %s"), globalNum, name.c_str(), type.c_str(), value.c_str());
				break;
			case DEBUG_SHOW_GLOBAL_APP_CONN:
				PRINTF(F("$app:G:%u:%s:%s:%s"), globalNum, name.c_str(), type.c_str(), value.c_str());
				break;
		}

	} else {

		printSerialDebug();
		Serial.println(F("Not possible show global variable\r\n*"));
		return false;
	}
	return true;
}

// Change content of global variable

static void changeGlobal(Fields& fields) {

	// Use a variable to reduce memory

#if DEBUG_USE_FLASH_F

	__FlashStringHelper* errorNoNumeric = F("Not numeric value is informed");

#else

	const char* errorNoNumeric = "Not numeric value is informed";

#endif

	// Extract options

	String globalId = fields.getString(1);
	String value = fields.getString(3);
	boolean noConfirm = (fields.size() == 4 && fields.getChar(4) == 'y');

	String type = "";
	String tolower = "";

	// Clean the value

	value.trim(); // TODO ver isto

	// Return the spaces

	char conv = 31;
	value.replace(conv, ' ');

	// Save value to show

	String show = value;

	if (value.length() == 0) {

		printSerialDebug();
		Serial.println(F("No value informed (right of '=' in command)"));
		return;
	}

	tolower = value;
	tolower.toLowerCase();

	// Show the global and get the index

	int8_t num = showGlobals(globalId, true);

	if (num == -1) {

		// Invalid or not found

		return;

	}

	Serial.println(F(" // <- This is a old value"));

//	D("changeGlobal: id->%s num=%d value = %s (%d)", globalId.c_str(), num, value.c_str(),value.length());

	// Verify data

	switch (_debugGlobals[num].type) {

		// Basic types

		case DEBUG_TYPE_BOOLEAN:
			{
				if (value == "0" ||
						tolower == "f" ||
						tolower == F("false") ||
						value == "1" ||
						tolower == "t" ||
						tolower == F("true")) {

					// Value ok

					type = F("boolean");
				} else {
					printSerialDebug();
					Serial.println(F("no boolean in value (0|1|false|true|f|t")) ;
					return;
				}
			}
			break;
		case DEBUG_TYPE_CHAR:

			removeQuotation(value, true);

			if (value.length() != 1) {
				printSerialDebug();
				Serial.println(F("Note: string too large, truncated to size of 1"));
				value = value.substring(0,1);
			}
			type = F("char");
			break;
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		case DEBUG_TYPE_BYTE:
			type = F("byte");
			break;
#endif
		case DEBUG_TYPE_INT:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("int");
			break;
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		case DEBUG_TYPE_U_INT:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("unsigned int");
			break;
		case DEBUG_TYPE_LONG:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("long");
			break;
#endif
		case DEBUG_TYPE_U_LONG:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("unsigned long");
			break;
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
		case DEBUG_TYPE_FLOAT:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("float");
			break;
		case DEBUG_TYPE_DOUBLE:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("double");
			break;

		// Integer C size _t

		case DEBUG_TYPE_INT8_T:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("int8_t");
			break;
		case DEBUG_TYPE_INT16_T:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("int16_t");
			break;
		case DEBUG_TYPE_INT32_T:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("int32_t");
			break;
//#ifdef ESP32
//			case DEBUG_TYPE_INT64_T:
//					value = String(*(int64_t*)_debugGlobals[i].pointer);
//					type = "int64_t";
//					break;
//#endif
		// Unsigned integer C size _t

		case DEBUG_TYPE_UINT8_T:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("uint8_t");
			break;
		case DEBUG_TYPE_UINT16_T:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("uint16_t");
			break;
		case DEBUG_TYPE_UINT32_T:
			if (!strIsNum(value)) {
				printSerialDebug();
				Serial.println(errorNoNumeric);
				return;
			}
			type = F("uint32_t");
			break;
//#ifdef ESP32
//			case DEBUG_TYPE_UINT64_T:
//					value = String(*(uint64_t*)_debugGlobals[i].pointer);
//					type = "uint64_t";
//					break;
//#endif
		// Strings

		case DEBUG_TYPE_CHAR_ARRAY:
			{

//				size_t size = sizeof((char*)_debugGlobals[num].pointer);
//				if (value.length() >= size) {
//					PRINTF(F("* SerialDebug: Note: string too large, truncated to size of array (%u)"), size);
//					value = value.substring(0,size-1);
//				}
//				type = "char array";
				printSerialDebug();
				Serial.println(F("Not allowed change char arrays (due memory issues)"));
				return;
			}
			break;
#endif // Not low memory board

		case DEBUG_TYPE_STRING:

			removeQuotation(value, false);

			type = F("String");
			break;

	}

	// Show again with new value to confirm

	if (_debugGlobals[num].name) { // RAM Memory
		PRINTF(F("* %02u %s(%s) = %s"), (num + 1), _debugGlobals[num].name, type.c_str(), show.c_str());
	} else if (_debugGlobals[num].nameF) { // Flash memory
		PRINTF(F("* %02u %s(%s) = %s"), (num + 1), _debugString.c_str(), type.c_str(), show.c_str());
	}
	Serial.println(F(" // <- This is a new value"));

	// Show a confirm message and wait from response (if not "y" passed to last field)

	String response;

	if (noConfirm) {
		response = "y";
	} else {

		response = debugBreak(F("*\r\n* Confirm do change value? (y-yes/n-no)"), DEBUG_BREAK_TIMEOUT, false);
	}

	if (response == "y" || response == "yes") {

		// Do change

		switch (_debugGlobals[num].type) {

				// Basic types

				case DEBUG_TYPE_BOOLEAN:
					{
						boolean change = (value == "1" || value[0] == 't');
						*(boolean*)_debugGlobals[num].pointer = change;
						value = (change) ? F("true") : F("false");
					}
					break;
				case DEBUG_TYPE_CHAR:
					{
						char change = value[0];
						*(char*)_debugGlobals[num].pointer = change;
					}
					break;
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
				case DEBUG_TYPE_BYTE:
					{
						char change = value[0];
						*(byte*)_debugGlobals[num].pointer = change;
					}
					break;
#endif
				case DEBUG_TYPE_INT:
					{
						int change = value.toInt();
						*(int*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
				case DEBUG_TYPE_U_INT:
					{
						unsigned int change = value.toInt();
						*(unsigned int*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
				case DEBUG_TYPE_LONG:
					{
						long change = value.toInt();
						*(long*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
#endif
				case DEBUG_TYPE_U_LONG:
					{
						unsigned long change = value.toInt();
						*(unsigned long*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
#ifndef BOARD_LOW_MEMORY // Not for low memory boards
				case DEBUG_TYPE_FLOAT:
					{
						float change = value.toFloat();
						*(float*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
				case DEBUG_TYPE_DOUBLE: // TODO no have toDouble in some archs - see
					{
						double change = value.toFloat();
						*(double*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;

				// Integer C size _t

				case DEBUG_TYPE_INT8_T:
					{
						int8_t change = value.toInt();
						*(int8_t*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
				case DEBUG_TYPE_INT16_T:
					{
						int16_t change = value.toInt();
						*(int16_t*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
				case DEBUG_TYPE_INT32_T:
					{
						int32_t change = value.toInt();
						*(int32_t*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
		//#ifdef ESP32
		//			case DEBUG_TYPE_INT64_T:
		//					value = String(*(int64_t*)_debugGlobals[i].pointer);
		//					type = "int64_t";
		//					break;
		//#endif

				// Unsigned integer C size _t

				case DEBUG_TYPE_UINT8_T:
					{
						uint8_t change = value.toInt();
						*(uint8_t*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
				case DEBUG_TYPE_UINT16_T:
					{
						uint16_t change = value.toInt();
						*(uint16_t*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
				case DEBUG_TYPE_UINT32_T:
					{
						uint32_t change = value.toInt();
						*(uint32_t*)_debugGlobals[num].pointer = change;
						value = String(change);
					}
					break;
		//#ifdef ESP32
		//			case DEBUG_TYPE_UINT64_T:
		//					value = String(*(uint64_t*)_debugGlobals[i].pointer);
		//					type = "uint64_t";
		//					break;
		//#endif
				// Strings

				case DEBUG_TYPE_CHAR_ARRAY:
					{
						strcpy((char*)(_debugGlobals[num].pointer), (char*)value.c_str());
					}
					break;
#endif // Not low memory board

				case DEBUG_TYPE_STRING:
					{
						*(String*)_debugGlobals[num].pointer = value;
					}
					break;
			}

			// Show again with new value

			if (_debugGlobals[num].name) { // RAM Memory
				PRINTF(F("* %02u %s(%s) = %s"), (num + 1), _debugGlobals[num].name, type.c_str(), show.c_str());
			} else if (_debugGlobals[num].nameF) { // Flash memory
				PRINTF(F("* %02u %s(%s) = %s"), (num + 1), _debugString.c_str(), type.c_str(), show.c_str());
			}
			Serial.println(F(" // <- This has changed w/ success"));


	} else {

		printSerialDebug();
		Serial.println(F("Variable global not changed"));

	}

	// Clear buffer

	_debugString = "";

	// Do a break, if not confirm

	if (!noConfirm) {

		String response = debugBreak(F("* Press enter to continue"), DEBUG_BREAK_TIMEOUT, true);

		if (response.length() > 0) { // Process command

			processCommand(response, false, false);
		}
	}
}

#endif // DEBUG_DISABLE_DEBUGGER

// Remove quotation marks from string

static void removeQuotation(String& string, boolean single) {

	if (single) {

		if (string.length() == 3) {
			string = string.charAt(1);
		}

	} else {

		if (string == "\"\"") {
			string = "";
		} else if (string.length() >= 3) {
			if (string.charAt(0) == '"') { // Retira as aspas
				string = string.substring(1, string.length() - 1);
			}
			if (string.charAt(string.length() - 1) == '"') { // Retira as aspas
				string = string.substring(0, string.length() - 1);
			}
		}
	}
}

#endif // DEBUG_DISABLE_DEBUGGER

// Free memory

#if defined ARDUINO_ARCH_AVR || defined __arm__

	// Based in https://forum.pjrc.com/threads/23256-Get-Free-Memory-for-Teensy-3-0

#ifdef __arm__

    // should use uinstd.h to define sbrk but Due causes a conflict
    extern "C" char* sbrk(int incr);
#else

    extern char *__brkval;
    extern char __bss_end;

#endif

#endif

int freeMemory() {

#if defined ESP8266 || defined ESP32

	 return ESP.getFreeHeap();

#elif defined ARDUINO_ARCH_AVR || defined __arm__

	// function from the sdFat library (SdFatUtil.cpp)
	// licensed under GPL v3
	// Full credit goes to William Greiman.
    char top;
    #ifdef __arm__
        return &top - reinterpret_cast<char*>(sbrk(0));
    #else
        return __brkval ? &top - __brkval : &top - &__bss_end;
    #endif

#else // Not known

	 return -1;

#endif

}

#endif // DEBUG_MINIMUM


#endif // DEBUG_DISABLED

/////// End
