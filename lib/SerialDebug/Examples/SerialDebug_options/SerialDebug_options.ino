////////
// Libraries Arduino
//
// Library: SerialDebug - Improved serial debugging to Arduino, with simple software debugger
// GitHub: https://github.com/JoaoLopesF/SerialDebug
// Author: Joao Lopes
//
// Example to show how to enabling or disabling features
//
///////

////// Includes

#include "Arduino.h"

// SerialDebug Library

// SerialDebug disables examples
// Please edit SerialDebug.h, if local defines not working (for Arduino IDE it not works)
// Note: for the Arduino recompile SerialDebugLibrary, please open and save SerialDebug.h
// Please try all and see memory afects

// Disable all debug ? Good to release builds (production)
// as nothing of SerialDebug is compiled, zero overhead :-)
// For it just uncomment the DEBUG_DISABLED
#define DEBUG_DISABLED true

// Minimum mode - only to show messages of debug
// No commands from serial, no debugger, no printf
// Minumum usage of memory RAM and program
// Uncomment this, to use this mode
//#define DEBUG_MINIMUM true

// Disable SerialDebug debugger ? No more commands and features as functions and globals
// Uncomment this to disable it 
//#define DEBUG_DISABLE_DEBUGGER true

// Disable Flash variables support - F()
// Used internally in SerialDebug and in public API
// If is a low memory board, like AVR, all strings in SerialDebug is using flash memory
// If have RAM memory, this is more fast than flash
//#define DEBUG_NOT_USE_FLASH_F true

// For Espressif boards, not flash support, due it have a lot of memory
// If you need more memory, can force it:
//#define DEBUG_USE_FLASH_F true

// Disable auto function name (good if your debug yet contains it)
//#define DEBUG_AUTO_FUNC_DISABLED true


// Include SerialDebug

#include "SerialDebug.h" //https://github.com/JoaoLopesF/SerialDebug

////// Variables

////// Setup

void setup() {

    // Initialize the Serial

    Serial.begin(115200);

#ifdef __AVR_ATmega32U4__ // Arduino AVR Leonardo

    while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
    }

#else

    delay(500); // Wait a time

#endif

}

////// Loop

void loop()
{

	// SerialDebug handle
	// Notes: if in inactive mode (until receive anything from serial),
	// it show only messages of always or errors level type
	// And the overhead during inactive mode is very low
	// Only if not DEBUG_DISABLED

	debugHandle();

	// Example to show that nothing is compiled if Serial.Debug is disabled
	// Good for production releases

#ifdef DEBUG_DISABLED

	debugV("this is not compiled. if compiled, cause a error, due variable not exist %d", not_existing_var);

	// Show memory (not using SerialDebug)

	Serial.print("* (by Serial.print) Free Heap RAM: ");
	Serial.println(freeMemory());

#endif


	// Show the memory - using SerialDebug
	// Note if set DEBUG_DISABLED, this not compile and not generate any output

	debugA("* (by SerialDebug) Free Heap RAM: %d", freeMemory());

	// Delay of 5 second

	delay(5000);
}

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

/////////// End
