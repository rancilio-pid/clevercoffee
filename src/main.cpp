/**
 * @file main.cpp
 *
 * @brief Main sketch
 *
 * @version 4.0.0 Master
 */

// Firmware version
#define FW_VERSION    4
#define FW_SUBVERSION 0
#define FW_HOTFIX     0
#define FW_BRANCH     "MASTER"

// Libraries
#include <ArduinoOTA.h>
#include <map>
#include <LittleFS.h>

#if TEMPSENSOR == 1
    #include <DallasTemperature.h>  // Library for dallas temp sensor
#endif

#include <WiFiManager.h>
#include <U8g2lib.h>            // i2c display
#include <ZACwire.h>            // new TSIC bus library
#include "PID_v1.h"             // for PID calculation

// Includes
#include "icon.h"               // user icons for display
#include "languages.h"          // for language translation
#include "Storage.h"
#include "ISR.h"
#include "debugSerial.h"
#include "pinmapping.h"
#include "userConfig.h"         // needs to be configured by the user
#include "defaults.h"
#include <os.h>

hw_timer_t *timer = NULL;


#if OLED_DISPLAY == 3
#include <SPI.h>
#endif

#if (BREWMODE == 2 || ONLYPIDSCALE == 1)
    #include <HX711_ADC.h>
#endif

// Version of userConfig need to match, checked by preprocessor
#if (FW_VERSION != USR_FW_VERSION) || \
    (FW_SUBVERSION != USR_FW_SUBVERSION) || \
    (FW_HOTFIX != USR_FW_HOTFIX)
    #error Version of userConfig file and main.cpp need to match!
#endif

MACHINE machine = (enum MACHINE)MACHINEID;

#define HIGH_ACCURACY

#include "PeriodicTrigger.h"
PeriodicTrigger logbrew(500);

enum MachineState {
    kInit = 0,
    kColdStart = 10,
    kAtSetpoint = 19,
    kPidNormal = 20,
    kBrew = 30,
    kShotTimerAfterBrew = 31,
    kBrewDetectionTrailing = 35,
    kSteam = 40,
    kCoolDown = 45,
    kBackflush = 50,
    kWaterEmpty = 70,
    kEmergencyStop = 80,
    kPidOffline = 90,
    kStandby = 95,
    kSensorError = 100,
    kEepromError = 110,
};

MachineState machineState = kInit;
int machinestatecold = 0;
unsigned long machinestatecoldmillis = 0;
MachineState lastmachinestate = kInit;
int lastmachinestatepid = -1;

// Definitions below must be changed in the userConfig.h file
int connectmode = CONNECTMODE;

int offlineMode = 0;
const int OnlyPID = ONLYPID;
const int TempSensor = TEMPSENSOR;
const int brewDetectionMode = BREWDETECTION;
const int triggerType = TRIGGERTYPE;
const int optocouplerType = OPTOCOUPLERTYPE;
const boolean ota = OTA;
int BrewMode = BREWMODE;

// Display
uint8_t oled_i2c = OLED_I2C;

// WiFi
uint8_t wifiCredentialsSaved = 0;
WiFiManager wm;
const unsigned long wifiConnectionDelay = WIFICONNECTIONDELAY;
const unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;
const char *hostname = HOSTNAME;
const char *pass = PASS;
unsigned long lastWifiConnectionAttempt = millis();
unsigned int wifiReconnects = 0;  // actual number of reconnects

// OTA
const char *OTApass = OTAPASS;

// Backflush values
const unsigned long fillTime = FILLTIME;
const unsigned long flushTime = FLUSHTIME;
int maxflushCycles = MAXFLUSHCYCLES;

// Optocoupler
unsigned long previousMillisOptocouplerReading = millis();
const unsigned long intervalOptocoupler = 200;
int optocouplerOn, optocouplerOff;

// QuickMill thermoblock steam-mode (only for BREWDETECTION = 3)
const int maxBrewDurationForSteamModeQM_ON = 200;           // if brewtime is shorter steam-mode starts
const int minOptocouplerOffTimedForSteamModeQM_Off = 1500;  // if optocoupler-off-time is longer steam-mode ends
unsigned long timeOptocouplerOn = 0;                        // time optocoupler switched to ON
unsigned long lastTimeOptocouplerOn = 0;                    // last time optocoupler was ON
bool steamQM_active = false;                                // steam-mode is active
bool brewSteamDetectedQM = false;                           // brew/steam detected, not sure yet what it is
bool coolingFlushDetectedQM = false;

// Pressure sensor
#if (PRESSURESENSOR == 1)   // Pressure sensor connected
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
void setNormalPIDTunings();
void setBDPIDTunings();
void loopcalibrate();
void looppid();
void loopLED();
void loopWater();
void printMachineState();
char const* machinestateEnumToString(MachineState machineState);
void initSteamQM();
boolean checkSteamOffQM();
char *number2string(double in);
char *number2string(float in);
char *number2string(int in);
char *number2string(unsigned int in);
float filterPressureValue(float input);
int writeSysParamsToMQTT(bool continueOnError);
void updateStandbyTimer(void);
void resetStandbyTimer(void);


// system parameters
uint8_t pidON = 0;                 // 1 = control loop in closed loop
double brewSetpoint = SETPOINT;
double brewTempOffset = TEMPOFFSET;
double setpoint = brewSetpoint;
double steamSetpoint = STEAMSETPOINT;
uint8_t usePonM = 0;               // 1 = use PonM for cold start PID, 0 = use normal PID for cold start
double steamKp = STEAMKP;
double startKp = STARTKP;
double startTn = STARTTN;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double aggIMax = AGGIMAX;
double brewtime = BREW_TIME;                        // brewtime in s
double preinfusion = PRE_INFUSION_TIME;             // preinfusion time in s
double preinfusionpause = PRE_INFUSION_PAUSE_TIME;  // preinfusion pause time in s
double weightSetpoint = SCALE_WEIGHTSETPOINT;

// PID - values for offline brew detection
uint8_t useBDPID = 0;
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;

#if aggbTn == 0
    double aggbKi = 0;
#else
    double aggbKi = aggbKp / aggbTn;
#endif

double aggbKd = aggbTv * aggbKp;
double brewtimesoftware = BREW_SW_TIME;  // use userConfig time until disabling BD PID
double brewSensitivity = BD_SENSITIVITY;  // use userConfig brew detection sensitivity
double brewPIDDelay = BREW_PID_DELAY;      // use userConfig brew detection PID delay

uint8_t standbyModeOn = 0;
double standbyModeTime = STANDBY_MODE_TIME;

#include "standby.h"

// system parameter EEPROM storage wrappers (current value as pointer to variable, minimum, maximum, optional storage ID)
SysPara<uint8_t> sysParaPidOn(&pidON, 0, 1, STO_ITEM_PID_ON);
SysPara<uint8_t> sysParaUsePonM(&usePonM, 0, 1, STO_ITEM_PID_START_PONM);
SysPara<double> sysParaPidKpStart(&startKp, PID_KP_START_MIN, PID_KP_START_MAX, STO_ITEM_PID_KP_START);
SysPara<double> sysParaPidTnStart(&startTn, PID_TN_START_MIN, PID_TN_START_MAX, STO_ITEM_PID_TN_START);
SysPara<double> sysParaPidKpReg(&aggKp, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, STO_ITEM_PID_KP_REGULAR);
SysPara<double> sysParaPidTnReg(&aggTn, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, STO_ITEM_PID_TN_REGULAR);
SysPara<double> sysParaPidTvReg(&aggTv, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, STO_ITEM_PID_TV_REGULAR);
SysPara<double> sysParaPidIMaxReg(&aggIMax, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, STO_ITEM_PID_I_MAX_REGULAR);
SysPara<double> sysParaPidKpBd(&aggbKp, PID_KP_BD_MIN, PID_KP_BD_MAX, STO_ITEM_PID_KP_BD);
SysPara<double> sysParaPidTnBd(&aggbTn, PID_TN_BD_MIN, PID_KP_BD_MAX, STO_ITEM_PID_TN_BD);
SysPara<double> sysParaPidTvBd(&aggbTv, PID_TV_BD_MIN, PID_TV_BD_MAX, STO_ITEM_PID_TV_BD);
SysPara<double> sysParaBrewSetpoint(&brewSetpoint, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, STO_ITEM_BREW_SETPOINT);
SysPara<double> sysParaTempOffset(&brewTempOffset, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, STO_ITEM_BREW_TEMP_OFFSET);
SysPara<double> sysParaBrewPIDDelay(&brewPIDDelay, BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, STO_ITEM_BREW_PID_DELAY);
SysPara<uint8_t> sysParaUseBDPID(&useBDPID, 0, 1, STO_ITEM_USE_BD_PID);
SysPara<double> sysParaBrewTime(&brewtime, BREW_TIME_MIN, BREW_TIME_MAX, STO_ITEM_BREW_TIME);
SysPara<double> sysParaBrewSwTime(&brewtimesoftware, BREW_SW_TIME_MIN, BREW_SW_TIME_MAX, STO_ITEM_BREW_SW_TIME);
SysPara<double> sysParaBrewThresh(&brewSensitivity, BD_THRESHOLD_MIN, BD_THRESHOLD_MAX, STO_ITEM_BD_THRESHOLD);
SysPara<uint8_t> sysParaWifiCredentialsSaved(&wifiCredentialsSaved, 0, 1, STO_ITEM_WIFI_CREDENTIALS_SAVED);
SysPara<double> sysParaPreInfTime(&preinfusion, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, STO_ITEM_PRE_INFUSION_TIME);
SysPara<double> sysParaPreInfPause(&preinfusionpause, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, STO_ITEM_PRE_INFUSION_PAUSE);
SysPara<double> sysParaPidKpSteam(&steamKp, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, STO_ITEM_PID_KP_STEAM);
SysPara<double> sysParaSteamSetpoint(&steamSetpoint, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, STO_ITEM_STEAM_SETPOINT);
SysPara<double> sysParaWeightSetpoint(&weightSetpoint, WEIGHTSETPOINT_MIN, WEIGHTSETPOINT_MAX, STO_ITEM_WEIGHTSETPOINT);
SysPara<uint8_t> sysParaStandbyModeOn(&standbyModeOn, 0, 1, STO_ITEM_STANDBY_MODE_ON);
SysPara<double> sysParaStandbyModeTime(&standbyModeTime, STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, STO_ITEM_STANDBY_MODE_TIME);

