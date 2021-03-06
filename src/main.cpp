/**
 * @file main.cpp
 *
 * @brief Main sketch
 *
 */


// Includes
#include <ArduinoOTA.h>
#include "defaults.h"
#include "pinmapping.h"
#include "userConfig.h"  // needs to be configured by the user
#include "icon.h"
#include "languages.h"
#include "Storage.h"
#include "SysPara.h"

// Libraries
#include <DallasTemperature.h>
#include <WiFiManager.h>
#include <U8g2lib.h>    // i2c display
#include <ZACwire.h>    // new TSIC bus library
#include "PID_v1.h"     // for PID calculation
#include "TSIC.h"       // library for TSIC temp sensor

#include <os.h>


hw_timer_t *timer = NULL;


#if (BREWMODE == 2 || ONLYPIDSCALE == 1)
    #include <HX711_ADC.h>
#endif

MACHINE machine = (enum MACHINE)MACHINEID;

#define HIGH_ACCURACY


enum MachineState {
    kInit = 0,
    kColdStart = 10,
    kSetPointNegative = 19,
    kPidNormal = 20,
    kBrew = 30,
    kShotTimerAfterBrew = 31,
    kBrewDetectionTrailing = 35,
    kSteam = 40,
    kCoolDown = 45,
    kBackflush = 50,
    kEmergencyStop = 80,
    kPidOffline = 90,
    kSensorError = 100,
    keepromError = 110,
};

MachineState machinestate = kInit;
int machinestatecold = 0;
unsigned long machinestatecoldmillis = 0;
MachineState lastmachinestate = kInit;
int lastmachinestatepid = -1;


// Definitions below must be changed in the userConfig.h file
int connectmode = CONNECTMODE;
int offlineMode = 0;
const int OnlyPID = ONLYPID;
const int TempSensorType = TEMPSENSORTYPE;
const int Brewdetection = BREWDETECTION;
const int triggerType = TRIGGERTYPE;
const int VoltageSensorType = VOLTAGESENSORTYPE;
const boolean ota = OTA;
const unsigned long brewswitchDelay = BREWSWITCHDELAY;
int BrewMode = BREWMODE;

// Display
uint8_t oled_i2c = OLED_I2C;

// WiFi
WiFiManager wm;
const unsigned long wifiConnectionDelay = WIFICINNECTIONDELAY;
const unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;
const char *hostname = HOSTNAME;
const char *pass = PASS;
unsigned long lastWifiConnectionAttempt = millis();
unsigned int wifiReconnects = 0;  // actual number of reconnects

// OTA
const char *OTAhost = OTAHOST;
const char *OTApass = OTAPASS;

// Backflush values
const unsigned long fillTime = FILLTIME;
const unsigned long flushTime = FLUSHTIME;
int maxflushCycles = MAXFLUSHCYCLES;

// Voltage Sensor
unsigned long previousMillisVoltagesensorreading = millis();
const unsigned long intervalVoltagesensor = 200;
int VoltageSensorON, VoltageSensorOFF;

// QuickMill thermoblock steam-mode (only for BREWDETECTION = 3)
const int maxBrewDurationForSteamModeQM_ON = 200;   // if brewtime is shorter steam-mode starts
const int minPVSOffTimedForSteamModeQM_OFF = 1500;  // if PVS-off-time is longer steam-mode ends
unsigned long timePVStoON = 0;      // time pinvoltagesensor switched to ON
unsigned long lastTimePVSwasON = 0; // last time pinvoltagesensor was ON
bool steamQM_active = false;        // steam-mode is active
bool brewSteamDetectedQM = false;   // brew/steam detected, not sure yet what it is
bool coolingFlushDetectedQM = false;

#if PRESSURESENSOR
    int offset = OFFSET;
    int fullScale = FULLSCALE;
    int maxPressure = MAXPRESSURE;
    float inputPressure = 0;
    const unsigned long intervalPressure = 200;
    unsigned long previousMillisPressure;  // initialisation at the end of init()
#endif

// Method forward declarations
void setSteamMode(int steamMode);
void setPidStatus(int pidStatus);
void setBackflush(int backflush);
void loopcalibrate();
void looppid();
void initSteamQM();
boolean checkSteamOffQM();
char *number2string(double in);
char *number2string(float in);
char *number2string(int in);
char *number2string(unsigned int in);
int filter(int input);
int readSysParamsFromStorage(void);
int writeSysParamsToStorage(void);

#if MQTT
    bool mqtt_publish(const char *reading, char *payload);
    void writeSysParamsToMQTT(void);
#endif

// Variable declarations
uint8_t pidON = 1;               // 1 = control loop in closed loop
int relayON, relayOFF;           // used for relay trigger type. Do not change!
boolean coldStart = true;        // true = machine just switched on
boolean emergencyStop = false;   // Notstop bei zu hoher Temperatur
double EmergencyStopTemp = 120;  // Temp EmergencyStopTemp
int inX = 0, inY = 0, inOld = 0, inSum = 0; // used for filter()
int bars = 0;  // used for getSignalStrength()
boolean brewDetected = 0;
boolean setupDone = false;
int backflushON = 0;      // 1 = activate backflush
int flushCycles = 0;      // number of active flush cycles
int backflushState = 10;  // counter for state machine

// Moving average - brewdetection
const int numReadings = 15;               // number of values per Array
double readingstemp[numReadings];         // the readings from Temp
unsigned long readingstime[numReadings];  // the readings from time
double readingchangerate[numReadings];

int readIndex = 1;           // the index of the current reading
double total = 0;            // total sum of readingchangerate[]
double heatrateaverage = 0;  // the average over the numReadings
double changerate = 0;       // local change rate of temprature
double heatrateaveragemin = 0;
unsigned long timeBrewdetection = 0;
int timerBrewdetection = 0;  // flag is set if brew was detected
int firstreading = 1;        // Ini of the field, also used for sensor check

// PID - values for offline brewdetection
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;

#if aggbTn == 0
    double aggbKi = 0;
#else
    double aggbKi = aggbKp / aggbTn;
#endif

