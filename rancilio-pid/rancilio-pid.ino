/********************************************************
 * BLEEDING EDGE FORK OF RANCILIO-PID.
 *   
 * This enhancement implementation is based on the
 * great work of the rancilio-pid (http://rancilio-pid.de/)
 * team. Hopefully it will be merged upstream soon. In case
 * of questions just contact, Tobias <medlor@web.de>
 * 
 *****************************************************/

#include <Arduino.h>

//Libraries for OTA
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <math.h>
#include <float.h>

#include "rancilio-pid.h"

RemoteDebug Debug;

const char* sysVersion PROGMEM  = "2.3.0 beta_8";

/********************************************************
  definitions below must be changed in the userConfig.h file
******************************************************/
const int Display = DISPLAY;
const int OnlyPID = ONLYPID;
const int TempSensor = TEMPSENSOR;
const int TempSensorRecovery = TEMPSENSORRECOVERY;
const int brewDetection = BREWDETECTION;
const int triggerType = TRIGGERTYPE;
const bool ota = OTA;
const int grafana=GRAFANA;

// Wifi
const char* hostname = HOSTNAME;
const char* ssid = D_SSID;
const char* pass = PASS;

unsigned long lastWifiConnectionAttempt = millis();
const unsigned long wifiReconnectInterval = 60000; // try to reconnect every 60 seconds
unsigned long wifiConnectWaitTime = 6000; //ms to wait for the connection to succeed
unsigned int wifiReconnects = 0; //number of reconnects

// OTA
const char* OTAhost = OTAHOST;
const char* OTApass = OTAPASS;

//Blynk
const char* blynkaddress = BLYNKADDRESS;
const int blynkport = BLYNKPORT;
const char* blynkauth = BLYNKAUTH;
unsigned long blynk_lastReconnectAttemptTime = 0;
unsigned int blynk_reconnectAttempts = 0;
unsigned long blynk_reconnect_incremental_backoff = 180000 ; //Failsafe: add 180sec to reconnect time after each connect-failure.
unsigned int blynk_max_incremental_backoff = 5 ; // At most backoff <mqtt_max_incremenatl_backoff>+1 * (<mqtt_reconnect_incremental_backoff>ms)


// MQTT
#if (MQTT_ENABLE==1)
#include "src/PubSubClient/PubSubClient.h"
#elif (MQTT_ENABLE==2)
//#include "src/uMQTTBroker/uMQTTBroker.h"
#include "uMQTTBroker.h"
#endif

#include "src/PubSubClient/PubSubClient.h"
//#include <PubSubClient.h>  // uncomment this line AND delete src/PubSubClient/ folder, if you want to use system lib
const int MQTT_MAX_PUBLISH_SIZE = 120; //see https://github.com/knolleary/pubsubclient/blob/master/src/PubSubClient.cpp
const char* mqtt_server_ip = MQTT_SERVER_IP;
const int mqtt_server_port = MQTT_SERVER_PORT;
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_topic_prefix = MQTT_TOPIC_PREFIX;
char topic_will[256];
char topic_set[256];
unsigned long lastMQTTStatusReportTime = 0;
unsigned long lastMQTTStatusReportInterval = 5000; //mqtt send status-report every 5 second
const bool mqtt_flag_retained = true;
unsigned long mqtt_dontPublishUntilTime = 0;
unsigned long mqtt_dontPublishBackoffTime = 60000; // Failsafe: dont publish if there are errors for 10 seconds
unsigned long mqtt_lastReconnectAttemptTime = 0;
unsigned int mqtt_reconnectAttempts = 0;
unsigned long mqtt_reconnect_incremental_backoff = 210000 ; //Failsafe: add 210sec to reconnect time after each connect-failure.
unsigned int mqtt_max_incremental_backoff = 5 ; // At most backoff <mqtt_max_incremenatl_backoff>+1 * (<mqtt_reconnect_incremental_backoff>ms)
bool mqtt_disabled_temporary = false;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
 
/********************************************************
   Vorab-Konfig
******************************************************/
int pidON = 1 ;             // 1 = control loop in closed loop
int relayON, relayOFF;      // used for relay trigger type. Do not change!
int activeState = 3;        // (0:= undefined / EMERGENCY_TEMP reached)
                            // 1:= Coldstart required (machine is cold) 
                            // 2:= Stabilize temperature after coldstart
                            // 3:= (default) Inner Zone detected (temperature near setPoint)
                            // 4:= Brew detected
                            // 5:= Outer Zone detected (temperature outside of "inner zone")
                            // (6:= steam mode activated (TODO))
                            // (7:= steam ready)
bool emergencyStop = false; // Notstop bei zu hoher Temperatur

/********************************************************
   history of temperatures
*****************************************************/
const int numReadings = 75;             // number of values per Array
double readingstemp[numReadings];       // the readings from Temp
float readingstime[numReadings];        // the readings from time
int readIndex = 0;                      // the index of the current reading
int totaltime = 0 ;                     // the running time
unsigned long  lastBrewTime = 0 ;
int timerBrewDetection = 0 ;
int i = 0;

/********************************************************
   PID Variables
*****************************************************/
const unsigned int windowSizeSeconds = 5;            // How often should PID.compute() run? must be >= 1sec
unsigned int windowSize = windowSizeSeconds * 1000;  // 1000=100% heater power => resolution used in TSR() and PID.compute().
volatile unsigned int isrCounter = 0;                // counter for heater ISR  //TODO byte +isrWakeupTime(=1000000) would be faster/safer
const int isrWakeupTime = 50000 ; // div 5000 => 10ms 
const float heater_overextending_factor = 1.2;
unsigned int heater_overextending_isrCounter = windowSize * heater_overextending_factor;
unsigned long pidComputeLastRunTime = 0;
double Input = 0, Output = 0;
double previousInput = 0;
double previousOutput = 0;
int pidMode = 1;                   //1 = Automatic, 0 = Manual

double setPoint = SETPOINT;
double starttemp = STARTTEMP;

// State 1: Coldstart PID values
const int coldStartStep1ActivationOffset = 5;
// ... none ...

// State 2: Coldstart stabilization PID values
// ... none ...

// State 3: Inner Zone PID values
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
#if (aggTn == 0)
double aggKi = 0;
#else
double aggKi = aggKp / aggTn;
#endif
double aggKd = aggTv * aggKp ;

// State 4: Brew PID values
// ... none ...
double brewDetectionPower = BREWDETECTION_POWER;

// State 5: Outer Zone Pid values
double aggoKp = AGGOKP;
double aggoTn = AGGOTN;
double aggoTv = AGGOTV;
#if (aggoTn == 0)
double aggoKi = 0;
#else
double aggoKi = aggoKp / aggoTn;
#endif
double aggoKd = aggoTv * aggoKp ;
const double outerZoneTemperatureDifference = 1;
unsigned long lastTempReport = 0;  //TODO remove

/********************************************************
   PID with Bias (steadyPower) Temperature Controller
*****************************************************/
#include "PIDBias.h"
double steadyPower = STEADYPOWER; // in percent 
double PreviousSteadyPower = 0;
int burstShot      = 0;   // this is 1, when the user wants to immediatly set the heater power to the value specified in burstPower
double burstPower  = 20;  // in percent

const int lastBrewTimeOffset = 4 * 1000;  //compensate for lag in software brew-detection

// If the espresso hardware itself is cold, we need additional power for steadyPower to hold the water temperature
double steadyPowerOffset     = STEADYPOWER_OFFSET;  // heater power (in percent) which should be added to steadyPower during steadyPowerOffsetTime
double steadyPowerOffsetModified = steadyPowerOffset;
int steadyPowerOffsetTime   = STEADYPOWER_OFFSET_TIME;  // timeframe (in ms) for which steadyPowerOffset_Activated should be active
unsigned long steadyPowerOffset_Activated = 0;
unsigned long steadyPowerOffsetDecreaseTimer = 0;
unsigned long lastUpdateSteadyPowerOffset = 0;  //last time steadyPowerOffset was updated
bool MachineColdOnStart = true;
double starttempOffset = 0;  // if not MachineColdOnStart then we want to substract offset to starttemp (TODO: value might be different on other machine-types)

PIDBias bPID(&Input, &Output, &steadyPower, &steadyPowerOffsetModified, &steadyPowerOffset_Activated, &steadyPowerOffsetTime, &setPoint, aggKp, aggKi, aggKd);

/********************************************************
   Analog Schalter Read
******************************************************/
double brewtime          = BREWTIME;
double preinfusion       = PREINFUSION;
double preinfusionpause  = PREINFUSION_PAUSE;
const int analogPin      = 0; // A0 pin to be used for button detection (either external multibuttons or brew-button)
int brewing              = 0;
int brewswitch           = 0;
bool waitingForBrewSwitchOff = false;
unsigned long totalbrewtime = 0;
unsigned long bezugsZeit = 0;
unsigned long startZeit  = 0;
unsigned long previousBrewCheck = 0;
unsigned long lastBrewMessage   = 0;

unsigned long previousControlButtonCheck = 0;

/********************************************************
   Sensor check
******************************************************/
bool sensorError = false;
int error           = 0;
int maxErrorCounter = 10 ;  //define maximum number of consecutive polls (of intervaltempmes* duration) to have errors

/********************************************************
 * Rest
 *****************************************************/
unsigned long previousMillistemp;       // initialisation at the end of init()
const long refreshTempInterval = 1000;  //How often to read the temperature sensor
unsigned long best_time_to_call_refreshTemp = refreshTempInterval;
unsigned int estimated_cycle_refreshTemp = 25;  // for my TSIC the hardware refresh happens every 76ms
int tsic_validate_count = 0;
int tsic_stable_count = 0;
unsigned int estimated_cycle_refreshTemp_stable_next_save = 1;
#ifdef EMERGENCY_TEMP
const unsigned int emergency_temperature = EMERGENCY_TEMP;  // temperature at which the emergency shutdown should take place. DONT SET IT ABOVE 120 DEGREE!!
#else
const unsigned int emergency_temperature = 120;             // fallback
#endif
double brewDetectionSensitivity = BREWDETECTION_SENSITIVITY ; // if temperature decreased within the last 6 seconds by this amount, then we detect a brew.
#ifdef BREW_READY_DETECTION
const int brew_ready_led_enabled = BREW_READY_LED;
float marginOfFluctuation = float(BREW_READY_DETECTION);
#else
const int brew_ready_led_enabled = 0;   // 0 = disable functionality
float marginOfFluctuation = 0;          // 0 = disable functionality
#endif
char* blynkReadyLedColor = "#000000";
unsigned long lastCheckBrewReady = 0;
unsigned long brewReadyStatisticStart = 0;    //used to determime the time it takes to reach brewReady==true
bool brewReady = false;
const int expected_eeprom_version = 4;        // EEPROM values are saved according to this versions layout. Increase if a new layout is implemented.
unsigned long eeprom_save_interval = 28*60*1000UL;  //save every 28min
unsigned long last_eeprom_save = 0;
char debugline[100];
unsigned long output_timestamp = 0;
unsigned long all_services_lastReconnectAttemptTime = 0;
unsigned long all_services_min_reconnect_interval = 160000; // 160sec minimum wait-time between service reconnections
bool force_offline = FORCE_OFFLINE;
unsigned long force_eeprom_sync = 0 ;
const int force_eeprom_sync_waitTime = 3000;  // after updating a setting wait this number of milliseconds before writing to eeprom

unsigned long loops = 0;
unsigned long max_micros = 0;
unsigned long last_report_micros = 0;
unsigned long cur_micros_previous_loop = 0;
const unsigned long loop_report_count = 100;