// Other variables
int relayON, relayOFF;           // used for relay trigger type. Do not change!
boolean coldstart = true;        // true = Rancilio started for first time
boolean emergencyStop = false;   // Emergency stop if temperature is too high
double EmergencyStopTemp = 120;  // Temp EmergencyStopTemp
float inX = 0, inY = 0, inOld = 0, inSum = 0; // used for filterPressureValue()
int signalBars = 0;              // used for getSignalStrength()
boolean brewDetected = 0;
boolean setupDone = false;
int backflushON = 0;             // 1 = backflush mode active
int flushCycles = 0;             // number of active flush cycles
int backflushState = 10;         // counter for state machine

// Water sensor
boolean waterFull = true;
unsigned long lastWaterCheck;
const unsigned long WaterCheckInterval = 200;   // Check water level every 200 ms
int waterCheckConsecutiveReads = 0;             // Counter for consecutive readings of water sensor
const int waterCountsNeeded = 3;                // Number of same readings to change water sensing


// Moving average for software brew detection
double tempRateAverage = 0;             // average value of temp values
double tempChangeRateAverageMin = 0;
unsigned long timeBrewDetection = 0;
int isBrewDetected = 0;                 // flag is set if brew was detected
bool movingAverageInitialized = false;  // flag set when average filter is initialized, also used for sensor check

// Brewing, 1 = Normal Preinfusion , 2 = Scale & Shottimer = 2
#include "brewscaleini.h"

// Sensor check
boolean sensorError = false;
int error = 0;
int maxErrorCounter = 10;        // depends on intervaltempmes* , define max seconds for invalid data

// PID controller
unsigned long previousMillistemp;    // initialisation at the end of init()
const unsigned long intervaltempmestsic = 400;
const unsigned long intervaltempmesds18b20 = 400;
int pidMode = 1;    // 1 = Automatic, 0 = Manual

double setpointTemp;
double previousInput = 0;

// Variables to hold PID values (Temp input, Heater output)
double temperature, pidOutput;
int steamON = 0;
int steamFirstON = 0;

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

// Timer - ISR for PID calculation and heat relay output
#include "ISR.h"

PID bPID(&temperature, &pidOutput, &setpoint, aggKp, aggKi, aggKd, 1, DIRECT);

// Dallas temp sensor
#if TEMPSENSOR == 1
    OneWire oneWire(PINTEMPSENSOR);         // Setup a OneWire instance to communicate with OneWire
                                            // devices (not just Maxim/Dallas temperature ICs)
    DallasTemperature sensors(&oneWire);
    DeviceAddress sensorDeviceAddress;      // arrays to hold device address
#endif

// TSIC 306 temp sensor
ZACwire Sensor2(PIN_TEMPSENSOR, 306);    // set pin to receive signal from the TSic 306

// Embedded HTTP Server
#include "EmbeddedWebserver.h"


enum SectionNames {
    sPIDSection,
    sTempSection,
    sBDSection,
    sPowerSection,
    sOtherSection
};

std::map<String, editable_t> editableVars = {};


struct cmp_str
{
   bool operator()(char const *a, char const *b) const
   {
      return strcmp(a, b) < 0;
   }
};

// MQTT
#include "MQTT.h"

std::map<const char*, std::function<editable_t*()>, cmp_str> mqttVars = {};
std::map<const char*, std::function<double()>, cmp_str> mqttSensors = {};

unsigned long lastTempEvent = 0;
unsigned long tempEventInterval = 1000;

#if MQTT_HASSIO_SUPPORT == 1
static unsigned long lastHomeAssistantDiscoveryExecutionTime = 0;
const unsigned long HomeAssistantDiscoveryExecutionInterval = 300000;  // 5 minute
#endif

bool mqtt_was_connected = false;

/**
 * @brief Get Wifi signal strength and set signalBars for display
 */
void getSignalStrength() {
    if (offlineMode == 1) return;

    long rssi;

    if (WiFi.status() == WL_CONNECTED) {
        rssi = WiFi.RSSI();
    } else {
        rssi = -100;
    }

    if (rssi >= -50) {
        signalBars = 4;
    } else if (rssi < -50 && rssi >= -65) {
        signalBars = 3;
    } else if (rssi < -65 && rssi >= -75) {
        signalBars = 2;
    } else if (rssi < -75 && rssi >= -80) {
        signalBars = 1;
    } else {
        signalBars = 0;
    }
}

// Display define & template
#if OLED_DISPLAY == 1
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_I2CSCL, PIN_I2CSDA);  // e.g. 1.3"
#endif
#if OLED_DISPLAY == 2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_I2CSCL, PIN_I2CSDA);  // e.g. 0.96"
#endif
#if OLED_DISPLAY == 3
    #define OLED_CS             5
    #define OLED_DC             2
    U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_DC, /* reset=*/U8X8_PIN_NONE); // e.g. 1.3"
#endif

// Update for Display
unsigned long previousMillisDisplay;  // initialisation at the end of init()
const unsigned long intervalDisplay = 500;

// Horizontal or vertical display
#if (OLED_DISPLAY != 0)
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


#if (PRESSURESENSOR == 1)
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
            inputPressureFilter = filterPressureValue(inputPressure);

            debugPrintf("pressure raw / filtered: %f / %f\n", inputPressure, inputPressureFilter);
        }
    }
#endif

// Emergency stop if temp is too high
void testEmergencyStop() {
    if (temperature > EmergencyStopTemp && emergencyStop == false) {
        emergencyStop = true;
    } else if (temperature < (brewSetpoint+5) && emergencyStop == true) {
        emergencyStop = false;
    }
}

/**
 * @brief FIR moving average filter for software brew detection
 */
void calculateTemperatureMovingAverage() {
    const int numValues = 15;                      // moving average filter length
    static double tempValues[numValues];           // array of temp values
    static unsigned long timeValues[numValues];    // array of time values
    static double tempChangeRates[numValues];
    static int valueIndex = 1;                     // the index of the current value

    if (brewDetectionMode == 1 && !movingAverageInitialized) {
        for (int index = 0; index < numValues; index++) {
            tempValues[index] = temperature;
            timeValues[index] = 0;
            tempChangeRates[index] = 0;
        }

        movingAverageInitialized = true;
    }

    timeValues[valueIndex] = millis();
    tempValues[valueIndex] = temperature;

    double tempChangeRate = 0;                     // local change rate of temperature
    if (valueIndex == numValues - 1) {
        tempChangeRate = (tempValues[numValues - 1] - tempValues[0]) /
                        (timeValues[numValues - 1] - timeValues[0]) * 10000;
    } else {
        tempChangeRate = (tempValues[valueIndex] - tempValues[valueIndex + 1]) /
                        (timeValues[valueIndex] - timeValues[valueIndex + 1]) * 10000;
    }
    tempChangeRates[valueIndex] = tempChangeRate;

    double totalTempChangeRateSum = 0;
    for (int i = 0; i < numValues; i++) {
        totalTempChangeRateSum += tempChangeRates[i];
    }

    tempRateAverage = totalTempChangeRateSum / numValues * 100;

    if (tempRateAverage < tempChangeRateAverageMin) {
        tempChangeRateAverageMin = tempRateAverage;
    }

    if (valueIndex >= numValues - 1) {
        // ...wrap around to the beginning:
        valueIndex = 0;
    } else {
        valueIndex++;
    }
}

/**
 * @brief check sensor value.
 * @return If < 0 or difference between old and new >25, then increase error.
 *      If error is equal to maxErrorCounter, then set sensorError
 */