double aggbKd = aggbTv * aggbKp;
double brewtimersoftware = BREW_SW_TIMER; // 20-5 for detection
double brewboarder = BREWDETECTIONLIMIT;  // brew detection limit
const int PonE = PONE;

// Brewing, 1 = Normale Prefinfusion , 2 = Scale & Shottimer = 2
#include "brewscaleini.h"

// Sensor check
boolean sensorError = false;
int error = 0;
int maxErrorCounter = 10;  // depends on intervaltempmes* , define max seconds for invalid data

// PID controller
unsigned long previousMillistemp;  // initialisation at the end of init()
const unsigned long intervaltempmestsic = 400;
const unsigned long intervaltempmesds18b20 = 400;
int pidMode = 1;  // 1 = Automatic, 0 = Manual

const unsigned int windowSize = 1000;
unsigned int isrCounter = 0;  // counter for ISR
unsigned long windowStartTime;
double Input, Output;
double setPointTemp;
double previousInput = 0;

double BrewSetPoint = SETPOINT;
double setPoint = BrewSetPoint;
double SteamSetPoint = STEAMSETPOINT;
int SteamON = 0;
int SteamFirstON = 0;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double startKp = STARTKP;
double startTn = STARTTN;
double steamKp = STEAMKP;

#if startTn == 0
    double startKi = 0;
#else
    double startKi = startKp / startTn;
#endif

#if aggTn == 0
    double aggKi = 0;
#else
    double aggKi = aggKp / aggTn;
#endif

double aggKd = aggTv * aggKp;

// Timer - ISR for PID calculation and heat realay output
#include "ISR.h"

PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, PonE, DIRECT);

// Dallas temp sensor
OneWire oneWire(PIN_ONEWIRE);  // Setup a oneWire instance to communicate with any OneWire
                                // devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);    // Pass our oneWire reference to Dallas Temperature.
DeviceAddress sensorDeviceAddress;      // arrays to hold device address

// TSIC 306 temp sensor
uint16_t temperature = 0;   // internal variable used to read temeprature
float Temperature_C = 0;    // internal variable that holds the converted temperature in ??C


ZACwire TempSensor(PIN_ONEWIRE, 306);  // set OneWire pin to receive signal from the TSic "306"


// System parameters (current value as pointer to variable, minimum, maximum, optional storage ID)
SysPara<double> sysParaPidKpStart(&startKp, 0, 200, STO_ITEM_PID_KP_START);
SysPara<double> sysParaPidTnStart(&startTn, 0, 999, STO_ITEM_PID_TN_START);
SysPara<double> sysParaPidKpReg(&aggKp, 0, 200, STO_ITEM_PID_KP_REGULAR);
SysPara<double> sysParaPidTnReg(&aggTn, 0, 999, STO_ITEM_PID_TN_REGULAR);
SysPara<double> sysParaPidTvReg(&aggTv, 0, 999, STO_ITEM_PID_TV_REGULAR);
SysPara<double> sysParaPidKpBd(&aggbKp, 0, 200, STO_ITEM_PID_KP_BD);
SysPara<double> sysParaPidTnBd(&aggbTn, 0, 999, STO_ITEM_PID_TN_BD);
SysPara<double> sysParaPidTvBd(&aggbTv, 0, 999, STO_ITEM_PID_TV_BD);
SysPara<double> sysParaBrewSetPoint(&BrewSetPoint, 20, 105, STO_ITEM_BREW_SETPOINT);
SysPara<double> sysParaBrewTime(&brewtime, 0, 60, STO_ITEM_BREW_TIME);
SysPara<double> sysParaBrewSwTimer(&brewtimersoftware, 0, 999, STO_ITEM_BREW_SW_TIMER);
SysPara<double> sysParaBrewThresh(&brewboarder, 0, 999, STO_ITEM_BD_THRESHOLD);
SysPara<double> sysParaPreInfTime(&preinfusion, 0, 10, STO_ITEM_PRE_INFUSION_TIME);
SysPara<double> sysParaPreInfPause(&preinfusionpause, 0, 20, STO_ITEM_PRE_INFUSION_PAUSE);
SysPara<double> sysParaWeightSetPoint(&weightSetpoint, 0, 500, STO_ITEM_WEIGHTSETPOINT);
SysPara<double> sysParaPidKpSteam(&steamKp, 0, 500, STO_ITEM_PID_KP_STEAM);
SysPara<uint8_t> sysParaPidOn(&pidON, 0, 1, STO_ITEM_PID_ON);

// Embedded HTTP Server
#include "EmbeddedWebserver.h"

std::vector<editable_t> editableVars = {
    {"PID_ON", "Enable PID Controller", kUInt8, (void *)&pidON},
    {"PID_KP", "PID P", kDouble, (void *)&aggKp},
    {"PID_TN", "PID I", kDouble, (void *)&aggTn},
    {"PID_TV", "PID D", kDouble, (void *)&aggTv},
    {"TEMP", "Temperature", kDouble, (void *)&Input},
    {"BREW_SET_POINT", "Set point (??C)", kDouble, (void *)&BrewSetPoint},
    {"BREW_TIME", "Brew Time (s)", kDouble, (void *)&brewtime},
    {"BREW_PREINFUSION", "Preinfusion Time (s)", kDouble, (void *)&preinfusion},
    {"BREW_PREINFUSUINPAUSE", "Pause (s)", kDouble, (void *)&preinfusionpause},
    {"PID_BD_KP", "BD P", kDouble, (void *)&aggbKp},
    {"PID_BD_TN", "BD I", kDouble, (void *)&aggbTn},
    {"PID_BD_TV", "BD D", kDouble, (void *)&aggbTv},
    {"PID_BD_TIMER", "PID BD Time (s)", kDouble, (void *)&brewtimersoftware},
    {"PID_BD_BREWBOARDER", "PID BD Sensitivity", kDouble, (void *)&brewboarder},
    {"START_KP", "Start P", kDouble, (void *)&startKp},
    {"START_TN", "Start I", kDouble, (void *)&startTn},
    {"STEAM_MODE", "Steam Mode", rInteger, (void *)&SteamON},
    {"BACKFLUSH_ON", "Backflush", rInteger, (void *)&backflushON},
    {"SCALE_WEIGHTSETPOINT", "Brew weight setpoint (g)",kDouble, (void *)&weightSetpoint},
    {"STEAM_KP", "Steam P",kDouble, (void *)&steamKp},
};

