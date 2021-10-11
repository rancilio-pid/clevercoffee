////////
// Libraries Arduino
//
// Library: SerialDebug - Improved serial debugging to Arduino, with simple software debugger
// Author: Joao Lopes
// GitHub: https://github.com/JoaoLopesF/SerialDebug
//
// Example to show how to use it.
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
// 	debug macros (printf formatting):
//
//		debugA("This is a always - var %d", var);
//
//		debugV("This is a verbose - var %d", var);
//		debugD("This is a debug - var %d", var);
//		debugI("This is a information - var %d", var);
//		debugW("This is a warning - var %d", var);
//		debugE("This is a error - var %d", var);
//
//		debugV("This not have args");
//
///////

////// Includes

#include "Arduino.h"

// SerialDebug Library

// Disable all debug ? Good to release builds (production)
// as nothing of SerialDebug is compiled, zero overhead :-)
// For it just uncomment the DEBUG_DISABLED
//#define DEBUG_DISABLED true

// Disable SerialDebug debugger ? No more commands and features as functions and globals
// Uncomment this to disable it 
//#define DEBUG_DISABLE_DEBUGGER true

// Define the initial debug level here (uncomment to do it)
// #define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_VERBOSE

// Force debug messages to can use flash ) ?
// Disable native Serial.printf (if have)
// Good for low memory, due use flash, but more slow and not use macros
//#define DEBUG_USE_FLASH_F true

// Include SerialDebug

#include "SerialDebug.h" //https://github.com/JoaoLopesF/SerialDebug

#ifdef DEBUG_MINIMUM
#error "For mode minimum, please open the basic example or disable this mode"
// If this error occurs, your board is a low memory board,
// and for this, the default is mininum mode,
// to especially to reduce program memory (flash)
// You can do it:
// - Open the basic example, or
// - Use this Advanced/Avr example,
//   and comment the DEBUG_MININUM in SerialDebug.h - line 64
#endif

////// Variables

// Time

int mRunSeconds = 0;
int mRunMinutes = 0;
int mRunHours = 0;

// Buildin Led ON ?

#ifndef LED_BUILTIN // For compatibility
	#define LED_BUILTIN BUILTIN_LED
#endif

// Serial speed (115k para o UNO, 250k para os Mega, Due, ESP, etc.)

#if defined(ARDUINO_AVR_ADK)
    #define SERIAL_SPEED 250000
#elif defined(ARDUINO_AVR_MEGA)
	#define SERIAL_SPEED 250000
#elif defined(ARDUINO_AVR_MEGA2560)
	#define SERIAL_SPEED 250000
#elif defined(ARDUINO_SAM_DUE)
	#define SERIAL_SPEED 250000
#elif defined(ARDUINO_SAMD_ZERO)
	#define SERIAL_SPEED 250000
#elif defined(ARDUINO_ARC32_TOOLS)
	#define SERIAL_SPEED 250000
#elif defined ESP8266 || defined ESP32
	#define SERIAL_SPEED 250000
#else
	#define SERIAL_SPEED 115200
#endif

// Led on ?

boolean mLedON = false;

// Globals for this example

boolean mBoolean = false;
char mChar = 'X';
int mInt = 1;
unsigned long mULong = 4;
float mFloat = 0.0f;

String mString = "This is a string";

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
String mStringLarge = "This is a large stringggggggggggggggggggggggggggggggggggggggggggggg";
#endif

int mIntArray[5] = {1 ,2 ,3, 4, 5};

//const char mCharArrayConst[] = "This is const";

////// Setup