boolean checkSensor(float tempInput) {
    boolean sensorOK = false;
    boolean badCondition = (tempInput < 0 || tempInput > 150 || fabs(tempInput - previousInput) > (5+brewTempOffset));

    if (badCondition && !sensorError) {
        error++;
        sensorOK = false;

        debugPrintf(
            "*** WARNING: temperature sensor reading: consec_errors = %i, temp_current = %.1f, temp_prev = %.1f\n",
            error, tempInput, previousInput);
    } else if (badCondition == false && sensorOK == false) {
        error = 0;
        sensorOK = true;
    }

    if (error >= maxErrorCounter && !sensorError) {
        sensorError = true;
        debugPrintf(
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
    previousInput = temperature;

    if (TempSensor == 1) {
        if (currentMillistemp - previousMillistemp >= intervaltempmesds18b20) {
            previousMillistemp = currentMillistemp;

        #if TEMPSENSOR == 1
            sensors.requestTemperatures();
            temperature = sensors.getTempCByIndex(0);
        #endif

            if (machineState != kSteam) {
                temperature -= brewTempOffset;
            }

            if (!checkSensor(temperature) && movingAverageInitialized) {
                temperature = previousInput;
                return; // if sensor data is not valid, abort function; Sensor must
                        // be read at least one time at system startup
            }

            if (brewDetectionMode == 1) {
                calculateTemperatureMovingAverage();
            } else if (!movingAverageInitialized) {
                movingAverageInitialized = true;
            }
        }
    }

    if (TempSensor == 2) {
        if (currentMillistemp - previousMillistemp >= intervaltempmestsic) {
            previousMillistemp = currentMillistemp;

            #if TEMPSENSOR == 2
                temperature = Sensor2.getTemp(15);
            #endif

            if (machineState != kSteam) {
                temperature -= brewTempOffset;
            }

            if (!checkSensor(temperature) && movingAverageInitialized) {
                temperature = previousInput;
                return; // if sensor data is not valid, abort function; Sensor must
                        // be read at least one time at system startup
            }

            if (brewDetectionMode == 1) {
                calculateTemperatureMovingAverage();
            } else if (!movingAverageInitialized) {
                movingAverageInitialized = true;
            }
        }
    }
}


#include "brewvoid.h"
#include "powerswitchvoid.h"
#include "steamswitchvoid.h"
#include "scalevoid.h"

/**
 * @brief Switch to offline mode if maxWifiReconnects were exceeded during boot
 */
void initOfflineMode() {
    #if OLED_DISPLAY != 0
        displayMessage("", "", "", "", "Begin Fallback,", "No Wifi");
    #endif

    debugPrintln("Start offline mode with eeprom values, no wifi :(");
    offlineMode = 1;

    if (readSysParamsFromStorage() != 0) {
        #if OLED_DISPLAY != 0
            displayMessage("", "", "", "", "No eeprom,", "Values");
        #endif

        debugPrintln(
            "No working eeprom value, I am sorry, but use default offline value "
            ":)");

        delay(1000);
    }
}

/**
 * @brief Check if Wifi is connected, if not reconnect abort function if offline, or brew is running
 */
void checkWifi() {
    if (offlineMode == 1 || brewcounter > kBrewIdle) return;

    /* if coldstart is still true when checkWifi() is called, then there was no WIFI connection
     * at boot -> connect and if it does not suceed, enter offlinemode
     */
    do {
        if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
            int statusTemp = WiFi.status();

            if (statusTemp != WL_CONNECTED) {  // check WiFi connection status
                lastWifiConnectionAttempt = millis();
                wifiReconnects++;
                debugPrintf("Attempting WIFI (re-)connection: %i\n", wifiReconnects);

                if (!setupDone) {
                    #if OLED_DISPLAY != 0
                        displayMessage("", "", "", "", langstring_wifirecon, String(wifiReconnects));
                    #endif
                }

                wm.disconnect();
                wm.autoConnect();

                int count = 1;

                while (WiFi.status() != WL_CONNECTED && count <= 20) {
                    delay(100); // give WIFI some time to connect
                    count++;    // reconnect counter, maximum waiting time for
                                // reconnect = 20*100ms
                }
            }
        }

        yield();  // Prevent WDT trigger
    } while (!setupDone && wifiReconnects < maxWifiReconnects && WiFi.status() != WL_CONNECTED);


    if (wifiReconnects >= maxWifiReconnects && WiFi.status() != WL_CONNECTED) { 
        // no wifi connection after trying connection, initiate offline mode
        initOfflineMode();
    } else { 
        wifiReconnects = 0;
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
 * @brief detect if a brew is running
 */
void brewDetection() {
    if (brewDetectionMode == 1 && brewSensitivity == 0) return;  // abort brewdetection if deactivated

    // Brew detection: 1 = software solution, 2 = hardware, 3 = voltage sensor
    if (brewDetectionMode == 1) {
        if (isBrewDetected == 1) {
            timeBrewed = millis() - timeBrewDetection;
        }

        // deactivate brewtimer after end of brewdetection pid
        if (millis() - timeBrewDetection > brewtimesoftware * 1000 && isBrewDetected == 1) {
            isBrewDetected = 0;  // rearm brewDetection
            timeBrewed = 0;
        }
    } else if (brewDetectionMode == 2) {
        if (millis() - timeBrewDetection > brewtimesoftware * 1000 && isBrewDetected == 1) {
            isBrewDetected = 0;  // rearm brewDetection
        }
    } else if (brewDetectionMode == 3) {
        // timeBrewed counter
        if ((digitalRead(PIN_BREWSWITCH) == optocouplerOn) && brewDetected == 1) {
            timeBrewed = millis() - startingTime;
            lastbrewTime = timeBrewed;
        }

        // OFF: reset brew
        if ((digitalRead(PIN_BREWSWITCH) == optocouplerOff) && (brewDetected == 1 || coolingFlushDetectedQM == true)) {
            isBrewDetected = 0;  // rearm brewDetection
            brewDetected = 0;
            timeOptocouplerOn = timeBrewed;  // for QuickMill
            timeBrewed = 0;
            startingTime = 0;
            coolingFlushDetectedQM = false;
            debugPrintln("HW Brew - optocoupler - End");
        }
    }

    // Activate brew detection
    if (brewDetectionMode == 1) {  // SW BD
        // BD PID only +/- 4 °C, no detection if HW was active
        if (tempRateAverage <= -brewSensitivity && isBrewDetected == 0 && (fabs(temperature - brewSetpoint) < 5)) {
            debugPrintln("SW Brew detected");
            timeBrewDetection = millis();
            isBrewDetected = 1;
        }
    } else if (brewDetectionMode == 2) {  // HW BD
        if (brewcounter > kBrewIdle && brewDetected == 0) {
            debugPrintln("HW Brew detected");
            timeBrewDetection = millis();
            isBrewDetected = 1;
            brewDetected = 1;
        }
    } else if (brewDetectionMode == 3) {  // voltage sensor
        switch (machine) {
            case QuickMill:
                if (!coolingFlushDetectedQM) {
                    int pvs = digitalRead(PIN_BREWSWITCH);

                    if (pvs == optocouplerOn && brewDetected == 0 &&
                        brewSteamDetectedQM == 0 && !steamQM_active) {
                        timeBrewDetection = millis();
                        timeOptocouplerOn = millis();
                        isBrewDetected = 1;
                        brewDetected = 0;
                        lastbrewTime = 0;
                        brewSteamDetectedQM = 1;
                        debugPrintln("Quick Mill: setting brewSteamDetectedQM = 1");
                        logbrew.reset();
                    }

                    const unsigned long minBrewDurationForSteamModeQM_ON = 50;
                    if (brewSteamDetectedQM == 1 && millis()-timeOptocouplerOn > minBrewDurationForSteamModeQM_ON) {
                        if (pvs == optocouplerOff) {
                            brewSteamDetectedQM = 0;

                            if (millis() - timeOptocouplerOn < maxBrewDurationForSteamModeQM_ON) {
                                debugPrintln("Quick Mill: steam-mode detected");
                                initSteamQM();
                            } else {
                                debugPrintf("*** ERROR: QuickMill: neither brew nor steam\n");
                            }
                        } else if (millis() - timeOptocouplerOn > maxBrewDurationForSteamModeQM_ON) {
                            if (temperature < brewSetpoint + 2) {
                                debugPrintln("Quick Mill: brew-mode detected");
                                startingTime = timeOptocouplerOn;
                                brewDetected = 1;
                                brewSteamDetectedQM = 0;
                            } else {
                                debugPrintln("Quick Mill: cooling-flush detected");
                                coolingFlushDetectedQM = true;
                                brewSteamDetectedQM = 0;
                            }
                        }
                    }
                }
                break;

            // no Quickmill:
            default:
                previousMillisOptocouplerReading = millis();

                if (digitalRead(PIN_BREWSWITCH) == optocouplerOn && brewDetected == 0) {
                    debugPrintln("HW Brew - Voltage Sensor - Start");
                    timeBrewDetection = millis();
                    startingTime = millis();
                    isBrewDetected = 1;
                    brewDetected = 1;
                    lastbrewTime = 0;
                }
        }
    }
}


/**
 * @brief Filter input value using exponential moving average filter (using fixed coefficients)
 *      After ~28 cycles the input is set to 99,66% if the real input value sum of inX and inY
 *      multiplier must be 1 increase inX multiplier to make the filter faster
 */
float filterPressureValue(float input) {
    inX = input * 0.3;
    inY = inOld * 0.7;
    inSum = inX + inY;
    inOld = inSum;

    return inSum;
}


/**
 * @brief steamON & Quickmill
 */
void checkSteamON() {
    if (STEAMSWITCHTYPE == 0) {
        return;
    }

    // check digital GIPO
    if (digitalRead(PIN_STEAMSWITCH) == HIGH) {
        steamON = 1;
    }

    // if activated via web interface then steamFirstON == 1, prevent override
    if (digitalRead(PIN_STEAMSWITCH) == LOW && steamFirstON == 0) {
        steamON = 0;
    }

    // monitor QuickMill thermoblock steam-mode
    if (machine == QuickMill) {
        if (steamQM_active == true) {
            if (checkSteamOffQM() == true) {  // if true: steam-mode can be turned off
                steamON = 0;
                steamQM_active = false;
                lastTimeOptocouplerOn = 0;
            } else {
                steamON = 1;
            }
        }
    }
}

void setEmergencyStopTemp() {
    if (machineState == kSteam || machineState == kCoolDown) {
        if (EmergencyStopTemp != 145) EmergencyStopTemp = 145;
    } else {
        if (EmergencyStopTemp != 120) EmergencyStopTemp = 120;
    }
}

void initSteamQM() {
    // Initialize monitoring for steam switch off for QuickMill thermoblock
    lastTimeOptocouplerOn = millis();  // time when optocoupler changes from ON to OFF
    steamQM_active = true;
    timeOptocouplerOn = 0;
    steamON = 1;
}

boolean checkSteamOffQM() {
    /* Monitor optocoupler during active steam mode of QuickMill
     * thermoblock. Once the optocoupler remains OFF for longer than a
     * pump-pulse time peride the switch is turned off and steam mode finished.
     */
    if (digitalRead(PIN_BREWSWITCH) == optocouplerOn) {
        lastTimeOptocouplerOn = millis();
    }

    if ((millis() - lastTimeOptocouplerOn) > minOptocouplerOffTimedForSteamModeQM_Off) {
        lastTimeOptocouplerOn = 0;
        return true;
    }

    return false;
}

/**
 * @brief Handle the different states of the machine
 */
void handleMachineState() {
    switch (machineState) {
        case kInit:
            // switch state to kColdStart if temperature is below BrewSetpoint or 150°
            if (temperature < (brewSetpoint - 1) || temperature < 150) {
                machineState = kColdStart;
                debugPrintf("%d\n", temperature);
                debugPrintf("%d\n", machineState);

                // reset PID (some users have 100% output in kInit / KColdstart)
                pidMode = 0;
                bPID.SetMode(pidMode);
                pidOutput = 0;
                digitalWrite(PIN_HEATER, LOW);  // Stop heating

                // start PID again
                pidMode = 1;
                bPID.SetMode(pidMode);
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kColdStart:
            // Once the temperature is above BrewSetpoint for 10 seconds, the machineState is set to kAtSetpoint.
            // if during the 10 seconds the temperature is below BrewSetpoint again,
            // reset machinestatecold to 0
            switch (machinestatecold) {
                case 0:
                    if (temperature >= (brewSetpoint - 1) && temperature < 150) {
                        machinestatecoldmillis = millis();  // get millis for interval calc
                        machinestatecold = 10;              // new state
                        debugPrintln(
                            "temperature >= (BrewSetpoint-1), waiting 10 sec before switching to kAtSetpoint");
                    }
                    break;

                case 10:
                    // if the temperature was not above BrewSetpoint - 1 long enough, reset machinestatecold
                    // This way, noisy temperature errors won't switch the machineState too early
                    if (temperature < (brewSetpoint - 1)) {
                        machinestatecold = 0;
                        debugPrintln("Resetting timer for kAtSetpoint: temperature < (BrewSetpoint-1) again");
                        break;
                    }

                    // 10 sec temperature above BrewSetpoint - 1, set new state
                    if (machinestatecoldmillis + 10 * 1000 < millis()) {
                        machineState = kAtSetpoint;
                        debugPrintln("temperature >= (BrewSetpoint-1) for 10 sec, switching to kAtSetpoint");
                    }
                    break;
            }

            if ((timeBrewed > 0 && ONLYPID == 1) ||  // timeBrewed with Only PID
                (ONLYPID == 0 && brewcounter > kBrewIdle && brewcounter <= kBrewFinished))
            {
                machineState = kBrew;

                if (standbyModeOn) {
                    resetStandbyTimer();
                } 
            }

            if (steamON == 1) {
                machineState = kSteam;

                if (standbyModeOn) {
                    resetStandbyTimer();
                } 
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;

                if (standbyModeOn) {
                    resetStandbyTimer();
                } 
            }

            if (standbyModeOn && standbyModeRemainingTimeMillis == 0) {
                machineState = kStandby;
                pidON = 0;
            }

            if (pidON == 0 && machineState != kStandby) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }

            break;

        // Current temperature is just below the setpoint
        case kAtSetpoint:
            // when temperature has reached BrewSetpoint properly, switch to kPidNormal
            if (temperature >= brewSetpoint) {
                machineState = kPidNormal;
            }

            // is a brew running? (values are set in brewDetection() above)
            if ((timeBrewed > 0 && ONLYPID == 1) ||
                (ONLYPID == 0 && brewcounter > kBrewIdle && brewcounter <= kBrewFinished))
            {
                machineState = kBrew;

                if (standbyModeOn) {
                    resetStandbyTimer();
                } 
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;

                if (standbyModeOn) {
                    resetStandbyTimer();
                } 
            }

            if (steamON == 1) {
                machineState = kSteam;

                if (standbyModeOn) {
                    resetStandbyTimer();
                } 
            }

            if (standbyModeOn && standbyModeRemainingTimeMillis == 0) {
                machineState = kStandby;
                pidON = 0;
            }

            if (pidON == 0 && machineState != kStandby) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }

            break;

        case kPidNormal:
            brewDetection();

            if ((timeBrewed > 0 && ONLYPID == 1) ||  // timeBrewed with Only PID
                (ONLYPID == 0 && brewcounter > kBrewIdle && brewcounter <= kBrewFinished))
            {
                machineState = kBrew;

                if (standbyModeOn) {
                    resetStandbyTimer();
                }
            }

            if (steamON == 1) {
                machineState = kSteam;

                if (standbyModeOn) {
                    resetStandbyTimer();
                } 
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;

                if (standbyModeOn) {
                    resetStandbyTimer();
                } 
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }
             
            if (standbyModeOn && standbyModeRemainingTimeMillis == 0) {
                machineState = kStandby;
                pidON = 0;
            }

            if (pidON == 0 && machineState != kStandby) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }

            break;

        case kBrew:
            brewDetection();

            // Output brew time, temp and tempRateAverage during brew (used for SW BD only)
            if (BREWDETECTION == 1 && logbrew.check()) {
                debugPrintf("(tB,T,hra) --> %5.2f %6.2f %8.2f\n",
                            (double)(millis() - startingTime) / 1000, temperature, tempRateAverage);
            }

            if ((timeBrewed == 0 && brewDetectionMode == 3 && ONLYPID == 1) || // OnlyPID+: Voltage sensor BD timeBrewed == 0 -> switch is off again
                ((brewcounter == kBrewIdle || brewcounter == kWaitBrewOff) && ONLYPID == 0)) // Hardware BD
            {
                // delay shot timer display for voltage sensor or hw brew toggle switch (brew counter)
                machineState = kShotTimerAfterBrew;
                lastbrewTimeMillis = millis();  // for delay
            } else if (brewDetectionMode == 1 && ONLYPID == 1 && isBrewDetected == 0) {   // SW BD, kBrew was active for set time
                // when Software brew is finished, direct to PID BD
                machineState = kBrewDetectionTrailing;
            }

            if (steamON == 1) {
                machineState = kSteam;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kShotTimerAfterBrew:
            brewDetection();

            if (millis() - lastbrewTimeMillis > BREWSWITCHDELAY) {
                debugPrintf("Shot time: %4.1f s\n", lastbrewTime / 1000);
                machineState = kBrewDetectionTrailing;
                lastbrewTime = 0;
            }

            if (steamON == 1) {
                machineState = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kBrewDetectionTrailing:
            brewDetection();

            if (isBrewDetected == 0) {
                //TODO: this needs to go back to kColdStart if kPidNormal was never reached before
                //(currently no brew detection is run during cold start though)
                machineState = kPidNormal;
            }

            if ((timeBrewed > 0 && ONLYPID == 1 && brewDetectionMode == 3) ||  // Allow brew directly after BD only when using OnlyPID AND hardware brew switch detection
                (ONLYPID == 0 && brewcounter > kBrewIdle && brewcounter <= kBrewFinished))
            {
                machineState = kBrew;
            }

            if (steamON == 1) {
                machineState = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kSteam:
            if (steamON == 0) {
                machineState = kCoolDown;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kCoolDown:
            if (brewDetectionMode == 2 || brewDetectionMode == 3) {
                /* For quickmill: steam detection only via switch, calling
                 * brewDetection() detects new steam request
                 */
                brewDetection();
            }

            if (brewDetectionMode == 1 && ONLYPID == 1) {
                // if machine cooled down to 2°C above setpoint, enabled PID again
                if (tempRateAverage > 0 && temperature < brewSetpoint + 2) {
                    machineState = kPidNormal;
                }
            }

            if ((brewDetectionMode == 3 || brewDetectionMode == 2) && temperature < brewSetpoint + 2) {
                machineState = kPidNormal;
            }

            if (steamON == 1) {
                machineState = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kBackflush:
            if (backflushON == 0) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kEmergencyStop:
            if (!emergencyStop) {
                machineState = kPidNormal;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kWaterEmpty:
            if (waterFull) {
                machineState = kPidNormal;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kPidOffline:
            if (pidON == 1) {
                if (coldstart) {
                    machineState = kColdStart;
                } else if (!coldstart && (temperature > (brewSetpoint - 10))) {  // temperature higher BrewSetpoint-10, normal PID
                    machineState = kPidNormal;
                } else if (temperature <= (brewSetpoint - 10)) {
                    machineState = kColdStart;  // temperature 10C below set point, enter cold start
                    coldstart = true;
                }
            }

            if (!waterFull) {
                machineState = kWaterEmpty;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;
        
        case kStandby:
            brewDetection();
            
            if (pidON || steamON || isBrewDetected) {
                pidON = 1;
                resetStandbyTimer();

                if (steamON) {
                    machineState = kSteam;
                } else if (isBrewDetected) {
                    machineState = kBrew;
                } else {
                    machineState = kPidNormal;
                }
            }
             
            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kSensorError:
            machineState = kSensorError;
            break;

        case kEepromError:
            machineState = kEepromError;
            break;
    }

    if (machineState != lastmachinestate) {
        printMachineState();
        lastmachinestate = machineState;
    }
}

void printMachineState() {
    debugPrintf("new machineState: %s -> %s\n",
                machinestateEnumToString(lastmachinestate), machinestateEnumToString(machineState));
}

char const* machinestateEnumToString(MachineState machineState) {
    switch (machineState) {
        case kInit:
            return "Init";
        case kColdStart:
            return "Cold Start";
        case kAtSetpoint:
            return "Above Set Point";
        case kPidNormal:
            return "PID Normal";
        case kBrew:
            return "Brew";
        case kShotTimerAfterBrew:
            return "Shot Timer After Brew";
        case kBrewDetectionTrailing:
            return "Brew Detection Trailing";
        case kSteam:
            return "Steam";
        case kCoolDown:
            return "Cool Down";
        case kBackflush:
            return "Backflush";
        case kWaterEmpty:
            return "Water Empty";
        case kEmergencyStop:
            return "Emergency Stop";
        case kPidOffline:
            return "PID Offline";
        case kStandby:
            return "Standby Mode";
        case kSensorError:
            return "Sensor Error";
        case kEepromError:
            return "EEPROM Error";
    }

    return "Unknown";
}


void debugVerboseOutput() {
    static PeriodicTrigger trigger(10000);

    if (trigger.check()) {
        debugPrintf(
            "Tsoll=%5.1f  Tist=%5.1f Machinestate=%2i KP=%4.2f "
            "KI=%4.2f KD=%4.2f\n",
            setpoint, temperature, machineState, bPID.GetKp(), bPID.GetKi(), bPID.GetKd());
    }
}


/**
 * @brief Set up internal WiFi hardware
 */
void wiFiSetup() {
    wm.setCleanConnect(true);
    wm.setConfigPortalTimeout(60);      // sec timeout for captive portal
    wm.setConnectTimeout(10);           // using 10s to connect to WLAN, 5s is sometimes too short!
    wm.setBreakAfterConfig(true);
    wm.setConnectRetries(3);

    sysParaWifiCredentialsSaved.getStorage();

    if (wifiCredentialsSaved == 0) {
        const char hostname[] = (STR(HOSTNAME));
        debugPrintf("Connecting to WiFi: %s \n", String(hostname));
        #if OLED_DISPLAY != 0
            displayLogo("Connecting to: ", HOSTNAME);
        #endif
    }

    wm.setHostname(hostname);

    if (wm.autoConnect(hostname, pass)) {
        wifiCredentialsSaved = 1;
        sysParaWifiCredentialsSaved.setStorage();
        storageCommit();
        debugPrintf("WiFi connected - IP = %i.%i.%i.%i\n", WiFi.localIP()[0],
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
        debugPrintf("MAC-ADDRESS: %s\n", completemac.c_str());

    } else {
        debugPrintln("WiFi connection timed out...");

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

    startRemoteSerialServer();
}


/**
 * @brief Set up embedded Website
 */
void websiteSetup() {
    setEepromWriteFcn(writeSysParamsToStorage);

    readSysParamsFromStorage();

    /*if (readSysParamsFromStorage() != 0) {
        #if OLED_DISPLAY != 0
            displayLogo("3:", "use eeprom values..");
        #endif
    } else {
        #if OLED_DISPLAY != 0
            displayLogo("3:", "config defaults..");
        #endif
    }*/

    serverSetup();
}

const char sysVersion[] = (STR(FW_VERSION) "." STR(FW_SUBVERSION) "." STR(FW_HOTFIX) " " FW_BRANCH " " AUTO_VERSION);

void setup() {
    editableVars["PID_ON"] = {
        .displayName = "Enable PID Controller",
        .hasHelpText = false,
        .helpText = "",
        .type = kUInt8,
        .section = sPIDSection,
        .position = 1,
        .show = [] { return true; },
        .minValue = 0,
        .maxValue = 1,
        .ptr = (void*)&pidON
    };

    editableVars["START_USE_PONM"] = {
        .displayName = F("Enable PonM"),
        .hasHelpText = true,
        .helpText =
            F("Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/"
              "introducing-proportional-on-measurement/' "
              "target='_blank'>details</a>) while heating up the machine. "
              "Otherwise, just use the same PID values that are used later"),
        .type = kUInt8,
        .section = sPIDSection,
        .position = 2,
        .show = [] { return true; },
        .minValue = 0,
        .maxValue = 1,
        .ptr = (void*)&usePonM
    };

    editableVars["START_KP"] = {
        .displayName = F("Start Kp"),
        .hasHelpText = true,
        .helpText = F(
            "Proportional gain for cold start controller. This value is not "
            "used with the the error as usual but the absolute value of the "
            "temperature and counteracts the integral part as the temperature "
            "rises. Ideally, both parameters are set so that they balance each "
            "other out when the target temperature is reached."),
        .type = kDouble,
        .section = sPIDSection,
        .position = 3,
        .show = [] { return true && usePonM; },
        .minValue = PID_KP_START_MIN,
        .maxValue = PID_KP_START_MAX,
        .ptr = (void*)&startKp
    };

    editableVars["START_TN"] = {
        .displayName = F("Start Tn"),
        .hasHelpText = true,
        .helpText = F("Integral gain for cold start controller (PonM mode, <a "
                      "href='http://brettbeauregard.com/blog/2017/06/"
                      "introducing-proportional-on-measurement/' target='_blank'>details</a>)"),
        .type = kDouble,
        .section = sPIDSection,
        .position = 4,
        .show = [] { return true && usePonM; },
        .minValue = PID_TN_START_MIN,
        .maxValue = PID_TN_START_MAX,
        .ptr = (void*)&startTn
    };

    editableVars["PID_KP"] = {
        .displayName = F("PID Kp"),
        .hasHelpText = true,
        .helpText =
            F("Proportional gain (in Watts/C°) for the main PID controller (in "
              "P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' "
              "target='_blank'>Details<a>). The higher this value is, the "
              "higher is the output of the heater for a given temperature "
              "difference. E.g. 5°C difference will result in P*5 Watts of heater output."),
        .type = kDouble,
        .section = sPIDSection,
        .position = 5,
        .show = [] { return true; },
        .minValue = PID_KP_REGULAR_MIN,
        .maxValue = PID_KP_REGULAR_MAX,
        .ptr = (void*)&aggKp
    };

    editableVars["PID_TN"] = {
        .displayName = F("PID Tn (=Kp/Ki)"),
        .hasHelpText = true,
        .helpText =
            F("Integral time constant (in seconds) for the main PID controller "
              "(in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' "
              "target='_blank'>Details<a>). The larger this value is, the slower the "
              "integral part of the PID will increase (or decrease) if the "
              "process value remains above (or below) the setpoint in spite of "
              "proportional action. The smaller this value, the faster the integral term changes."),
        .type = kDouble,
        .section = sPIDSection,
        .position = 6,
        .show = [] { return true; },
        .minValue = PID_TN_REGULAR_MIN,
        .maxValue = PID_TN_REGULAR_MAX,
        .ptr = (void*)&aggTn
    };

    editableVars["PID_TV"] = {
        .displayName = F("PID Tv (=Kd/Kp)"),
        .hasHelpText = true,
        .helpText = F(
            "Differential time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a "
            "href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). "
            "This value determines how far the PID equation projects the current trend into the future. "
            "The higher the value, the greater the dampening. Select it carefully, it can cause oscillations "
            "if it is set too high or too low."),
        .type = kDouble,
        .section = sPIDSection,
        .position = 7,
        .show = [] { return true; },
        .minValue = PID_TV_REGULAR_MIN,
        .maxValue = PID_TV_REGULAR_MAX,
        .ptr = (void *)&aggTv
    };

    editableVars["PID_I_MAX"] = {
        .displayName = F("PID Integrator Max"),
        .hasHelpText = true,
        .helpText = F(
            "Internal integrator limit to prevent windup (in Watts). This will allow the integrator to only grow to "
            "the specified value. This should be approximally equal to the output needed to hold the temperature after the "
            "setpoint has been reached and is depending on machine type and whether the boiler is insulated or not."),
        .type = kDouble,
        .section = sPIDSection,
        .position = 8,
        .show = [] { return true; },
        .minValue = PID_I_MAX_REGULAR_MIN,
        .maxValue = PID_I_MAX_REGULAR_MAX,
        .ptr = (void *)&aggIMax
    };

    editableVars["STEAM_KP"] = {
        .displayName = F("Steam Kp"),
        .hasHelpText = true,
        .helpText = F("Proportional gain for the steaming mode (I or D are not used)"),
        .type = kDouble,
        .section = sPIDSection,
        .position = 9,
        .show = [] { return true; },
        .minValue = PID_KP_STEAM_MIN,
        .maxValue = PID_KP_STEAM_MAX,
        .ptr = (void *)&steamKp
    };

    editableVars["TEMP"] = {
        .displayName = F("Temperature"),
        .hasHelpText = false,
        .helpText = "",
        .type = kDouble,
        .section = sPIDSection,
        .position = 10,
        .show = [] { return false; },
        .minValue = 0,
        .maxValue = 200,
        .ptr = (void*)&temperature
    };

    editableVars["BREW_SETPOINT"] = {
        .displayName = F("Set point (°C)"),
        .hasHelpText = true,
        .helpText =
            F("The temperature that the PID will attempt to reach and hold"),
        .type = kDouble,
        .section = sTempSection,
        .position = 11,
        .show = [] { return true; },
        .minValue = BREW_SETPOINT_MIN,
        .maxValue = BREW_SETPOINT_MAX,
        .ptr = (void*)&brewSetpoint
    };

    editableVars["BREW_TEMP_OFFSET"] = {
        .displayName = F("Offset (°C)"),
        .hasHelpText = true,
        .helpText = F("Optional offset that is added to the user-visible "
                      "setpoint. Can be used to compensate sensor offsets and "
                      "the average temperature loss between boiler and group "
                      "so that the setpoint represents the approximate brew temperature."),
        .type = kDouble,
        .section = sTempSection,
        .position = 12,
        .show = [] { return true; },
        .minValue = BREW_TEMP_OFFSET_MIN,
        .maxValue = BREW_TEMP_OFFSET_MAX,
        .ptr = (void*)&brewTempOffset
    };

    editableVars["STEAM_SETPOINT"] = {
        .displayName = F("Steam Set point (°C)"),
        .hasHelpText = true,
        .helpText = F("The temperature that the PID will use for steam mode"),
        .type = kDouble,
        .section = sTempSection,
        .position = 13,
        .show = [] { return true; },
        .minValue = STEAM_SETPOINT_MIN,
        .maxValue = STEAM_SETPOINT_MAX,
        .ptr = (void*)&steamSetpoint
    };

    editableVars["BREW_TIME"] = {
        .displayName = F("Brew Time (s)"),
        .hasHelpText = true,
        .helpText = F("Stop brew after this time"),
        .type = kDouble,
        .section = sTempSection,
        .position = 14,
        .show = [] { return true && ONLYPID == 0; },
        .minValue = BREW_TIME_MIN,
        .maxValue = BREW_TIME_MAX,
        .ptr = (void *)&brewtime
    };

    editableVars["BREW_PREINFUSIONPAUSE"] = {
        .displayName = F("Preinfusion Pause Time (s)"),
        .hasHelpText = false,
        .helpText = "",
        .type = kDouble,
        .section = sTempSection,
        .position = 15,
        .show = [] { return true && ONLYPID == 0; },
        .minValue = PRE_INFUSION_PAUSE_MIN,
        .maxValue = PRE_INFUSION_PAUSE_MAX,
        .ptr = (void *)&preinfusionpause
    };

    editableVars["BREW_PREINFUSION"] = {
        .displayName = F("Preinfusion Time (s)"),
        .hasHelpText = false,
        .helpText = "",
        .type = kDouble,
        .section = sTempSection,
        .position = 16,
        .show = [] { return true && ONLYPID == 0; },
        .minValue = PRE_INFUSION_TIME_MIN,
        .maxValue = PRE_INFUSION_TIME_MAX,
        .ptr = (void *)&preinfusion
    };

    editableVars["SCALE_WEIGHTSETPOINT"] = {
        .displayName = F("Brew weight setpoint (g)"),
        .hasHelpText = true,
        .helpText = F("Brew until this weight has been measured."),
        .type = kDouble,
        .section = sTempSection,
        .position = 17,
        .show = [] { return true && (ONLYPIDSCALE == 1 || BREWMODE == 2); },
        .minValue = WEIGHTSETPOINT_MIN,
        .maxValue = WEIGHTSETPOINT_MAX,
        .ptr = (void *)&weightSetpoint
    };

    editableVars["PID_BD_DELAY"] = {
        .displayName = F("Brew PID Delay (s)"),
        .hasHelpText = true,
        .helpText = F("Delay time in seconds during which the PID will be "
                      "disabled once a brew is detected. This prevents too "
                      "high brew temperatures with boiler machines like Rancilio "
                      "Silvia. Set to 0 for thermoblock machines."),
        .type = kDouble,
        .section = sBDSection,
        .position = 18,
        .show =
            [] { return true; },
        .minValue = BREW_PID_DELAY_MIN,
        .maxValue = BREW_PID_DELAY_MAX,
        .ptr = (void *)&brewPIDDelay
    };

    editableVars["PID_BD_ON"] = {
        .displayName = F("Enable Brew PID"),
        .hasHelpText = true,
        .helpText = F("Use separate PID parameters while brew is running"),
        .type = kUInt8,
        .section = sBDSection,
        .position = 19,
        .show = [] { return true && BREWDETECTION > 0; },
        .minValue = 0,
        .maxValue = 1,
        .ptr = (void *)&useBDPID
    };

    editableVars["PID_BD_KP"] = {
        .displayName = F("BD Kp"),
        .hasHelpText = true,
        .helpText = F(
            "Proportional gain (in Watts/°C) for the PID when brewing has been "
            "detected. Use this controller to either increase heating during the "
            "brew to counter temperature drop from fresh cold water in the boiler. "
            "Some machines, e.g. Rancilio Silvia, actually need to heat less or not "
            "at all during the brew because of high temperature stability "
            "(<a href='https://www.kaffee-netz.de/threads/"
            "installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/"
            "#post-1453641' target='_blank'>Details<a>)"),
        .type = kDouble,
        .section = sBDSection,
        .position = 20,
        .show = [] { return true && BREWDETECTION > 0 && useBDPID; },
        .minValue = PID_KP_BD_MIN,
        .maxValue = PID_KP_BD_MAX,
        .ptr = (void *)&aggbKp
    };

    editableVars["PID_BD_TN"] = {
        .displayName = F("BD Tn (=Kp/Ki)"),
        .hasHelpText = true,
        .helpText = F("Integral time constant (in seconds) for the PID when "
                      "brewing has been detected."),
        .type = kDouble,
        .section = sBDSection,
        .position = 21,
        .show = [] { return true && BREWDETECTION > 0 && useBDPID; },
        .minValue = PID_TN_BD_MIN,
        .maxValue = PID_TN_BD_MAX,
        .ptr = (void *)&aggbTn
    };

    editableVars["PID_BD_TV"] = {
        .displayName = F("BD Tv (=Kd/Kp)"),
        .hasHelpText = true,
        .helpText = F("Differential time constant (in seconds) for the PID "
                      "when brewing has been detected."),
        .type = kDouble,
        .section = sBDSection,
        .position = 22,
        .show = [] { return true && BREWDETECTION > 0 && useBDPID; },
        .minValue = PID_TV_BD_MIN,
        .maxValue = PID_TV_BD_MAX,
        .ptr = (void *)&aggbTv
    };

    editableVars["PID_BD_TIME"] = {
        .displayName = F("PID BD Time (s)"),
        .hasHelpText = true,
        .helpText = F("Fixed time in seconds for which the BD PID will stay "
                      "enabled (also after Brew switch is inactive again)."),
        .type = kDouble,
        .section = sBDSection,
        .position = 23,
        .show =
            [] {
              return true && BREWDETECTION > 0 &&
                     (useBDPID || BREWDETECTION == 1);
            },
        .minValue = BREW_SW_TIME_MIN,
        .maxValue = BREW_SW_TIME_MAX,
        .ptr = (void *)&brewtimesoftware
    };

    editableVars["PID_BD_SENSITIVITY"] = {
        .displayName = F("PID BD Sensitivity"),
        .hasHelpText = true,
        .helpText = F("Software brew detection sensitivity that looks at "
                      "average temperature, <a href='https://manual.rancilio-pid.de/de/customization/"
                      "brueherkennung.html' target='_blank'>Details</a>. "
                      "Needs to be &gt;0 also for Hardware switch detection."),
        .type = kDouble,
        .section = sBDSection,
        .position = 24,
        .show = [] { return true && BREWDETECTION == 1; },
        .minValue = BD_THRESHOLD_MIN,
        .maxValue = BD_THRESHOLD_MAX,
        .ptr = (void *)&brewSensitivity
    };

    editableVars["STEAM_MODE"] = {
        .displayName = F("Steam Mode"),
        .hasHelpText = false,
        .helpText = "",
        .type = kUInt8,
        .section = sOtherSection,
        .position = 25,
        .show = [] { return false; },
        .minValue = 0,
        .maxValue = 1,
        .ptr = (void *)&steamON
    };

    editableVars["BACKFLUSH_ON"] = {
        .displayName = F("Backflush"),
        .hasHelpText = false,
        .helpText = "",
        .type = kUInt8,
        .section = sOtherSection,
        .position = 26,
        .show = [] { return false; },
        .minValue = 0,
        .maxValue = 1,
        .ptr = (void *)&backflushON
    };

    editableVars["STANDBY_MODE_ON"] = {
        .displayName = F("Enable Standby Timer"),
        .hasHelpText = true,
        .helpText = F("Turn heater off after standby time has elapsed."),
        .type = kUInt8,
        .section = sPowerSection,
        .position = 27,
        .show = [] { return true; },
        .minValue = 0,
        .maxValue = 1,
        .ptr = (void *)&standbyModeOn
    };

    editableVars["STANDBY_MODE_TIMER"] = {
        .displayName = F("Standby Time"),
        .hasHelpText = true,
        .helpText = F(
            "Time in minutes until the heater is turned off. Timer is reset by brew detection."),
        .type = kDouble,
        .section = sPowerSection,
        .position = 28,
        .show = [] { return true; },
        .minValue = STANDBY_MODE_TIME_MIN,
        .maxValue = STANDBY_MODE_TIME_MAX,
        .ptr = (void *)&standbyModeTime
    };

    editableVars["VERSION"] = {
        .displayName = F("Version"),
        .hasHelpText = false,
        .helpText = "",
        .type = kCString,
        .section = sOtherSection,
        .position = 29,
        .show = [] { return false; },
        .minValue = 0,
        .maxValue = 1,
        .ptr = (void *)sysVersion
    };
    // when adding parameters, set EDITABLE_VARS_LEN to max of .position

    // Editable values reported to MQTT
    mqttVars["pidON"] = []{ return &editableVars.at("PID_ON"); };
    mqttVars["brewSetpoint"] = []{ return &editableVars.at("BREW_SETPOINT"); };
    mqttVars["brewTempOffset"] = []{ return &editableVars.at("BREW_TEMP_OFFSET"); };
    mqttVars["steamON"] = []{ return &editableVars.at("STEAM_MODE"); };
    mqttVars["steamSetpoint"] = []{ return &editableVars.at("STEAM_SETPOINT"); };
    mqttVars["brewPidDelay"] = []{ return &editableVars.at("PID_BD_DELAY"); };
    mqttVars["backflushON"] = []{ return &editableVars.at("BACKFLUSH_ON"); };
    mqttVars["startUsePonM"] = []{ return &editableVars.at("START_USE_PONM"); };
    mqttVars["startKp"] = []{ return &editableVars.at("START_KP"); };
    mqttVars["startTn"] = []{ return &editableVars.at("START_TN"); };
    mqttVars["aggKp"] = []{ return &editableVars.at("PID_KP"); };
    mqttVars["aggTn"] = []{ return &editableVars.at("PID_TN"); };
    mqttVars["aggTv"] = []{ return &editableVars.at("PID_TV"); };
    mqttVars["aggIMax"] = []{ return &editableVars.at("PID_I_MAX"); };
    mqttVars["steamKp"] = []{ return &editableVars.at("STEAM_KP"); };
    mqttVars["standbyModeOn"] = []{ return &editableVars.at("STANDBY_MODE_ON"); };

    if (ONLYPID == 0) {
        mqttVars["brewtime"] = []{ return &editableVars.at("BREW_TIME"); };
        mqttVars["preinfusionpause"] = []{ return &editableVars.at("BREW_PREINFUSIONPAUSE"); };
        mqttVars["preinfusion"] = []{ return &editableVars.at("BREW_PREINFUSION"); };
    }

    if (ONLYPIDSCALE == 1 || BREWMODE == 2) {
        mqttVars["weightSetpoint"] = []{ return &editableVars.at("SCALE_WEIGHTSETPOINT"); };
    }

    if (BREWDETECTION > 0) {
        mqttVars["pidUseBD"] = []{ return &editableVars.at("PID_BD_ON"); };
        mqttVars["aggbKp"] = []{ return &editableVars.at("PID_BD_KP"); };
        mqttVars["aggbTn"] = []{ return &editableVars.at("PID_BD_TN"); };
        mqttVars["aggbTv"] = []{ return &editableVars.at("PID_BD_TV"); };

        if (BREWDETECTION == 1) {
            mqttVars["brewTimer"] = []{ return &editableVars.at("PID_BD_TIME"); };
            mqttVars["brewLimit"] = []{ return &editableVars.at("PID_BD_SENSITIVITY"); };
        }
    }
    
    // Values reported to MQTT
    mqttSensors["temperature"] = []{ return temperature; };
    mqttSensors["heaterPower"] = []{ return pidOutput / 10; };
    mqttSensors["standbyModeTimeRemaining"] = []{ return standbyModeRemainingTimeMillis / 1000; };
    mqttSensors["currentKp"] = []{ return bPID.GetKp(); };
    mqttSensors["currentKi"] = []{ return bPID.GetKi(); };
    mqttSensors["currentKd"] = []{ return bPID.GetKd(); };
    mqttSensors["machineState"] = []{ return machineState; };

    Serial.begin(115200);

    initTimer1();

    storageSetup();

    // Define trigger type
    if (triggerType) {
        relayON = HIGH;
        relayOFF = LOW;
    } 
    else {
        relayON = LOW;
        relayOFF = HIGH;
    }

    if (optocouplerType == HIGH) {
        optocouplerOn = HIGH;
        optocouplerOff = LOW;
    } 
    else {
        optocouplerOn = LOW;
        optocouplerOff = HIGH;
    }

    // Initialize Pins
    pinMode(PIN_VALVE, OUTPUT);
    pinMode(PIN_PUMP, OUTPUT);
    pinMode(PIN_HEATER, OUTPUT);
    digitalWrite(PIN_VALVE, relayOFF);
    digitalWrite(PIN_PUMP, relayOFF);
    digitalWrite(PIN_HEATER, LOW);

    // IF POWERSWITCH is connected
    if (POWERSWITCHTYPE > 0) {
        pinMode(PIN_POWERSWITCH, INPUT_PULLDOWN);
    }

    // IF STEAMSWITCH is connected
    if (STEAMSWITCHTYPE > 0) {
        pinMode(PIN_STEAMSWITCH, INPUT_PULLDOWN);
    }

    // IF optocoupler selected
    if (BREWDETECTION == 3) {
        if (optocouplerType == HIGH) {
            pinMode(PIN_BREWSWITCH, INPUT_PULLDOWN);
        } 
        else {
            pinMode(PIN_BREWSWITCH, INPUT_PULLUP);
        }
    }
    else {
        pinMode(PIN_BREWSWITCH, INPUT_PULLDOWN);
    }

    if (TEMP_LED) {
        pinMode(PIN_STATUSLED, OUTPUT);
    }

    #if WATER_SENS == 1
        pinMode(PIN_WATERSENSOR, INPUT_PULLUP);
    #elif WATER_SENS == 2
        pinMode(PIN_WATERSENSOR, INPUT_PULLDOWN);
    #endif

    #if OLED_DISPLAY != 0
        u8g2.setI2CAddress(oled_i2c * 2);
        u8g2.begin();
        u8g2_prepare();
        displayLogo(String("Version "), String(sysVersion));
        delay(2000); // caused crash with wifi manager on esp8266, should be ok on esp32
    #endif

    // Init Scale by BREWMODE 2 or SHOTTIMER 2
    #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
        initScale();
    #endif


    // Fallback offline
    if (connectmode == 1) {  // WiFi Mode
        wiFiSetup();
        websiteSetup();

        // OTA Updates
        if (ota && WiFi.status() == WL_CONNECTED) {
            ArduinoOTA.setHostname(hostname);  //  Device name for OTA
            ArduinoOTA.setPassword(OTApass);  //  Password for OTA
            ArduinoOTA.begin();
        }

        if (MQTT == 1) {
            snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "status");
            snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
            mqtt.setServer(mqtt_server_ip, mqtt_server_port);
            mqtt.setCallback(mqtt_callback);
            checkMQTT();
            #if MQTT_HASSIO_SUPPORT == 1  // Send Home Assistant MQTT discovery messages
            sendHASSIODiscoveryMsg();
            #endif
        }
    }
    else if (connectmode == 0) {
        wm.disconnect();              // no wm
        readSysParamsFromStorage();   // get all parameters from storage
        offlineMode = 1;              // offline mode
        pidON = 1;                    // pid on
    }

    // Initialize PID controller
    bPID.SetSampleTime(windowSize);
    bPID.SetOutputLimits(0, windowSize);
    bPID.SetIntegratorLimits(0, AGGIMAX);
    bPID.SetSmoothingFactor(EMA_FACTOR);
    bPID.SetMode(AUTOMATIC);

    // Dallas temp sensor
    #if TEMPSENSOR == 1
        sensors.begin();
        sensors.getAddress(sensorDeviceAddress, 0);
        sensors.setResolution(sensorDeviceAddress, 10);
        sensors.requestTemperatures();
        temperature = sensors.getTempCByIndex(0);
    #endif

    // TSic 306 temp sensor
    #if TEMPSENSOR == 2
        temperature = Sensor2.getTemp(15);
    #endif

    temperature -= brewTempOffset;

    // Initialisation MUST be at the very end of the init(), otherwise the
    // time comparision in loop() will have a big offset
    unsigned long currentTime = millis();
    previousMillistemp = currentTime;
    windowStartTime = currentTime;
    previousMillisDisplay = currentTime;
    previousMillisMQTT = currentTime;
    previousMillisOptocouplerReading = currentTime;
    lastMQTTConnectionAttempt = currentTime;

    #if (BREWMODE == 2)
        previousMillisScale = currentTime;
    #endif
    #if (PRESSURESENSOR == 1)
        previousMillisPressure = currentTime;
    #endif

    setupDone = true;

    enableTimer1();

    double fsUsage = ((double)LittleFS.usedBytes() / LittleFS.totalBytes()) * 100;
    debugPrintf("LittleFS: %d%% (used %ld bytes from %ld bytes)\n",
        (int)ceil(fsUsage), LittleFS.usedBytes(), LittleFS.totalBytes());
}


void loop() {
    looppid();

    if (TEMP_LED) {
        loopLED();
    }

    if (WATER_SENS > 0) {
        loopWater();
    }

    checkForRemoteSerialClients();
}


void looppid() {
    // Only do Wifi stuff, if Wifi is connected
    if (WiFi.status() == WL_CONNECTED && offlineMode == 0) {
        if (MQTT == 1) {
            checkMQTT();
            writeSysParamsToMQTT(true); // Continue on error
            if (mqtt.connected() == 1) {
                mqtt.loop();
                #if MQTT_HASSIO_SUPPORT == 1
                if (millis() - lastHomeAssistantDiscoveryExecutionTime >= HomeAssistantDiscoveryExecutionInterval) {
                    sendHASSIODiscoveryMsg();
                    lastHomeAssistantDiscoveryExecutionTime = millis();
                }
                #endif
                mqtt_was_connected = true;
            }
            // Supress debug messages until we have a connection etablished
            else if(mqtt_was_connected) {
                debugPrintln("MQTT disconnected\n");
                mqtt_was_connected = false;
            }
        }

        ArduinoOTA.handle();  // For OTA

        // Disable interrupt if OTA is starting, otherwise it will not work
        ArduinoOTA.onStart([]() {
            disableTimer1();
            digitalWrite(PIN_HEATER, LOW);  // Stop heating
        });

        ArduinoOTA.onError([](ota_error_t error) { enableTimer1(); });

        // Enable interrupts if OTA is finished
        ArduinoOTA.onEnd([]() { enableTimer1(); });

        wifiReconnects = 0;  // reset wifi reconnects if connected
    } else {
        checkWifi();
    }

    refreshTemp();        // update temperature values
    testEmergencyStop();  // test if temp is too high
    bPID.Compute();       // the variable pidOutput now has new values from PID (will be written to heater pin in ISR.cpp)

    if ((millis() - lastTempEvent) > tempEventInterval) {
        //send temperatures to website endpoint
        sendTempEvent(temperature, brewSetpoint, pidOutput/10);       //pidOutput is promill, so /10 to get percent value
        lastTempEvent = millis();

        #if VERBOSE
        if (pidON) {
            debugPrintf("Current PID mode: %s\n", bPID.GetPonE() ? "PonE" : "PonM");

            //P-Part
            debugPrintf("Current PID input error: %f\n", bPID.GetInputError());
            debugPrintf("Current PID P part: %f\n", bPID.GetLastPPart());
            debugPrintf("Current PID kP: %f\n", bPID.GetKp());
            //I-Part
            debugPrintf("Current PID I sum: %f\n", bPID.GetLastIPart());
            debugPrintf("Current PID kI: %f\n", bPID.GetKi());
            //D-Part
            debugPrintf("Current PID diff'd input: %f\n", bPID.GetDeltaInput());
            debugPrintf("Current PID D part: %f\n", bPID.GetLastDPart());
            debugPrintf("Current PID kD: %f\n", bPID.GetKd());

            //Combined PID output
            debugPrintf("Current PID Output: %f\n\n", pidOutput);
            debugPrintf("Current Machinestate: %s\n\n", machinestateEnumToString(machineState));
            debugPrintf("timeBrewed %f\n", timeBrewed);
            debugPrintf("brewtimesoftware %f\n", brewtimesoftware);
            debugPrintf("isBrewDetected %i\n", isBrewDetected);
            debugPrintf("brewDetectionMode %i\n", brewDetectionMode);
        }
        #endif
    }

    #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
        checkWeight();  // Check Weight Scale in the loop
    #endif

    #if (PRESSURESENSOR == 1)
        checkPressure();
    #endif

    checkSteamON();

    if(machineState != kWaterEmpty) {
        brew();
    }

    setEmergencyStopTemp();
    checkSteamSwitch();
    checkPowerSwitch();

    // set setpoint depending on steam or brew mode
    if (steamON == 1) {
        setpoint = steamSetpoint;
    } else if (steamON == 0) {
        setpoint = brewSetpoint;
    }

    setEmergencyStopTemp();

    if (standbyModeOn && machineState != kStandby) {
        updateStandbyTimer();
    }
    
    handleMachineState(); 

    #if (ONLYPIDSCALE == 1)  // only by shottimer 2, scale
        shottimerscale();
    #endif

    // Check if PID should run or not. If not, set to manual and force output to zero
#if OLED_DISPLAY != 0
    unsigned long currentMillisDisplay = millis();
    if (currentMillisDisplay - previousMillisDisplay >= 100) {
        displayShottimer();
    }
    if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
        previousMillisDisplay = currentMillisDisplay;
    #if DISPLAYTEMPLATE < 20  // not using vertical template
        Displaymachinestate();
    #endif
        printScreen();  // refresh display
    }
#endif

    if (machineState == kPidOffline || machineState == kWaterEmpty || machineState == kSensorError || machineState == kEmergencyStop || machineState == kEepromError || machineState == kStandby || brewPIDdisabled) {
        if (pidMode == 1) {
            // Force PID shutdown
            pidMode = 0;
            bPID.SetMode(pidMode);
            pidOutput = 0;
            digitalWrite(PIN_HEATER, LOW);  // Stop heating
        }
    } else {  // no sensorerror, no pid off or no Emergency Stop
        if (pidMode == 0) {
            pidMode = 1;
            bPID.SetMode(pidMode);
        }
    }

    // Set PID if first start of machine detected, and no steamON
    if ((machineState == kInit || machineState == kColdStart || machineState == kAtSetpoint)) {
        if (usePonM) {
            if (startTn != 0) {
                startKi = startKp / startTn;
            } else {
                startKi = 0;
            }

            if (lastmachinestatepid != machineState) {
                debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", startKp, startKi, 0.0);
                lastmachinestatepid = machineState;
            }

            bPID.SetTunings(startKp, startKi, 0, P_ON_M);
        } else {
            setNormalPIDTunings();
        }
    }

    if (machineState == kPidNormal) {
        setNormalPIDTunings();
        coldstart = false;
    }

    // BD PID
    if (machineState >= kBrew && machineState <= kBrewDetectionTrailing) {
        if (brewPIDDelay > 0 && timeBrewed > 0 && timeBrewed < brewPIDDelay*1000) {
            //disable PID for brewPIDDelay seconds, enable PID again with new tunings after that
            if (!brewPIDdisabled) {
                brewPIDdisabled = true;
                bPID.SetMode(MANUAL);
                debugPrintf("disabled PID, waiting for %d seconds before enabling PID again\n", brewPIDDelay);
            }
        } else {
            if (brewPIDdisabled) {
                //enable PID again
                bPID.SetMode(AUTOMATIC);
                brewPIDdisabled = false;
                debugPrintln("Enabled PID again after delay");
            }

            if (useBDPID) {
                setBDPIDTunings();
            } else {
                setNormalPIDTunings();
            }
        }
    }

    // Steam on
    if (machineState == kSteam) {
        if (lastmachinestatepid != machineState) {
            debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", 150.0, 0.0, 0.0);
            lastmachinestatepid = machineState;
        }

        bPID.SetTunings(steamKp, 0, 0, 1);
    }

    // chill-mode after steam
    if (machineState == kCoolDown) {
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

        if (lastmachinestatepid != machineState) {
            debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggbKp, aggbKi, aggbKd);
            lastmachinestatepid = machineState;
        }

        bPID.SetTunings(aggbKp, aggbKi, aggbKd, 1);
    }
    // sensor error OR Emergency Stop
}

void loopLED() {
    if ((machineState == kPidNormal && (fabs(temperature  - setpoint) < 0.3)) || (temperature > 115 && fabs(temperature - setpoint) < 5)) {
        digitalWrite(PIN_STATUSLED, HIGH);
    }
    else {
        digitalWrite(PIN_STATUSLED, LOW);
    }
}

void loopWater() {
    if ((millis() - lastWaterCheck) > WaterCheckInterval) {
        lastWaterCheck = millis();

        bool isWaterDetected = digitalRead(PIN_WATERSENSOR) == (WATER_SENS == 1 ? LOW : HIGH);
    
        if (isWaterDetected) {
            // Water is detected, increment counter if it was previously empty
            if (!waterFull) {
                waterCheckConsecutiveReads++;

                if (waterCheckConsecutiveReads >= waterCountsNeeded) {
                    waterFull = true;
                    debugPrintln("Water full");
                    waterCheckConsecutiveReads = 0;
                }
            } 
            else {
                waterCheckConsecutiveReads = 0;
            }
        } 
        else {
            // No water detected, increment counter if it was previously full
            if (waterFull) {
                waterCheckConsecutiveReads++;
                
                if (waterCheckConsecutiveReads >= waterCountsNeeded) {
                    waterFull = false;
                    debugPrintln("Water empty");
                    waterCheckConsecutiveReads = 0;
                }
            } 
            else {
                waterCheckConsecutiveReads = 0;
            }
        }
    }
}

void setBackflush(int backflush) {
    backflushON = backflush;
}

void setSteamMode(int steamMode) {
    steamON = steamMode;

    if (steamON == 1) {
        steamFirstON = 1;
    }

    if (steamON == 0) {
        steamFirstON = 0;
    }
}

void setPidStatus(int pidStatus) {
    pidON = pidStatus;
    writeSysParamsToStorage();
}

void setNormalPIDTunings() {
    // Prevent overwriting of brewdetection values
    // calc ki, kd
    if (aggTn != 0) {
        aggKi = aggKp / aggTn;
    } else {
        aggKi = 0;
    }

    aggKd = aggTv * aggKp;

    bPID.SetIntegratorLimits(0, aggIMax);

    if (lastmachinestatepid != machineState) {
        debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggKp, aggKi, aggKd);
        lastmachinestatepid = machineState;
    }

    bPID.SetTunings(aggKp, aggKi, aggKd, 1);
}

void setBDPIDTunings() {
    // calc ki, kd
    if (aggbTn != 0) {
        aggbKi = aggbKp / aggbTn;
    } else {
        aggbKi = 0;
    }

    aggbKd = aggbTv * aggbKp;

    if (lastmachinestatepid != machineState) {
        debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggbKp, aggbKi, aggbKd);
        lastmachinestatepid = machineState;
    }

    bPID.SetTunings(aggbKp, aggbKi, aggbKd, 1);
}

/**
 * @brief Reads all system parameter values from non-volatile storage
 *
 * @return 0 = success, < 0 = failure
 */
int readSysParamsFromStorage(void) {
    if (sysParaPidOn.getStorage() != 0) return -1;
    if (sysParaUsePonM.getStorage() != 0) return -1;
    if (sysParaPidKpStart.getStorage() != 0) return -1;
    if (sysParaPidTnStart.getStorage() != 0) return -1;
    if (sysParaPidKpReg.getStorage() != 0) return -1;
    if (sysParaPidTnReg.getStorage() != 0) return -1;
    if (sysParaPidTvReg.getStorage() != 0) return -1;
    if (sysParaPidIMaxReg.getStorage() != 0) return -1;
    if (sysParaBrewSetpoint.getStorage() != 0) return -1;
    if (sysParaTempOffset.getStorage() != 0) return -1;
    if (sysParaBrewPIDDelay.getStorage() != 0) return -1;
    if (sysParaUseBDPID.getStorage() != 0) return -1;
    if (sysParaPidKpBd.getStorage() != 0) return -1;
    if (sysParaPidTnBd.getStorage() != 0) return -1;
    if (sysParaPidTvBd.getStorage() != 0) return -1;
    if (sysParaBrewTime.getStorage() != 0) return -1;
    if (sysParaBrewSwTime.getStorage() != 0) return -1;
    if (sysParaBrewThresh.getStorage() != 0) return -1;
    if (sysParaPreInfTime.getStorage() != 0) return -1;
    if (sysParaPreInfPause.getStorage() != 0) return -1;
    if (sysParaPidKpSteam.getStorage() != 0) return -1;
    if (sysParaSteamSetpoint.getStorage() != 0) return -1;
    if (sysParaWeightSetpoint.getStorage() != 0) return -1;
    if (sysParaWifiCredentialsSaved.getStorage() != 0) return -1;
    if (sysParaStandbyModeOn.getStorage() != 0) return -1;
    if (sysParaStandbyModeTime.getStorage() != 0) return -1;

    return 0;
}

/**
 * @brief Writes all current system parameter values to non-volatile storage
 *
 * @return 0 = success, < 0 = failure
 */
int writeSysParamsToStorage(void) {
    if (sysParaPidOn.setStorage() != 0) return -1;
    if (sysParaUsePonM.setStorage() != 0) return -1;
    if (sysParaPidKpStart.setStorage() != 0) return -1;
    if (sysParaPidTnStart.setStorage() != 0) return -1;
    if (sysParaPidKpReg.setStorage() != 0) return -1;
    if (sysParaPidTnReg.setStorage() != 0) return -1;
    if (sysParaPidTvReg.setStorage() != 0) return -1;
    if (sysParaPidIMaxReg.setStorage() != 0) return -1;
    if (sysParaBrewSetpoint.setStorage() != 0) return -1;
    if (sysParaTempOffset.setStorage() != 0) return -1;
    if (sysParaBrewPIDDelay.setStorage() != 0) return -1;
    if (sysParaUseBDPID.setStorage() != 0) return -1;
    if (sysParaPidKpBd.setStorage() != 0) return -1;
    if (sysParaPidTnBd.setStorage() != 0) return -1;
    if (sysParaPidTvBd.setStorage() != 0) return -1;
    if (sysParaBrewTime.setStorage() != 0) return -1;
    if (sysParaBrewSwTime.setStorage() != 0) return -1;
    if (sysParaBrewThresh.setStorage() != 0) return -1;
    if (sysParaPreInfTime.setStorage() != 0) return -1;
    if (sysParaPreInfPause.setStorage() != 0) return -1;
    if (sysParaPidKpSteam.setStorage() != 0) return -1;
    if (sysParaSteamSetpoint.setStorage() != 0) return -1;
    if (sysParaWeightSetpoint.setStorage() != 0) return -1;
    if (sysParaWifiCredentialsSaved.setStorage() != 0) return -1;
    if (sysParaStandbyModeOn.setStorage() != 0) return -1;
    if (sysParaStandbyModeTime.setStorage() != 0) return -1;

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
