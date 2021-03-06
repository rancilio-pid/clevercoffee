#include <Adafruit_VL53L0X.h>
#include <Wire.h>

// Define which Wire objects to use, may depend on platform
// or on your configurations.
#define SENSOR1_WIRE Wire
#define SENSOR2_WIRE Wire
#if defined(WIRE_IMPLEMENT_WIRE1)
#define SENSOR3_WIRE Wire1
#define SENSOR4_WIRE Wire1
#else
#define SENSOR3_WIRE Wire
#define SENSOR4_WIRE Wire
#endif
// Setup mode for doing reads
typedef enum {
  RUN_MODE_DEFAULT = 1,
  RUN_MODE_ASYNC,
  RUN_MODE_GPIO,
  RUN_MODE_CONT
} runmode_t;

runmode_t run_mode = RUN_MODE_DEFAULT;
uint8_t show_command_list = 1;

typedef struct {
  Adafruit_VL53L0X *psensor; // pointer to object
  TwoWire *pwire;
  int id;            // id for the sensor
  int shutdown_pin;  // which pin for shutdown;
  int interrupt_pin; // which pin to use for interrupts.
  Adafruit_VL53L0X::VL53L0X_Sense_config_t
      sensor_config;     // options for how to use the sensor
  uint16_t range;        // range value used in continuous mode stuff.
  uint8_t sensor_status; // status from last ranging in continous.
} sensorList_t;

// Actual object, could probalby include in structure above61
Adafruit_VL53L0X sensor1;
Adafruit_VL53L0X sensor2;
#ifndef ARDUINO_ARCH_AVR // not enough memory on uno for 4 objects
Adafruit_VL53L0X sensor3;
Adafruit_VL53L0X sensor4;
#endif
// Setup for 4 sensors
sensorList_t sensors[] = {
#ifndef ARDUINO_ARCH_AVR // not enough memory on uno for 4 objects
    {&sensor1, &SENSOR1_WIRE, 0x30, 0, 1,
     Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE, 0, 0},
    {&sensor2, &SENSOR2_WIRE, 0x31, 2, 3,
     Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED, 0, 0},
    {&sensor3, &SENSOR3_WIRE, 0x32, 4, 5,
     Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT, 0, 0},
    {&sensor4, &SENSOR4_WIRE, 0x33, 6, 7,
     Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT, 0, 0}
#else
    // AVR sensors move to other pins
    {&sensor1, &SENSOR1_WIRE, 0x30, 6, 8,
     Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE, 0, 0},
    {&sensor2, &SENSOR2_WIRE, 0x31, 7, 9,
     Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED, 0, 0},
#endif
};

const int COUNT_SENSORS = sizeof(sensors) / sizeof(sensors[0]);

const uint16_t ALL_SENSORS_PENDING = ((1 << COUNT_SENSORS) - 1);
uint16_t sensors_pending = ALL_SENSORS_PENDING;
uint32_t sensor_last_cycle_time;

/*
    Reset all sensors by setting all of their XSHUT pins low for delay(10), then
   set all XSHUT high to bring out of reset
    Keep sensor #1 awake by keeping XSHUT pin high
    Put all other sensors into shutdown by pulling XSHUT pins low
    Initialize sensor #1 with lox.begin(new_i2c_address) Pick any number but
   0x29 and it must be under 0x7F. Going with 0x30 to 0x3F is probably OK.
    Keep sensor #1 awake, and now bring sensor #2 out of reset by setting its
   XSHUT pin high.
    Initialize sensor #2 with lox.begin(new_i2c_address) Pick any number but
   0x29 and whatever you set the first sensor to
*/
void Initialize_sensors() {
  bool found_any_sensors = false;
  // Set all shutdown pins low to shutdown sensors
  for (int i = 0; i < COUNT_SENSORS; i++)
    digitalWrite(sensors[i].shutdown_pin, LOW);
  delay(10);

  for (int i = 0; i < COUNT_SENSORS; i++) {
    // one by one enable sensors and set their ID
    digitalWrite(sensors[i].shutdown_pin, HIGH);
    delay(10); // give time to wake up.
    if (sensors[i].psensor->begin(sensors[i].id, false, sensors[i].pwire,
                                  sensors[i].sensor_config)) {
      found_any_sensors = true;
    } else {
      Serial.print(i, DEC);
      Serial.print(F(": failed to start\n"));
    }
  }
  if (!found_any_sensors) {
    Serial.println("No valid sensors found");
    while (1)
      ;
  }
}
//====================================================================
// Simple Sync read sensors.
//====================================================================
void read_sensors() {
  // First use simple function
  uint16_t ranges_mm[COUNT_SENSORS];
  bool timeouts[COUNT_SENSORS];
  uint32_t stop_times[COUNT_SENSORS];

  digitalWrite(13, HIGH);
  uint32_t start_time = millis();
  for (int i = 0; i < COUNT_SENSORS; i++) {
    ranges_mm[i] = sensors[i].psensor->readRange();
    timeouts[i] = sensors[i].psensor->timeoutOccurred();
    stop_times[i] = millis();
  }
  uint32_t delta_time = millis() - start_time;
  digitalWrite(13, LOW);

  Serial.print(delta_time, DEC);
  Serial.print(F(" "));
  for (int i = 0; i < COUNT_SENSORS; i++) {
    Serial.print(i, DEC);
    Serial.print(F(":"));
    Serial.print(ranges_mm[i], DEC);
    Serial.print(F(" "));
    Serial.print(stop_times[i] - start_time, DEC);
    if (timeouts[i])
      Serial.print(F("(TIMEOUT) "));
    else
      Serial.print(F("          "));
    start_time = stop_times[i];
  }
  Serial.println();
}

