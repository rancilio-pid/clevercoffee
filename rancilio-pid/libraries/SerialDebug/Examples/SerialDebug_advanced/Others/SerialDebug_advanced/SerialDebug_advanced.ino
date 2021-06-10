////////
// Libraries Arduino
//
// Library: SerialDebug - Improved serial debugging to Arduino, with simple software debugger
// Author: Joao Lopes
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

///////
// Note: this version is for Espressif or ARM boards,
//       Not using F() to reduce memory,
//       due these boards have memory a lot,
//	     and RAM memory is much faster than Flash memory
//       If want or need, please open the example in Directory Avr.
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

// Debug TAG ?
// Usefull with debug any modules
// For it, each module must have a TAG variable:
// 		const char* TAG = "...";
// Uncomment this to enable it
//#define DEBUG_USE_TAG true

// Define the initial debug level here (uncomment to do it)
// #define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_VERBOSE

// Force debug messages to can use flash ) ?
// Disable native Serial.printf (if have)
// Good for low memory, due use flash, but more slow and not use macros
//#define DEBUG_USE_FLASH_F true

// Include SerialDebug

#include "SerialDebug.h" //https://github.com/JoaoLopesF/SerialDebug

#ifdef BOARD_LOW_MEMORY
	#error "This is not for low memoy boards, please use the basic example"
	// If this error occurs, your board is a low memory board,
	// and for this, the default is mininum mode,
	// to especially to reduce program memory (flash)
	// You can do it:
	// - Open the basic example, or
    // - Open Advanced/Avr example (this uses F() to reduce RAM usage),
    //   and before, comment the DEBUG_MININUM in SerialDebug.h - line 64
#endif

////// Variables

// Time

uint8_t mRunSeconds = 0;
uint8_t mRunMinutes = 0;
uint8_t mRunHours = 0;

// Buildin Led ON ?

#ifndef LED_BUILTIN // For compatibility
	#ifdef BUILTIN_LED
		#define LED_BUILTIN BUILTIN_LED
	#else
		#define LED_BUILTIN 2
	#endif
#endif

boolean mLedON = false;

// Globals for this example

boolean mBoolean = false;
char mChar = 'X';
byte mByte = 'Y';
int mInt = 1;
unsigned int mUInt = 2;
long mLong = 3;
unsigned long mULong = 4;
float mFloat = 5.0f;
double mDouble = 6.0;

String mString = "This is a string";
String mStringLarge = "This is a large stringggggggggggggggggggggggggggggggggggggggggggggg";

char mCharArray[] = "This is a char array";
char mCharArrayLarge[] = "This is a large char arrayyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy";

int mIntArray[5] = {1 ,2 ,3, 4, 5};

//const char mCharArrayConst[] = "This is const";

////// Setup

