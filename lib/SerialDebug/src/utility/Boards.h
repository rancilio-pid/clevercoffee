/*
 * Boards.h
 *
 */

#ifndef LIBRARIES_SERIALDEBUG_UTILITY_BOARDS_H_
#define LIBRARIES_SERIALDEBUG_UTILITY_BOARDS_H_

// TODO: see ARDUINO_BOARD
// AVR Unsupported boards - based on avr_cpunames.h

#ifdef ARDUINO_ARCH_AVR
	#if defined (__AVR_AT94K__) || \
		defined (__AVR_ATmega128__) || \
		defined (__AVR_ATmega168__) || \
	    defined (__AVR_AT90CAN32__) || \
		defined (__AVR_AT90CAN64__) || \
		defined (__AVR_AT90CAN128__) || \
		defined (__AVR_ATmega64__) || \
		defined (__AVR_ATmega103__) || \
		defined (__AVR_ATmega406__) || \
		defined (__AVR_ATmega16__) || \
		defined (__AVR_ATmega8HVA__) || \
		defined (__AVR_ATmega16HVA__) || \
		defined (__AVR_ATmega8__) || \
		defined (__AVR_ATmega48__) || \
		defined (__AVR_ATmega48P__) || \
		defined (__AVR_ATmega88__) || \
		defined (__AVR_ATmega88P__) || \
		defined (__AVR_ATmega8515__) || \
		defined (__AVR_ATmega8535__) || \
		defined (__AVR_ATtiny22__) || \
		defined (__AVR_ATtiny26__) || \
		defined (__AVR_ATtiny2313__) || \
		defined (__AVR_ATtiny13__) || \
		defined (__AVR_ATtiny13A__) || \
		defined (__AVR_ATtiny25__) || \
		defined (__AVR_ATtiny45__) || \
		defined (__AVR_ATtiny85__) || \
		defined (__AVR_ATtiny24__) || \
		defined (__AVR_ATtiny44__) || \
		defined (__AVR_ATtiny84__) || \
		defined (__AVR_ATtiny261__) || \
		defined (__AVR_ATtiny461__) || \
		defined (__AVR_ATtiny861__) || \
		defined (__AVR_ATtiny43U__) || \
		defined (__AVR_ATtiny48__) || \
		defined (__AVR_ATtiny88__) || \
		defined (__AVR_ATtiny167__) || \
		defined (__AVR_ATmega8U2__)
		#error "SerialDebug: this board is unsupported"
	#endif
#endif

// Based on: https://arduino.stackexchange.com/questions/21137/arduino-how-to-get-the-board-type-in-code

#if defined(ESP8266) // ------------- Espressif ------------------
	#define BOARD F("ESP8266")
	#define BOARD_ENOUGH_MEMORY true
#elif defined(ESP32)
	#define BOARD F("ESP32")
	#define BOARD_ENOUGH_MEMORY true

#elif defined(TEENSYDUINO) //  --------------- Teensy -----------------

    #if defined(__AVR_ATmega32U4__)
        #define BOARD F("Teensy 2.0")
		#define BOARD_LOW_MEMORY true
    #elif defined(__AVR_AT90USB1286__)
        #define BOARD F("Teensy++ 2.0")
		#define BOARD_ENOUGH_MEMORY true
    #elif defined(__MK20DX128__)
        #define BOARD F("Teensy 3.0")
		#define BOARD_ENOUGH_MEMORY true
    #elif defined(__MK20DX256__)
        #define BOARD F("Teensy 3.1/3.2") // and Teensy 3.1 (obsolete)
		#define BOARD_ENOUGH_MEMORY true
    #elif defined(__MKL26Z64__)
        #define BOARD F("Teensy LC")
		#define BOARD_ENOUGH_MEMORY true
    #elif defined(__MK64FX512__)
        #define BOARD F("Teensy 3.5")
		#define BOARD_ENOUGH_MEMORY true
    #elif defined(__MK66FX1M0__)
		#define BOARD F("Teensy 3.6")
		#define BOARD_ENOUGH_MEMORY true
    #else
		#define BOARD F("Teensy - unknown board")
		#define BOARD_ENOUGH_MEMORY true
    #endif

#else // --------------- Arduino ------------------

    #if defined(ARDUINO_AVR_ADK)
        #define BOARD F("Mega Adk")
    #elif defined(ARDUINO_AVR_BT)    // Bluetooth
        #define BOARD F("Bt")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_DUEMILANOVE)
        #define BOARD F("Duemilanove")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_ESPLORA)
        #define BOARD F("Esplora")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_ETHERNET)
        #define BOARD F("Ethernet")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_FIO)
        #define BOARD F("Fio")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_GEMMA)
        #define BOARD F("Gemma")
		#error "SerialDebug: this board is unsupported")
    #elif defined(ARDUINO_AVR_LEONARDO)
        #define BOARD F("Leonardo")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_LILYPAD)
        #define BOARD F("Lilypad")
		#error "SerialDebug: this board is unsupported")
    #elif defined(ARDUINO_AVR_LILYPAD_USB)
        #define BOARD F("Lilypad Usb")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_MEGA)
        #define BOARD F("Mega")
    #elif defined(ARDUINO_AVR_MEGA2560)
        #define BOARD F("Mega 2560")
    #elif defined(ARDUINO_AVR_MICRO)
        #define BOARD F("Micro")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_MINI)
        #define BOARD F("Mini")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_NANO)
        #define BOARD F("Nano")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_NG)
        #define BOARD F("NG")
		#error "SerialDebug: this board is unsupported"
    #elif defined(ARDUINO_AVR_PRO)
        #define BOARD F("Pro")
		#error "SerialDebug: this board is unsupported"
    #elif defined(ARDUINO_AVR_ROBOT_CONTROL)
        #define BOARD F("Robot Ctrl")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_ROBOT_MOTOR)
        #define BOARD F("Robot Motor")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_UNO)
        #define BOARD F("Uno")
		#define BOARD_LOW_MEMORY true
    #elif defined(ARDUINO_AVR_YUN)
        #define BOARD F("Yun")
    // These boards must be installed separately:
    #elif defined(ARDUINO_SAM_DUE)
        #define BOARD F("Due")
		#define BOARD_ENOUGH_MEMORY true
    #elif defined(ARDUINO_SAMD_ZERO)
        #define BOARD F("Zero")
		#define BOARD_ENOUGH_MEMORY true
    #elif defined(ARDUINO_ARC32_TOOLS)
        #define BOARD F("101")
		#define BOARD_ENOUGH_MEMORY true
    #elif defined(__arm__)
        #define BOARD F("ARM generic")
		#define BOARD_ENOUGH_MEMORY true
    #else
    	#define BOARD F("Unknown board")
		#define BOARD_LOW_MEMORY true // For safe
    #endif

#endif

#endif /* LIBRARIES_SERIALDEBUG_UTILITY_BOARDS_H_ */