//====================================================================
// ASync read sensors.
//====================================================================
void timed_async_read_sensors() {
  // First use simple function
  uint16_t ranges_mm[COUNT_SENSORS];
  bool timeouts[COUNT_SENSORS];
  uint32_t stop_times[COUNT_SENSORS];

  digitalWrite(13, HIGH);
  uint32_t start_time = millis();

  // Tell all sensors to start.
  for (int i = 0; i < COUNT_SENSORS; i++) {
    ranges_mm[i] = sensors[i].psensor->startRange();
  }
  // We could call to see if done, but this version the readRange will wait
  // until ready
  for (int i = 0; i < COUNT_SENSORS; i++) {
    ranges_mm[i] = sensors[i].psensor->readRangeResult();
    timeouts[i] = sensors[i].psensor->timeoutOccurred();
    stop_times[i] = millis();
  }
  uint32_t delta_time = millis() - start_time;
  digitalWrite(13, LOW);

  Serial.print(delta_time, DEC);
  Serial.print(F(" "));
  for (int i = 0; i < COUNT_SENSORS; i++) {
    Serial.print(i, DEC);
    Serial.print(F(":"));
    Serial.print(ranges_mm[i], DEC);
    Serial.print(F(" "));
    Serial.print(stop_times[i] - start_time, DEC);
    if (timeouts[i])
      Serial.print(F("(TIMEOUT) "));
    else
      Serial.print(F("          "));
    start_time = stop_times[i];
  }
  Serial.println();
}

//====================================================================
// ASync read sensors.
//====================================================================
void timed_async_read_gpio() {
  // First use simple function
  uint16_t ranges_mm[COUNT_SENSORS];
  bool timeouts[COUNT_SENSORS];
  uint32_t stop_times[COUNT_SENSORS];

  digitalWrite(13, HIGH);
  uint32_t start_time = millis();
  uint32_t mask = 1;

  // Tell all sensors to start.
  for (int i = 0; i < COUNT_SENSORS; i++) {
    ranges_mm[i] = sensors[i].psensor->startRange();
    sensors_pending |= mask;
    mask <<= 1;
  }

  // lets play by reading the different GPIO pins until we find all of them went
  // low.
  while (sensors_pending &&
         ((millis() - start_time) < 1000)) { // break if not all after a second
    // We could call to see if done, but this version the readRange will wait
    // until ready
    mask = 1;
    for (int i = 0; i < COUNT_SENSORS; i++) {
      if ((sensors_pending & mask) && !digitalRead(sensors[i].interrupt_pin)) {
        ranges_mm[i] = sensors[i].psensor->readRangeResult();
        timeouts[i] = sensors[i].psensor->timeoutOccurred();
        stop_times[i] = millis();
        sensors_pending ^= mask;
      }
      mask <<= 1;
    }
  }

  uint32_t delta_time = millis() - start_time;
  digitalWrite(13, LOW);

  Serial.print(delta_time, DEC);
  Serial.print(F(" "));
  for (int i = 0; i < COUNT_SENSORS; i++) {
    Serial.print(i, DEC);
    Serial.print(F(":"));
    Serial.print(ranges_mm[i], DEC);
    Serial.print(F(" "));
    Serial.print(stop_times[i] - start_time, DEC);
    if (timeouts[i])
      Serial.print(F("(TIMEOUT) "));
    else
      Serial.print(F("          "));
    start_time = stop_times[i];
  }
  Serial.println();
}
//===============================================================
// Continuous range test code
//===============================================================

void start_continuous_range(uint16_t cycle_time) {
  if (cycle_time == 0)
    cycle_time = 100;
  Serial.print(F("start Continuous range mode cycle time: "));
  Serial.println(cycle_time, DEC);
  for (uint8_t i = 0; i < COUNT_SENSORS; i++) {
    sensors[i].psensor->startRangeContinuous(cycle_time); // do 100ms cycle
  }
  sensors_pending = ALL_SENSORS_PENDING;
  sensor_last_cycle_time = millis();
}

void stop_continuous_range() {
  Serial.println(F("Stop Continuous range mode"));
  for (uint8_t i = 0; i < COUNT_SENSORS; i++) {
    sensors[i].psensor->stopRangeContinuous();
  }
  delay(100); // give time for it to complete.
}