unsigned long lastTempEvent = 0;
unsigned long tempEventInterval = 1000;

// MQTT includes and data structure
#if MQTT
    #include "mqtt.h"

    std::vector<mqttVars_t> mqttVars = {
        {"BrewSetPoint", tDouble, 20, 105, (void *)&BrewSetPoint},
        {"brewtime", tDouble, 0, 60, (void *)&brewtime},
        {"preinfusion", tDouble, 0, 10, (void *)&preinfusion},
        {"preinfusionpause", tDouble, 0, 20, (void *)&preinfusionpause},
        {"pidON", tUInt8, 0, 1, (void *)&pidON},
        {"backflushON", tUInt8, 0, 1, (void *)&backflushON},
        {"aggKp", tDouble, 0, 100, (void *)&aggKp},
        {"aggTn", tDouble, 0, 999, (void *)&aggTn},
        {"aggTv", tDouble, 0, 999, (void *)&aggTv},
        {"aggbKp", tDouble, 0, 100, (void *)&aggbKp},
        {"aggbTn", tDouble, 0, 999, (void *)&aggbTn},
        {"aggbTv", tDouble, 0, 999, (void *)&aggbTv},
        {"steamKp", tDouble, 0, 500, (void *)&steamKp},
    };
#endif

// InfluxDB includes
#if INFLUXDB
    #include "influxdb.h"
#endif


/**
 * @brief Get Wifi signal strength and set bars for display
 */
void getSignalStrength() {
    if (offlineMode) return;

    long rssi;

    if (WiFi.status() == WL_CONNECTED) {
        rssi = WiFi.RSSI();
    } else {
        rssi = -100;
    }

    if (rssi >= -50) {
        bars = 4;
    } else if (rssi < -50 && rssi >= -65) {
        bars = 3;
    } else if (rssi < -65 && rssi >= -75) {
        bars = 2;
    } else if (rssi < -75 && rssi >= -80) {
        bars = 1;
    } else {
        bars = 0;
    }
}

// Display define & template
#if OLED_DISPLAY == 1
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);  // e.g. 1.3"
#endif
#if OLED_DISPLAY == 2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);  // e.g. 0.96"
#endif

// Update for Display
unsigned long previousMillisDisplay;  // initialisation at the end of init()
const unsigned long intervalDisplay = 500;

// Horizontal or vertical display
#if (OLED_DISPLAY == 1 || OLED_DISPLAY == 2)
    #if (DISPLAYTEMPLATE < 20)  // horizontal templates
        #include "display.h"
    #endif

    #if (DISPLAYTEMPLATE >= 20)  // vertical templates
        #include "Displayrotateupright.h"
    #endif

    #if (DISPLAYTEMPLATE == 1)
        #include "Displaytemplatestandard.h"
    #endif

    #if (DISPLAYTEMPLATE == 2)
        #include "Displaytemplateminimal.h"
    #endif

    #if (DISPLAYTEMPLATE == 3)
        #include "Displaytemplatetemponly.h"
    #endif

    #if (DISPLAYTEMPLATE == 4)
        #include "Displaytemplatescale.h"
    #endif

    #if (DISPLAYTEMPLATE == 20)
        #include "Displaytemplateupright.h"
    #endif
#endif

#if (PRESSURESENSOR)  // Pressure sensor connected
    /**
     * Pressure sensor
     * Verify before installation: meassured analog input value (should be 3,300 V
     * for 3,3 V supply) and respective ADC value (3,30 V = 1023)
     */
    void checkPressure() {
        float inputPressureFilter = 0;
        unsigned long currentMillisPressure = millis();

        if (currentMillisPressure - previousMillisPressure >= intervalPressure) {
            previousMillisPressure = currentMillisPressure;

            inputPressure =
                ((analogRead(PIN_PRESSURESENSOR) - offset) * maxPressure * 0.0689476) /
                (fullScale - offset);   // pressure conversion and unit
                                        // conversion [psi] -> [bar]
            inputPressureFilter = filter(inputPressure);

            Serial.printf("pressure raw: %f\n", inputPressure);
            Serial.printf("pressure filtered: %f\n", inputPressureFilter);
        }
    }
#endif


/**
 * @brief Emergency stop if temp is too high
 */
void testEmergencyStop() {
    if (Input > EmergencyStopTemp && emergencyStop == false) {
        emergencyStop = true;
    } else if (Input < 100 && emergencyStop == true) {
        emergencyStop = false;
    }
}


/**
 * @brief Moving average - brewdetection (SW)
 */
void movAvg() {
    if (firstreading) {
        for (int thisReading = 0; thisReading < numReadings; thisReading++) {
            readingstemp[thisReading] = Input;
            readingstime[thisReading] = 0;
            readingchangerate[thisReading] = 0;
        }

        firstreading = 0;
    }

    readingstime[readIndex] = millis();
    readingstemp[readIndex] = Input;

    if (readIndex == numReadings - 1) {
        changerate = (readingstemp[numReadings - 1] - readingstemp[0]) /
                        (readingstime[numReadings - 1] - readingstime[0]) * 10000;
    } else {
        changerate = (readingstemp[readIndex] - readingstemp[readIndex + 1]) /
                        (readingstime[readIndex] - readingstime[readIndex + 1]) *
                        10000;
    }

    readingchangerate[readIndex] = changerate;
    total = 0;

    for (int i = 0; i < numReadings; i++) {
        total += readingchangerate[i];
    }

    heatrateaverage = total / numReadings * 100;

    if (heatrateaveragemin > heatrateaverage) {
        heatrateaveragemin = heatrateaverage;
    }

    if (readIndex >= numReadings - 1) {
        // ...wrap around to the beginning:
        readIndex = 0;
    } else {
        readIndex++;
    }
}