/********************************************************
   DISPLAY
******************************************************/
#include "icon.h"
#if (ICON_COLLECTION == 1)
#include "icon_smiley.h"
#else
#include "icon_simple.h"
#endif
#include <U8g2lib.h>
#include <Wire.h>
#define OLED_RESET 16     //Output pin for disply reset pin  //TODO
#define OLED_SCL 5        //Output pin for dispaly clock pin
#define OLED_SDA 4        //Output pin for dispaly data pin
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#if (DISPLAY == 1)
// Attention: refresh takes around 42ms!
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   //e.g. 1.3"
#else
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);  //e.g. 0.96"
#endif
unsigned long previousMillisDisplay = 0;  // initialisation at the end of init()
const long intervalDisplay = 1000;     // Update für Display   //TODO: Sync this with global isrCounter
bool image_flip = true;

/********************************************************
   DALLAS TEMP
******************************************************/
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS pinTemperature  // Data wire is plugged into port 2 on the Arduino
OneWire oneWire(ONE_WIRE_BUS);       // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
DeviceAddress sensorDeviceAddress;   // arrays to hold device address

/********************************************************
   TSIC 30x TEMP
******************************************************/
#define NEW_TSIC
#ifdef NEW_TSIC
#include "src/ZACwire-Library/ZACwire.h"
ZACwire<pinTemperature> TSIC;
#endif

uint16_t temperature = 0;
float Temperatur_C = 0;
volatile uint16_t temp_value[2] = {0};
volatile byte tsicDataAvailable = 0;
unsigned int isrCounterStripped = 0;
const int isrCounterFrame = 1000;

/********************************************************
   BLYNK
******************************************************/
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#define BLYNK_GREEN     "#23C48E"
#define BLYNK_YELLOW    "#ED9D00"
#define BLYNK_RED       "#D3435C"
unsigned long previousMillisBlynk = 0;
const long intervalBlynk = 1000;      // Update Intervall zur App
int blynksendcounter = 1;
bool blynk_sync_run_once = false;
String PreviousError = "";
String PreviousOutputString = "";
String PreviousPastTemperatureChange = "";
String PreviousInputString = "";
bool blynk_disabled_temporary = false;

/******************************************************
 * Receive following BLYNK PIN values from app/server
 ******************************************************/
BLYNK_CONNECTED() {
  if (!blynk_sync_run_once) {
    blynk_sync_run_once = true;
    Blynk.syncAll();  //get all values from server/app when connected
  }
}
// This is called when Smartphone App is opened
BLYNK_APP_CONNECTED() {
  DEBUG_print("Blynk Client Connected.\n");
  print_settings();
}
// This is called when Smartphone App is closed
BLYNK_APP_DISCONNECTED() {
  DEBUG_print("Blynk Client Disconnected.\n");
}
BLYNK_WRITE(V4) {
  aggKp = param.asDouble();
}
BLYNK_WRITE(V5) {
  aggTn = param.asDouble();
}
BLYNK_WRITE(V6) {
  aggTv = param.asDouble();
}
BLYNK_WRITE(V7) {
  setPoint = param.asDouble();
}
BLYNK_WRITE(V8) {
  brewtime = param.asDouble();
}
BLYNK_WRITE(V9) {
  preinfusion = param.asDouble();
}
BLYNK_WRITE(V10) {
  preinfusionpause = param.asDouble();
}
BLYNK_WRITE(V12) {
  starttemp = param.asDouble();
}
BLYNK_WRITE(V13) {
  pidON = param.asInt();
}
BLYNK_WRITE(V30) {
  aggoKp = param.asDouble();
}
BLYNK_WRITE(V31) {
  aggoTn = param.asDouble();
}
BLYNK_WRITE(V32) {
  aggoTv = param.asDouble();
}
BLYNK_WRITE(V34) {
  brewDetectionSensitivity = param.asDouble();
}
BLYNK_WRITE(V36) {
  brewDetectionPower = param.asDouble();
}
BLYNK_WRITE(V40) {
  burstShot = param.asInt();
}
BLYNK_WRITE(V41) {
  steadyPower = param.asDouble();
  // TODO fix this bPID.SetSteadyPowerDefault(steadyPower); //TOBIAS: working?
}
BLYNK_WRITE(V42) {
  steadyPowerOffset = param.asDouble();
}
BLYNK_WRITE(V43) {
  steadyPowerOffsetTime = param.asInt();
}
BLYNK_WRITE(V44) {
  burstPower = param.asDouble();
}

/******************************************************
 * Type Definition of "sending" BLYNK PIN values from 
 * hardware to app/server (only defined if required)
 ******************************************************/
WidgetLED brewReadyLed(V14);

/******************************************************
 * HELPER
 ******************************************************/
bool wifi_working() {
  return ((!force_offline) && (WiFi.status() == WL_CONNECTED) && (WiFi.localIP() != IPAddress(0U)));
}

bool blynk_working() {
  return ((BLYNK_ENABLE == 1) && (wifi_working()) && (Blynk.connected()));
}

bool in_sensitive_phase() {
  return (Input >=115 || brewing || activeState==4 || isrCounter > 1000);
}

/********************************************************
  Emergency Stop when temp too high
*****************************************************/
void testEmergencyStop(){
  if (getCurrentTemperature() >= emergency_temperature) {
    if (emergencyStop != true) {
      snprintf(debugline, sizeof(debugline), "EmergencyStop because temperature>%u (temperature=%0.2f)", emergency_temperature, getCurrentTemperature());
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
      emergencyStop = true;
    }
  } else if (emergencyStop == true && getCurrentTemperature() < 100) {
    snprintf(debugline, sizeof(debugline), "EmergencyStop ended because temperature<100 (temperature=%0.2f)", getCurrentTemperature());
    ERROR_println(debugline);
    mqtt_publish("events", debugline);
    emergencyStop = false;
  }
}

/********************************************************
  history temperature data
*****************************************************/
void updateTemperatureHistory(double myInput) {
  if (readIndex >= numReadings -1) {
    readIndex = 0;
  } else {
    readIndex++;
  }
  readingstime[readIndex] = millis();
  readingstemp[readIndex] = myInput;
}

//calculate the temperature difference between NOW and a datapoint in history
double pastTemperatureChange(int lookback) {
  if (lookback >= numReadings) lookback=numReadings -1;
  int offset = lookback % numReadings;
  int historicIndex = (readIndex - offset);
  if ( historicIndex < 0 ) {
    historicIndex += numReadings;
  }
  //ignore not yet initialized values
  if (readingstime[readIndex] == 0 || readingstime[historicIndex] == 0) return 0;
  return readingstemp[readIndex] - readingstemp[historicIndex];
}

//calculate the average temperature over the last (lookback) temperatures samples
double getAverageTemperature(int lookback) {
  double averageInput = 0;
  int count = 0;
  if (lookback >= numReadings) lookback=numReadings -1;
  for (int offset = 0; offset < lookback; offset++) {
    int thisReading = readIndex - offset;
    if (thisReading < 0) thisReading = numReadings + thisReading;
    if (readingstime[thisReading] == 0) break;
    averageInput += readingstemp[thisReading];
    count += 1;
  }
  if (count > 0) {
    return averageInput / count;
  } else {
    DEBUG_print("getAverageTemperature() returned 0\n");
    return 0;
  }
}

double getCurrentTemperature() {
  return readingstemp[readIndex];
}

double getTemperature(int lookback) {
  if (lookback >= numReadings) lookback=numReadings -1;
  int offset = lookback % numReadings;
  int historicIndex = (readIndex - offset);
  if ( historicIndex < 0 ) {
    historicIndex += numReadings;
  }
  //ignore not yet initialized values
  if (readingstime[historicIndex] == 0) return 0;
  return readingstemp[historicIndex];
}

//returns heater utilization in percent
double convertOutputToUtilisation(double Output) {
  return (100 * Output) / windowSize;
}

//returns heater utilization in Output
double convertUtilisationToOutput(double utilization) {
  return (utilization / 100 ) * windowSize;
}

bool checkBrewReady(double setPoint, float marginOfFluctuation, int lookback) {
  if (almostEqual(marginOfFluctuation, 0)) return false;
  if (lookback >= numReadings) lookback=numReadings -1;
  for (int offset = 0; offset < lookback; offset++) {
    int thisReading = readIndex - offset;
    if (thisReading < 0) thisReading = numReadings + thisReading;
    if (readingstime[thisReading] == 0) return false;
    if (fabs(setPoint - readingstemp[thisReading]) > (marginOfFluctuation + FLT_EPSILON)) return false;
  }
  return true;
}

void refreshBrewReadyHardwareLed(bool brewReady) {
  static bool lastBrewReady = false;
  if (brew_ready_led_enabled && brewReady != lastBrewReady) {
      digitalWrite(pinLed, brewReady);
      lastBrewReady = brewReady;
  }
}

/*****************************************************
 * fast temperature reading with TSIC 306
 * Code by Adrian with minor adaptions
******************************************************/
void ICACHE_RAM_ATTR readTSIC() { //executed every ~100ms by interrupt
  isrCounterStripped = isrCounter % isrCounterFrame;
  if (isrCounterStripped >= (isrCounterFrame - 20 - 100) && isrCounterStripped < (isrCounterFrame - 20)) {
    byte strobelength = 6;
    byte timeout = 0;
    for (byte ByteNr=0; ByteNr<2; ++ByteNr) {
      if (ByteNr) {                                    //measure strobetime between bytes
        for (timeout = 0; digitalRead(pinTemperature); ++timeout){
          delayMicroseconds(10);
          if (timeout > 20) return;
        }
        strobelength = 0;
        for (timeout = 0; !digitalRead(pinTemperature); ++timeout) {    // wait for rising edge
          ++strobelength;
          delayMicroseconds(10);
          if (timeout > 20) return;
        }
      }
      for (byte i=0; i<9; ++i) {
        for (timeout = 0; digitalRead(pinTemperature); ++timeout) {    // wait for falling edge
          delayMicroseconds(10);
          if (timeout > 20) return;
        }
        if (!i) temp_value[ByteNr] = 0;            //reset byte before start measuring
        delayMicroseconds(10 * strobelength);
        temp_value[ByteNr] <<= 1;
        if (digitalRead(pinTemperature)) temp_value[ByteNr] |= 1;        // Read bit
        for (timeout = 0; !digitalRead(pinTemperature); ++timeout) {     // wait for rising edge
          delayMicroseconds(10);
          if (timeout > 20) return;
        }
      }
    }
    tsicDataAvailable++;
  }
}

double temperature_simulate_steam() {
    unsigned long now = millis();
    if ( now <= 20000 ) return 115;
    if ( now <= 26000 ) return 117;
    if ( now <= 29000 ) return 120;
    if (now <= 32000) return 116;
    if (now <= 35000) return 113;
    if (now <= 37000) return 109;
    if (now <= 39000) return 105;
    if (now <= 40000) return 101;
    if (now <= 43000) return 97;
    return setPoint;
}

double temperature_simulate_normal() {
    unsigned long now = millis();
    if ( now <= 15000 ) return 88;
    if ( now <= 25000 ) return 91;
    if (now <= 28000) return 92;
    return setPoint; 
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
      temperature1 >>= 1;                         // delete parity bits
      temperature2 >>= 1;
      temperature = (temperature1 << 8) + temperature2; //joints high and low significant figures
      // TSIC 20x,30x
      return (float((temperature * 250L) >> 8) - 500) / 10;
      // TSIC 50x
      // return (float((temperature * 175L) >> 9) - 100) / 10;
    }
    else return -50;    //set to -50 if reading failed
}


