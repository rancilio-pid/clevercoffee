
#include "TSIC.h"       // include the library

TSIC Sensor1(2, NO_VCC_PIN, TSIC_30x); 
uint16_t temperature = 0;
float Temperatur_C = 0;
float Input = 0;

int enable_tic_interrupt = 1;
unsigned long previousMillistemp;       // initialisation at the end of init()
const long refreshTempInterval = 100;  //How often to read the temperature sensor
unsigned long best_time_to_call_refreshTemp = refreshTempInterval;
unsigned int estimated_cycle_refreshTemp = 25;  // for my TSIC the hardware refresh happens every 76ms
int tsic_validate_count = 0;
int tsic_stable_count = 0;
unsigned int estimated_cycle_refreshTemp_stable_next_save = 1;


char debugline[120];
unsigned long loops = 0;
unsigned long max_micros = 0;
unsigned long last_report_micros = 0;
unsigned long cur_micros_previous_loop = 0;
const unsigned long loop_report_count = 100;

int tsic_sleep_counter = 0;
volatile uint16_t disable_tsic_isr = 0 ;

void loop() {
  loops += 1 ;
  unsigned long cur_micros = micros();
  if (max_micros < cur_micros-cur_micros_previous_loop) {
      max_micros = cur_micros-cur_micros_previous_loop;
  }

  if ( cur_micros >= last_report_micros + 10000 ) {
    snprintf(debugline, sizeof(debugline), "%lu loop() temp=%0.2f | loops/ms=%lu |cur_micros=%lu | max_micros=%lu | avg_micros=%lu\n", 
        cur_micros/1000, Input, loops/10, (cur_micros-cur_micros_previous_loop), max_micros, (cur_micros - last_report_micros)/loops );
    Serial.print(debugline);
    //Serial.println(max_micros);
    //Serial.println(loops/10);
    last_report_micros = cur_micros;
    max_micros = 0;
    loops=0;
    
  }
  cur_micros_previous_loop = cur_micros; // micros();

  unsigned long currentMillistemp = millis();
  if (enable_tic_interrupt == 1) {
      //interrupt way
      if (currentMillistemp >= previousMillistemp + refreshTempInterval)
      {
        previousMillistemp = currentMillistemp;
        //unsigned long start = millis();
        Temperatur_C = getTSICvalue();
        //unsigned long stop = millis();
        //snprintf(debugline, sizeof(debugline), "temp=%0.2f |  time_spend=%0.2f\n", Temperatur_C, stop-start);
        //Serial.print(debugline);
        if (Temperatur_C > 0) {
            Input = Temperatur_C;
        } else {
          Serial.print("sensor error\n");
        }

        
        if ( tsic_sleep_counter <= 0 ) {
          disable_tsic_isr = 0;
          tsic_sleep_counter = 5;
        } else {
          disable_tsic_isr = 1;
        }
        tsic_sleep_counter--;

    
      }
        
    } else {
      //auto-tune way
      if (currentMillistemp >= best_time_to_call_refreshTemp)
      {
        previousMillistemp = currentMillistemp;
        temperature = 0;
        unsigned long start = millis();
        Sensor1.getTemperature(&temperature);
        unsigned long stop = millis();
        // temperature must be between 0x000 and 0x7FF(=DEC2047)
        Temperatur_C = Sensor1.calc_Celsius(&temperature);

        if (Temperatur_C > 0) {
          Input = Temperatur_C;
        } else {
          stop = start + 999;
          Serial.print("sensor error\n");
        }
        if ( 0 == 1 ) {  //switch
          tsicAutoTune(start, stop);
        } else {
          //enable old way
          best_time_to_call_refreshTemp = previousMillistemp + refreshTempInterval;
        }

      }
    }
    
}


/*****************************************************
 * fast temperature reading with TSIC 306
 * Code by Adrian with minor adaptions
******************************************************/
volatile uint16_t temp_value[2] = {0};
unsigned long readTSIC_max = 0;
//TODO: only activate ISR when there is no chance for pid.compute()

void ICACHE_RAM_ATTR readTSIC() {                    //executed every 100ms by interrupt
  if (disable_tsic_isr) return;
  unsigned long start = micros();
  unsigned long diff = start;
  byte strobelength = 6;
  byte timeout = 0;
  for (byte ByteNr=0; ByteNr<2; ++ByteNr) {
    diff = micros() - start;
    if (ByteNr) {                                    //measure strobetime between bytes
        for (timeout = 0; digitalRead(2); ++timeout){
          delayMicroseconds(10);
          if (timeout > 20) return;
        }
        strobelength = 0;
        for (timeout = 0; !digitalRead(2); ++timeout) {    // wait for rising edge
          ++strobelength;
          delayMicroseconds(10);
          if (timeout > 20) return;
        }
    }
    for (byte i=0; i<9; ++i) {
        for (timeout = 0; digitalRead(2); ++timeout) {    // wait for falling edge
          delayMicroseconds(10);
          if (timeout > 20) return;
        }
        if (!i) temp_value[ByteNr] = 0;            //reset byte before start measuring
        delayMicroseconds(10 * strobelength);
        temp_value[ByteNr] <<= 1;
        if (digitalRead(2)) temp_value[ByteNr] |= 1;         // Read bit
        for (timeout = 0; !digitalRead(2); ++timeout) {     // wait for rising edge
          delayMicroseconds(10);
          if (timeout > 20) return;
        }
    }
  }
  //unsigned long diff = micros() - start;
  if (diff > readTSIC_max && millis() > 10000) {
    readTSIC_max = diff;
    //DEBUG_print("readTSIC: spend_time=%ul\n", readTSIC_max);
  }
  //DEBUG_print("readTSIC: spend_time=%ul (max=%ul)\n", diff, readTSIC_max);
}