/**
 * @brief check sensor value.
 * @return If < 0 or difference between old and new >25, then increase error.
 *      If error is equal to maxErrorCounter, then set sensorError
 */
boolean checkSensor(float tempInput) {
    boolean sensorOK = false;
    boolean badCondition = (tempInput < 0 || tempInput > 150 || fabs(tempInput - previousInput) > 5);

    if (badCondition && !sensorError) {
        error++;
        sensorOK = false;

        if (error >= 5) {  // warning after 5 times error
            Serial.printf(
                "*** WARNING: temperature sensor reading: consec_errors = %i, "
                "temp_current = %.1f\n",
                error, tempInput);
        }
    } else if (badCondition == false && sensorOK == false) {
        error = 0;
        sensorOK = true;
    }

    if (error >= maxErrorCounter && !sensorError) {
        sensorError = true;
        Serial.printf(
            "*** ERROR: temperature sensor malfunction: temp_current = %.1f\n",
            tempInput);
    } else if (error == 0 && sensorError) {
        sensorError = false;
    }

    return sensorOK;
}

/**
 * @brief Refresh temperature.
 *      Each time checkSensor() is called to verify the value.
 *      If the value is not valid, new data is not stored.
 */
void refreshTemp() {
  unsigned long currentMillistemp = millis();
  previousInput = Input;

    if (TempSensorType == 1) {
        if (currentMillistemp - previousMillistemp >= intervaltempmesds18b20) {
            previousMillistemp = currentMillistemp;
            sensors.requestTemperatures();

            if (!checkSensor(sensors.getTempCByIndex(0)) && firstreading == 0)
                return; // if sensor data is not valid, abort function; Sensor must
                        // be read at least one time at system startup

            Input = sensors.getTempCByIndex(0);

            if (Brewdetection != 0) {
                movAvg();
            } else if (firstreading != 0) {
                firstreading = 0;
            }
        }
    }

    if (TempSensorType == 2) {
        if (currentMillistemp - previousMillistemp >= intervaltempmestsic) {
            previousMillistemp = currentMillistemp;

            /* variable "temperature" must be set to zero, before reading new
            * data getTemperature only updates if data is valid, otherwise
            * "temperature" will still hold old values
            */
            temperature = 0;

            Temperature_C = TempSensor.getTemp();

            if (!checkSensor(Temperature_C) && firstreading == 0)
                return; // if sensor data is not valid, abort function; Sensor must
                        // be read at least one time at system startup

            Input = Temperature_C;

            if (Brewdetection != 0) {
                movAvg();
            } else if (firstreading != 0) {
                firstreading = 0;
            }
        }
    }
}


#include "brewvoid.h"
#include "scalevoid.h"

/**
 * @brief Switch to offline mode if maxWifiReconnects were exceeded during boot
 */
void initOfflineMode() {
    #if OLED_DISPLAY != 0
        displayMessage("", "", "", "", "Begin Fallback,", "No Wifi");
    #endif

    Serial.println("Start offline mode with eeprom values, no wifi :(");
    offlineMode = 1;

    if (readSysParamsFromStorage() != 0) {
        #if OLED_DISPLAY != 0
            displayMessage("", "", "", "", "No eeprom,", "Values");
        #endif

        Serial.println(
            "No working eeprom value, I am sorry, but use default offline value "
            ":)");

        delay(1000);
    }
}


/**
 * @brief Brewdetection
 */
void brewdetection() {
    if (brewboarder == 0) return;  // abort brewdetection if deactivated

    // Brew detecion: 1 = software solution, 2 = hardware, 3 = voltage sensor
    if (Brewdetection == 1) {
        if (timerBrewdetection == 1) {
            brewTime = millis() - timeBrewdetection;
        }

        // deactivate brewtimer after end of brewdetection pid
        if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1) {
            timerBrewdetection = 0;  // rearm brewdetection

            if (machinestate != 30) { // if Onlypid = 1, brewTime > 0, no reset of brewTime in case of brewing.
                brewTime = 0;
            }
        }
    } else if (Brewdetection == 2) {
        if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1) {
            timerBrewdetection = 0;  // rearm brewdetection
        }
    } else if (Brewdetection == 3) {
        // brewTime counter
        if ((digitalRead(PIN_BREWSWITCH) == VoltageSensorON) && brewDetected == 1) {
            brewTime = millis() - startingTime;
            lastbrewTime = brewTime;
        }

        //  OFF: reset brew
        if ((digitalRead(PIN_BREWSWITCH) == VoltageSensorOFF) &&(brewDetected == 1 || coolingFlushDetectedQM == true)) {
            brewDetected = 0;
            timePVStoON = brewTime;  // for QuickMill
            brewTime = 0;
            startingTime = 0;
            coolingFlushDetectedQM = false;
            Serial.println("HW Brew - Voltage Sensor - End");
        }

        if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1) {  // reset PID Brew
            timerBrewdetection = 0; // rearm brewdetection
        }
    }

    // Activate brew detection
    if (Brewdetection == 1) {  // SW BD
        // BD PID only +/- 4 Grad Celsius, no detection if HW was active
        if (heatrateaverage <= -brewboarder && timerBrewdetection == 0 && (fabs(Input - BrewSetPoint) < 5)) {
            Serial.println("SW Brew detected");
            timeBrewdetection = millis();
            timerBrewdetection = 1;
        }
    } else if (Brewdetection == 2) {  // HW BD
        if (brewcounter > 10 && brewDetected == 0 && brewboarder != 0) {
            Serial.println("HW Brew detected");
            timeBrewdetection = millis();
            timerBrewdetection = 1;
            brewDetected = 1;
        }
    } else if (Brewdetection == 3) {  // voltage sensor
        switch (machine) {
            case QuickMill:

                if (!coolingFlushDetectedQM) {
                    int pvs = digitalRead(PIN_BREWSWITCH);

                    if (pvs == VoltageSensorON && brewDetected == 0 &&
                        brewSteamDetectedQM == 0 && !steamQM_active) {
                        timeBrewdetection = millis();
                        timePVStoON = millis();
                        timerBrewdetection = 1;
                        brewDetected = 0;
                        lastbrewTime = 0;
                        brewSteamDetectedQM = 1;
                        Serial.println("Quick Mill: setting brewSteamDetectedQM = 1");
                    }

                    if (brewSteamDetectedQM == 1) {
                        if (pvs == VoltageSensorOFF) {
                            brewSteamDetectedQM = 0;

                            if (millis() - timePVStoON < maxBrewDurationForSteamModeQM_ON) {
                                Serial.println("Quick Mill: steam-mode detected");
                                initSteamQM();
                            } else {
                                Serial.printf("*** ERROR: QuickMill: neither brew nor steam\n");
                            }
                        } else if (millis() - timePVStoON > maxBrewDurationForSteamModeQM_ON) {
                            if (Input < BrewSetPoint + 2) {
                                Serial.println("Quick Mill: brew-mode detected");
                                startingTime = timePVStoON;
                                brewDetected = 1;
                                brewSteamDetectedQM = 0;
                            } else {
                                Serial.println("Quick Mill: cooling-flush detected");
                                coolingFlushDetectedQM = true;
                                brewSteamDetectedQM = 0;
                            }
                        }
                    }
                }
                break;

            // no Quickmill:
            default:
                previousMillisVoltagesensorreading = millis();

                if (digitalRead(PIN_BREWSWITCH) == VoltageSensorON && brewDetected == 0) {
                    Serial.println("HW Brew - Voltage Sensor -  Start");
                    timeBrewdetection = millis();
                    startingTime = millis();
                    timerBrewdetection = 1;
                    brewDetected = 1;
                    lastbrewTime = 0;
                }
        }
    }
}


