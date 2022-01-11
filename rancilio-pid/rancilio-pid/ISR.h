/********************************************************
    Timer 1 - ISR for PID calculation and heat realay output
******************************************************/

#include <Arduino.h>

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
    if (isrCounter >= windowSize) {
        isrCounter = 0;
    }

    //run PID calculation
    bPID.Compute();
    }
#endif

#if defined(ESP32) // ESP32

int T1C;
int TCTE;

  void IRAM_ATTR onTimer(){
    
    //timer1_write(50000); // set interrupt time to 10ms
      timerAlarmWrite(timer, 10000, true);
    if (Output <= isrCounter) {
      digitalWrite(pinRelayHeater, LOW);
    } else {
      digitalWrite(pinRelayHeater, HIGH);
    }
  
    isrCounter += 10; // += 10 because one tick = 10ms
    //set PID output as relais commands
    if (isrCounter >= windowSize) {
      isrCounter = 0;
    }
  
    //run PID calculation
    bPID.Compute();
  }

 #endif   


void initTimer1(void)
{
  #if defined(ESP8266)
  /********************************************************
    Timer1 ISR - Initialisierung
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  ******************************************************/
  timer1_isr_init();
  timer1_attachInterrupt(onTimer1ISR);
  //timer1_write(50000); // DIV16: set interrupt time to 10ms
  timer1_write(6250); // DIV256: set interrupt time to 20ms
  #elif defined(ESP32) // ESP32
  /********************************************************
    Timer1 ISR - Initialisierung
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  ******************************************************/
  timer = timerBegin(0, 80, true); //m
  timerAttachInterrupt(timer, &onTimer, true);//m
  timerAlarmWrite(timer, 10000, true);//m
  #else
  #error("not supported MCU");
  #endif
}



void enableTimer1(void)
{
  #if defined(ESP8266)
  //timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
  #elif defined(ESP32) // ESP32
  timerAlarmEnable(timer);
  #else
  #error("not supported MCU");
  #endif
}


void disableTimer1(void)
{
  #if defined(ESP8266)
  timer1_disable();
  #elif defined(ESP32) // ESP32
  timerAlarmDisable(timer);
  #else
  #error("not supported MCU");
  #endif
}


bool isTimer1Enabled(void)
{
  return ((T1C & (1 << TCTE)) != 0);
}