void setup() {

    // Initialize the Serial

    Serial.begin(SERIAL_SPEED);

#ifdef __AVR_ATmega32U4__ // For Arduino AVR Leonardo

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

    Serial.println(); // To not stay in end of dirty chars in boot

    debugA(F("**** Setup: initializing ..."));

    // Buildin led

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // WiFi connection, etc ....

    // ...

#ifndef DEBUG_DISABLE_DEBUGGER

    // Add Functions and global variables to SerialDebug

    // Notes: descriptions is optionals

    // Add functions that can called from SerialDebug

    if (debugAddFunctionVoid(F("benchInt"), &benchInt) >= 0) {
    	debugSetLastFunctionDescription(F("To run a benchmark of integers"));
    }
    if (debugAddFunctionVoid(F("benchFloat"), &benchFloat) >= 0) {
    	debugSetLastFunctionDescription(F("To run a benchmark of float"));
    }
    if (debugAddFunctionVoid(F("benchGpio"), &benchGpio) >= 0) {
    	debugSetLastFunctionDescription(F( 		"To run a benchmark of Gpio operations"));
    }
    if (debugAddFunctionVoid(F("benchAll"), &benchAll) >= 0) {
    	debugSetLastFunctionDescription(F("To run all benchmarks"));
    }

    if (debugAddFunctionVoid(F("benchSerialPrint"), &benchSerialPrint) >= 0) {
    	debugSetLastFunctionDescription(F("To benchmarks standard Serial debug"));
    }
    if (debugAddFunctionVoid(F("benchSerialDebug"), &benchSerialDebug) >= 0) {
    	debugSetLastFunctionDescription(F("To benchmarks SerialDebug"));
    }
    if (debugAddFunctionVoid(F("benchSerialDbgPr"), &benchSerialDbgPr) >= 0) {
    	debugSetLastFunctionDescription(F("To benchmarks SerialDebug print macros"));
    }
    if (debugAddFunctionVoid(F("benchSerialAll"), &benchSerialAll) >= 0) {
    	debugSetLastFunctionDescription(F("To benchmarks all Serial"));
    }

    if (debugAddFunctionStr(F("funcArgStr"),&funcArgStr) >= 0) {
    	debugSetLastFunctionDescription(F("To run with String arg"));
    }
    if (debugAddFunctionChar(F("funcArgChar"), &funcArgChar) >= 0) {
    	debugSetLastFunctionDescription(F("To run with Character arg"));
    }
    if (debugAddFunctionInt(F("funcArgInt"), &funcArgInt) >= 0) {
    	debugSetLastFunctionDescription(F("To run with Integer arg"));
    }
    // Add global variables that can showed/changed from SerialDebug
    // Note: Only global, if pass local for SerialDebug, can be dangerous

    if (debugAddGlobalInt(F("mRunSeconds"), &mRunSeconds) >= 0) {
    	debugSetLastGlobalDescription(F("Seconds of run time"));
    }
    if (debugAddGlobalInt(F("mRunMinutes"), &mRunMinutes) >= 0) {
    	debugSetLastGlobalDescription(F("Minutes of run time"));
    }
    if (debugAddGlobalInt(F("mRunHours"), &mRunHours) >= 0) {
    	debugSetLastGlobalDescription(F("Hours of run time"));
    }

    // Note: easy way, no descriptions ....

    debugAddGlobalBoolean(F("mBoolean"), 	&mBoolean);
    debugAddGlobalChar(F("mChar"), 			&mChar);
    debugAddGlobalInt(F("mInt"), 			&mInt);
    debugAddGlobalULong(F("mULong"), 		&mULong);

    debugAddGlobalString(F("mString"), 		&mString);

#ifndef BOARD_LOW_MEMORY // Not for low memory boards
    // Note, here inform to show only 20 characteres of this string or char array

    debugAddGlobalString(F("mStringLarge"), &mStringLarge, 20);
#endif

    // For arrays, need add for each item (not use loop for it, due the name can not by a variable)

	debugAddGlobalInt(F("mIntArray[0]"), 	&mIntArray[0]);
	debugAddGlobalInt(F("mIntArray[1]"), 	&mIntArray[1]);
	debugAddGlobalInt(F("mIntArray[2]"), 	&mIntArray[2]);
	debugAddGlobalInt(F("mIntArray[3]"), 	&mIntArray[3]);
	debugAddGlobalInt(F("mIntArray[4]"), 	&mIntArray[4]);

#ifndef BOARD_LOW_MEMORY // Not for low memory boards

    // Add watches for some global variables
    // Note: watches can be added/changed in serial monitor too

	// Watch -> mBoolean when changed (put 0 on value)

	debugAddWatchBoolean(F("mBoolean"), DEBUG_WATCH_CHANGED, 0);

	// Watch -> mRunSeconds == 10

	debugAddWatchInt(F("mRunSeconds"), DEBUG_WATCH_EQUAL, 10);

	// Watch -> mRunMinutes == mRunSeconds (just for test)

	debugAddWatchCross(F("mRunMinutes"), DEBUG_WATCH_EQUAL, F("mRunSeconds"));

#endif

#endif // DEBUG_DISABLE_DEBUGGER

    // End

    debugA(F("**** Setup: initialized."));

}