char number2string_double[22];

char *number2string(double in) {
    snprintf(number2string_double, sizeof(number2string_double), "%0.2f", in);

    return number2string_double;
}


char number2string_float[22];

char *number2string(float in) {
    snprintf(number2string_float, sizeof(number2string_float), "%0.2f", in);

    return number2string_float;
}


char number2string_int[22];

char *number2string(int in) {
    snprintf(number2string_int, sizeof(number2string_int), "%d", in);

    return number2string_int;
}


char number2string_uint[22];

char *number2string(unsigned int in) {
    snprintf(number2string_uint, sizeof(number2string_uint), "%u", in);

    return number2string_uint;
}


/**
 * @brief after ~28 cycles the input is set to 99,66% if the real input value sum of inX and inY
 *      multiplier must be 1 increase inX multiplier to make the filter faster
 */
int filter(int input) {
    inX = input * 0.3;
    inY = inOld * 0.7;
    inSum = inX + inY;
    inOld = inSum;

    return inSum;
}


/**
 * @brief SteamON & Quickmill
 */
void checkSteamON() {
    // check digital GIPO
    if (digitalRead(PIN_STEAMSWITCH) == HIGH) {
        SteamON = 1;
    }

    // If switched on via web interface, then SteamFirstON == 1, prevent override
    if (digitalRead(PIN_STEAMSWITCH) == LOW && SteamFirstON == 0) {
        SteamON = 0;
    }

    // monitor QuickMill thermoblock steam-mode
    if (machine == QuickMill) {
        if (steamQM_active == true) {
            if (checkSteamOffQM() == true) {  // if true: steam-mode can be turned off
                SteamON = 0;
                steamQM_active = false;
                lastTimePVSwasON = 0;
            } else {
                SteamON = 1;
            }
        }
    }

    if (SteamON) {
        setPoint = SteamSetPoint;
    }

    if (!SteamON) {
        setPoint = BrewSetPoint;
    }
}

void setEmergencyStopTemp() {
    if (machinestate == kSteam || machinestate == kCoolDown) {
        if (EmergencyStopTemp != 145) EmergencyStopTemp = 145;
    } else {
        if (EmergencyStopTemp != 120) EmergencyStopTemp = 120;
    }
}

void initSteamQM() {
    // Initialize monitoring for steam switch off for QuickMill thermoblock
    lastTimePVSwasON = millis();  // time when pinvoltagesensor changes from ON to OFF
    steamQM_active = true;
    timePVStoON = 0;
    SteamON = 1;
}

boolean checkSteamOffQM() {
    /* Monitor pinvoltagesensor during active steam mode of QuickMill
     * thermoblock. Once the pinvolagesenor remains OFF for longer than a
     * pump-pulse time peride the switch is turned off and steam mode finished.
     */
    if (digitalRead(PIN_BREWSWITCH) == VoltageSensorON) {
        lastTimePVSwasON = millis();
    }

    if ((millis() - lastTimePVSwasON) > minPVSOffTimedForSteamModeQM_OFF) {
        lastTimePVSwasON = 0;
        return true;
    }

    return false;
}


/**
 * @brief State machine
 */
