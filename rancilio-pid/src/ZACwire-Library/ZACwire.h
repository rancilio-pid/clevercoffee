/*	ZACwire - Library for reading temperature sensors TSIC 206/306/506
	created by Adrian Immer in 2020
	v1.1.2 stable
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
	  deltaTime = micros();
	  if (!pulseInLong(pin, LOW)) return false;
	  isrPin = digitalPinToInterrupt(pin);
	  if (isrPin == -1) return false;
	  attachInterrupt(isrPin, read, RISING);
	  return true;
  	}
  
	float getTemp() {	    	//gives back temperature in °C
		static bool misreading = false;
		static byte newBitWindow;
		byte parity1 = 0, parity2 = 0;
		if (!bitWindow) {	//check if begin() was already called
			begin();
			delay(110);
		}
		noInterrupts();  				//no ISRs because tempValue might change during reading
		if (BitCounter != 20) misreading = true;
		else newBitWindow = ((ByteTime << 5) + (ByteTime << 4) + ByteTime >> 9) + 20;	//+20 found out by trial and error
		uint16_t tempHigh = tempValue[0][backUP^misreading];		//get high significant bits from ISR
		uint16_t tempLow = tempValue[1][backUP^misreading];		//get low significant bits from ISR
		interrupts();
		if (abs(bitWindow-newBitWindow) < 20) bitWindow += (newBitWindow >> 3) - (bitWindow >> 3);	//adjust bitWindow time, which varies with rising temperature
		for (byte i = 0; i < 9; ++i) {
		  if (tempHigh & 1 << i) ++parity1;
		  if (tempLow & 1 << i) ++parity2;
		}
		if (tempHigh | tempLow && ~(parity1 | parity2) & 1) {       // check for failure
			tempHigh >>= 1;               	// delete parity bits
			tempLow >>= 1;
			tempLow |= tempHigh << 8;		//joints high and low significant figures
			misreading = false;
			if (_Sensortype < 400) return (float(tempLow * 250L >> 8) - 499) / 10;	//calculates °C
			else return (float(tempLow * 175L >> 9) - 99) / 10;
		}
		else if (!misreading) {
			misreading = true;
			getTemp();
		}
		else {
			misreading = false;
			return 222;		//set to 222 if reading failed
		}
  	}
  
  	void end() {			//stop reading
  		detachInterrupt(isrPin);
  	}

  private:
  
  	static void ICACHE_RAM_ATTR read() {			//gets called with every rising edge
		static bool ByteNr;
		unsigned long microtime = micros();
  		deltaTime = microtime - deltaTime;	//measure time to previous rising edge
  		if (deltaTime > 1000) {		  	//true at start bit
  			ByteTime = microtime;			//for measuring Tstrobe/bitWindow
			backUP = !backUP;
  			BitCounter = ByteNr = tempValue[0][backUP] = tempValue[1][backUP] = 0;
  		}
		if (++BitCounter == 11) {		//after stop bit
			ByteTime = microtime - ByteTime;
			ByteNr = 1;
		}
		tempValue[ByteNr][backUP] <<= 1;
		if (deltaTime > bitWindow);		//Logic 0
		else if (deltaTime < bitWindow - 40 || tempValue[ByteNr][backUP] & 2) tempValue[ByteNr][backUP] |= 1;	//Logic 1
  		deltaTime = microtime;
  	}
	
  	int isrPin;
  	int _Sensortype;					//either 206, 306 or 506
  	static volatile byte BitCounter;
  	static volatile unsigned long ByteTime;
  	static volatile uint16_t tempValue[2][2];
  	static unsigned long deltaTime;
  	static byte bitWindow;
  	static volatile bool backUP;
};

template<uint8_t pin>
volatile byte ZACwire<pin>::BitCounter;
template<uint8_t pin>
volatile unsigned long ZACwire<pin>::ByteTime;
template<uint8_t pin>
volatile bool ZACwire<pin>::backUP;
template<uint8_t pin>
volatile uint16_t ZACwire<pin>::tempValue[2][2];
template<uint8_t pin>
unsigned long ZACwire<pin>::deltaTime;
template<uint8_t pin>
byte ZACwire<pin>::bitWindow = 0;

#endif