////// Loop

void loop()
{
	// SerialDebug handle
	// NOTE: if in inactive mode (until receive anything from serial),
	// it show only messages of always or errors level type
	// And the overhead during inactive mode is very low

	debugHandle();

	// Blink the led

	mLedON = !mLedON;
	digitalWrite(LED_BUILTIN, (mLedON)?LOW:HIGH);

	// Debug the time (verbose level)

	debugV(F("* Run time: %02d:%02d:%02d (VERBOSE)"), mRunHours, mRunMinutes, mRunSeconds);

	if (mRunSeconds % 5 == 0) { // Each 5 seconds

		// Debug levels

		debugV(F("* This is a message of debug level VERBOSE"));
		debugD(F("* This is a message of debug level DEBUG"));
		debugI(F("* This is a message of debug level INFO"));
		debugW(F("* This is a message of debug level WARNING"));

		if (mRunSeconds == 55) { // Just for not show initially
			debugE(F("* This is a message of debug level ERROR"));
		}

		mBoolean = (mRunSeconds == 30); // Just to trigger the watch

		// Functions example to show auto function name feature

		foo();

		bar();

		// Example of float formatting:

		mFloat += 0.01f;

#ifndef ARDUINO_ARCH_AVR // Native float printf support
		debugV(F("mFloat = %0.3f"), mFloat);
#else // For AVR, it is not supported, using String instead
		debugV(F("mFloat = %s"), String(mFloat).c_str());
#endif

		// New print macros for debug

		printlnV(F("Test of print macro"));
		printV(F("secs: "));
		printlnV(mRunSeconds);

	}

	// Count run time (just a test - for real suggest the TimeLib and NTP, if board have WiFi)

	mRunSeconds++;

	if (mRunSeconds == 60) {
		mRunMinutes++;
		mRunSeconds = 0;
	}
	if (mRunMinutes == 60) {
		mRunHours++;
		mRunMinutes = 0;
	}
	if (mRunHours == 24) {
		mRunHours = 0;
	}

	// Delay of 1 second

	delay(1000);
}


// Functions example to show auto function name feature

void foo() {

  uint8_t var = 1;

  debugV(F("this is a debug - var %u"), var);
}

void bar() {

  uint8_t var = 2;

  debugD(F("this is a debug - var %u"), var);
}

////// Benchmarks - simple

// Note: how it as called by SerialDebug, must be return type void and no args
// Note: Flash F variables is not used during the tests, due it is slow to use in loops

#define BENCHMARK_EXECS 10000

// Simple benckmark of integers

void benchInt() {

	int test = 0;

	for (int i = 0; i < BENCHMARK_EXECS; i++) {

		// Some integer operations

		test++;
		test += 2;
		test -= 2;
		test *= 2;
		test /= 2;
	}

	// Note: Debug always is used here

	debugA(F("*** Benchmark of integers. %u exec."), BENCHMARK_EXECS);

}

// Simple benckmark of floats

void benchFloat() {

	float test = 0;

	for (int i = 0; i < BENCHMARK_EXECS; i++) {

		// Some float operations

		test++;
		test += 2;
		test -= 2;
		test *= 2;
		test /= 2;
	}

	// Note: Debug always is used here

	debugA(F("*** Benchmark of floats, %u exec."), BENCHMARK_EXECS);

}

// Simple benckmark of GPIO