void setup() {

    // Initialize the Serial

    Serial.begin(250000); // Can change it to 115200, if you want use debugIsr* macros

    delay(500); // Wait a time

  	// Debug

	// Attention:
    // SerialDebug starts disabled and it only is enabled if have data avaliable in Serial
    // Good to reduce overheads.
	// if You want debug, just press any key and enter in monitor serial

    // Note: all debug in setup must be debugA (always), due it is disabled now.

    Serial.println(); // To not stay in end of dirty chars in boot

    debugA("**** Setup: initializing ...");

    // Buildin led

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // WiFi connection, etc ....

    // ...

#ifndef DEBUG_DISABLE_DEBUGGER

    // Add Functions and global variables to SerialDebug

    // Notes: descriptions is optionals

    // Add functions that can called from SerialDebug

    if (debugAddFunctionVoid("benchInt", &benchInt) >= 0) {
    	debugSetLastFunctionDescription("To run a benchmark of integers");
    }
    if (debugAddFunctionVoid("benchFloat", &benchFloat) >= 0) {
    	debugSetLastFunctionDescription("To run a benchmark of float");
    }
    if (debugAddFunctionVoid("benchGpio", &benchGpio) >= 0) {
    	debugSetLastFunctionDescription("To run a benchmark of Gpio operations");
    }
    if (debugAddFunctionVoid("benchAll", &benchAll) >= 0) {
    	debugSetLastFunctionDescription("To run all benchmarks");
    }

    if (debugAddFunctionVoid("benchSerialPrints", &benchSerialPrint) >= 0) {
    	debugSetLastFunctionDescription("To benchmarks standard Serial debug");
    }
    if (debugAddFunctionVoid("benchSerialDebug", &benchSerialDebug) >= 0) {
    	debugSetLastFunctionDescription("To benchmarks SerialDebug");
    }
    if (debugAddFunctionVoid("benchSerialDbgPr", &benchSerialDbgPr) >= 0) {
    	debugSetLastFunctionDescription("To benchmarks SerialDebug print macros");
    }
    if (debugAddFunctionVoid("benchSerialAll", &benchSerialAll) >= 0) {
    	debugSetLastFunctionDescription("To benchmarks all Serial");
    }

    if (debugAddFunctionStr("funcArgStr", &funcArgStr) >= 0) {
    	debugSetLastFunctionDescription("To run with String arg");
    }
    if (debugAddFunctionChar("funcArgChar", &funcArgChar) >= 0) {
    	debugSetLastFunctionDescription("To run with Character arg");
    }
    if (debugAddFunctionInt("funcArgInt", &funcArgInt) >= 0) {
    	debugSetLastFunctionDescription("To run with Integer arg");
    }

    // Add global variables that can showed/changed from SerialDebug
    // Note: Only global, if pass local for SerialDebug, can be dangerous

    if (debugAddGlobalUInt8_t("mRunSeconds", &mRunSeconds) >= 0) {
    	debugSetLastGlobalDescription("Seconds of run time");
    }
    if (debugAddGlobalUInt8_t("mRunMinutes", &mRunMinutes) >= 0) {
    	debugSetLastGlobalDescription("Minutes of run time");
    }
    if (debugAddGlobalUInt8_t("mRunHours", &mRunHours) >= 0) {
    	debugSetLastGlobalDescription("Hours of run time");
    }

    // Note: easy way, no descriptions ....

    debugAddGlobalBoolean("mBoolean", 	&mBoolean);
    debugAddGlobalChar("mChar", 		&mChar);
    debugAddGlobalByte("mByte", 		&mByte);
    debugAddGlobalInt("mInt", 			&mInt);
    debugAddGlobalUInt("mUInt", 		&mUInt);
    debugAddGlobalLong("mLong", 		&mLong);
    debugAddGlobalULong("mULong", 		&mULong);
    debugAddGlobalFloat("mFloat", 		&mFloat);
    debugAddGlobalDouble("mDouble", 	&mDouble);

    debugAddGlobalString("mString", 	&mString);

    // Note: For char arrays, not use the '&'

    debugAddGlobalCharArray("mCharArray", mCharArray);

    // Note, here inform to show only 20 characteres of this string or char array

    debugAddGlobalString("mStringLarge", &mStringLarge, 20);

    debugAddGlobalCharArray("mCharArrayLarge",
    									mCharArrayLarge, 20);

    // For arrays, need add for each item (not use loop for it, due the name can not by a variable)
    // Notes: Is good added arrays in last order, to help see another variables
    //        In next versions, we can have a helper to do it in one command

	debugAddGlobalInt("mIntArray[0]", 	&mIntArray[0]);
	debugAddGlobalInt("mIntArray[1]", 	&mIntArray[1]);
	debugAddGlobalInt("mIntArray[2]", 	&mIntArray[2]);
	debugAddGlobalInt("mIntArray[3]",	&mIntArray[3]);
	debugAddGlobalInt("mIntArray[4]",	&mIntArray[4]);

    // Add watches for some global variables
    // Note: watches can be added/changed in serial monitor too

	// Watch -> mBoolean when changed (put 0 on value)

	debugAddWatchBoolean("mBoolean", DEBUG_WATCH_CHANGED, 0);

	// Watch -> mRunSeconds == 10

	debugAddWatchUInt8_t("mRunSeconds", DEBUG_WATCH_EQUAL, 10);

	// Watch -> mRunMinutes > 3

	debugAddWatchUInt8_t("mRunMinutes", DEBUG_WATCH_GREAT, 3);

	// Watch -> mRunMinutes == mRunSeconds (just for test)

	debugAddWatchCross("mRunMinutes", DEBUG_WATCH_EQUAL, "mRunSeconds");

#endif // DEBUG_DISABLE_DEBUGGER

    // End

    debugA("**** Setup: initialized.");

}

////// Loop