double getTSICvalue() {
    byte parity1 = 0;
    byte parity2 = 0;
    noInterrupts();                               //no ISRs because temp_value might change during reading
    uint16_t temperature1 = temp_value[0];        //get high significant bits from ISR
    uint16_t temperature2 = temp_value[1];        //get low significant bits from ISR
    interrupts();
    for (uint8_t i = 0; i < 9; ++i) {
      if (temperature1 & (1 << i)) ++parity1;
      if (temperature2 & (1 << i)) ++parity2;
    }
    if (!(parity1 % 2) && !(parity2 % 2)) {       // check parities
      temperature1 >>= 1;                           // delete parity bits
      temperature2 >>= 1;
      temperature = (temperature1 << 8) + temperature2;        //joints high and low significant figures
      return (float((temperature * 250L) >> 8) - 500) / 10;
      //return (float) (((temperature * 250L) >> 8) - 500) / 10.0;    //calculates Temperature in Celsius
      //return (((temperature * 250L) >> 8) - 500) / 10 + (float) ((((temperature * 250L) >> 8) - 500) % 10) / 10; 
    }
    else return -50;    //set to -50 if reading failed
}



void tsicAutoTune(unsigned long start, unsigned long stop) {
  const int auto_tuning_enable_refreshTemp = 15000; //after how many seconds since start should be begin tuning
  const int spend_time_threshold = 16;  //this might not yet be optimally choosen
  unsigned long spend_time=stop-start;
  if (start > auto_tuning_enable_refreshTemp) {
    if (spend_time <= 3 || spend_time >= spend_time_threshold) {
      if (tsic_stable_count <= 5) {
        if (tsic_validate_count == 0) {
          if (estimated_cycle_refreshTemp < 95) {
            if (spend_time >= 40) { //room for improvements
              estimated_cycle_refreshTemp += 2;
            } else {
              estimated_cycle_refreshTemp += 1;
            }
            tsic_validate_count = 2;     //should be an even number to trigger flapping
          } else {
            estimated_cycle_refreshTemp = 25;
          }
          tsic_stable_count = 0;
        } else {
          tsic_validate_count -= 1;
        }
      } else {
        if (tsic_stable_count >= 1) {
          tsic_stable_count -= 1;
        }
        if (tsic_stable_count == 5) {
          //DEBUG_print("estimated_cycle_refreshTemp: unstable=%u (time_spend_reading_sensor=%lu)\n", estimated_cycle_refreshTemp, spend_time);
          tsic_stable_count = 0;
        }
      }
      estimated_cycle_refreshTemp_stable_next_save = millis() + 900000;  //15min timer
    } else {  //we are in target range
      if (tsic_stable_count <= 10) {
            tsic_stable_count += 1;
      } else {
        tsic_validate_count = 0;
        if (estimated_cycle_refreshTemp_stable_next_save != 0 && (millis() >= estimated_cycle_refreshTemp_stable_next_save)) {
          estimated_cycle_refreshTemp_stable_next_save = 0;
          //DEBUG_print("estimated_cycle_refreshTemp: stable=%u (time_spend_reading_sensor=%lu)\n", estimated_cycle_refreshTemp, spend_time);
          //noInterrupts();
          //sync_eeprom();  //TODO do we really need to save it
          //interrupts();
        }
      }
    }
    best_time_to_call_refreshTemp = millis() + (round((float)refreshTempInterval / estimated_cycle_refreshTemp)) *estimated_cycle_refreshTemp;
  } else {
    best_time_to_call_refreshTemp = millis() + refreshTempInterval;
  }
  /* Uncomment to debug 
  if (stop-start > spend_time_threshold) {
    DEBUG_print("TSIC Auto-Tune: next_refreshTemp=%lu(#%u) | Temperatur_C=%0.3f | time_spend_reading_sensor=%lu\n", 
               best_time_to_call_refreshTemp -  millis(), estimated_cycle_refreshTemp, Temperatur_C, spend_time);
  }
  */
}

void enable_tsic() {
  attachInterrupt(digitalPinToInterrupt(2), readTSIC, RISING);
}

void disable_tsic() {
  detachInterrupt(digitalPinToInterrupt(2));
}


void setup() {
  Serial.begin(115200); // set up the serial port
  delay(500);
  
  if (enable_tic_interrupt == 1) {
      attachInterrupt(digitalPinToInterrupt(2), readTSIC, RISING);    //activate TSIC reading
  }
}