/********************************************************
  check sensor value. If < 0 or difference between old and new >10, then increase error.
  If error is equal to maxErrorCounter, then set sensorError
*****************************************************/
bool checkSensor(float tempInput, float temppreviousInput) {
  bool sensorOK = false;
  if (!sensorError) {
    if ( ( tempInput < 0 || tempInput > 150 || fabs(tempInput - temppreviousInput) > 5)) {
      error++;
      DEBUG_print("temperature sensor reading: consec_errors=%d, temp_current=%0.2f, temp_prev=%0.2f\n", error, tempInput, temppreviousInput);
    } else {
      error = 0;
      sensorOK = true;
    }
    if (error >= maxErrorCounter) {
      sensorError = true;
      snprintf(debugline, sizeof(debugline), "temperature sensor malfunction: temp_current=%0.2f, temp_prev=%0.2f", tempInput, previousInput);
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
    }
  } else if (TempSensorRecovery == 1 &&
             (!(tempInput < 0 || tempInput > 150))) {
      sensorError = false;
      error = 0;
      sensorOK = true;
  }
  return sensorOK;
}


/********************************************************
  Refresh temperature.
  Each time checkSensor() is called to verify the value.
  If the value is not valid, new data is not stored.
*****************************************************/
void refreshTemp() {
  unsigned long currentMillistemp = millis();
  if (TempSensor == 1) {
    //unsigned long currentMillistemp = millis(); TODO READD?
    long millis_elapsed = currentMillistemp - previousMillistemp ;
    if ( floor(millis_elapsed / refreshTempInterval) >= 2) {
        snprintf(debugline, sizeof(debugline), "Main loop() hang: refreshTemp() missed=%g, millis_elapsed=%lu, isrCounter=%u", floor(millis_elapsed / refreshTempInterval) -1, millis_elapsed, isrCounter);
        ERROR_println(debugline);
        mqtt_publish("events", debugline);
    }
    if (currentMillistemp >= previousMillistemp + refreshTempInterval)
    {
      previousInput = getCurrentTemperature();
      previousMillistemp = currentMillistemp;
      sensors.requestTemperatures();  
      if (!checkSensor(sensors.getTempCByIndex(0), previousInput)) return;  //if sensor data is not valid, abort function
      updateTemperatureHistory(sensors.getTempCByIndex(0));
      Input = getAverageTemperature(5);  //TODO: perhaps it is better to use 3 instead of emperically tested best value of 5
    }
  } else if (TempSensor == 2) {
      #ifdef NEW_TSIC
      if (currentMillistemp >= previousMillistemp + refreshTempInterval)
      {
        previousInput = getCurrentTemperature();
        previousMillistemp = currentMillistemp;
        //Temperatur_C = temperature_simulate_steam();
        // Temperatur_C = temperature_simulate_normal();
        Temperatur_C = TSIC.getTemp();
        //unsigned long stop = millis();
        //DEBUG_print("%lu | temp=%0.2f | time_spend=%lu\n", start, Temperatur_C, stop-start);
        if (checkSensor(Temperatur_C, previousInput)) {
            updateTemperatureHistory(Temperatur_C);
            //DEBUG_print("temp=%0.3f\n", Temperatur_C);
            Input = getAverageTemperature(5);  //TODO: perhaps it is better to use 3 instead of emperically tested best value of 5
        }
      }
      
      #else
      if (tsicDataAvailable >0) { // TODO Failsafe? || currentMillistemp >= previousMillistemp + 1.2* refreshTempInterval ) {
        previousInput = getCurrentTemperature();
        //previousMillistemp = currentMillistemp;
        //unsigned long start = millis();
        Temperatur_C = getTSICvalue();
        tsicDataAvailable = 0;
        //unsigned long stop = millis();
        //DEBUG_print("%lu | temp=%0.2f | time_spend=%lu\n", start, Temperatur_C, stop-start);
        if (checkSensor(Temperatur_C, previousInput)) {
            updateTemperatureHistory(Temperatur_C);
            //DEBUG_print("temp=%0.3f\n", Temperatur_C);
            Input = getAverageTemperature(5);  //TODO: perhaps it is better to use 3 instead of emperically tested best value of 5
        }
      }
      #endif
  }
}


/********************************************************
    PreInfusion, Brew , if not Only PID
******************************************************/
int checkControlButtons() {
  //TODO add DEFINE
  if ( millis() >= previousControlButtonCheck + 50 ) {  //50ms
    previousControlButtonCheck = millis();
    int signal = analogRead(analogPin);  ///CCC
    //DEBUG_print("ControlButton signal: %d\n", signal);
    if (signal > 830 && signal < 840) {
      return 1; 
    } else if (signal > 900 && signal < 910) {
      return 2; 
    } else if (signal > 985 && signal < 995) {
      return 3; 
    }
  }
  return 0;
}


/********************************************************
    PreInfusion, Brew , if not Only PID
******************************************************/
void brew() {
  if (OnlyPID == 0) {
    unsigned long aktuelleZeit = millis();
    
    if ( aktuelleZeit >= previousBrewCheck + 50 ) {  //50ms
      previousBrewCheck = aktuelleZeit;
      brewswitch = analogRead(analogPin);

      //if (aktuelleZeit >= output_timestamp + 500) {
      //  DEBUG_print("brew(): brewswitch=%u | brewing=%u | waitingForBrewSwitchOff=%u\n", brewswitch, brewing, waitingForBrewSwitchOff);
      //  output_timestamp = aktuelleZeit;
      //}
      if (brewswitch > 700 && not (brewing == 0 && waitingForBrewSwitchOff) ) {
        totalbrewtime = (preinfusion + preinfusionpause + brewtime) * 1000;
        
        if (brewing == 0) {
          brewing = 1;
          startZeit = aktuelleZeit;
          waitingForBrewSwitchOff = true;
          DEBUG_print("brewswitch=on - Starting brew()\n");
        }
        bezugsZeit = aktuelleZeit - startZeit; 
  
        //if (aktuelleZeit >= lastBrewMessage + 500) {
        //  lastBrewMessage = aktuelleZeit;
        //  DEBUG_print("brew(): bezugsZeit=%lu totalbrewtime=%lu\n", bezugsZeit/1000, totalbrewtime/1000);
        //}
        if (bezugsZeit <= totalbrewtime) {
          if (bezugsZeit <= preinfusion*1000) {
            //DEBUG_println("preinfusion");
            digitalWrite(pinRelayVentil, relayON);
            digitalWrite(pinRelayPumpe, relayON);
          } else if (bezugsZeit > preinfusion*1000 && bezugsZeit <= (preinfusion + preinfusionpause)*1000) {
            //DEBUG_println("Pause");
            digitalWrite(pinRelayVentil, relayON);
            digitalWrite(pinRelayPumpe, relayOFF);
          } else if (bezugsZeit > (preinfusion + preinfusionpause)*1000) {
            //DEBUG_println("Brew");
            digitalWrite(pinRelayVentil, relayON);
            digitalWrite(pinRelayPumpe, relayON);
          }
        } else {
          DEBUG_print("End brew()\n");
          brewing = 0;
        }
      }
  
      if (brewswitch <= 700) {
        if (waitingForBrewSwitchOff) {
          DEBUG_print("brewswitch=off\n");
        }
        waitingForBrewSwitchOff = false;
        brewing = 0;
        bezugsZeit = 0;
      }
      if (brewing == 0) {
          digitalWrite(pinRelayVentil, relayOFF);
          digitalWrite(pinRelayPumpe, relayOFF);
      }
    }
  }
}

 /********************************************************
   Check if Wifi is connected, if not reconnect
 *****************************************************/
 void checkWifi() {checkWifi(false, wifiConnectWaitTime); }
 void checkWifi(bool force_connect, unsigned long wifiConnectWaitTime_tmp) {
  if (force_offline) return;  //remove this to allow wifi reconnects even when DISABLE_SERVICES_ON_STARTUP_ERRORS=1 
  if ((!force_connect) && (wifi_working() || in_sensitive_phase())) return;
  //if ( !force_offline && 
  //     (!(!force_connect && (wifi_working() || in_sensitive_phase() ) ) ) && 
  //     (force_connect || (millis() > lastWifiConnectionAttempt + 5000 + (wifiReconnectInterval * wifiReconnects)))
  //   ) {
  if (force_connect || (millis() > lastWifiConnectionAttempt + 5000 + (wifiReconnectInterval * wifiReconnects))) {
    lastWifiConnectionAttempt = millis();
    //noInterrupts();
    DEBUG_print("Connecting to WIFI with SID %s ...\n", ssid);
    WiFi.persistent(false);   // Don't save WiFi configuration in flash
    WiFi.disconnect(true);    // Delete SDK WiFi config
    WiFi.setSleepMode(WIFI_NONE_SLEEP);  // needed for some disconnection bugs?
    //WiFi.setSleep(false);              // needed?
    //displaymessage(0, "Connecting Wifi", "");
    #ifdef STATIC_IP
    IPAddress STATIC_IP;      //ip(192, 168, 10, 177);
    IPAddress STATIC_GATEWAY; //gateway(192, 168, 10, 1);
    IPAddress STATIC_SUBNET;  //subnet(255,255,255,0);
    WiFi.config(ip, gateway, subnet);
    #endif
    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
      would try to act as both a client and an access-point and could cause
      network-issues with your other WiFi-devices on your WiFi-network. */
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(false);    //disable auto-connect
    WiFi.setAutoReconnect(false);  //disable auto-reconnect
    WiFi.hostname(hostname);
    WiFi.begin(ssid, pass);
    while (!wifi_working() && (millis() < lastWifiConnectionAttempt + wifiConnectWaitTime_tmp)) {
        yield(); //Prevent Watchdog trigger
    }
    if (wifi_working()) {
      DEBUG_print("Wifi connection attempt (#%u) successfull (%lu secs)\n", wifiReconnects, (millis() - lastWifiConnectionAttempt) /1000);
      wifiReconnects = 0;
    } else {
      ERROR_print("Wifi connection attempt (#%u) not successfull (%lu secs)\n", wifiReconnects, (millis() - lastWifiConnectionAttempt) /1000);
      wifiReconnects++;
    }
    //interrupts();
  }
}

