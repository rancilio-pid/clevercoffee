////////
// Libraries Arduino
//
// Library: SerialDebug - Improved serial debugging to Arduino, with simple software debugger
// Author: Joao Lopes
// GitHub: https://github.com/JoaoLopesF/SerialDebug
//
// Basic example to show how to use it.
//
// Example of use:
//
//   print macros:
//
//		printA(F("This is a always - var "));
//		printlnA(var);
//		printV(F("This is a verbose - var "));
//		printlnV(var);
//		printD(F("This is a debug - var "));
//		printlnD(var);
//		printI(F("This is a information - var "));
//		printlnI(var);
//		printW(F("This is a warning - var "));
//		printlnW(var);
//		printE(F("This is a error - var "));
//		printlnE(var);
//
//		printlnV("This not have args");
//
///////

////// Includes

#include "Arduino.h"

// SerialDebug Library

// Disable all debug ? Good to release builds (production)
// as nothing of SerialDebug is compiled, zero overhead :-)
// For it just uncomment the DEBUG_DISABLED
//#define DEBUG_DISABLED true

// Define the initial debug level here (uncomment to do it)
// #define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_VERBOSE

// Disable SerialDebug debugger ? No more commands and features as functions and globals
// Uncomment this to disable it 
//#define DEBUG_DISABLE_DEBUGGER true

// Disable auto function name (good if your debug yet contains it)
//#define DEBUG_AUTO_FUNC_DISABLED true

// Force debug messages to can use flash ) ?
// Disable native Serial.printf (if have)
// Good for low memory, due use flash, but more slow and not use macros
//#define DEBUG_USE_FLASH_F true

// Include SerialDebug

#include "SerialDebug.h" //https://github.com/JoaoLopesF/SerialDebug

////// Variables

// Time

uint32_t mTimeSeconds = 0;

// Buildin Led ON ?

boolean mLedON = false;

////// Setup

void setup() {

    // Initialize the Serial

    Serial.begin(115200); // Can change it to 230400, if you dont use debugIsr* macros

#ifdef __AVR_ATmega32U4__ // Arduino AVR Leonardo

    while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
    }

#else

    delay(500); // Wait a time

#endif

  	// Debug

	// Attention:
    // SerialDebug starts disabled and it only is enabled if have data avaliable in Serial
    // Good to reduce overheads.
	// if You want debug, just press any key and enter in monitor serial

    // Note: all debug in setup must be debugA (always), due it is disabled now.

    printlnA(F("**** Setup: initializing ..."));

    // Buildin led

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // WiFi connection, etc ....

    // ...

    // End

    printlnA(F("*** Setup end"));

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

	// Blink the led

	mLedON = !mLedON;
	digitalWrite(LED_BUILTIN, (mLedON)?LOW:HIGH);

	// Debug the time (verbose level)

	printV(F("Time: "));
	printV(mTimeSeconds);
	printlnV(F(" seconds (VERBOSE)"));

	if (mTimeSeconds % 5 == 0) { // Each 5 seconds

		// Debug levels

		printlnV(F("This is a message of debug level VERBOSE"));
		printlnD(F("This is a message of debug level DEBUG"));
		printlnI(F("This is a message of debug level INFO"));
		printlnW(F("This is a message of debug level WARNING"));
		printlnE(F("This is a message of debug level ERROR"));

		// Functions example to show auto function name feature

		foo();

		bar();
	}

	// Time

	mTimeSeconds++;

	// Delay of 1 second

	delay(1000);
}


// Functions example to show auto function name feature

void foo() {

  uint8_t var = 1;

  printV(F("This is a debug - var "));
  printlnV(var);
}

void bar() {

  uint8_t var = 2;

  printD(F("This is a debug - var "));
  printlnD(var);

}

/////////// End
