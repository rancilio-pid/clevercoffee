/*	ZACwire - Library for reading temperature sensors TSIC 206/306/506
	created by Adrian Immer in 2020
	v1.0.2 stable
*/

#ifndef ZACwire_h
#define ZACwire_h

#include "Arduino.h"

template <uint8_t pin>
class ZACwire {
	
  public:
  
  	ZACwire(int Sensortype = 306){
  		_Sensortype = Sensortype;
  	}
	
	bool begin() {			//start reading sensor, needs to be called 100ms before the first getTemp()
	  pinMode(pin, INPUT);
	  bitWindow = 125;		//bitWindow is twice the Tstrobe
	  deltaMicrotime = micros();
	  if (!pulseInLong(pin, LOW)) return false;
	  isrPin = digitalPinToInterrupt(pin);
	  if (isrPin == -1) return false;
	  attachInterrupt(isrPin, read, RISING);
	  return true;
  	}
  
	float getTemp() {	    	//gives back temperature in °C
		if (!bitWindow) {	//check if begin() was already called
			begin();
			delay(110);
		}
		byte parity1 = 0, parity2 = 0, timeout = 10;
		while (BitCounter != 20 && --timeout) delay(1);
		noInterrupts();  				//no ISRs because tempValue might change during reading
		uint16_t tempHigh = tempValue[0];		//get high significant bits from ISR
		uint16_t tempLow = tempValue[1];		//get low significant bits from ISR
		byte newBitWindow = ((ByteTime << 5) + (ByteTime << 4) + ByteTime >> 9) + 20;
		if (abs(bitWindow-newBitWindow) < 20) bitWindow += (newBitWindow >> 3) - (bitWindow >> 3);	//adjust bitWindow time, which varies with rising temperature
		interrupts();
		for (byte i = 0; i < 9; ++i) {
		  if (tempHigh & (1 << i)) ++parity1;
		  if (tempLow & (1 << i)) ++parity2;
		}
		if (timeout && tempHigh | tempLow && ~(parity1 | parity2) & 1) {       // check for failure
			tempHigh >>= 1;               	// delete parity bits
			tempLow >>= 1;
			tempLow += tempHigh << 8;		//joints high and low significant figures
			if (_Sensortype < 400) return (float(tempLow * 250L >> 8) - 499) / 10;	//calculates °C
			else return (float(tempLow * 175L >> 9) - 99) / 10;
		}
		else return 222;	//set to 222 if reading failed
  	}
  
  	void end() {			//stop reading -> for time critical tasks
  		for (byte timeout = 10; BitCounter != 20 && timeout; --timeout) delay(1);
  		detachInterrupt(isrPin);
  	}

  private:
  
  	static void ICACHE_RAM_ATTR read() {			//gets called with every rising edge
		unsigned long microtime = micros();
  		deltaMicrotime = microtime - deltaMicrotime;	//measure time to previous rising edge
  		if (deltaMicrotime > 1000) {		  	//if last reading a long time ago -> begin new reading cycle
  			ByteTime = microtime;			//for measuring Tstrobe/bitWindow
  			BitCounter = ByteNr = tempValue[0] = tempValue[1] = 0;
  		}
		if (++BitCounter == 11 && !ByteNr) {		//after stop bit
			ByteTime = microtime - ByteTime;
			ByteNr = 1;
		}
		tempValue[ByteNr] <<= 1;
		if (deltaMicrotime > bitWindow);		//Logic 0
		else if (deltaMicrotime < bitWindow - 40 || tempValue[ByteNr] & 2) tempValue[ByteNr] |= 1;	//Logic 1
  		deltaMicrotime = microtime;
  	}
	
  	int isrPin;
  	int _Sensortype;					//either 206, 306 or 506
  	static volatile byte BitCounter;
  	static volatile unsigned long ByteTime;
  	static volatile uint16_t tempValue[2];
  	static unsigned long deltaMicrotime;
  	static byte bitWindow;
  	static bool ByteNr;
};

template<uint8_t pin>
volatile byte ZACwire<pin>::BitCounter;
template<uint8_t pin>
volatile unsigned long ZACwire<pin>::ByteTime;
template<uint8_t pin>
volatile uint16_t ZACwire<pin>::tempValue[2];
template<uint8_t pin>
unsigned long ZACwire<pin>::deltaMicrotime;
template<uint8_t pin>
byte ZACwire<pin>::bitWindow;
template<uint8_t pin>
bool ZACwire<pin>::ByteNr;

#endif