/********************************************************
  send data to Blynk server
*****************************************************/
void sendToBlynk() {
  if (force_offline || !blynk_working() || blynk_disabled_temporary) return;
  unsigned long currentMillisBlynk = millis();
  if (currentMillisBlynk >= previousMillisBlynk + intervalBlynk) {
    //DEBUG_print("inside sendToBlynk()\n");  //TODO remove
    previousMillisBlynk = currentMillisBlynk;
    if (brewReady) {
      if (blynkReadyLedColor != BLYNK_GREEN) {
        blynkReadyLedColor = BLYNK_GREEN;
        brewReadyLed.setColor(blynkReadyLedColor);
      }
    } else if (marginOfFluctuation != 0 && checkBrewReady(setPoint, marginOfFluctuation * 2, 40)) {
      if (blynkReadyLedColor != BLYNK_YELLOW) {
        blynkReadyLedColor = BLYNK_YELLOW;
        brewReadyLed.setColor(blynkReadyLedColor);
      }
    } else {
      if (blynkReadyLedColor != BLYNK_RED) {
        brewReadyLed.on();
        blynkReadyLedColor = BLYNK_RED;
        brewReadyLed.setColor(blynkReadyLedColor);
      }
    }
    if (grafana == 1 && blynksendcounter == 1) {
      Blynk.virtualWrite(V60, Input, Output, bPID.GetKp(), bPID.GetKi(), bPID.GetKd(), setPoint );
    }
    //performance tests has shown to only send one api-call per sendToBlynk() 
    if (blynksendcounter == 1) {
      if (steadyPower != PreviousSteadyPower) {
        Blynk.virtualWrite(V41, steadyPower);  //auto-tuning params should be saved by Blynk.virtualWrite()
        PreviousSteadyPower = steadyPower;
      } else {
        blynksendcounter++;
      }
    }
    if (blynksendcounter == 2) {
      if (String(pastTemperatureChange(10)/2, 2) != PreviousPastTemperatureChange) {
        Blynk.virtualWrite(V35, String(pastTemperatureChange(10)/2, 2));
        PreviousPastTemperatureChange = String(pastTemperatureChange(10)/2, 2);
      } else {
        blynksendcounter++;
      }
    } 
    if (blynksendcounter == 3) {
      if (String(Input - setPoint, 2) != PreviousError) {
        Blynk.virtualWrite(V11, String(Input - setPoint, 2));
        PreviousError = String(Input - setPoint, 2);
      } else {
        blynksendcounter++;
      }
    }
    if (blynksendcounter == 4) {
      if (String(convertOutputToUtilisation(Output), 2) != PreviousOutputString) {
        Blynk.virtualWrite(V23, String(convertOutputToUtilisation(Output), 2));
        PreviousOutputString = String(convertOutputToUtilisation(Output), 2);
      } else {
        blynksendcounter++;
      }
    }
    if (blynksendcounter >= 5) {
      if (String(Input, 2) != PreviousInputString) {
        Blynk.virtualWrite(V2, String(Input, 2)); //send value to server
        PreviousInputString = String(Input, 2);
      }
      //Blynk.syncVirtual(V2);  //get value from server 
      blynksendcounter = 0;  
    }
    blynksendcounter++;
  }
}