void Process_continuous_range() {

  uint16_t mask = 1;
  for (uint8_t i = 0; i < COUNT_SENSORS; i++) {
    bool range_complete = false;
    if (sensors_pending & mask) {
      if (sensors[i].interrupt_pin >= 0)
        range_complete = !digitalRead(sensors[i].interrupt_pin);
      else
        range_complete = sensors[i].psensor->isRangeComplete();
      if (range_complete) {
        sensors[i].range = sensors[i].psensor->readRangeResult();
        sensors[i].sensor_status = sensors[i].psensor->readRangeStatus();
        sensors_pending ^= mask;
      }
    }
    mask <<= 1; // setup to test next one
  }
  // See if we have all of our sensors read OK
  uint32_t delta_time = millis() - sensor_last_cycle_time;
  if (!sensors_pending || (delta_time > 1000)) {
    digitalWrite(13, !digitalRead(13));
    Serial.print(delta_time, DEC);
    Serial.print(F("("));
    Serial.print(sensors_pending, HEX);
    Serial.print(F(")"));
    mask = 1;
    for (uint8_t i = 0; i < COUNT_SENSORS; i++) {
      Serial.print(F(" : "));
      if (sensors_pending & mask)
        Serial.print(F("TTT")); // show timeout in this one
      else {
        Serial.print(sensors[i].range, DEC);
        if (sensors[i].sensor_status == VL53L0X_ERROR_NONE)
          Serial.print(F("  "));
        else {
          Serial.print(F("#"));
          Serial.print(sensors[i].sensor_status, DEC);
        }
      }
    }
    // setup for next pass
    Serial.println();
    sensor_last_cycle_time = millis();
    sensors_pending = ALL_SENSORS_PENDING;
  }
}

//====================================================================
// Setup
//====================================================================
void setup() {
  Serial.begin(115200);
  Wire.begin();
#if defined(WIRE_IMPLEMENT_WIRE1)
  Wire1.begin();
#endif
  // wait until serial port opens ... For 5 seconds max
  while (!Serial && millis() < 5000)
    ;

  pinMode(13, OUTPUT);

  // initialize all of the pins.
  Serial.println(F("VL53LOX_multi start, initialize IO pins"));
  for (int i = 0; i < COUNT_SENSORS; i++) {
    pinMode(sensors[i].shutdown_pin, OUTPUT);
    digitalWrite(sensors[i].shutdown_pin, LOW);

    if (sensors[i].interrupt_pin >= 0)
      pinMode(sensors[i].interrupt_pin, INPUT_PULLUP);
  }
  Serial.println(F("Starting..."));
  Initialize_sensors();
}

//====================================================================
// loop
//====================================================================
void loop() {
  if (Serial.available()) {
    uint16_t cycle_time = 0;
    uint8_t ch = Serial.read();
    int t;
    while ((t = Serial.read()) != -1) {
      if ((t >= '0') && (t <= '9'))
        cycle_time = cycle_time * 10 + t - '0';
    }

    runmode_t prev_run_mode = run_mode;
    // See what the user typed in
    switch (ch) {
    case 'd':
    case 'D':
      run_mode = RUN_MODE_DEFAULT;
      Serial.println(F("\n*** Default mode ***"));
      break;
    case 'a':
    case 'A':
      Serial.println(F("\n*** Async mode ***"));
      run_mode = RUN_MODE_ASYNC;
      break;
    case 'g':
    case 'G':
      Serial.println(F("\n*** GPIO mode ***"));
      run_mode = RUN_MODE_GPIO;
      break;
    case 'c':
    case 'C':
      run_mode = RUN_MODE_CONT;
      break;

    default:
      show_command_list = 1;
      run_mode = RUN_MODE_DEFAULT;
    }
    if (run_mode != prev_run_mode) {
      // if previous mode was continuous mode, shut it down
      if (prev_run_mode == RUN_MODE_CONT)
        stop_continuous_range();
      if (run_mode == RUN_MODE_CONT)
        start_continuous_range(cycle_time);
    } else if (run_mode == RUN_MODE_CONT) {
      // Already was in continuous but maybe different speed, try to update
      start_continuous_range(cycle_time);
    }
  }
  if (show_command_list) {
    Serial.println(
        F("\nSet run mode by entering one of the following letters"));
    Serial.println(F("    D - Default mode"));
    Serial.println(
        F("    A - Asynchronous mode - Try starting all Seonsors at once"));
    Serial.println(F("    G - Asynchronous mode - Like above use GPIO pins"));
    Serial.println(
        F("    C - Continuous mode - Try starting all Seonsors at once"));
    show_command_list = 0;
  }
  switch (run_mode) {
  case RUN_MODE_DEFAULT:
    read_sensors();
    break;
  case RUN_MODE_ASYNC:
    timed_async_read_sensors();
    break;
  case RUN_MODE_GPIO:
    timed_async_read_gpio();
    break;
  case RUN_MODE_CONT:
    Process_continuous_range();
    break;
  }
  if (run_mode != RUN_MODE_CONT)
    delay(250);
}