void machinestatevoid() {
    switch (machinestate) {
        case kInit:
            // Prevent coldstart leave by Input 222
            if (Input < (BrewSetPoint - 1) || Input < 150) {
                machinestate = kColdStart;
                Serial.println(Input);
                Serial.println(machinestate);

                // some users have 100 % Output in kInit / Koldstart, reset PID
                pidMode = 0;
                bPID.SetMode(pidMode);
                Output = 0;
                digitalWrite(PIN_HEATER, LOW);  // Stop heating

                // start PID
                pidMode = 1;
                bPID.SetMode(pidMode);
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kColdStart:
            /* One high Input let the state jump to 19.
            * switch (machinestatecold) prevent it, we wait 10 sec with new state.
            * during the 10 sec the Input has to be Input >= (BrewSetPoint-1),
            * If not, reset machinestatecold
            */
            switch (machinestatecold) {
                case 0:
                    if (Input >= (BrewSetPoint - 1) && Input < 150) {
                        machinestatecoldmillis = millis();  // get millis for interval calc
                        machinestatecold = 10;              // new state
                        Serial.println(
                            "Input >= (BrewSetPoint-1), wait 10 sec before machinestate "
                            "19");
                    }
                    break;

                case 10:
                    if (Input < (BrewSetPoint - 1)) {
                        machinestatecold = 0;  //  Input was only one time above
                                            //  BrewSetPoint, reset machinestatecold
                        Serial.println(
                            "Reset timer for machinestate 19: Input < (BrewSetPoint-1)");
                    }

                    if (machinestatecoldmillis + 10 * 1000 <
                        millis())  // 10 sec Input above BrewSetPoint, no set new state
                    {
                        machinestate = kSetPointNegative;
                        Serial.println(
                            "10 sec Input >= (BrewSetPoint-1) finished, switch to state "
                            "19");
                    }
                    break;
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if ((brewTime > 0 && ONLYPID == 1) ||  // brewTime with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)) {
                machinestate = kBrew;
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        // Setpoint -1 Celsius
        case kSetPointNegative:
            brewdetection();  // if brew detected, set PID values

            if (Input >= (BrewSetPoint)) {
                machinestate = kPidNormal;
            }

            if ((brewTime > 0 && ONLYPID == 1) ||  // brewTime with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)) {
                machinestate = kBrew;
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kPidNormal:
            brewdetection();  // if brew detected, set PID values

            if ((brewTime > 0 && ONLYPID == 1) ||  // brewTime with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)) {
                machinestate = kBrew;
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kBrew:
            brewdetection();

            if ((brewTime > 35 * 1000 && Brewdetection == 1 &&
                ONLYPID == 1) ||  // 35 sec later and BD PID active SW Solution
                (brewTime == 0 && Brewdetection == 3 &&
                ONLYPID == 1) ||  // Voltagesensor reset brewTime == 0
                ((brewcounter == 10 || brewcounter == 43) && ONLYPID == 0)) {
                if ((ONLYPID == 1 && Brewdetection == 3) ||
                    ONLYPID ==
                        0) {  // only delay of shotimer for voltagesensor or brewcounter
                    machinestate = kShotTimerAfterBrew;
                    lastbrewTimeMillis = millis();  // for delay
                }

                if (ONLYPID == 1 && Brewdetection == 1 && timerBrewdetection == 1) {  // direct to PID BD
                    machinestate = kBrewDetectionTrailing;
                }
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kShotTimerAfterBrew:
            brewdetection();

            if (millis() - lastbrewTimeMillis > BREWSWITCHDELAY) {
                Serial.printf("Bezugsdauer: %4.1f s\n", lastbrewTime / 1000);
                machinestate = kBrewDetectionTrailing;
                lastbrewTime = 0;
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kBrewDetectionTrailing:
            brewdetection();

            if (timerBrewdetection == 0) {
                machinestate = kPidNormal;
            }

            if ((brewTime > 0 && ONLYPID == 1 && Brewdetection == 3) ||  // New Brew inner BD only by Only PID AND Voltage Sensor
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)) {
                machinestate = kBrew;
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kSteam:
            if (!SteamON) {
                machinestate = kCoolDown;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kCoolDown:
            if (Brewdetection == 2 || Brewdetection == 3) {
                /* For quickmill:  steam detection only via switch, calling
                 * brewdetection() detects new steam request
                 */
                brewdetection();
            }

            if (Brewdetection == 1 && ONLYPID == 1) {
                // if local max reached enable state 20, then heat to settemp.
                if (heatrateaverage > 0 && Input < BrewSetPoint + 2) {
                    machinestate = kPidNormal;
                }
            }

            if ((Brewdetection == 3 || Brewdetection == 2) &&
                Input < BrewSetPoint + 2) {
                machinestate = kPidNormal;
            }

            if (SteamON) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kBackflush:
            if (backflushON == 0) {
                machinestate = kPidNormal;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kEmergencyStop:
            if (!emergencyStop) {
                machinestate = kPidNormal;
            }

            if (!pidON) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kPidOffline:
            if (pidON) {
                if (coldStart) {
                    machinestate = kColdStart;
                } else if (!coldStart && (Input > (BrewSetPoint - 10))) {  // Input higher BrewSetPoint-10, normal PID
                    machinestate = kPidNormal;
                } else if (Input <= (BrewSetPoint - 10)) {
                    machinestate =
                        kColdStart;  // Input 10C below set point, enter cold start
                    coldStart = true;
                }
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kSensorError:
            machinestate = kSensorError;
            break;

        case keepromError:
            machinestate = keepromError;
            break;
    }

    if (machinestate != lastmachinestate) {
        Serial.printf("new machinestate: %i -> %i\n", lastmachinestate, machinestate);
        lastmachinestate = machinestate;
    }
}


/**
 * @brief Set up internal WiFi hardware
 */
void wiFiSetup() {
    wm.setCleanConnect(true);
    wm.setBreakAfterConfig(true);

    if (wm.autoConnect(hostname, pass)) {
        Serial.printf("WiFi connected - IP = %i.%i.%i.%i\n", WiFi.localIP()[0],
                    WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);

        byte mac[6];
        WiFi.macAddress(mac);
        String macaddr0 = number2string(mac[0]);
        String macaddr1 = number2string(mac[1]);
        String macaddr2 = number2string(mac[2]);
        String macaddr3 = number2string(mac[3]);
        String macaddr4 = number2string(mac[4]);
        String macaddr5 = number2string(mac[5]);
        String completemac = macaddr0 + macaddr1 + macaddr2 + macaddr3 + macaddr4 + macaddr5;
        Serial.printf("MAC-ADRESSE: %s\n", completemac.c_str());

    } else {
        Serial.println("WiFi connection timed out...");

        #if OLED_DISPLAY != 0
            displayLogo(langstring_nowifi[0], langstring_nowifi[1]);
        #endif

        wm.disconnect();
        delay(1000);

        offlineMode = 1;
    }

    #if OLED_DISPLAY != 0
        displayLogo(langstring_connectwifi1, wm.getWiFiSSID(true));
    #endif
}


/**
 * @brief Set up embedded Website
 */
void websiteSetup() {
    setEepromWriteFcn(writeSysParamsToStorage);

    if (readSysParamsFromStorage() != 0) {
        #if OLED_DISPLAY != 0
            displayLogo("3:", "use eeprom values..");
        #endif
    } else {
        #if OLED_DISPLAY != 0
            displayLogo("3:", "config defaults..");
        #endif
    }

  serverSetup();
}


void setup() {
    Serial.begin(115200);

    initTimer1();

    storageSetup();

    // Define trigger type
    if (triggerType) {
        relayON = HIGH;
        relayOFF = LOW;
    } else {
        relayON = LOW;
        relayOFF = HIGH;
    }

    if (VOLTAGESENSORTYPE) {
        VoltageSensorON = HIGH;
        VoltageSensorOFF = LOW;
    } else {
        VoltageSensorON = LOW;
        VoltageSensorOFF = HIGH;
    }

    // Initialize Pins
    pinMode(PIN_VALVE, OUTPUT);
    pinMode(PIN_PUMP, OUTPUT);
    pinMode(PIN_HEATER, OUTPUT);
    pinMode(PIN_STEAMSWITCH, INPUT);
    digitalWrite(PIN_VALVE, relayOFF);
    digitalWrite(PIN_PUMP, relayOFF);
    digitalWrite(PIN_HEATER, LOW);

    // IF Voltage sensor selected
    if (BREWDETECTION == 3) {
        pinMode(PIN_BREWSWITCH, INPUT);
    }

    // IF PIN_BREWSWITCH & Steam selected
    if (PIN_BREWSWITCH > 0) {
        pinMode(PIN_BREWSWITCH, INPUT_PULLDOWN);
    }

    pinMode(PIN_STEAMSWITCH, INPUT_PULLDOWN);

    #if OLED_DISPLAY != 0
        u8g2.setI2CAddress(oled_i2c * 2);
        u8g2.begin();
        u8g2_prepare();
        displayLogo("Firmware Revision: ", AUTO_VERSION);
        delay(2000);
    #endif

    // Init Scale by BREWMODE 2 or SHOTTIMER 2
    #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
        initScale();
    #endif

    // WiFi and Fallback offline
    if (connectmode == 1) {  // WiFi Mode
        wiFiSetup();
        websiteSetup();

        // OTA Updates
        if (ota && WiFi.status() == WL_CONNECTED) {
            ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
            ArduinoOTA.setPassword(OTApass);  //  Password for OTA
            ArduinoOTA.begin();
        }

        #if MQTT
            snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "will");
            snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
            mqtt.setServer(mqtt_server_ip, mqtt_server_port);
            mqtt.setCallback(mqtt_callback);
            checkMQTT();
        #endif

        #if INFLUXDB
            influxDbSetup();
        #endif
    }

    // Initialize PID controller
    bPID.SetSampleTime(windowSize);
    bPID.SetOutputLimits(0, windowSize);
    bPID.SetMode(AUTOMATIC);

    // Temp sensor
    if (TempSensorType == 1) {
        sensors.begin();
        sensors.getAddress(sensorDeviceAddress, 0);
        sensors.setResolution(sensorDeviceAddress, 10);
        sensors.requestTemperatures();
        Input = sensors.getTempCByIndex(0);
    }

    if (TempSensorType == 2) {
        temperature = 0;
        Input = TempSensor.getTemp();
    }

    // moving average ini array
    if (Brewdetection == 1) {
        for (int thisReading = 0; thisReading < numReadings; thisReading++) {
            readingstemp[thisReading] = 0;
            readingstime[thisReading] = 0;
            readingchangerate[thisReading] = 0;
        }
    }

    // Initialisation MUST be at the very end of the init(), otherwise the
    // time comparision in loop() will have a big offset
    unsigned long currentTime = millis();
    previousMillistemp = currentTime;
    windowStartTime = currentTime;
    previousMillisDisplay = currentTime;
    previousMillisVoltagesensorreading = currentTime;

    #if MQTT
        lastMQTTConnectionAttempt = currentTime;
        previousMillisMQTT = currentTime;
    #endif

    #if INFLUXDB
        previousMillisInflux = currentTime;
    #endif

    #if (BREWMODE == 2)
        previousMillisScale = currentTime;
    #endif

    #if (PRESSURESENSOR == 1)
        previousMillisPressure = currentTime;
    #endif

    setupDone = true;

    enableTimer1();
}

void loop() {
    // Only do Wifi stuff, if Wifi is connected
    if (WiFi.status() == WL_CONNECTED && offlineMode == 0) {
        #if MQTT
            checkMQTT();
            writeSysParamsToMQTT();

            if (mqtt.connected() == 1) {
                mqtt.loop();
            }
        #endif

        ArduinoOTA.handle();  // For OTA

        // Disable interrupt it OTA is starting, otherwise it will not work
        ArduinoOTA.onStart([]() {
            disableTimer1();
            digitalWrite(PIN_HEATER, LOW);  // Stop heating
        });

        ArduinoOTA.onError([](ota_error_t error) { enableTimer1(); });

        // Enable interrupts if OTA is finished
        ArduinoOTA.onEnd([]() { enableTimer1(); });
    }

    refreshTemp();        // update temperature values
    testEmergencyStop();  // test if temp is too high
    bPID.Compute();

    if ((millis() - lastTempEvent) > tempEventInterval) {
        sendTempEvent(Input, BrewSetPoint, Output);
        lastTempEvent = millis();
    }

    #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
        checkWeight();  // Check Weight Scale in the loop
    #endif

    #if PRESSURESENSOR
        checkPressure();
    #endif

    brew();                     // start brewing if button pressed
    checkSteamON();             // check for steam
    setEmergencyStopTemp();
    machinestatevoid();         // calculate machinestate

    #if INFLUXDB
        sendInflux();
    #endif

    #if (ONLYPIDSCALE == 1)  // only by shottimer 2, scale
        shottimerscale();
    #endif

    // Check if PID should run or not. If not, set to manuel and force output to zero
    #if OLED_DISPLAY != 0
        unsigned long currentMillisDisplay = millis();

        if (currentMillisDisplay - previousMillisDisplay >= 100) {
            displayShottimer();
        }

        if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
            previousMillisDisplay = currentMillisDisplay;

            #if DISPLAYTEMPLATE < 20  // not in vertikal template
                Displaymachinestate();
            #endif

            printScreen();  // refresh display
        }
    #endif

    if (machinestate == kPidOffline || machinestate == kSensorError || machinestate == kEmergencyStop || machinestate == keepromError) {
        if (pidMode == 1) {
            // Force PID shutdown
            pidMode = 0;
            bPID.SetMode(pidMode);
            Output = 0;
            digitalWrite(PIN_HEATER, LOW);  // Stop heating
        }
    } else {  // no sensorerror, no pid off or no Emergency Stop
        if (pidMode == 0) {
            pidMode = 1;
            bPID.SetMode(pidMode);
        }
    }

    // Set PID if first start of machine detected, and no SteamON
    if (machinestate == kInit || machinestate == kColdStart || machinestate == kSetPointNegative) {
        if (startTn != 0) {
            startKi = startKp / startTn;
        } else {
            startKi = 0;
        }

        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", startKp, startKi, 0.0);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(startKp, startKi, 0, P_ON_M);
    }

    if (machinestate == kPidNormal) {
        // Prevent overwriting of brewdetection values
        // calc ki, kd
        if (aggTn != 0) {
            aggKi = aggKp / aggTn;
        } else {
            aggKi = 0;
        }

        aggKd = aggTv * aggKp;

        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggKp, aggKi, aggKd);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(aggKp, aggKi, aggKd, PonE);
        coldStart = false;
    }

    // BD PID
    if (machinestate >= 30 && machinestate <= 35) {
        // calc ki, kd
        if (aggbTn != 0) {
            aggbKi = aggbKp / aggbTn;
        } else {
            aggbKi = 0;
        }

        aggbKd = aggbTv * aggbKp;

        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggbKp, aggbKi, aggbKd);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(aggbKp, aggbKi, aggbKd, PonE);
    }

    // Steam on
    if (machinestate == kSteam) {
        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", 150.0, 0.0, 0.0);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(steamKp, 0, 0, PonE);
    }

    // chill-mode after steam
    if (machinestate == kCoolDown) {
        switch (machine) {
            case QuickMill:
                aggbKp = 150;
                aggbKi = 0;
                aggbKd = 0;
                break;

            default:
                // calc ki, kd
                if (aggbTn != 0) {
                    aggbKi = aggbKp / aggbTn;
                } else {
                    aggbKi = 0;
                }

                aggbKd = aggbTv * aggbKp;
        }

        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggbKp, aggbKi, aggbKd);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(aggbKp, aggbKi, aggbKd, PonE);
    }
    // sensor error OR Emergency Stop
}

void setBackflush(int backflush) {
    backflushON = backflush;
}


void setSteamMode(int steamMode) {
    SteamON = steamMode;

    if (SteamON) {
        SteamFirstON = 1;
    }

    if (!SteamON) {
        SteamFirstON = 0;
    }
}

void setPidStatus(int pidStatus) {
    pidON = pidStatus;
}

/**
 * @brief Reads all system parameter values from non-volatile storage
 *
 * @return 0 = success, < 0 = failure
 */
int readSysParamsFromStorage(void) {
    if (sysParaPidKpStart.getStorage() != 0) return -1;
    if (sysParaPidTnStart.getStorage() != 0) return -1;
    if (sysParaPidKpReg.getStorage() != 0) return -1;
    if (sysParaPidTnReg.getStorage() != 0) return -1;
    if (sysParaPidTvReg.getStorage() != 0) return -1;
    if (sysParaPidKpBd.getStorage() != 0) return -1;
    if (sysParaPidTnBd.getStorage() != 0) return -1;
    if (sysParaPidTvBd.getStorage() != 0) return -1;
    if (sysParaBrewSetPoint.getStorage() != 0) return -1;
    if (sysParaBrewTime.getStorage() != 0) return -1;
    if (sysParaBrewSwTimer.getStorage() != 0) return -1;
    if (sysParaBrewThresh.getStorage() != 0) return -1;
    if (sysParaPreInfTime.getStorage() != 0) return -1;
    if (sysParaPreInfPause.getStorage() != 0) return -1;
    if (sysParaWeightSetPoint.getStorage() != 0) return -1;
    if (sysParaPidOn.getStorage() != 0) return -1;
    if (sysParaPidKpSteam.getStorage() != 0) return -1;

    return 0;
}

/**
 * @brief Writes all current system parameter values to non-volatile storage
 *
 * @return 0 = success, < 0 = failure
 */
int writeSysParamsToStorage(void) {
    if (sysParaPidKpStart.setStorage() != 0) return -1;
    if (sysParaPidTnStart.setStorage() != 0) return -1;
    if (sysParaPidKpReg.setStorage() != 0) return -1;
    if (sysParaPidTnReg.setStorage() != 0) return -1;
    if (sysParaPidTvReg.setStorage() != 0) return -1;
    if (sysParaPidKpBd.setStorage() != 0) return -1;
    if (sysParaPidTnBd.setStorage() != 0) return -1;
    if (sysParaPidTvBd.setStorage() != 0) return -1;
    if (sysParaBrewSetPoint.setStorage() != 0) return -1;
    if (sysParaBrewTime.setStorage() != 0) return -1;
    if (sysParaBrewSwTimer.setStorage() != 0) return -1;
    if (sysParaBrewThresh.setStorage() != 0) return -1;
    if (sysParaPreInfTime.setStorage() != 0) return -1;
    if (sysParaPreInfPause.setStorage() != 0) return -1;
    if (sysParaWeightSetPoint.setStorage() != 0) return -1;
    if (sysParaPidOn.setStorage() != 0) return -1;
    if (sysParaPidKpSteam.setStorage() != 0) return -1;
    return storageCommit();
}


/**
 * @brief Performs a factory reset.
 *
 * @return 0 = success, < 0 = failure
 */
int factoryReset(void) {
    int stoStatus;

    if ((stoStatus = storageFactoryReset()) != 0)
        return stoStatus;

    return readSysParamsFromStorage();
}