/********************************************************
    state Detection
******************************************************/
void updateState() {
  static bool runOnce = false;
  switch (activeState) {
    case 1: // state 1 running, that means full heater power. Check if target temp is reached
    {
      if (!runOnce) {
        runOnce = true;
        const int machineColdStartLimit = 45;
        if (Input <= starttemp && Input >= machineColdStartLimit) {  //special auto-tuning settings when maschine is already warm
          MachineColdOnStart = false;
          steadyPowerOffsetDecreaseTimer = millis();
          steadyPowerOffsetModified /= 2;   //OK
          starttempOffset = +0.0; //Inreasing this lead to too high temp and emergency measures taking place.
          snprintf(debugline, sizeof(debugline), "steadyPowerOffset halved because maschine is already warm");       
        }
      }
      bPID.SetFilterSumOutputI(100);
      if (Input >= starttemp + starttempOffset  || !pidMode ) {  //80.5 if 44C. | 79,7 if 30C | 
        snprintf(debugline, sizeof(debugline), "** End of Coldstart. Transition to step 2 (constant steadyPower)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSumOutputI(0);
        activeState = 2;
      }
      break;
    }
    case 2: // state 2 running, that means heater is on steadyState and we are waiting to temperature to stabilize
    {
      bPID.SetFilterSumOutputI(30);

      if ( (Input - setPoint >= 0) || (Input - setPoint <= -20) ||
           (Input - setPoint <= 0  && pastTemperatureChange(20) <= 0.3) ||
           (Input - setPoint >= -1.0  && pastTemperatureChange(10) > 0.2) ||
           (Input - setPoint >= -1.5  && pastTemperatureChange(10) >= 0.45) ||
           !pidMode ) {
          //auto-tune starttemp
          if (millis() < 400000 && steadyPowerOffset_Activated > 0 && pidMode && MachineColdOnStart) {  //ugly hack to only adapt setPoint after power-on
          double tempChange = pastTemperatureChange(10);
          if (Input - setPoint >= 0) {
            if (tempChange > 0.05 && tempChange <= 0.15) {
              DEBUG_print("Auto-Tune starttemp(%0.2f -= %0.2f) | steadyPowerOffset=%0.2f | steadyPowerOffsetTime=%d\n", starttemp, 0.5, steadyPowerOffset, steadyPowerOffsetTime);  //ABC
              starttemp -= 0.5;
            } else if (tempChange > 0.15) {
              DEBUG_print("Auto-Tune starttemp(%0.2f -= %0.2f) | steadyPowerOffset=%0.2f | steadyPowerOffsetTime=%d\n", starttemp, 1.0, steadyPowerOffset, steadyPowerOffsetTime);
              starttemp -= 1;
            }
          } else if (Input - setPoint >= -1.5 && tempChange >= 0.45) {  // OK (-0.10)!
            DEBUG_print("Auto-Tune starttemp(%0.2f -= %0.2f, too fast) | steadyPowerOffset=%0.2f | steadyPowerOffsetTime=%d\n", starttemp, 0.2, steadyPowerOffset, steadyPowerOffsetTime);
            starttemp -= 0.2;
          } else if (Input - setPoint >= -1.0 && tempChange > 0.2) {  //TODO working correctly? -> OK (+0.10)!
            DEBUG_print("Auto-Tune starttemp(%0.2f -= %0.2f, too fast) | steadyPowerOffset=%0.2f | steadyPowerOffsetTime=%d\n", starttemp, 0.1, steadyPowerOffset, steadyPowerOffsetTime);
            starttemp -= 0.1;
          } else if (Input - setPoint <= -1.2) {
            DEBUG_print("Auto-Tune starttemp(%0.2f += %0.2f) | steadyPowerOffset=%0.2f | steadyPowerOffsetTime=%d\n", starttemp, 0.3, steadyPowerOffset, steadyPowerOffsetTime);
            starttemp += 0.3;
          } else if (Input - setPoint <= -0.6) {
            DEBUG_print("Auto-Tune starttemp(%0.2f += %0.2f) | steadyPowerOffset=%0.2f | steadyPowerOffsetTime=%d\n", starttemp, 0.2, steadyPowerOffset, steadyPowerOffsetTime);
            starttemp += 0.2;
           } else if (Input - setPoint >= -0.4) {
            DEBUG_print("Auto-Tune starttemp(%0.2f -= %0.2f) | steadyPowerOffset=%0.2f | steadyPowerOffsetTime=%d\n", starttemp, 0.1, steadyPowerOffset, steadyPowerOffsetTime);
            starttemp -= 0.1;
          }
          force_eeprom_sync = millis();
          /*
          noInterrupts();  //TODO only update if starttemp is changed
          sync_eeprom();
          interrupts();
          */
        } else {
          DEBUG_print("Auto-Tune starttemp disabled\n");
        }
        
        snprintf(debugline, sizeof(debugline), "** End of stabilizing. Transition to step 3 (normal mode)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSumOutputI(0);
        activeState = 3;
        bPID.SetAutoTune(true);
      }
      break;
    }
    case 4: // state 4 running = Brew running
    {
      bPID.SetFilterSumOutputI(100);
      bPID.SetAutoTune(false);
      //const double brewDetectionOffSensitivity = 0.5; //0.7;
      //if (bezugsZeit - lastTempReport >= 1000) {  //TODO remove
      //  lastTempReport = bezugsZeit;
      //  DEBUG_print("brew temp: t(0)=%0.2f\n", getTemperature(0));
      //}
      if ((!OnlyPID && !brewing) || 
           (OnlyPID && bezugsZeit >= lastBrewTimeOffset + 3 && 
            (bezugsZeit >= brewtime*1000 || 
              setPoint - Input < 0 //||
              //pastTemperatureChange(1) >= brewDetectionOffSensitivity
            ) 
           )
        ) {
        DEBUG_print("Out Zone Detection: past(2)=%0.2f, past(3)=%0.2f | past(5)=%0.2f | past(10)=%0.2f | bezugsZeit=%lu\n", pastTemperatureChange(2), pastTemperatureChange(3), pastTemperatureChange(5), pastTemperatureChange(10), bezugsZeit / 1000);
        DEBUG_print("t(0)=%0.2f | t(1)=%0.2f | t(2)=%0.2f | t(3)=%0.2f | t(5)=%0.2f | t(10)=%0.2f | t(13)=%0.2f\n", getTemperature(0), getTemperature(1), getTemperature(2), getTemperature(3), getTemperature(5), getTemperature(7), getTemperature(10), getTemperature(13));
        //snprintf(debugline, sizeof(debugline), "** End of Brew. Transition to step 3 (normal mode)"); //TODO
        snprintf(debugline, sizeof(debugline), "** End of Brew. Transition to step 2 (constant steadyPower)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        //bPID.SetAutoTune(true);
        bPID.SetAutoTune(false);
        bPID.SetSumOutputI(0);
        timerBrewDetection = 0 ;
        mqtt_publish("brewDetected", "0");
        activeState = 2;  //TODO 3
        brewReadyStatisticStart = millis();
      }
      break;
    }
    case 5: // state 5 in outerZone
    {
      if (Input >= setPoint - outerZoneTemperatureDifference - 1.5) {
        bPID.SetFilterSumOutputI(4.5);
      } else {
        bPID.SetFilterSumOutputI(9);
      }
      if ( fabs(Input - setPoint) < outerZoneTemperatureDifference) {
        snprintf(debugline, sizeof(debugline), "** End of outerZone. Transition to step 3 (normal mode)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSumOutputI(0);
        timerBrewDetection = 0 ;
        activeState = 3;
      }
      break;
    }
    case 3: // normal PID mode
    default:
    {
      if (!pidMode) break;

      //set maximum allowed filterSumOutputI based on error/marginOfFluctuation
      if (Input >= setPoint - marginOfFluctuation) {
        if (bPID.GetFilterSumOutputI() != 1.0) {
          bPID.SetFilterSumOutputI(0);
        }
        bPID.SetFilterSumOutputI(1.0);
      } else if ( Input >= setPoint - 0.5) {
        bPID.SetFilterSumOutputI(2.0);
      } else {
        bPID.SetFilterSumOutputI(4.5);
      } 
      
      /* STATE 1 (COLDSTART) DETECTION */
      if (Input <= starttemp - coldStartStep1ActivationOffset) {
        snprintf(debugline, sizeof(debugline), "** End of normal mode. Transition to step 1 (coldstart)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        steadyPowerOffset_Activated = millis();
        DEBUG_print("Enable steadyPowerOffset (%0.2f)\n", steadyPowerOffset);
        //bPID.SetSteadyPowerOffset(steadyPowerOffset);  //ABC
        //setSteadyPowerOffset();
        bPID.SetAutoTune(false);  //do not tune during coldstart + phase2
        bPID.SetSumOutputI(0);
        activeState = 1;
        break;
      }

      /* STATE 4 (BREW) DETECTION */
      if (brewDetectionSensitivity != 0 && brewDetection == 1) {
        //enable brew-detection if not already running and diff temp is > brewDetectionSensitivity
        if ( (!OnlyPID && brewing) || 
             (OnlyPID && (pastTemperatureChange(3) <= -brewDetectionSensitivity) && 
               fabs(getTemperature(5) - setPoint) <= outerZoneTemperatureDifference && 
               millis() - lastBrewTime >= BREWDETECTION_WAIT * 1000)
           ) {
          DEBUG_print("Brew Detect: prev(5)=%0.2f past(3)=%0.2f past(5)=%0.2f | Avg(3)=%0.2f | Avg(10)=%0.2f Avg(2)=%0.2f\n", getTemperature(5), pastTemperatureChange(3), pastTemperatureChange(5), getAverageTemperature(3), getAverageTemperature(10), getAverageTemperature(2));  
          //Sample: Brew Detection: past(3)=-1.70 past(5)=-2.10 | Avg(3)=91.50 | Avg(10)=92.52 Avg(20)=92.81
          if (OnlyPID) {
            bezugsZeit = 0 ;
            lastBrewTime = millis() - lastBrewTimeOffset;
          } //else {  //not needed for hardware brew detection
            //lastBrewTime = millis();            
          //}
          timerBrewDetection = 1 ;
          mqtt_publish("brewDetected", "1");
          snprintf(debugline, sizeof(debugline), "** End of normal mode. Transition to step 4 (brew)");
          DEBUG_println(debugline);
          mqtt_publish("events", debugline);
          bPID.SetSumOutputI(0);
          activeState = 4;
          break;
        }
      }

      /* STATE 5 (OUTER ZONE) DETECTION */
      if ( Input > starttemp - coldStartStep1ActivationOffset && 
           (fabs(Input - setPoint) > outerZoneTemperatureDifference) ) { 
        //DEBUG_print("Out Zone Detection: Avg(3)=%0.2f | Avg(5)=%0.2f Avg(20)=%0.2f Avg(2)=%0.2f\n", getAverageTemperature(3), getAverageTemperature(5), getAverageTemperature(20), getAverageTemperature(2));  
        snprintf(debugline, sizeof(debugline), "** End of normal mode. Transition to step 5 (outerZone)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSumOutputI(0);
        activeState = 5;
        break;
      }

      break;
    }
  }
  
  // steadyPowerOffset_Activated handling
  if ( steadyPowerOffset_Activated >0 ) {
    if (Input - setPoint >= 1) {
      //bPID.SetSteadyPowerOffset(0);
      steadyPowerOffset_Activated = 0;
      snprintf(debugline, sizeof(debugline), "ATTENTION: Disabled steadyPowerOffset because its too large or starttemp too high");
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
      //bPID.UpdateSteadyPowerOffset(steadyPowerOffset_Activated, steadyPowerOffsetTime*1000);
      bPID.SetAutoTune(true);
    } else if (Input - setPoint >= 0.4  && millis() >= steadyPowerOffsetDecreaseTimer + 90000) {
      steadyPowerOffsetDecreaseTimer = millis();
      steadyPowerOffsetModified /= 2;
      snprintf(debugline, sizeof(debugline), "ATTENTION: steadyPowerOffset halved because its too large or starttemp too high");
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
      //bPID.UpdateSteadyPowerOffset(steadyPowerOffset_Activated, steadyPowerOffsetTime*1000);
    } else if (millis() >= steadyPowerOffset_Activated + steadyPowerOffsetTime*1000) {
      //DEBUG_print("millis=%lu | steadyPowerOffset_Activated=%0.2f | steadyPowerOffsetTime=%d\n", millis(), steadyPowerOffset_Activated, steadyPowerOffsetTime*1000);
      //bPID.SetSteadyPowerOffset(0);
      steadyPowerOffset_Activated = 0;
      DEBUG_print("Disable steadyPowerOffset\n");
      //bPID.UpdateSteadyPowerOffset(steadyPowerOffset_Activated, steadyPowerOffsetTime*1000);
      bPID.SetAutoTune(true);
    } //else if (millis() >= lastUpdateSteadyPowerOffset + 10000) {
      //bPID.UpdateSteadyPowerOffset(steadyPowerOffset_Activated, steadyPowerOffsetTime*1000);
      //lastUpdateSteadyPowerOffset = millis();  //remove var
      //DEBUG_print("Updated steadyPowerOffset=%0.2f\n", bPID.GetSteadyPowerOffset());  //TODO remove
    //}
  }
}


/***********************************
 * PID & HEATER ISR
 ***********************************/
unsigned long pidComputeDelay = 0;
void pidCompute() {
  float Output_save;
  //certain activeState set Output to fixed values
  if (activeState == 1 || activeState == 2 || activeState == 4) {
    Output_save = Output;
  }
  int ret = bPID.Compute();
  if ( ret == 1) {  // compute() did run successfully
    if (isrCounter>5100) {
      ERROR_print("pidCompute() delay: isrCounter=%d, heater_overextending_isrCounter=%d, heater=%d\n", isrCounter, heater_overextending_isrCounter, digitalRead(pinRelayHeater));
    }
    isrCounter = 0; // Attention: heater might not shutdown if bPid.SetSampleTime(), windowSize, timer1_write() and are not set correctly!
    pidComputeDelay = millis()+5 - pidComputeLastRunTime - windowSize;
    if (pidComputeDelay > 50 && pidComputeDelay < 100000000) {
      DEBUG_print("pidCompute() delay of %lu ms (loop() hang?)\n", pidComputeDelay);
    }
    pidComputeLastRunTime = millis();
    if (activeState == 1 || activeState == 2 || activeState == 4) {
      Output = Output_save;
    }
    DEBUG_print("Input=%6.2f | error=%5.2f delta=%5.2f | Output=%6.2f = b:%5.2f + p:%5.2f + i:%5.2f(%5.2f) + d:%5.2f\n", 
      Input,
      (setPoint - Input),
      pastTemperatureChange(10)/2,
      convertOutputToUtilisation(Output),
      steadyPower + bPID.GetSteadyPowerOffsetCalculated(),
      convertOutputToUtilisation(bPID.GetOutputP()),
      convertOutputToUtilisation(bPID.GetSumOutputI()),
      convertOutputToUtilisation(bPID.GetOutputI()),
      convertOutputToUtilisation(bPID.GetOutputD())
    );
  } else if (ret == 2) { // PID is disabled but compute() should have run
    isrCounter = 0;
    pidComputeLastRunTime = millis();
    DEBUG_print("Input=%6.2f | error=%5.2f delta=%5.2f | Output=%6.2f (PID disabled)\n",
      Input,
      (setPoint - Input),
      pastTemperatureChange(10)/2,
      convertOutputToUtilisation(Output));
  }
}
 
void ICACHE_RAM_ATTR onTimer1ISR() {
  timer1_write(isrWakeupTime); // set interrupt time to 10ms
  if (isrCounter >= heater_overextending_isrCounter) {
    //turn off when when compute() is not run in time (safetly measure)
    digitalWrite(pinRelayHeater, LOW);
    //ERROR_print("onTimer1ISR has stopped heater because pid.Compute() did not run\n");
    //TODO: add more emergency handling?
  } else if (isrCounter > windowSize) {
    //dont change output when overextending withing overextending_factor threshold
    //DEBUG_print("onTimer1ISR over extending due to processing delays: isrCounter=%u\n", isrCounter);  //TODO remove  DDD
  } else if (isrCounter >= Output) {  // max(Output) = windowSize
    digitalWrite(pinRelayHeater, LOW);
  } else {
    digitalWrite(pinRelayHeater, HIGH);
  }
  if (isrCounter <= (heater_overextending_isrCounter + 100)) {
    isrCounter += 10; // += 10 because one tick = 10ms
  }
}


/***********************************
 * LOOP()
 ***********************************/
static unsigned long cur_micros;  //TODO do not initialize new variables in function!!
void loop() {
  /*
  loops += 1 ;
  cur_micros = micros();
  if (max_micros < cur_micros-cur_micros_previous_loop) {
      max_micros = cur_micros-cur_micros_previous_loop;
  }
  if ( cur_micros >= last_report_micros + 250000 ) {
    DEBUG_print("%lu loop() temp=%0.2f | loops/ms=%0.2f |cur_micros=%lu | max_micros=%lu | avg_micros=%lu\n", 
        cur_micros/1000, Input, (float) loops/250, (cur_micros-cur_micros_previous_loop), max_micros, (cur_micros - last_report_micros)/loops );
    last_report_micros = cur_micros;
    max_micros = 0;
    loops=0;
  }
  cur_micros_previous_loop = micros(); //cur_micros;
  */
  refreshTemp();        // save new temperature values
  testEmergencyStop();  // test if Temp is to high
  pidCompute();         // call PID for Output calculation
  brew();   //start brewing if button pressed
  if (millis() > lastCheckBrewReady + refreshTempInterval) {
    lastCheckBrewReady = millis();
    bool brewReadyCurrent = checkBrewReady(setPoint, marginOfFluctuation, 60);
    if (!brewReady && brewReadyCurrent) {
      snprintf(debugline, sizeof(debugline), "brewReady (stable last 60 secs. Tuning took %lu secs)", (lastCheckBrewReady - brewReadyStatisticStart) / 1000);
      DEBUG_println(debugline);
      mqtt_publish("events", debugline);
    }
    brewReady = brewReadyCurrent;
  }
  refreshBrewReadyHardwareLed(brewReady);
  //TODO: Readd
  //int controlButtonPressed = checkControlButtons();
  //if (controlButtonPressed != 0) {
  //  DEBUG_print("Pressed Button: %d\n", controlButtonPressed);
  //}

  if (!force_offline) {
    if (!wifi_working()) {
      #if (MQTT_ENABLE ==2)
      MQTT_server_cleanupClientCons();
      #endif
      checkWifi();
    } else {
      
      static bool runOnceOTASetup = true;
      if (runOnceOTASetup) {
        runOnceOTASetup = false;
        // Disable interrupt when OTA starts, otherwise it will not work
        ArduinoOTA.onStart([](){
          DEBUG_print("OTA update initiated\n");
          Output = 0;
          timer1_disable();
          digitalWrite(pinRelayHeater, LOW); //Stop heating
        });
        ArduinoOTA.onError([](ota_error_t error) {
          ERROR_print("OTA update error\n");
          timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
        });
        // Enable interrupts if OTA is finished
        ArduinoOTA.onEnd([](){
          timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
        });
      }
      ArduinoOTA.handle();

      if (BLYNK_ENABLE && !blynk_disabled_temporary) {
        if (blynk_working()) {
          Blynk.run(); //Do Blynk household stuff. (On reconnect after disconnect, timeout seems to be 5 seconds)
        } else {
          unsigned long now = millis();
          if ((now > blynk_lastReconnectAttemptTime + (blynk_reconnect_incremental_backoff * (blynk_reconnectAttempts))) 
               && now > all_services_lastReconnectAttemptTime + all_services_min_reconnect_interval
               && !in_sensitive_phase() ) {
              blynk_lastReconnectAttemptTime = now;
              all_services_lastReconnectAttemptTime = now;
              ERROR_print("Blynk disconnected. Reconnecting...\n");
              if ( Blynk.connect(2000) ) { // Attempt to reconnect
                blynk_lastReconnectAttemptTime = 0;
                blynk_reconnectAttempts = 0;
                DEBUG_print("Blynk reconnected in %lu seconds\n", (millis() - now)/1000);
              } else if (blynk_reconnectAttempts < blynk_max_incremental_backoff) {
                blynk_reconnectAttempts++;
              }
          }
        }
      }
  
      //Check mqtt connection
      if (MQTT_ENABLE && !mqtt_disabled_temporary) {
        if (!mqtt_working()) {

          mqtt_reconnect(false);
        } else {
          if (MQTT_ENABLE == 1) mqtt_client.loop(); // mqtt client connected, do mqtt housekeeping
          unsigned long now = millis();
          if (now >= lastMQTTStatusReportTime + lastMQTTStatusReportInterval) {
            lastMQTTStatusReportTime = now;
            mqtt_publish("temperature", number2string(Input));
            mqtt_publish("temperatureAboveTarget", number2string((Input - setPoint)));
            mqtt_publish("heaterUtilization", number2string(convertOutputToUtilisation(Output)));
            //mqtt_publish("kp", number2string(bPID.GetKp()));
            //mqtt_publish("ki", number2string(bPID.GetKi()));
            //mqtt_publish("kd", number2string(bPID.GetKd()));
            //mqtt_publish("outputP", number2string(convertOutputToUtilisation(bPID.GetOutputP())));
            //mqtt_publish("outputI", number2string(convertOutputToUtilisation(bPID.GetOutputI())));
            //mqtt_publish("outputD", number2string(convertOutputToUtilisation(bPID.GetOutputD())));
            mqtt_publish("pastTemperatureChange", number2string(pastTemperatureChange(10)));
            mqtt_publish("brewReady", bool2string(brewReady));
            //mqtt_publish_settings();  //not needed because we update live on occurance
           }
        }
      }
    }
  }

  //check if PID should run or not. If not, set to manuel and force output to zero
  if (pidON == 0 && pidMode == 1) {
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0 ;
    DEBUG_print("Current config has disabled PID\n");
  } else if (pidON == 1 && pidMode == 0 && !emergencyStop) {
    Output = 0; // safety: be 100% sure that PID.compute() starts fresh.
    pidMode = 1;
    bPID.SetMode(pidMode);
    if ( millis() - output_timestamp > 21000) {
      DEBUG_print("Current config has enabled PID\n");
      output_timestamp = millis();
    }
  }

  //Sicherheitsabfrage
  if (!sensorError && !emergencyStop && Input > 0) {
    updateState();

    /* state 1: Water is very cold, set heater to full power */
    if (activeState == 1) {
      Output = windowSize;

    /* state 2: ColdstartTemp reached. Now stabilizing temperature after coldstart */
    } else if (activeState == 2) {
      //Output = convertUtilisationToOutput(steadyPower + bPID.GetSteadyPowerOffsetCalculated());
      Output = convertUtilisationToOutput(steadyPower);

    /* state 4: Brew detected. Increase heater power */
    } else if (activeState == 4) {
      if (OnlyPID == 0) {
         Output = convertUtilisationToOutput(brewDetectionPower);
      } else if (OnlyPID == 1) {
        if (setPoint - Input <= (outerZoneTemperatureDifference + 0.5) //||
          //pastTemperatureChange(1) >= brewDetectionOffSensitivity
         ) {
          //DEBUG_print("BREWDETECTION_POWER(%0.2f) might be too high\n", brewDetectionPower);
          Output = convertUtilisationToOutput(steadyPower + bPID.GetSteadyPowerOffsetCalculated());
        } else {
          Output = convertUtilisationToOutput(brewDetectionPower);
        }
        if (timerBrewDetection == 1) {
          bezugsZeit = millis() - lastBrewTime;
        }
      }

    /* state 5: Outer Zone reached. More power than in inner zone */
    } else if (activeState == 5) {
      if (aggoTn != 0) {
        aggoKi = aggoKp / aggoTn ;
      } else {
        aggoKi = 0;
      }
      aggoKd = aggoTv * aggoKp ;
      if (pidMode == 1) bPID.SetMode(AUTOMATIC);
      bPID.SetTunings(aggoKp, aggoKi, aggoKd);

    /* state 3: Inner zone reached = "normal" low power mode */
    } else {
      if (!pidMode) {
        Output = 0;
      } else {
        bPID.SetMode(AUTOMATIC);
        if (aggTn != 0) {
          aggKi = aggKp / aggTn ;
        } else {
          aggKi = 0 ;
        }
        aggKd = aggTv * aggKp ;
        bPID.SetTunings(aggKp, aggKi, aggKd);
      }
    }

    if (burstShot == 1 && pidMode == 1) {
      burstShot = 0;
      bPID.SetBurst(burstPower);
      snprintf(debugline, sizeof(debugline), "BURST Output=%0.2f", convertOutputToUtilisation(Output));
      DEBUG_println(debugline);
      mqtt_publish("events", debugline);
    }

    displaymessage(activeState, "", "");
    sendToBlynk();

  } else if (sensorError) {
    //Deactivate PID
    if (pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
      if ( millis() - output_timestamp > 15000) {
        ERROR_print("sensorError detected. Shutdown PID and heater\n");
        output_timestamp = millis();
      }
    }
    digitalWrite(pinRelayHeater, LOW); //Stop heating
    char line2[17];
    snprintf(line2, sizeof(line2), "Temp. %0.2f", getCurrentTemperature());
    displaymessage(0, "Check Temp. Sensor!", line2);
    
  } else if (emergencyStop) {
    //Deactivate PID
    if (pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
      if ( millis() - output_timestamp > 10000) {
         ERROR_print("emergencyStop detected. Shutdown PID and heater (temp=%0.2f)\n", getCurrentTemperature());
         output_timestamp = millis();
      }
    }
    digitalWrite(pinRelayHeater, LOW); //Stop heating
    char line2[17];
    snprintf(line2, sizeof(line2), "%0.0f\xB0""C", getCurrentTemperature());
    if (EMERGENCY_ICON == "steam") {
      displaymessage(7, "", "");
    } else {
      displaymessage(0, "Emergency Stop!", line2);
    }

  } else {
    if ( millis() - output_timestamp > 15000) {
       ERROR_print("unknown error\n");
       output_timestamp = millis();
    }
  }

  if (!in_sensitive_phase() &&
      (millis() >= last_eeprom_save + eeprom_save_interval ||
       (force_eeprom_sync > 0 && millis() >= force_eeprom_sync + force_eeprom_sync_waitTime)
      )
     ) {
    last_eeprom_save = millis();
    force_eeprom_sync = 0;
    noInterrupts();
    sync_eeprom();
    interrupts();
  }
  Debug.handle();
}


/***********************************
 * EEPROM
 ***********************************/
void sync_eeprom() { sync_eeprom(false, false); }
void sync_eeprom(bool startup_read, bool force_read) {
  int current_version;
  DEBUG_print("EEPROM: sync_eeprom(force_read=%d) called\n", force_read);
  EEPROM.begin(1024);
  EEPROM.get(290, current_version);
  DEBUG_print("EEPROM: Detected Version=%d Expected Version=%d\n", current_version, expected_eeprom_version);
  if (current_version != expected_eeprom_version) {
    ERROR_print("EEPROM: Version has changed or settings are corrupt or not previously set. Ignoring..\n");
    EEPROM.put(290, expected_eeprom_version);
  }

  //if variables are not read from blynk previously, always get latest values from EEPROM
  if (force_read && (current_version == expected_eeprom_version)) {
    DEBUG_print("EEPROM: Blynk not active. Reading settings from EEPROM\n");
    EEPROM.get(0, aggKp);
    EEPROM.get(10, aggTn);
    EEPROM.get(20, aggTv);
    EEPROM.get(30, setPoint);
    EEPROM.get(40, brewtime);
    EEPROM.get(50, preinfusion);
    EEPROM.get(60, preinfusionpause);
    EEPROM.get(80, starttemp);
    EEPROM.get(90, aggoKp);
    EEPROM.get(100, aggoTn);
    EEPROM.get(110, aggoTv);
    EEPROM.get(130, brewDetectionSensitivity);
    EEPROM.get(140, steadyPower);
    EEPROM.get(150, steadyPowerOffset);
    EEPROM.get(160, steadyPowerOffsetTime);
    EEPROM.get(170, burstPower);
    //180 is used
    EEPROM.get(190, brewDetectionPower);
    EEPROM.get(200, pidON);
    //Reminder: 290 is reserved for "version"
  }
  //always read the following values during setup() (which are not saved in blynk)
  if (startup_read && (current_version == expected_eeprom_version)) {
    EEPROM.get(180, estimated_cycle_refreshTemp);
  }

  //if blynk vars are not read previously, get latest values from EEPROM
  double aggKp_latest_saved = 0;
  double aggTn_latest_saved = 0;
  double aggTv_latest_saved = 0;
  double aggoKp_latest_saved = 0;
  double aggoTn_latest_saved = 0;
  double aggoTv_latest_saved = 0;
  double setPoint_latest_saved = 0;
  double brewtime_latest_saved = 0;
  double preinfusion_latest_saved = 0;
  double preinfusionpause_latest_saved = 0;
  double starttemp_latest_saved = 0;
  double brewDetectionSensitivity_latest_saved = 0;
  double steadyPower_latest_saved = 0;
  double steadyPowerOffset_latest_saved = 0;
  int steadyPowerOffsetTime_latest_saved = 0;
  double burstPower_latest_saved = 0;
  int estimated_cycle_refreshTemp_latest_saved = 0;
  double brewDetectionPower_latest_saved = 0;
  int pidON_latest_saved = 0;
  if (current_version == expected_eeprom_version) {
    EEPROM.get(0, aggKp_latest_saved);
    EEPROM.get(10, aggTn_latest_saved);
    EEPROM.get(20, aggTv_latest_saved);
    EEPROM.get(30, setPoint_latest_saved);
    EEPROM.get(40, brewtime_latest_saved);
    EEPROM.get(50, preinfusion_latest_saved);
    EEPROM.get(60, preinfusionpause_latest_saved);
    EEPROM.get(80, starttemp_latest_saved);
    EEPROM.get(90, aggoKp_latest_saved);
    EEPROM.get(100, aggoTn_latest_saved);
    EEPROM.get(110, aggoTv_latest_saved);
    EEPROM.get(130, brewDetectionSensitivity_latest_saved);
    EEPROM.get(140, steadyPower_latest_saved);
    EEPROM.get(150, steadyPowerOffset_latest_saved);
    EEPROM.get(160, steadyPowerOffsetTime_latest_saved);
    EEPROM.get(170, burstPower_latest_saved);
    EEPROM.get(180, estimated_cycle_refreshTemp_latest_saved);
    EEPROM.get(190, brewDetectionPower_latest_saved);
    EEPROM.get(200, pidON_latest_saved);
  }

  //get saved userConfig.h values
  double aggKp_config_saved;
  double aggTn_config_saved;
  double aggTv_config_saved;
  double aggoKp_config_saved;
  double aggoTn_config_saved;
  double aggoTv_config_saved;
  double setPoint_config_saved;
  double brewtime_config_saved;
  double preinfusion_config_saved;
  double preinfusionpause_config_saved;
  double starttemp_config_saved;
  double brewDetectionSensitivity_config_saved;
  double steadyPower_config_saved;
  double steadyPowerOffset_config_saved;
  int steadyPowerOffsetTime_config_saved;
  double burstPower_config_saved;
  double brewDetectionPower_config_saved;
  EEPROM.get(300, aggKp_config_saved);
  EEPROM.get(310, aggTn_config_saved);
  EEPROM.get(320, aggTv_config_saved);
  EEPROM.get(330, setPoint_config_saved);
  EEPROM.get(340, brewtime_config_saved);
  EEPROM.get(350, preinfusion_config_saved);
  EEPROM.get(360, preinfusionpause_config_saved);
  EEPROM.get(380, starttemp_config_saved);
  EEPROM.get(390, aggoKp_config_saved);
  EEPROM.get(400, aggoTn_config_saved);
  EEPROM.get(410, aggoTv_config_saved);
  EEPROM.get(430, brewDetectionSensitivity_config_saved);
  EEPROM.get(440, steadyPower_config_saved);
  EEPROM.get(450, steadyPowerOffset_config_saved);
  EEPROM.get(460, steadyPowerOffsetTime_config_saved);
  EEPROM.get(470, burstPower_config_saved);
  EEPROM.get(480, brewDetectionPower_config_saved);

  //use userConfig.h value if if differs from *_config_saved
  if (AGGKP != aggKp_config_saved) { aggKp = AGGKP; EEPROM.put(300, aggKp); }
  if (AGGTN != aggTn_config_saved) { aggTn = AGGTN; EEPROM.put(310, aggTn); }
  if (AGGTV != aggTv_config_saved) { aggTv = AGGTV; EEPROM.put(320, aggTv); }
  if (AGGOKP != aggoKp_config_saved) { aggoKp = AGGOKP; EEPROM.put(390, aggoKp); }
  if (AGGOTN != aggoTn_config_saved) { aggoTn = AGGOTN; EEPROM.put(400, aggoTn); }
  if (AGGOTV != aggoTv_config_saved) { aggoTv = AGGOTV; EEPROM.put(410, aggoTv); }
  if (SETPOINT != setPoint_config_saved) { setPoint = SETPOINT; EEPROM.put(330, setPoint); DEBUG_print("EEPROM: setPoint (%0.2f) is read from userConfig.h\n", setPoint); }
  if (BREWTIME != brewtime_config_saved) { brewtime = BREWTIME; EEPROM.put(340, brewtime); DEBUG_print("EEPROM: brewtime (%0.2f) is read from userConfig.h\n", brewtime); }
  if (PREINFUSION != preinfusion_config_saved) { preinfusion = PREINFUSION; EEPROM.put(350, preinfusion); }
  if (PREINFUSION_PAUSE != preinfusionpause_config_saved) { preinfusionpause = PREINFUSION_PAUSE; EEPROM.put(360, preinfusionpause); }
  if (STARTTEMP != starttemp_config_saved) { starttemp = STARTTEMP; EEPROM.put(380, starttemp); }
  if (BREWDETECTION_SENSITIVITY != brewDetectionSensitivity_config_saved) { brewDetectionSensitivity = BREWDETECTION_SENSITIVITY; EEPROM.put(430, brewDetectionSensitivity); }
  if (STEADYPOWER != steadyPower_config_saved) { steadyPower = STEADYPOWER; EEPROM.put(440, steadyPower); }
  if (STEADYPOWER_OFFSET != steadyPowerOffset_config_saved) { steadyPowerOffset = STEADYPOWER_OFFSET; EEPROM.put(450, steadyPowerOffset); }
  if (STEADYPOWER_OFFSET_TIME != steadyPowerOffsetTime_config_saved) { steadyPowerOffsetTime = STEADYPOWER_OFFSET_TIME; EEPROM.put(460, steadyPowerOffsetTime); }
  //if (BURSTPOWER != burstPower_config_saved) { burstPower = BURSTPOWER; EEPROM.put(470, burstPower); }
  if (BREWDETECTION_POWER != brewDetectionPower_config_saved) { brewDetectionPower = BREWDETECTION_POWER; EEPROM.put(480, brewDetectionPower); DEBUG_print("EEPROM: brewDetectionPower (%0.2f) is read from userConfig.h\n", brewDetectionPower); }

  //save latest values to eeprom and sync back to blynk
  if ( aggKp != aggKp_latest_saved) { EEPROM.put(0, aggKp); Blynk.virtualWrite(V4, aggKp); }
  if ( aggTn != aggTn_latest_saved) { EEPROM.put(10, aggTn); Blynk.virtualWrite(V5, aggTn); }
  if ( aggTv != aggTv_latest_saved) { EEPROM.put(20, aggTv); Blynk.virtualWrite(V6, aggTv); }
  if ( setPoint != setPoint_latest_saved) { EEPROM.put(30, setPoint); Blynk.virtualWrite(V7, setPoint); DEBUG_print("EEPROM: setPoint (%0.2f) is saved\n", setPoint); }
  if ( brewtime != brewtime_latest_saved) { EEPROM.put(40, brewtime); Blynk.virtualWrite(V8, brewtime); DEBUG_print("EEPROM: brewtime (%0.2f) is saved (previous:%0.2f)\n", brewtime, brewtime_latest_saved); }
  if ( preinfusion != preinfusion_latest_saved) { EEPROM.put(50, preinfusion); Blynk.virtualWrite(V9, preinfusion); }
  if ( preinfusionpause != preinfusionpause_latest_saved) { EEPROM.put(60, preinfusionpause); Blynk.virtualWrite(V10, preinfusionpause); }
  if ( starttemp != starttemp_latest_saved) { EEPROM.put(80, starttemp); Blynk.virtualWrite(V12, starttemp); }
  if ( aggoKp != aggoKp_latest_saved) { EEPROM.put(90, aggoKp); Blynk.virtualWrite(V30, aggoKp); }
  if ( aggoTn != aggoTn_latest_saved) { EEPROM.put(100, aggoTn); Blynk.virtualWrite(V31, aggoTn); }
  if ( aggoTv != aggoTv_latest_saved) { EEPROM.put(110, aggoTv); Blynk.virtualWrite(V32, aggoTv); }
  if ( brewDetectionSensitivity != brewDetectionSensitivity_latest_saved) { EEPROM.put(130, brewDetectionSensitivity); Blynk.virtualWrite(V34, brewDetectionSensitivity); }
  if ( steadyPower != steadyPower_latest_saved) { EEPROM.put(140, steadyPower); Blynk.virtualWrite(V41, steadyPower); DEBUG_print("EEPROM: steadyPower (%0.2f) is saved (previous:%0.2f)\n", steadyPower, steadyPower_latest_saved); }
  if ( steadyPowerOffset != steadyPowerOffset_latest_saved) { EEPROM.put(150, steadyPowerOffset); Blynk.virtualWrite(V42, steadyPowerOffset); }
  if ( steadyPowerOffsetTime != steadyPowerOffsetTime_latest_saved) { EEPROM.put(160, steadyPowerOffsetTime); Blynk.virtualWrite(V43, steadyPowerOffsetTime); }
  if ( burstPower != burstPower_latest_saved) { EEPROM.put(170, burstPower); Blynk.virtualWrite(V44, burstPower); }
  if ( estimated_cycle_refreshTemp != estimated_cycle_refreshTemp_latest_saved) { EEPROM.put(180, estimated_cycle_refreshTemp); DEBUG_print("EEPROM: estimated_cycle_refreshTemp (%u) is saved (previous:%u)\n", estimated_cycle_refreshTemp, estimated_cycle_refreshTemp_latest_saved); }
  if ( brewDetectionPower != brewDetectionPower_latest_saved) { EEPROM.put(190, brewDetectionPower); Blynk.virtualWrite(V36, brewDetectionPower); DEBUG_print("EEPROM: brewDetectionPower (%0.2f) is saved (previous:%0.2f)\n", brewDetectionPower, brewDetectionPower_latest_saved); }
  if ( pidON != pidON_latest_saved) { EEPROM.put(200, pidON); Blynk.virtualWrite(V13, pidON); DEBUG_print("EEPROM: pidON (%d) is saved (previous:%d)\n", pidON, pidON_latest_saved); }
  EEPROM.commit();
  DEBUG_print("EEPROM: sync_eeprom() finished.\n");
}

void print_settings() {
  DEBUG_print("========================\n");
  DEBUG_print("Machine: %s | Version: %s\n", MACHINE_TYPE, sysVersion);
  DEBUG_print("aggKp: %0.2f | aggTn: %0.2f | aggTv: %0.2f\n", aggKp, aggTn, aggTv);
  DEBUG_print("aggoKp: %0.2f | aggoTn: %0.2f | aggoTv: %0.2f\n", aggoKp, aggoTn, aggoTv);
  DEBUG_print("setPoint: %0.2f | starttemp: %0.2f | burstPower: %0.2f\n", setPoint, starttemp, burstPower);
  DEBUG_print("brewDetection: %d | brewDetectionSensitivity: %0.2f | brewDetectionPower: %0.2f\n", brewDetection, brewDetectionSensitivity, brewDetectionPower);
  DEBUG_print("brewtime: %0.2f | preinfusion: %0.2f | preinfusionpause: %0.2f\n", brewtime, preinfusion, preinfusionpause);
  DEBUG_print("steadyPower: %0.2f | steadyPowerOffset: %0.2f | steadyPowerOffsetTime: %d\n", steadyPower, steadyPowerOffset, steadyPowerOffsetTime);
  DEBUG_print("pidON: %d\n", pidON);
  DEBUG_print("========================\n");
}


/***********************************
 * DISPLAY
 ***********************************/
void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_profont11_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void displaymessage(int activeState, char* displaymessagetext, char* displaymessagetext2) {
  if (Display > 0) {
    unsigned long currentMillisDisplay = millis();
    if (currentMillisDisplay >= previousMillisDisplay + intervalDisplay || previousMillisDisplay == 0) {
      previousMillisDisplay = currentMillisDisplay;
      image_flip = !image_flip;
      unsigned int align_right;
      const unsigned int align_right_2digits = LCDWidth - 56;
      const unsigned int align_right_3digits = LCDWidth - 56 - 12;
      const unsigned int align_right_2digits_decimal = LCDWidth - 56 +28;
      u8g2.clearBuffer();
      u8g2.setBitmapMode(1);
      //u8g2.drawFrame(0, 0, 128, 64);

      //display icons
      switch(activeState) {
        case 1:
        case 2:
          if (image_flip) {
            u8g2.drawXBMP(0,0, icon_width, icon_height, coldstart_rotate_bits);
          } else {
            u8g2.drawXBMP(0,0, icon_width, icon_height, coldstart_bits);
          }
          break;
        case 4: //brew
          if (image_flip) {
            u8g2.drawXBMP(0,0, icon_width, icon_height, brewing_bits);
          } else {
            u8g2.drawXBMP(0,0, icon_width, icon_height, brewing_rotate_bits);
          }
          break;
        case 3: 
          if (brewReady) {
            if (image_flip) {
              u8g2.drawXBMP(0,0, icon_width, icon_height, brew_ready_bits);
            } else {
              u8g2.drawXBMP(0,0, icon_width, icon_height, brew_ready_rotate_bits);
            }
          } else {  //inner zone
            if (image_flip) {
              u8g2.drawXBMP(0,0, icon_width, icon_height, brew_acceptable_bits);
            } else {
              u8g2.drawXBMP(0,0, icon_width, icon_height, brew_acceptable_rotate_bits);
            }
          }
          break;
        case 5:
          if (image_flip) {
            u8g2.drawXBMP(0,0, icon_width, icon_height, outer_zone_bits);
          } else {
            u8g2.drawXBMP(0,0, icon_width, icon_height, outer_zone_rotate_bits);
          }
          break;
        case 7:  //steam possible
          if (image_flip) {
            u8g2.drawXBMP(0,0, icon_width, icon_height, steam_bits);
          } else {
            u8g2.drawXBMP(0,0, icon_width, icon_height, steam_rotate_bits);
          }
          break;
        default:
          if (MACHINE_TYPE == "rancilio") {
            u8g2.drawXBMP(41,0, rancilio_logo_width, rancilio_logo_height, rancilio_logo_bits);
          } else if (MACHINE_TYPE == "gaggia") {
            u8g2.drawXBMP(1, 0, gaggia_logo_width, gaggia_logo_height, gaggia_logo_bits);
          }
          break;
      }

      //display current and target temperature
      if (activeState > 0 && activeState != 4) {
        if (Input - 100 > FLT_EPSILON) {
          align_right = align_right_3digits;
        } else {
          align_right = align_right_2digits;
        }
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.setCursor(align_right, 3);
        u8g2.print(Input, 1);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print((char)176);
        u8g2.println("C");
        u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
        u8g2.drawGlyph(align_right-11, 3+7, 0x0046);

        if (Input <= 105) { //only show setpoint if we are not steaming
          if (setPoint >= 100) {
            align_right = align_right_3digits;
          } else {
            align_right = align_right_2digits;
          }
          u8g2.setFont(u8g2_font_profont22_tf);
          u8g2.setCursor(align_right, 20);
          u8g2.print(setPoint, 1);
          u8g2.setFont(u8g2_font_profont10_tf);
          u8g2.print((char)176);
          u8g2.println("C");
          u8g2.setFont(u8g2_font_open_iconic_other_1x_t);
          u8g2.drawGlyph(align_right - 11 , 20+7, 0x047); 
        }
      } else if (activeState == 4) {
        totalbrewtime = (preinfusion + preinfusionpause + brewtime) * 1000;
        align_right = align_right_2digits_decimal;
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.setCursor(align_right, 3);
        if (bezugsZeit < 10000) u8g2.print("0");
        // TODO: Use print(u8x8_u8toa(value, digits)) or print(u8x8_u16toa(value, digits)) to print numbers with constant width (numbers are prefixed with 0 if required).
        u8g2.print(bezugsZeit / 1000);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.println("s");
        if (totalbrewtime >0) { 
          u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
          u8g2.drawGlyph(align_right-11, 3+7, 0x0046);
          u8g2.setFont(u8g2_font_profont22_tf);
          u8g2.setCursor(align_right, 20);
          u8g2.print(totalbrewtime / 1000);
          u8g2.setFont(u8g2_font_profont10_tf);
          u8g2.println("s");
          u8g2.setFont(u8g2_font_open_iconic_other_1x_t);
          u8g2.drawGlyph(align_right-11 , 20+7, 0x047);
        }
      }

      //(optional) add 2 text lines 
      u8g2.setFont(u8g2_font_profont11_tf);
      u8g2.setCursor(ALIGN_CENTER(displaymessagetext), 44);  // 9 pixel space between lines
      u8g2.print(displaymessagetext);
      u8g2.setCursor(ALIGN_CENTER(displaymessagetext2), 53);
      u8g2.print(displaymessagetext2);

      //add status icons
      #if (ENABLE_FAILURE_STATUS_ICONS == 1)
      if (image_flip) {
        byte icon_y = 64-(status_icon_height-1);
        byte icon_counter = 0;
        if ((!force_offline && !wifi_working()) || (force_offline && !FORCE_OFFLINE)) {
          u8g2.drawXBMP(0, 64-status_icon_height+1, status_icon_width, status_icon_height, wifi_not_ok_bits);
          u8g2.drawXBMP(icon_counter*(status_icon_width-1) , icon_y, status_icon_width, status_icon_height, wifi_not_ok_bits);
          icon_counter++;
        }
        if (BLYNK_ENABLE && !blynk_working()) {
          u8g2.drawXBMP(icon_counter*(status_icon_width-1), icon_y, status_icon_width, status_icon_height, blynk_not_ok_bits);
          icon_counter++;
        }
        if (MQTT_ENABLE && !mqtt_working()) {
          u8g2.drawXBMP(icon_counter*(status_icon_width-1), icon_y, status_icon_width, status_icon_height, mqtt_not_ok_bits);
          icon_counter++;
        }
      }
      #endif    

      u8g2.sendBuffer();
    }
  }
}


/***********************************
 * SETUP()
 ***********************************/
void setup() {
  DEBUGSTART(115200);
  Debug.begin(hostname, Debug.DEBUG);
  Debug.setResetCmdEnabled(true); // Enable the reset command
  Debug.showProfiler(true); // Profiler (Good to measure times, to optimize codes)
  Debug.showColors(true); // Colors
  Debug.setSerialEnabled(true); // log to Serial also
  //Serial.setDebugOutput(true); // enable diagnostic output of WiFi libraries
  Debug.setCallBackNewClient(&print_settings);

  /********************************************************
    Define trigger type
  ******************************************************/
  if (triggerType)
  {
    relayON = HIGH;
    relayOFF = LOW;
  } else {
    relayON = LOW;
    relayOFF = HIGH;
  }

  /********************************************************
    Ini Pins
  ******************************************************/
  pinMode(pinRelayVentil, OUTPUT);
  digitalWrite(pinRelayVentil, relayOFF);
  pinMode(pinRelayPumpe, OUTPUT);
  digitalWrite(pinRelayPumpe, relayOFF);
  pinMode(pinRelayHeater, OUTPUT);
  digitalWrite(pinRelayHeater, LOW);
  #ifdef BREW_READY_LED
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, LOW);
  #endif

  DEBUG_print("\nMachine: %s\nVersion: %s\n", MACHINE_TYPE, sysVersion);
  if (Display > 0) {
    //u8g2.setBusClock(400000);  //any use?
    u8g2.begin();
    u8g2_prepare();
    u8g2.clearBuffer();
  }
  displaymessage(0, (char*) DISPLAY_TEXT,  (char*) sysVersion);
  delay(1000);

  //if brewswitch is already "on" on startup, then we brew should not start automatically
  if (OnlyPID == 0 && (analogRead(analogPin) >= 700)) { 
    DEBUG_print("brewsitch is already on. Dont brew until it is turned off.");
    waitingForBrewSwitchOff=true; 
  }

  /********************************************************
   Ini PID
  ******************************************************/
  bPID.SetSampleTime(windowSize);
  bPID.SetOutputLimits(0, windowSize);
  bPID.SetMode(AUTOMATIC);

  /********************************************************
     BLYNK & Fallback offline
  ******************************************************/
  if (!force_offline) {
    checkWifi(true, 12000); // wait up to 12 seconds for connection
    if (!wifi_working()) {
      ERROR_print("Cannot connect to WIFI %s. Disabling WIFI\n", ssid);
      if (DISABLE_SERVICES_ON_STARTUP_ERRORS) {
        force_offline = true;
        mqtt_disabled_temporary = true;
        blynk_disabled_temporary = true;
        lastWifiConnectionAttempt = millis();
      }
      displaymessage(0, "Cant connect to Wifi", "");
      delay(1000);
    } else {
      DEBUG_print("IP address: %s\n", WiFi.localIP().toString().c_str());

      // MQTT
      #if (MQTT_ENABLE == 1)
        snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "will");
        snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
        mqtt_client.setServer(mqtt_server_ip, mqtt_server_port);
        mqtt_client.setCallback(mqtt_callback); // implement when functionality is needed
        if (!mqtt_reconnect(true)) {
          if (DISABLE_SERVICES_ON_STARTUP_ERRORS) mqtt_disabled_temporary = true;
          ERROR_print("Cannot connect to MQTT. Disabling...\n");
          //displaymessage(0, "Cannt connect to MQTT", "");
          //delay(1000);
        }
      #elif (MQTT_ENABLE == 2) //XXX
        DEBUG_print("Starting MQTT service\n");
        const unsigned int max_subscriptions = 30;
        const unsigned int max_retained_topics = 30;
        const unsigned int mqtt_service_port = 1883;
        snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
        MQTT_server_onData(mqtt_callback);
        if (MQTT_server_start(mqtt_service_port, max_subscriptions, max_retained_topics)) {
          if (!MQTT_local_subscribe((unsigned char *)topic_set, 0)) {
            ERROR_print("Cannot subscribe to local MQTT service\n");
          }
          
        } else {
          if (DISABLE_SERVICES_ON_STARTUP_ERRORS) mqtt_disabled_temporary = true;
          ERROR_print("Cannot create MQTT service. Disabling...\n");
          //displaymessage(0, "Cannt create MQTT service", "");
          //delay(1000);
        }
      #endif

      if (BLYNK_ENABLE) {
        DEBUG_print("Connecting to Blynk ...\n");
        Blynk.config(blynkauth, blynkaddress, blynkport) ;
        if (!Blynk.connect(5000)) {
          if (DISABLE_SERVICES_ON_STARTUP_ERRORS) blynk_disabled_temporary = true;
          ERROR_print("Cannot connect to Blynk. Disabling...\n");
          //displaymessage(0, "Cannt connect to Blynk", "");
          //delay(1000);
        } else {
          //displaymessage(0, "3: Blynk connected", "sync all variables...");
          DEBUG_print("Blynk is online, get latest values\n");
          unsigned long started = millis();
          while (blynk_working() && (millis() < started + 2000))
          {
            Blynk.run();
          }
        }
      }  
    }
    
  } else {
    DEBUG_print("Staying offline due to force_offline=1\n");
  }

  /********************************************************
   * READ/SAVE EEPROM
   *  get latest values from EEPROM if blynk is not working/enabled. 
   *  Additionally this function honors changed values in userConfig.h (changed values have priority)
  ******************************************************/
  sync_eeprom(true, !blynk_working());  //TODO: implement MQTT_ENABLE==1 mqtt reading + sync.
  print_settings();
  if (mqtt_working()) mqtt_publish_settings();

  /********************************************************
     OTA
  ******************************************************/
  if (ota && !force_offline) {
    //wifi connection is done during blynk connection
    ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
    ArduinoOTA.setPassword(OTApass);  //  Password for OTA
    ArduinoOTA.begin();
  }

  /********************************************************
    movingaverage ini array
  ******************************************************/
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readingstemp[thisReading] = 0;
    readingstime[thisReading] = 0;
  }

  /********************************************************
     TEMP SENSOR
  ******************************************************/
  //displaymessage(0, "Init. vars", "");
  if (TempSensor == 1) {
    sensors.begin();
    sensors.getAddress(sensorDeviceAddress, 0);
    sensors.setResolution(sensorDeviceAddress, 10) ;
    while (true) {
      sensors.requestTemperatures();
      previousInput = sensors.getTempCByIndex(0);
      delay(400);
      sensors.requestTemperatures();
      Input = sensors.getTempCByIndex(0);
      if (checkSensor(Input, previousInput)) {
        updateTemperatureHistory(Input);
        break;
      }
      displaymessage(0, "Temp. sensor defect", "");
      ERROR_print("Temp. sensor defect. Cannot read consistant values. Retrying\n");
      delay(400);
    }
  } else if (TempSensor == 2) {
    isrCounter = 950;  //required
    #ifdef NEW_TSIC
    while (true) {
      //previousInput = temperature_simulate_steam();
      previousInput = TSIC.getTemp();
      delay(200);
      //Input = temperature_simulate_steam();
      Input = TSIC.getTemp();
      if (checkSensor(Input, previousInput)) {
        updateTemperatureHistory(Input);
        break;
      }
      displaymessage(0, "Temp. sensor defect", "");
      ERROR_print("Temp. sensor defect. Cannot read consistant values. Retrying\n");
      delay(1000);
    }
    #else
    attachInterrupt(digitalPinToInterrupt(pinTemperature), readTSIC, RISING); //activate TSIC reading
    delay(200);
    while (true) {
      previousInput = getTSICvalue();
      delay(200);
      Input = getTSICvalue();
      if (checkSensor(Input, previousInput)) {
        updateTemperatureHistory(Input);
        break;
      }
      displaymessage(0, "Temp. sensor defect", "");
      ERROR_print("Temp. sensor defect. Cannot read consistant values. Retrying\n");
      delay(1000);
    }
    #endif
  }

  /********************************************************
     REST INIT()
  ******************************************************/
  //Initialisation MUST be at the very end of the init(), otherwise the time comparison in loop() will have a big offset
  unsigned long currentTime = millis();
  previousMillistemp = currentTime;
  previousMillisDisplay = 0;
  previousMillisBlynk = currentTime + 800;
  lastMQTTStatusReportTime = currentTime + 300;
  pidComputeLastRunTime = currentTime;

  /********************************************************
    Timer1 ISR - Initialisierung
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  ******************************************************/
  isrCounter = 0;
  timer1_isr_init();
  timer1_attachInterrupt(onTimer1ISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(isrWakeupTime); // set interrupt time to 10ms
  DEBUG_print("End of setup()\n");
}
