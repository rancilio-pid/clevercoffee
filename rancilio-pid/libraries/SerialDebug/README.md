# SerialDebug Library for Arduino

<a href="#releases">![build badge](https://img.shields.io/badge/version-v0.9.82-blue.svg)</a> [![Codacy Badge](https://api.codacy.com/project/badge/Grade/5ddb5c53fa29416eb1d1eaaf6f201ec6)](https://app.codacy.com/app/JoaoLopesF/SerialDebug?utm_source=github.com&utm_medium=referral&utm_content=JoaoLopesF/SerialDebug&utm_campaign=Badge_Grade_Settings)
<a href="https://github.com/JoaoLopesF/SerialDebug/blob/master/LICENSE.txt">![GitHub](https://img.shields.io/github/license/mashape/apistatus.svg)</a>
[![Gitter chat](https://badges.gitter.im/SerialDebug/gitter.png)](https://gitter.im/SerialDebug/Public)

Improved serial debugging to Arduino, with with debug levels and simple software debugger,
to see/change global variables, to add watch for these variables,
or call a function, in runtime, using serial monitor or SerialDebugApp.

[![randomnerdtutorials](http://joaolopesf.net/images/serialdebugapp/Serial-debug-app.jpg)](https://randomnerdtutorials.com/serialdebug-library-arduino-ide/)
_Note: This image is from the tutorial for this library at [randomnerdtutorials.com](https://randomnerdtutorials.com/serialdebug-library-arduino-ide/) - Image by Sara Santos_
## Contents

- [About](#about)
- [News](#news)
- [Beta version](#beta-version)
- [Github](#github)
- [Benefits](#benefits)
- [SerialDebugApp](#serialdebugapp)
- [How it looks](#how-it-looks)
- [Commands](#commands)
- [Install](#install)
- [Usage](#usage)
- [Options](#options)
- [Watches](#watches)
- [Tutorial](#tutorial)
- [Khow issues](#khow-issues)
- [Releases](#releases)
- [Links](#links)
- [Thanks](#thanks)

## About

Generally debugs messages in Arduino is done by Serial.print*,
and show in this monitor serial, or another serial tool, as screen, pyserial, coolterm, etc.

The Arduino official IDE, not have debugger.
Only way, until now, is using a another IDE, that can be paid, and a hardware debugger, as JTAG.

__SerialDebug__ is a library to Improved serial debugging and simple software debugger to Arduino.

Yes, now we can use one debugger, simple but functional,
and not need a extra hardware to do it.

## News

### 2018-11-16

  - Now the __SerialDebug__ print macros support the second argument of Serial.print.
    Thanks to @wjwieland to open a issue about this.
    E.g.: printlnA(10, HEX);

### 2018-10-26

  - Now the [__SerialDebugApp__](#serialdebugapp) show debugger elements on screen, please update to 0.9.2 to see it in action

  - In library version >= 0.9.75, have a new _minimum_ mode, to limit to only output debugs,
    no debugger, no printf, no extra functions or processing on library.
    And by default, low memory boards, as Uno, is in _minimum_ mode.

## Beta version

This is a beta version. 
Not yet fully tested, optimized, and documented.

This is a previous documentation.
It will be better documented before first RC version.

## Github

Contribute to this library development by creating an account on GitHub.

Please give a star, if you find this library usefull, 
this help a another people, discover it too.

Please add a issue for problems or suggestion.

And please join in the Gitter chat room ([SerialDebug on Gitter](https://gitter.im/SerialDebug/Public)).

I suggest you use a Github Desktop New app to clone, 
it help to keep updated.

## Benefits

__SerialDebug__ is bether than Arduino default serial debugging:

- This is __optimized__ for features that it have

  The initial status of __SerialDebug__ is inactive,
  where no normal debug outputs, and no CPU waste time for debugs.
  Well, there may not be anyone seeing it.
  It is good for not always USB connected project,
  as one powered by battery or external power supply.

  Only messages that are processed and displayed,
  are of type Error or Always (important ones).

  After first command received, __SerialDebug__ will become active

  All routines to show debug is a C/C++ precompiler macros,
  so no extra functions calls, only Serial.print*

  Except when use printf formatter (__debug*__ macros),
  for boards that no have it native.

  For simple software debugger, have memory optimizations:

  - No fixed arrays, is used C++ Vector to dynamic arrays

  - Is used void* pointer to store values, when it is need.
    Is more complicate, but it dramatically reduces use of memory,
    compared to store 17 variables for support 17 kinds of this.

  Note: due a extra overhead in processing simple software debugger,
        it starts disabled. You can enable when you need (dbg command)

  Now (>= 0.9.5) __SerialDebug__ have a new performance,
  compared with standard print, no difference for __print*__ macros
  and about slower only 1% with __debug*__ macros (due printf processing).
  Boards tested: Uno, Mega, Due, Esp8266 and Esp32.

  To reproduce it, just upload the advanced example,
  and call function 8 (just command: f 8) to run all benchmarks of serial.

  Now (>= 0.9.74) __SerialDebug__ have a mode mininum,
  for boards with low memory to program, as Arduino UNO,
  if this mode is set, SerialDebug only show messages,
  no extra functions or processing

  In future versions, the __SerialDebug__ will more otimized, for CPU and memory

- It is good for __any__ Arduino

  Is good for new boards, that have good CPU and memory,
  like Espressif (ESP8266 and ESP32) and ARM arch (Due, Teensy, etc.).

  But it runs in older Arduino, as UNO, Leonardo, Mega, ...

  In UNO or similar (e.g. Leonardo),
  some features as Watches in debugger is not implemented,
  due full library is huge for it (more than 5k lines of code, without comments).

  For the Mega, some features are reduced, but have watches.

  If debugger is disabled in code,
  or in mode mininum (default for low memory boards),
   __SerialDebug__ in Uno,
  consumes only about 50 bytes of memory.
  And it not fully otimized yet.

  The default speed of serial is 250000, for Espressif, ARM or Mega boards
  and 115200 for UNO, Leonardo, etc.

  Only exception is boards with Tiny* AVR MCU, 
  due it not have CPU and memory to this library.

- Have __debug levels__

  During the development, we can put a lot of debug messages...

  But with __SerialDebug__, we can put a level in each one.

  For all messages (except any (debug\*A) or error (debug\*E)),
  the message only is processed and showed,
  if debug level is equal or higher than it level

  __SerialDebug__ have 7 debug levels, in order of priority:

  - Alway showed:

    - __Error__:    Critical errors
    - __Always__:   Important messages

  - No debug:

    - __None__:     No debug output

  - Another levels (showed if level is equal or higher that actual one):

    - __Warning__:  Error conditions but not critical
    - __Info__:     Information messages
    - __Debug__:    Extra information
    - __Verbose__:  More information than the usual  

  So We can change the level to Verbose, to see all messages.
  Or to Debug to see only debug or higher level, etc.

  Is very good to reduce a quantity of messages that a project can generate,
  in serial monitor.

- It is __easy__ to migrate

  From the 0.9.5 version, SerialDebug have a new macros to help migrate more easy

  For example: the code extracted from [www.ladyada.net](http://www.ladyada.net/learn/arduino/lesson4.html):

  ```cpp
  Serial.println("Here is some math: ");

  Serial.print("a = ");
  Serial.println(a);
  Serial.print("b = ");
  Serial.println(b);
  Serial.print("c = ");
  Serial.println(c);

  Serial.print("a + b = ");       // add
  Serial.println(a + b);

  Serial.print("a * c = ");       // multiply
  Serial.println(a * c);

  Serial.print("c / b = ");       // divide
  Serial.println(c / b);

  Serial.print("b - c = ");       // subtract
  Serial.println(b - c);
  ```

  Only replace __Serial.println__ to __printlnD__
  and replace __Serial.print__ to __printD__
  Note: this order of replaces is important.
  Note: D is to debug level, can be another, as V to verbose

  ```cpp
  printlnD("Here is some math: ");

  printD("a = ");
  printlnD(a);
  printD("b = ");
  printlnD(b);
  printD("c = ");
  printlnD(c);

  printD("a + b = ");       // add
  printlnD(a + b);

  printD("a * c = ");       // multiply
  printlnD(a * c);

  printD("c / b = ");       // divide
  printlnD(c / b);

  printD("b - c = ");       // subtract
  printlnD(b - c);
  ```

  __SerialDebug__ has a converter to help migrate your Arduino codes,
  from Serial.prints to this library.

  [SerialDebugConverter](https://github.com/JoaoLopesF/SerialDebugConverter)

- Have __printf__ support to serial

  Regardless of whether the board has it native or not.
  That I know, only Espressif boards have it native

  For example:

  ```cpp
    Serial.print("*** Example - varA = ");
    Serial.print(varA);
    Serial.print(" varB =  ");
    Serial.print(varB);
    Serial.print(" varC =  ");
    Serial.print(varC);
    Serial.println(); 
  ```
  Can be converted to a single command:

  ```cpp
  debugD("*** Example - varA = %d varB = %d varC = %d", varA, varB, varC);
  ````

  Improving the example of previous topic, w/ debug with printf formatter:

  ```cpp
  debugD("Here is some math: ");

  debugD("a = %d", a);
  debugD("b = %d", b);
  debugD("c = %d", c);

  debugD("a + b = %d", (a + b)); // add
  debugD("a * c = %d", (a * c)); // multiply
  debugD("c / b = %d", (c / b)); // divide
  debugD("b - c = %d", (b - c)); // subtract
  ```
  Note: 50% less code

  Note: With __debug*__ macros, __SerialDebug__ follows the same concept, 
  that modern debug/logging messages model,
  as ESP-IDF, Android, iOS, etc.
  In these, each call generates a formatted output line.

  Note: this is not for low memory boards as Arquino UNO

- Have __auto__ function name and simple __profiler__

  A simple debug:

  ```cpp
  debugV("* Run time: %02u:%02u:%02u (VERBOSE)", mRunHours, mRunMinutes, mRunSeconds);
  ````

  Can generate this output in serial monitor:

  ```txt
  (V p:^3065 loop C1) * Run time: 00:41:23 (VERBOSE)
  ```

        Where:  V: is the level
                p: is a profiler time, elased, between this and previous debug
                loop: is a function name, that executed this debug
                C1: is a core that executed this debug (and a function of this) (only for ESP32)
                The remaining is the message formatted (printf)

  Note how __printf__ is powerfull, %02u means a unsigned integer with minimum lenght of 2,
  and leading by zeros

  For ESP32, the core id in each debug is very good to optimizer multicore programming.

- Have __commands__ to execute from serial monitor

  __SerialDebug__ takes care of inputs from serial, and process predefined commands

  For example:

  - Show help (__?__)
  - Change the level of debug (__v__,__d__,__i__,__w__,__e__,__n__),
    to show less or more messages.
  - See memory (__m__)
  - Reset the board (__reset__)

    See about __SerialDebug__ commands below.

- Have a simple __software debugger__ 

  This starts disabled, to avoid extra overhead.

  If enabled (dbg command), you can command in serial monitor:

  - Show and change values of global variables
  - Call a function
  - Add or change watches for global variables

  It not have some features than a real hardware debugger,
  but is good features, for when yet have none of this ...
  It is for when not have a real hardware debugger, 
  e.g. GDB w/ JTAG, or not have skill on it.

  It can be disabled (no compile), if not want it.

  See about this below.

  Note: this is not for low memory boards as Arquino UNO
  
- Ready for __production__ (release compiler))

    For release your device, just uncomment DEBUG_DISABLED in your project
    Done this, and no more serial messages, or debug things. A
    And better for DEBUG_DISABLED, __SerialDebug__ have ZERO overhead, 
    due is nothing of this is compiled

## SerialDebugApp

__SerialDebugApp__ is a freeware desktop app, companion for this library.

Note: this library is not depending on this software,
you can use default serial monitor of Arduino IDE,
or another program.

Why this app is an freeware and not open source ?

This app is based in my another commercial projects,
through the serial port, control devices with the Arduino.

__SerialDebugApp__ is a serial monitor, made for this library:

- Show debug messages with different colors, depending on each level.

- Have buttons to most of commands of __SerialDebug__

- And more, filter, converter, auto-disconnection, etc.

__SerialDebugApp__ page: [SerialDebugApp](https://github.com/JoaoLopesF/SerialDebug/tree/master/SerialDebugApp)

## How it looks

__SerialDebug__ in Arduino serial monitor:

[![Youtube](http://img.youtube.com/vi/EfvF55Ww-lU/0.jpg)](https://www.youtube.com/watch?v=EfvF55Ww-lU)

__SerialDebug__ in __SerialDebugApp__:

[![Youtube](https://img.youtube.com/vi/ba_eu06mkng/0.jpg)](https://www.youtube.com/watch?v=ba_eu06mkng)

__SerialDebugApp__: new version, with simple software debugger on screen:

[![Youtube](https://img.youtube.com/vi/C4qRwwjyZwg/0.jpg)](https://www.youtube.com/watch?v=C4qRwwjyZwg)

## Commands

__SerialDebug__ takes care of inputs from serial, and process predefined commands as:

      ? or help -> display these help of commands
      m -> show free memory
      n -> set debug level to none
      v -> set debug level to verbose
      d -> set debug level to debug
      i -> set debug level to info
      w -> set debug level to warning
      e -> set debug level to errors
      s -> silence (Not to show anything else, good for analysis)
      p -> profiler:
        p      -> show time between actual and last message (in millis)
        p min  -> show only if time is this minimal
      r -> repeat last command (in each debugHandle)
        r ? -> to show more help 
      reset -> reset the Arduino board

      f -> call the function
          f ?  -> to show more help 

      dbg [on|off] -> enable/disable the simple software debugger

      Only if debugger is enabled:
          g -> see/change global variables
            g ?  -> to show more help 
          wa -> see/change watches for global variables
            wa ?  -> to show more help 

      Not yet implemented:
        gpio -> see/control gpio

For simple software debugger:

    - For functions:

      - To show help: f ?
      - To show: f [name|num]
      - To search with start of name (case insensitive): f name
      - To call it, just command: f [name|number] [arg]

    - Enable/disable the debugger:

      - To enable: dbg on
      - To disable: dbg off

      Note: the debugger starts disabled, to avoid extra overhead to processing it
      You can enable it when need 

    - For global variables:

      - To show help: g ?
      - To show: g [name|num]
      - To search by start of name: g name
      - To change global variable, g [name|number] = value [y]
        note: y is to confirm it (without confirm message)

    - For watches:

      - To show help: wa ?
      - To show: wa [num]
      - To add: wa a {global [name|number]} [==|!=|<|>|<=|>=|change] [value] [as]
        notes: change not need a value, and as -> set watch to stop always
      - To add cross (2 globals): wa ac {global [name|number]} [=|!=|<|>|<=|>=] {global [name|number]} [as]
      - To change: wa u {global [name|number]} [==|!=|<|>|<=|>=|change] [value] [as]
      - To change cross (not yet implemented)
      - To disable: wa d [num|all]
      - To enable: wa e [num|all]
      - To nonstop on watches: wa ns
      - To stop on watches: wa s

Notes:

- watches is not for low memory boards, as Uno.
- memory and reset, yet implemented only to AVR, Espressif, Teensy, and ARM arch (e.g. Arduino Due).
  
## Install

Just download or clone this repository.

You can use the library manager to install and update the library.

For install help, please see: [https://www.arduino.cc/en/Guide/Libraries](https://www.arduino.cc/en/Guide/Libraries)

Note: In some boards, after upload if you see only dirty characteres in serial monitor,
please reset the board. There is possibly some glitch in the serial monitor of Arduino

## Usage

### examples

    Please open the examples to see it working:

      - Basic -> for basic usage, without debugger

      - Advanced/Avr -> for Arduino AVR arch. uses F() to save memory

      - Advanded/Others -> for new boards, with enough memory,
                          not use F(), due RAM is more faster than Flash memory

      - Disabled -> example of how disable features, or entire SerialDebug

    Note: for low memory boards, as UNO, please open only the basic example.

To add __SerialDebug__ to your Arduino project:

### include

Place it, in top of code:

```cpp
#include "SerialDebug.h" //https://github.com/JoaoLopesF/SerialDebug
```

### setup

  Setup code is only necessary for __debugger__ elements.
  As this library not uses a hardware debugger,
  this codes are necessary to add this elements,
  into "simple software debugger" of SerialDebug. 

For example, for __functions__:

```cpp
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

if (debugAddFunctionVoid("benchSerial", &benchSerial) >= 0) {
  debugSetLastFunctionDescription("To benchmarks standard Serial debug");
}
if (debugAddFunctionVoid("benchSerialDebug", &benchSerialDebug) >= 0) {
  debugSetLastFunctionDescription("To benchmarks SerialDebug");
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
```

Or short use, if you not want descriptions:

```cpp
// Add functions that can called from SerialDebug

debugAddFunctionVoid("benchInt", &benchInt);
debugAddFunctionVoid("benchFloat", &benchFloat);
debugAddFunctionVoid("benchGpio", &benchGpio);
debugAddFunctionVoid("benchAll", &benchAll);
debugAddFunctionVoid("benchSerial", &benchSerial);
debugAddFunctionVoid("benchSerialDebug", &benchSerialDebug);
debugAddFunctionStr("funcArgStr", &funcArgStr);
debugAddFunctionChar("funcArgChar", &funcArgChar);
debugAddFunctionInt("funcArgInt", &funcArgInt);
debugSetLastFunctionDescription("To run with Integer arg");
```

Note: If it is for old boards, as UNO, Leornardo, etc.
You must use F() to save memory:

```cpp
// Add functions that can called from SerialDebug

debugAddFunctionVoid(F("benchInt"), &benchInt);
```

Notes: It is too for all examples showed below

For __global variables__ (note: only global ones):

```cpp

// Add global variables that can showed/changed from SerialDebug
// Note: Only globlal, if pass local for SerialDebug, can be dangerous

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

debugAddGlobalBoolean("mBoolean", &mBoolean);
debugAddGlobalChar("mChar",       &mChar);
debugAddGlobalByte("mByte",       &mByte);
debugAddGlobalInt("mInt",         &mInt);
debugAddGlobalUInt("mUInt",       &mUInt);
debugAddGlobalLong("mLong",       &mLong);
debugAddGlobalULong("mULong",     &mULong);
debugAddGlobalFloat("mFloat",     &mFloat);
debugAddGlobalDouble("mDouble",   &mDouble);

debugAddGlobalString("mString",   &mString);

// Note: For char arrays, not use the '&'

debugAddGlobalCharArray("mCharArray", mCharArray);

// Note, here inform to show only 20 characteres of this string or char array

debugAddGlobalString("mStringLarge", &mStringLarge, 20);

debugAddGlobalCharArray("mCharArrayLarge",
                  mCharArrayLarge, 20);

// For arrays, need add for each item (not use loop for it, due the name can not by a variable)
// Notes: Is good added arrays in last order, to help see another variables
//        In next versions, we can have a helper to do it in one command

debugAddGlobalInt("mIntArray[0]", &mIntArray[0]);
debugAddGlobalInt("mIntArray[1]", &mIntArray[1]);
debugAddGlobalInt("mIntArray[2]", &mIntArray[2]);
debugAddGlobalInt("mIntArray[3]", &mIntArray[3]);
debugAddGlobalInt("mIntArray[4]", &mIntArray[4]);

```

Note: Has a converter to do it for You: [SerialDebugConverter](https://github.com/JoaoLopesF/SerialDebugConverter)

And for __watches__ (not for low memory boards, as UNO):

```cpp
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
```

### loop

- In the begin of loop function

```cpp
// SerialDebug handle
// NOTE: if in inactive mode (until receive anything from serial),
// it show only messages of always or errors level type
// And the overhead during inactive mode is very much low

debugHandle();

```

### How use __SerialDebug__ macros

Instead _Serial.print*_, use __print*__ or __debug*__ macros:

  __print*__: easy to migrate and use, just replace each Serial.print for this

  __println*__: same, but add a new line on output

  __debug*__: w/ powerfull printf formatter, one command generate one serial line

See example of how convert it, in [Benefits](#benefits) topic below.

Using macros to show debug:

- For always show (for example in setup, here the debug output is disabled)

``` cpp
debugA("**** Setup: initialized.");

// or

printlnA("**** Setup: initialized.");
```

- For errors:

```cpp
debugE("* This is a message of debug level ERROR");

// or

printlnE("* This is a message of debug level ERROR");
```

- For another levels:

```cpp
debugV("* This is a message of debug level VERBOSE");
debugD("* This is a message of debug level DEBUG");
debugI("* This is a message of debug level INFO");
debugW("* This is a message of debug level WARNING");

// or

printlnV("* This is a message of debug level VERBOSE");
printlnD("* This is a message of debug level DEBUG");
printlnI("* This is a message of debug level INFO");
printlnW("* This is a message of debug level WARNING");

```

### printf formatting (for __debug*__ macros)

__SerialDebug__ use prinf native (for Espressif boards), 
or implements it in _depugPrintf_ function.

For Example:

```cpp
debugA("This is a always - var %02d", var);
debugV("This is a verbose - var %02d", var);
debugD("This is a debug - var %02d", var);
debugI("This is a information - var %02d", var);
debugW("This is a warning - var %02d", var);
debugE("This is a error - var %02d", var);
```

See more about printf formatting: [printf reference](http://www.cplusplus.com/reference/cstdio/printf/)

Notes:

- __SeriaDebug__ use the standard printf of Arduino

  For String variables, you must use the c_str() method:

```cpp
debugA("*** called with arg.: %s", str.c_str());


```

For AVR MCUs, as UNO, Leonardo, Mega and Esp8266,
no have support to %f (format floats)

If you need this, use:

```cpp
#ifndef ARDUINO_ARCH_AVR // Native float printf support
  debugV("mFloat = %0.3f", mFloat);
#else // For AVR and ESP8266, it is not supported, using String instead
  debugV("mFloat = %s", String(mFloat).c_str());
#endif
````

(in future versions of __SerialDebug__, can be have a better solution)

Note: this is only for __debug*__ macros , thats uses printf

For __print*_ macros, no need extra codes:

```cpp

printV("*** called with arg.: ");
printlnV(str);

printV("mFloat = ");
printlnV(mFloat);

```

## Watches

Watches is usefull to warning when the content of global variable,
is changed or reaches a certain condition.

How this works, without a real hardware debugger? :

- If have any watch (enabled),

- Is verified this content is changed,

- And is verified the watch,
  And trigger it, if value is changed, or the operation is true

This is done before each _debug*_ show messages or in _debugHandle_ function.

## Tutorial

Have a nice tutorial about __SerialDebug__ in [randomnerdtutorials.com](https://randomnerdtutorials.com/serialdebug-library-arduino-ide/):

  - Better Debugging for Arduino IDE: SerialDebug Library (Part 1): [access part 1](https://randomnerdtutorials.com/serialdebug-library-arduino-ide/)
  - Better Debugging for Arduino IDE using Software Debugger (Part 2): [access part 2](https://randomnerdtutorials.com/software-debugger-arduino-ide-serialdebug-library/)
  - Better Debugging for Arduino IDE: SerialDebugApp (Part 3): [access part 3](https://randomnerdtutorials.com/software-debugger-arduino-ide-serialdebug-library/)

Please access this tutorial, to give more information about how use __SerialDebug__ and __SerialDebugApp__.

## Khow issues

- Error on use debug* macros with F(). workaround for now: print* macros is ok for it.

## Releases

### 0.9.82 - 2018-11-25

    - corrected bug on debugHandleEvent
### 0.9.81 - 2018-11-16

    - print macros now support second arg, e.g.: printlnA(10, HEX);
      thanks to @wjwieland to open a issue about this.

### 0.9.80 - 2018-11-15

    - Few adjustments in header files

### 0.9.79 - 2018-11-06
  
    - Examples and README update

### 0.9.78 - 2018-10-28
  
    - Examples update

### 0.9.77 - 2018-10-26
  
    - Adjustments for minimum mode
    - #include stdarg, to avoid error in Arduino IDE 1.8.5 and Linux - thanks to @wd5gnr

### 0.9.76 - 2018-10-26

    -	#includes for Arduino.h corrected to work in Linux (case sensitive F.S.) - thanks @wd5gnr

### 0.9.75 - 2018-10-25
  
    -  Few Adjustments (bug on declare prototype)

### 0.9.74 - 2018-10-25	
  
    - Adjustments to SerialDebugApp show debugger info in App
    - Now low memory boards have debugger disabled by default, but enabled commands (debug level, help ...)
    - Create an mode minimum to low memory boards - only debug output enabled to save memory

### 0.9.73 - 2018-10-24

    - Adjustments to SerialDebugApp show debugger panel in App

### 0.9.72 - 2018-10-21

    - Corrected bug on basic example

### 0.9.71 - 2018-10-19

    - Just for new release, due problems on library.proprierties

### 0.9.7 - 2018-10-18

    - Checking if debugger is enabled

### 0.9.6	- 2018-10-09

    - New debug format output

### 0.9.5 - 2018-10-07

    - New print macros
    - Optimization on debugPrintf logic

### 0.9.4 - 2018-10-04

    - Now debugger starts disabled

### 0.9.3 - 2018-10-01

    - Few adjustments

### 0.9.2 - 2018-09-29

    - Few adjustments

### 0.9.1 - 2018-09-28

    - Few adjustments

### 0.9.0 - 2018-09-26

    - First beta

## Links

If you need a remote debug for ESP8266 or ESP32,
see my other library -> [RemoteDebug library](https://github.com/JoaoLopesF/RemoteDebug)

This library is made on Slober - the Eclipse for Arduino
See it in -> [Eclipse for Arduino](http://eclipse.baeyens.it/)

## Thanks

Special thanks to:

    - Arduino, for bring open hardware to us.

    - Good people, that work hard, to bring to us excellent open source,
      as libraries, examples, etc..
      That inspire me to make new libraries, as SerialDebug, RemoteDebug, etc.

    - Makers people, that works together as a big family.