void loop()
{
	// SerialDebug handle
	// NOTE: if in inactive mode (until receive anything from serial),
	// it show only messages of always or errors level type
	// And the overhead during inactive mode is very much low

	debugHandle();

	// Blink the led

	mLedON = !mLedON;
	digitalWrite(LED_BUILTIN, (mLedON)?LOW:HIGH);

	// Debug the time (verbose level)

	debugV("* Run time: %02u:%02u:%02u (VERBOSE)", mRunHours, mRunMinutes, mRunSeconds);

	if (mRunSeconds % 5 == 0) { // Each 5 seconds

		// Debug levels

		debugV("* This is a message of debug level VERBOSE");
		debugD("* This is a message of debug level DEBUG");
		debugI("* This is a message of debug level INFO");
		debugW("* This is a message of debug level WARNING");

		if (mRunSeconds == 55) { // Just for not show initially
			debugE("* This is a message of debug level ERROR");
		}

		mBoolean = (mRunSeconds == 30); // Just to trigger the watch

		// Functions example to show auto function name feature

		foo();

		bar();

		// Example of float formatting:

		mFloat += 0.01f;

#ifndef ARDUINO_ARCH_AVR // Native float printf support
		debugV("mFloat = %.3f", mFloat);
#else // For AVR, it is not supported, using String instead
		debugV("mFloat = %s", String(mFloat).c_str());
#endif

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

  debugV("This is a debug - var %u", var);
}

void bar() {

  uint8_t var = 2;

  debugD("This is a debug - var %u", var);
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

	debugA("*** Benchmark of integers. %u exec.", BENCHMARK_EXECS);

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

	debugA("*** Benchmark of floats, %u exec.", BENCHMARK_EXECS);

}

// Simple benckmark of GPIO

void benchGpio() {

//	const int execs = (BENCHMARK_EXECS / 10); // Reduce it
	const int execs = BENCHMARK_EXECS;

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

	debugA("*** Benchmark of GPIO. %u exec.", execs);

}

// Run all benchmarks

void benchAll() {

	benchInt();
	benchFloat();
	benchGpio();

	// Note: Debug always is used here

	debugA("*** All Benchmark done.");

}

// Serial benchmarks, to compare Serial.prints with SerialDebug

#define BENCHMARK_SERIAL 25

void benchSerialPrint() {

	// Note: Serial.printf is not used, due most Arduino not have this
	// Show same info to compare real speed
	// Same data size of SerialDebug to compare speeds of processing and not the time elapsed to send it

	unsigned long timeBegin = micros();

	for (uint16_t i = 0; i < BENCHMARK_SERIAL; i++) {

		Serial.print("(A ");
		Serial.print(millis());
		Serial.print(" benchSerialPrint");
#ifdef ESP32
		Serial.print(" C");
		Serial.print(xPortGetCoreID());
#endif
		Serial.print(") Exec.: ");
		Serial.print(i+1);
		Serial.print(" of ");
		Serial.println(BENCHMARK_SERIAL);

	}

	unsigned long elapsed = (micros() - timeBegin);

	Serial.print("*** Benchmark of Serial prints. Execs.: ");
	Serial.print(BENCHMARK_SERIAL);
	Serial.print(" time elapsed -> ");
	Serial.print(elapsed);
	Serial.println(" us");

}

void benchSerialDebug() {

	// Notes: printf formats can be used,
	// even if Arduino not have this, this is done in internal debugPrintf
	// Debug always level is used here

	debugShowProfiler(false, 0, false); // Disable the profiler during the test (the Serial.print not have it)

	unsigned long timeBegin = micros();

	for (uint16_t i = 0; i < BENCHMARK_SERIAL; i++) {

		debugA("Exec.: %u of %u", (i+1), BENCHMARK_SERIAL);

	}

	unsigned long elapsed = (micros() - timeBegin);

	// Note not using SerialDebug macros below, to show equals that SerialPrints

	Serial.print("*** Benchmark of Serial debugA. Execs.: ");
	Serial.print(BENCHMARK_SERIAL);
	Serial.print(" time elapsed -> ");
	Serial.print(elapsed);
	Serial.println(" us");

	debugShowProfiler(true, 0, false); // Reenable

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

	Serial.print("*** Benchmark of Serial printA. Execs.: ");
	Serial.print(BENCHMARK_SERIAL);
	Serial.print(" time elapsed -> ");
	Serial.print(elapsed);
	Serial.println(" us");

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

	debugA("*** called with arg.: %s", str.c_str());
}
void funcArgChar (char character) {

	debugA("*** called with arg.: %c", character);
}
void funcArgInt (int number) {

	debugA("*** called with arg.: %d", number);
}

/////////// End
