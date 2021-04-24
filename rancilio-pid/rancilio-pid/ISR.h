/********************************************************
Timer 1 - ISR for PID calculation and heat realay output
******************************************************/

#ifdef ESP8266
	void ICACHE_RAM_ATTR onTimer1ISR() {
		timer1_write(6250); // set interrupt time to 20ms

		if (Output <= isrCounter) {
			digitalWrite(pinRelayHeater, LOW);
		} else {
			digitalWrite(pinRelayHeater, HIGH);
		}

		isrCounter += 20; // += 20 because one tick = 20ms

		//set PID output as relay command
		if (isrCounter > windowSize) {
			isrCounter = 0;
		}
	}

#elif defined(ESP32)
	void IRAM_ATTR onTimer() {
		if (Output <= isrCounter) {
			digitalWrite(pinRelayHeater, LOW);
		} else {
			digitalWrite(pinRelayHeater, HIGH);
		}

		isrCounter += 10; // += 10 because one tick = 10ms

		//set PID output as relay command
		if (isrCounter > windowSize) {
			isrCounter = 0;
		}
	}
#endif
