/********************************************************
    Timer 1 - ISR for PID calculation and heat realay output
******************************************************/







#if defined(ESP8266) // ESP8266
    
    void ICACHE_RAM_ATTR onTimer1ISR() {
    timer1_write(6250); // set interrupt time to 20ms

    if (Output <= isrCounter) {
        digitalWrite(pinRelayHeater, LOW);
    } else {
        digitalWrite(pinRelayHeater, HIGH);
    }

    isrCounter += 20; // += 20 because one tick = 20ms
    //set PID output as relais commands
    if (isrCounter > windowSize) {
        isrCounter = 0;
    }

    //run PID calculation
    bPID.Compute();
    }
#endif

#if defined(ESP32) // ESP32
  void IRAM_ATTR onTimer(){
    
    //timer1_write(50000); // set interrupt time to 10ms
      timerAlarmWrite(timer, 10000, true);
    if (Output <= isrCounter) {
      digitalWrite(pinRelayHeater, LOW);
      DEBUG_println("Power off!");
    } else {
      digitalWrite(pinRelayHeater, HIGH);
      DEBUG_println("Power on!");
    }
  
    isrCounter += 10; // += 10 because one tick = 10ms
    //set PID output as relais commands
    if (isrCounter > windowSize) {
      isrCounter = 0;
    }
  
    //run PID calculation
    bPID.Compute();
  }

 #endif   