void benchGpio() {

	const int execs = (BENCHMARK_EXECS / 10); // Reduce it

	for (int i = 0; i < execs; i++) {

		// Some GPIO operations

		digitalWrite(LED_BUILTIN, HIGH);
		digitalRead(LED_BUILTIN);
		digitalWrite(LED_BUILTIN, LOW);

		analogRead(A0);
		analogRead(A0);
		analogRead(A0);

	}

	// Note: Debug always is used here

	debugA(F("*** Benchmark of GPIO. %u exec."), execs);

}

// Run all benchmarks

void benchAll() {

	benchInt();
	benchFloat();
	benchGpio();

	// Note: Debug always is used here

	debugA(F("*** All Benchmark done."));

}

// Serial benchmarks, to compare Serial.prints with SerialDebug

// Note no using F() in loops, due it compare only speed of processing

#define BENCHMARK_SERIAL 25

void benchSerialPrint() {

	// Note: Serial.printf is not used, due most Arduino not have this
	// Same data size of SerialDebug to compare speeds of processing and not the time elapsed to send it

	unsigned long timeBegin = micros();

	for (uint16_t i = 0; i < BENCHMARK_SERIAL; i++) {

		Serial.print("(A ");
		Serial.print(millis());
		Serial.print(" benchSerialPrint) Exec.: ");
		Serial.print(i+1);
		Serial.print(" of ");
		Serial.println(BENCHMARK_SERIAL);

	}

	unsigned long elapsed = (micros() - timeBegin);

	Serial.print(F("*** Benchmark of Serial prints. Execs.: "));
	Serial.print(BENCHMARK_SERIAL);
	Serial.print(F(" time elapsed -> "));
	Serial.print(elapsed);
	Serial.println(F(" us"));

}

void benchSerialDebug() {

	// Note: printf formats can be used, even if Arduino not have this,
	// This is done in internal debugPrintf

	debugSetProfiler(false); // Disable it, due standard prints not have it

	unsigned long timeBegin = micros();

	for (uint16_t i = 0; i < BENCHMARK_SERIAL; i++) {

		debugA("Exec.: %02u of %02u", (i+1), BENCHMARK_SERIAL);

	}

	unsigned long elapsed = (micros() - timeBegin);

	debugSetProfiler(true); // Restore

	// Note not using SerialDebug macros below, to show equals that SerialPrints

	Serial.print(F("*** Benchmark of Serial debugA. Execs.: "));
	Serial.print(BENCHMARK_SERIAL);
	Serial.print(F(" time elapsed -> "));
	Serial.print(elapsed);
	Serial.println(F(" us"));

}

void benchSerialDbgPr() {

	// Using print macros to avoid printf
	// Same data size of SerialDebug to compare speeds of processing and not the time elapsed to send it

	debugSetProfiler(false); // Disable it, due standard prints not have it

	unsigned long timeBegin = micros();

	for (uint16_t i = 0; i < BENCHMARK_SERIAL; i++) {

		printA("Exec.: ");
		printA(i+1);
		printA(" of ");
		printlnA(BENCHMARK_SERIAL);

	}

	unsigned long elapsed = (micros() - timeBegin);

	debugSetProfiler(true); // Restore

	// Note not using SerialDebug macros below, to show equals that SerialPrints

	Serial.print(F("*** Benchmark of Serial printA. Execs.: "));
	Serial.print(BENCHMARK_SERIAL);
	Serial.print(F(" time elapsed -> "));
	Serial.print(elapsed);
	Serial.println(F(" us"));

}

void benchSerialAll() {

	benchSerialPrint();

	delay(1000); // To give time to send any buffered

	benchSerialDebug();

	delay(1000); // To give time to send any buffered

	benchSerialDbgPr();
}


// Example functions with argument (only 1) to call from serial monitor
// Note others types is not yet available in this version of SerialDebug

void funcArgStr (String str) {

	debugA(F("*** called with arg.: %s"), str.c_str());
}
void funcArgChar (char character) {

	debugA(F("*** called with arg.: %c"), character);
}
void funcArgInt (int number) {

	debugA(F("*** called with arg.: %d"), number);
}

/////////// End
