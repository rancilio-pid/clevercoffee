/**
 * @file rancilio-pid.cpp
 *
 * @brief Main sketch
 *
 * @version 3.1.1 Master
 */

// Firmware version
#define FW_VERSION    3
#define FW_SUBVERSION 1
#define FW_HOTFIX     1
#define FW_BRANCH     "MASTER"

// User Config
#include "userConfig.h"         // needs to be configured by the user

// Libraries
#include <ArduinoOTA.h>
#if TOF == 1
    #include <Adafruit_VL53L0X.h>   // for ToF Sensor
#endif
#if TEMPSENSOR == 1
    #include <DallasTemperature.h>  // Library for dallas temp sensor
#endif
#include <WiFiManager.h>
#include <InfluxDbClient.h>
#include <PubSubClient.h>
#include <U8g2lib.h>            // i2c display
#include "TSIC.h"               // old library for TSIC temp sensor
#include <ZACwire.h>            // new TSIC bus library
#include "PID_v1.h"             // for PID calculation

// Includes
#include "icon.h"               // user icons for display
#include "languages.h"          // for language translation
#include "Storage.h"
#include "ISR.h"
#include "debugSerial.h"
#include "rancilio-pid.h"

#if defined(ESP32)
    #include <os.h>
    hw_timer_t *timer = NULL;
#endif

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
    #error Version of userConfig file and rancilio-pid.cpp need to match!
#endif

MACHINE machine = (enum MACHINE)MACHINEID;

#define HIGH_ACCURACY

#include "PeriodicTrigger.h"
PeriodicTrigger writeDebugTrigger(5000);  // returns true every 5000 ms
PeriodicTrigger logbrew(500);

enum MachineState {
    kInit = 0,
    kColdStart = 10,
    kBelowSetPoint = 19,
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
const int VoltageSensorType = VOLTAGESENSORTYPE;
const boolean ota = OTA;
int BrewMode = BREWMODE;

// Display
uint8_t oled_i2c = OLED_I2C;

// ToF Sensor
#if TOF == 1
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
int calibration_mode = CALIBRATION_MODE;
uint8_t tof_i2c = TOF_I2C;
int water_full = WATER_FULL;
int water_empty = WATER_EMPTY;
unsigned long previousMillisTOF;         // initialisation at the end of init()
const unsigned long intervalTOF = 5000;  // ms
double distance;
double percentage;
#endif

// WiFi
WiFiManager wm;
const unsigned long wifiConnectionDelay = WIFICONNECTIONDELAY;
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

// InfluxDB Client
InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point influxSensor("machineState");
const unsigned long intervalInflux = INFLUXDB_INTERVAL;
unsigned long previousMillisInflux;  // initialisation at the end of init()

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
bool mqtt_publish(const char *reading, char *payload);
void setSteamMode(int steamMode);
void setPidStatus(int pidStatus);
void setBackflush(int backflush);
void setNormalPIDTunings();
void setBDPIDTunings();
void loopcalibrate();
void looppid();
void printMachineState();
char const* machinestateEnumToString(MachineState machineState);
void initSteamQM();
boolean checkSteamOffQM();
void writeSysParamsToMQTT(void);
char *number2string(double in);
char *number2string(float in);
char *number2string(int in);
char *number2string(unsigned int in);
float filterPressureValue(float input);

// system parameters
uint8_t pidON = 0;                 // 1 = control loop in closed loop
double brewSetPoint = SETPOINT;
double brewTempOffset = TEMPOFFSET;
double setPoint = brewSetPoint;
double steamSetPoint = STEAMSETPOINT;
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
double brewSensitivity = BREWSENSITIVITY;  // use userConfig brew detection sensitivity

// system parameter EEPROM storage wrappers (current value as pointer to variable, minimum, maximum, optional storage ID)
SysPara<double> sysParaPidKpStart(&startKp, PID_KP_START_MIN, PID_KP_START_MAX, STO_ITEM_PID_KP_START);
SysPara<double> sysParaPidTnStart(&startTn, PID_TN_START_MIN, PID_TN_START_MAX, STO_ITEM_PID_TN_START);
SysPara<double> sysParaPidKpReg(&aggKp, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, STO_ITEM_PID_KP_REGULAR);
SysPara<double> sysParaPidTnReg(&aggTn, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, STO_ITEM_PID_TN_REGULAR);
SysPara<double> sysParaPidTvReg(&aggTv, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, STO_ITEM_PID_TV_REGULAR);
SysPara<double> sysParaPidIMaxReg(&aggIMax, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, STO_ITEM_PID_I_MAX_REGULAR);
SysPara<double> sysParaPidKpBd(&aggbKp, PID_KP_BD_MIN, PID_KP_BD_MAX, STO_ITEM_PID_KP_BD);
SysPara<double> sysParaPidTnBd(&aggbTn, PID_TN_BD_MIN, PID_KP_BD_MAX, STO_ITEM_PID_TN_BD);
SysPara<double> sysParaPidTvBd(&aggbTv, PID_TV_BD_MIN, PID_TV_BD_MAX, STO_ITEM_PID_TV_BD);
SysPara<double> sysParaBrewSetPoint(&brewSetPoint, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, STO_ITEM_BREW_SETPOINT);
SysPara<double> sysParaTempOffset(&brewTempOffset, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, STO_ITEM_BREW_TEMP_OFFSET);
SysPara<double> sysParaBrewTime(&brewtime, BREW_TIME_MIN, BREW_TIME_MAX, STO_ITEM_BREW_TIME);
SysPara<double> sysParaBrewSwTime(&brewtimesoftware, BREW_SW_TIME_MIN, BREW_SW_TIME_MAX, STO_ITEM_BREW_SW_TIME);
SysPara<double> sysParaBrewThresh(&brewSensitivity, BD_THRESHOLD_MIN, BD_THRESHOLD_MAX, STO_ITEM_BD_THRESHOLD);
SysPara<double> sysParaPreInfTime(&preinfusion, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, STO_ITEM_PRE_INFUSION_TIME);
SysPara<double> sysParaPreInfPause(&preinfusionpause, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, STO_ITEM_PRE_INFUSION_PAUSE);
SysPara<double> sysParaWeightSetPoint(&weightSetpoint, WEIGHTSETPOINT_MIN, WEIGHTSETPOINT_MAX, STO_ITEM_WEIGHTSETPOINT);
SysPara<double> sysParaPidKpSteam(&steamKp, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, STO_ITEM_PID_KP_STEAM);
SysPara<double> sysParaSteamSetPoint(&steamSetPoint, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, STO_ITEM_STEAM_SETPOINT);
SysPara<uint8_t> sysParaPidOn(&pidON, 0, 1, STO_ITEM_PID_ON);
SysPara<uint8_t> sysParaUsePonM(&usePonM, 0, 1, STO_ITEM_PID_START_PONM);
SysPara<uint8_t> sysParaUseBDPID(&useBDPID, 0, 1, STO_ITEM_USE_BD_PID);
SysPara<double> sysParaSteamSetPoint(&steamSetPoint, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, STO_ITEM_STEAM_SETPOINT);

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

double setPointTemp;
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

PID bPID(&temperature, &pidOutput, &setPoint, aggKp, aggKi, aggKd, 1, DIRECT);

// Dallas temp sensor
#if TEMPSENSOR == 1
    OneWire oneWire(PINTEMPSENSOR);         // Setup a OneWire instance to communicate with OneWire
                                            // devices (not just Maxim/Dallas temperature ICs)
    DallasTemperature sensors(&oneWire);
    DeviceAddress sensorDeviceAddress;      // arrays to hold device address
#endif

// TSIC 306 temp sensor
#if (PINTEMPSENSOR == 16 && TEMPSENSOR == 2 && defined(ESP8266))
    TSIC Sensor1(PINTEMPSENSOR);            // only Signal pin, VCC pin unused by default
#else
    ZACwire Sensor2(PINTEMPSENSOR, 306);    // set pin to receive signal from the TSic 306
#endif


// MQTT
WiFiClient net;
PubSubClient mqtt(net);
const char *mqtt_server_ip = MQTT_SERVER_IP;
const int mqtt_server_port = MQTT_SERVER_PORT;
const char *mqtt_username = MQTT_USERNAME;
const char *mqtt_password = MQTT_PASSWORD;
const char *mqtt_topic_prefix = MQTT_TOPIC_PREFIX;
char topic_will[256];
char topic_set[256];
unsigned long lastMQTTConnectionAttempt = millis();
unsigned int MQTTReCnctFlag;
unsigned int MQTTReCnctCount = 0;
unsigned long previousMillisMQTT;           // initialised at the end of init()
const unsigned long intervalMQTT = 5000;

enum MQTTSettableType {
    tUInt8,
    tDouble
};

struct mqttVars_t {
    String mqttParamName;
    MQTTSettableType type;
    int minValue;
    int maxValue;
    void *mqttVarPtr;
};

std::vector<mqttVars_t> mqttVars = {
    {"brewSetPoint", tDouble, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, (void *)&brewSetPoint},
    {"brewTempOffset", tDouble, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, (void *)&brewTempOffset},
    {"brewtime", tDouble, BREW_TIME_MIN, BREW_TIME_MAX, (void *)&brewtime},
    {"preinfusion", tDouble, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, (void *)&preinfusion},
    {"preinfusionpause", tDouble, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, (void *)&preinfusionpause},
    {"pidON", tUInt8, 0, 1, (void *)&pidON},
    {"steamON", tUInt8, 0, 1, (void *)&steamON},
    {"steamSetPoint", tDouble, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, (void *)&steamSetPoint},
    {"backflushON", tUInt8, 0, 1, (void *)&backflushON},
    {"aggKp", tDouble, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, (void *)&aggKp},
    {"aggTn", tDouble, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, (void *)&aggTn},
    {"aggTv", tDouble, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, (void *)&aggTv},
    {"aggIMax", tDouble, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, (void *)&aggIMax},
    {"aggbKp", tDouble, PID_KP_BD_MIN, PID_KP_BD_MAX, (void *)&aggbKp},
    {"aggbTn", tDouble, PID_TN_BD_MIN, PID_TN_BD_MAX, (void *)&aggbTn},
    {"aggbTv", tDouble, PID_TV_BD_MIN, PID_TV_BD_MAX, (void *)&aggbTv},
    {"steamKp", tDouble, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, (void *)&steamKp},
    {"startKp", tDouble, PID_KP_START_MIN, PID_KP_START_MAX, (void *)&startKp},
    {"startTn", tDouble, PID_TN_START_MIN, PID_TN_START_MAX, (void *)&startTn},
};

// Embedded HTTP Server
#include "RancilioServer.h"


enum SectionNames {
    sPIDSection,
    sTempSection,
    sBDSection,
    sOtherSection
};

std::vector<editable_t> editableVars = {};

unsigned long lastTempEvent = 0;
unsigned long tempEventInterval = 1000;

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
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);  // e.g. 1.3"
#endif
#if OLED_DISPLAY == 2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);  // e.g. 0.96"
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


#if (PRESSURESENSOR == 1)  // Pressure sensor connected
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
                ((analogRead(PINPRESSURESENSOR) - offset) * maxPressure * 0.0689476) /
                (fullScale - offset);   // pressure conversion and unit
                                        // conversion [psi] -> [bar]
            inputPressureFilter = filterPressureValue(inputPressure);

            debugPrintf("pressure raw / filtered: %f / %f\n", inputPressure, inputPressureFilter);
        }
    }
#endif

// Trigger for Rancilio E Machine
unsigned long previousMillisETrigger;  // initialisation at the end of init()
const unsigned long intervalETrigger = ETRIGGERTIME;  // in Seconds
int relayETriggerON, relayETriggerOFF;

// Emergency stop if temp is too high
void testEmergencyStop() {
    if (temperature > EmergencyStopTemp && emergencyStop == false) {
        emergencyStop = true;
    } else if (temperature < (brewSetPoint+5) && emergencyStop == true) {
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
        #if (PINTEMPSENSOR == 16 && defined(ESP8266))
            uint16_t temp = 0;
            Sensor1.getTemperature(&temp);
            temperature = Sensor1.calc_Celsius(&temp);
        #endif

        #if ((PINTEMPSENSOR != 16 && defined(ESP8266)) || defined(ESP32))
            temperature = Sensor2.getTemp();
        #endif
       
    #endif
      // temperature = 94;
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
    if (offlineMode == 1 || brewcounter > 11) return;

    /* if coldstart ist still true when checkWifi() is called, then there was no WIFI connection
     * at boot -> connect or offlinemode
     */
    do {
        if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
            int statusTemp = WiFi.status();

            if (statusTemp != WL_CONNECTED) {  // check WiFi connection status
                lastWifiConnectionAttempt = millis();
                wifiReconnects++;
                debugPrintf("Attempting WIFI reconnection: %i\n", wifiReconnects);

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


    if (wifiReconnects >= maxWifiReconnects && !setupDone) {  // no wifi connection after boot, initiate offline mode
        // (only directly after boot)
        initOfflineMode();
    }
}

void sendInflux() {
    unsigned long currentMillisInflux = millis();

    if (currentMillisInflux - previousMillisInflux >= intervalInflux) {
        previousMillisInflux = currentMillisInflux;
        influxSensor.clearFields();
        influxSensor.addField("value", temperature);
        influxSensor.addField("setPoint", setPoint);
        influxSensor.addField("HeaterPower", pidOutput);
        influxSensor.addField("Kp", bPID.GetKp());
        influxSensor.addField("Ki", bPID.GetKi());
        influxSensor.addField("Kd", bPID.GetKd());
        influxSensor.addField("pidON", pidON);
        influxSensor.addField("brewtime", brewtime);
        influxSensor.addField("preinfusionpause", preinfusionpause);
        influxSensor.addField("preinfusion", preinfusion);
        influxSensor.addField("steamON", steamON);

        byte mac[6];
        WiFi.macAddress(mac);
        String macaddr0 = number2string(mac[0]);
        String macaddr1 = number2string(mac[1]);
        String macaddr2 = number2string(mac[2]);
        String macaddr3 = number2string(mac[3]);
        String macaddr4 = number2string(mac[4]);
        String macaddr5 = number2string(mac[5]);
        String completemac = macaddr0 + macaddr1 + macaddr2 + macaddr3 + macaddr4 + macaddr5;
        influxSensor.addField("mac", completemac);

        // Write point
        if (!influxClient.writePoint(influxSensor)) {
            debugPrintf("InfluxDB write failed: %s\n", influxClient.getLastErrorMessage().c_str());
        }
    }
}


/**
 * @brief Check if MQTT is connected, if not reconnect abort function if offline, or brew is running
 *      MQTT is also using maxWifiReconnects!
 */
void checkMQTT() {
    if (offlineMode == 1 || brewcounter > 11) return;

    if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
        int statusTemp = mqtt.connected();

        if (statusTemp != 1) {
            lastMQTTConnectionAttempt = millis();  // Reconnection Timer Function
            MQTTReCnctCount++;                     // Increment reconnection Counter
            debugPrintf("Attempting MQTT reconnection: %i\n", MQTTReCnctCount);

            if (mqtt.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, 0, "offline") == true) {
                mqtt.subscribe(topic_set);
                debugPrintln("Subscribe to MQTT Topics");
            }   // Try to reconnect to the server; connect() is a blocking
                // function, watch the timeout!
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
 * @brief Publish Data to MQTT
 */
bool mqtt_publish(const char *reading, char *payload) {
    #if MQTT
        char topic[120];
        snprintf(topic, 120, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
        return mqtt.publish(topic, payload, true);
    #else
        return false;
    #endif
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
        if ((digitalRead(PINVOLTAGESENSOR) == VoltageSensorON) && brewDetected == 1) {
            timeBrewed = millis() - startingTime;
            lastbrewTime = timeBrewed;
        }

        // OFF: reset brew
        if ((digitalRead(PINVOLTAGESENSOR) == VoltageSensorOFF) && (brewDetected == 1 || coolingFlushDetectedQM == true)) {
            isBrewDetected = 0;  // rearm brewDetection
            brewDetected = 0;
            timePVStoON = timeBrewed;  // for QuickMill
            timeBrewed = 0;
            startingTime = 0;
            coolingFlushDetectedQM = false;
            debugPrintln("HW Brew - Voltage Sensor - End");
        }
    }

    // Activate brew detection
    if (brewDetectionMode == 1) {  // SW BD
        // BD PID only +/- 4 °C, no detection if HW was active
        if (tempRateAverage <= -brewSensitivity && isBrewDetected == 0 && (fabs(temperature - brewSetPoint) < 5)) {
            debugPrintln("SW Brew detected");
            timeBrewDetection = millis();
            isBrewDetected = 1;
        }
    } else if (brewDetectionMode == 2) {  // HW BD
        if (brewcounter > 10 && brewDetected == 0) {
            debugPrintln("HW Brew detected");
            timeBrewDetection = millis();
            isBrewDetected = 1;
            brewDetected = 1;
        }
    } else if (brewDetectionMode == 3) {  // voltage sensor
        switch (machine) {
            case QuickMill:
                if (!coolingFlushDetectedQM) {
                    int pvs = digitalRead(PINVOLTAGESENSOR);
                    if (pvs == VoltageSensorON && brewDetected == 0 &&
                        brewSteamDetectedQM == 0 && !steamQM_active) {
                        timeBrewDetection = millis();
                        timePVStoON = millis();
                        isBrewDetected = 1;
                        brewDetected = 0;
                        lastbrewTime = 0;
                        brewSteamDetectedQM = 1;
                        debugPrintln("Quick Mill: setting brewSteamDetectedQM = 1");
                        logbrew.reset();
                    }

                    const unsigned long minBrewDurationForSteamModeQM_ON = 50;
                    if (brewSteamDetectedQM == 1 && millis()-timePVStoON > minBrewDurationForSteamModeQM_ON) {
                        if (pvs == VoltageSensorOFF) {
                            brewSteamDetectedQM = 0;

                            if (millis() - timePVStoON < maxBrewDurationForSteamModeQM_ON) {
                                debugPrintln("Quick Mill: steam-mode detected");
                                initSteamQM();
                            } else {
                                debugPrintf("*** ERROR: QuickMill: neither brew nor steam\n");
                            }
                        } else if (millis() - timePVStoON > maxBrewDurationForSteamModeQM_ON) {
                            if (temperature < brewSetPoint + 2) {
                                debugPrintln("Quick Mill: brew-mode detected");
                                startingTime = timePVStoON;
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
                previousMillisVoltagesensorreading = millis();
                if (digitalRead(PINVOLTAGESENSOR) == VoltageSensorON && brewDetected == 0) {
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
 * @brief Assign the value of the mqtt parameter to the associated variable
 *
 * @param param MQTT parameter name
 * @param value MQTT value
 */
void assignMQTTParam(char *param, double value) {
    String key = String(param);
    boolean paramValid = false;
    boolean paramInRange = false;

    for (mqttVars_t m : mqttVars) {
        if (m.mqttParamName.equals(key)) {
            if (value >= m.minValue && value <= m.maxValue) {
                switch (m.type) {
                    case tDouble:
                        *(double *)m.mqttVarPtr = value;
                        paramValid = true;
                        break;
                    case tUInt8:
                        *(uint8_t *)m.mqttVarPtr = value;
                        paramValid = true;
                        break;
                    default:
                        debugPrintln((String(m.type) + " is not a recognized type for this MQTT parameter.").c_str());
                }

                paramInRange = true;
            }
            else {
                debugPrintln(("Value out of range for MQTT parameter "+ key + ".").c_str());
                paramInRange = false;
            }

            break;
        }
    }

    if (paramValid && paramInRange) {
        if (key.equals("steamON")) {
            steamFirstON = value;
        }

        mqtt_publish(param, number2string(value));
        writeSysParamsToStorage();
    }
    else {
        debugPrintln((key + " is not a valid MQTT parameter.").c_str());
    }
}

/**
 * @brief MQTT Callback Function: set Parameters through MQTT
 */
void mqtt_callback(char *topic, byte *data, unsigned int length) {
    char topic_str[256];
    os_memcpy(topic_str, topic, sizeof(topic_str));
    topic_str[255] = '\0';
    char data_str[length + 1];
    os_memcpy(data_str, data, length);
    data_str[length] = '\0';
    char topic_pattern[255];
    char configVar[120];
    char cmd[64];
    double data_double;

    snprintf(topic_pattern, sizeof(topic_pattern), "%s%s/%%[^\\/]/%%[^\\/]", mqtt_topic_prefix, hostname);
    debugPrintln(topic_pattern);

    if ((sscanf(topic_str, topic_pattern, &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0)) {
        debugPrintln(topic_str);
        return;
    }

    debugPrintln(topic_str);
    debugPrintln(data_str);

    sscanf(data_str, "%lf", &data_double);

    assignMQTTParam(configVar, data_double);
}

/**
 * @brief E-Trigger for Silvia E
 */
void handleETrigger() {
    // Static variable only one time is 0
    static int ETriggeractive = 0;
    unsigned long currentMillisETrigger = millis();

    if (ETRIGGER == 1) {  // E Trigger has been activated in userconfig
        if (currentMillisETrigger - previousMillisETrigger >= (1000 * intervalETrigger))  { // s to ms * 1000
            ETriggeractive = 1;
            previousMillisETrigger = currentMillisETrigger;

            digitalWrite(PINETRIGGER, relayETriggerON);
        }

        // 10 Seconds later
        else if (ETriggeractive == 1 && previousMillisETrigger + (10 * 1000) < (currentMillisETrigger)) {
            digitalWrite(PINETRIGGER, relayETriggerOFF);
            ETriggeractive = 0;
        }
    }
}

/**
 * @brief steamON & Quickmill
 */
void checkSteamON() {
    // check digital GIPO
    if (digitalRead(PINSTEAMSWITCH) == HIGH) {
        steamON = 1;
    }

    // if activated via web interface then steamFirstON == 1, prevent override
    if (digitalRead(PINSTEAMSWITCH) == LOW && steamFirstON == 0) {
        steamON = 0;
    }

    // monitor QuickMill thermoblock steam-mode
    if (machine == QuickMill) {
        if (steamQM_active == true) {
            if (checkSteamOffQM() == true) {  // if true: steam-mode can be turned off
                steamON = 0;
                steamQM_active = false;
                lastTimePVSwasON = 0;
            } else {
                steamON = 1;
            }
        }
    }

    if (steamON == 1) {
        setPoint = steamSetPoint;
    } else if (steamON == 0) {
        setPoint = brewSetPoint;
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
    lastTimePVSwasON = millis();  // time when pinvoltagesensor changes from ON to OFF
    steamQM_active = true;
    timePVStoON = 0;
    steamON = 1;
}

boolean checkSteamOffQM() {
    /* Monitor pinvoltagesensor during active steam mode of QuickMill
     * thermoblock. Once the pinvolagesenor remains OFF for longer than a
     * pump-pulse time peride the switch is turned off and steam mode finished.
     */
    if (digitalRead(PINVOLTAGESENSOR) == VoltageSensorON) {
        lastTimePVSwasON = millis();
    }

    if ((millis() - lastTimePVSwasON) > minPVSOffTimedForSteamModeQM_OFF) {
        lastTimePVSwasON = 0;
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
            // Prevent coldstart leave by temperature 222
            if (temperature < (brewSetPoint - 1) || temperature < 150) {
                machineState = kColdStart;
                debugPrintf("%d\n", temperature);
                debugPrintf("%d\n", machineState);

                // some users have 100 % Output in kInit / KColdstart, reset PID
                pidMode = 0;
                bPID.SetMode(pidMode);
                pidOutput = 0;
                digitalWrite(PINHEATER, LOW);  // Stop heating

                // start PID
                pidMode = 1;
                bPID.SetMode(pidMode);
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kColdStart:
            /* One high temperature let the state jump to 19.
            * switch (machinestatecold) prevent it, we wait 10 sec with new state.
            * during the 10 sec the temperature has to be temperature >= (BrewSetPoint-1),
            * If not, reset machinestatecold
            */
            switch (machinestatecold) {
                case 0:
                    if (temperature >= (brewSetPoint - 1) && temperature < 150) {
                        machinestatecoldmillis = millis();  // get millis for interval calc
                        machinestatecold = 10;              // new state
                        debugPrintln(
                            "temperature >= (BrewSetPoint-1), wait 10 sec before machineState BelowSetPoint");
                    }
                    break;

                case 10:
                    if (temperature < (brewSetPoint - 1)) {
                        machinestatecold = 0;  //  temperature was only one time above
                                               //  BrewSetPoint, reset machinestatecold
                        debugPrintln("Reset timer for machineState BelowSetPoint: temperature < (BrewSetPoint-1)");

                        break;
                    }
                    
                    // 10 sec temperature above BrewSetPoint, no set new state
                    if (machinestatecoldmillis + 10 * 1000 < millis()) {
                        machineState = kBelowSetPoint;
                        debugPrintln("5 sec temperature >= (BrewSetPoint-1) finished, switch to state BelowSetPoint");
                    }
                    break;
            }

            if ((timeBrewed > 0 && ONLYPID == 1) ||  // timeBrewed with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42))
            {
                machineState = kBrew;
            }

            if (steamON == 1) {
                machineState = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        // Setpoint is below current temperature
        case kBelowSetPoint:
            brewDetection();

            if (temperature >= (brewSetPoint)) {
                machineState = kPidNormal;
            }

            if ((timeBrewed > 0 && ONLYPID == 1) ||  // timeBrewed with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42))
            {
                machineState = kBrew;
            }

            if (backflushON || backflushState > 10) {
                machineState = kBackflush;
            }

            if (steamON == 1) {
                machineState = kSteam;
            }

            if (pidON == 0) {
                machineState = kPidOffline;
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kPidNormal:
            brewDetection();     // if brew detected, set BD PID values (if enabled)

            if ((timeBrewed > 0 && ONLYPID == 1) ||  // timeBrewed with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42))
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
                ((brewcounter == 10 || brewcounter == 43) && ONLYPID == 0)) // Hardware BD
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

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kBrewDetectionTrailing:
            brewDetection();

            if (isBrewDetected == 0) {
                machineState = kPidNormal;
            }

            if ((timeBrewed > 0 && ONLYPID == 1 && brewDetectionMode == 3) ||  // Allow brew directly after BD only when using OnlyPID AND hardware brew switch detection
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42))
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
                if (tempRateAverage > 0 && temperature < brewSetPoint + 2) {
                    machineState = kPidNormal;
                }
            }

            if ((brewDetectionMode == 3 || brewDetectionMode == 2) && temperature < brewSetPoint + 2) {
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

        case kPidOffline:
            if (pidON == 1) {
                if (coldstart) {
                    machineState = kColdStart;
                } else if (!coldstart && (temperature > (brewSetPoint - 10))) {  // temperature higher BrewSetPoint-10, normal PID
                    machineState = kPidNormal;
                } else if (temperature <= (brewSetPoint - 10)) {
                    machineState = kColdStart;  // temperature 10C below set point, enter cold start
                    coldstart = true;
                }
            }

            if (sensorError) {
                machineState = kSensorError;
            }
            break;

        case kSensorError:
            machineState = kSensorError;
            break;

        case keepromError:
            machineState = keepromError;
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
        case kBelowSetPoint:
            return "Set Point Negative";
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
        case kEmergencyStop:
            return "Emergency Stop";
        case kPidOffline:
            return "PID Offline";
        case kSensorError:
            return "Sensor Error";
        case keepromError:
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
            setPoint, temperature, machineState, bPID.GetKp(), bPID.GetKi(), bPID.GetKd());
    }
}

/**
 * @brief TODO
 */
void tempLed() {
    if (TEMPLED == 1) {
        pinMode(LEDPIN, OUTPUT);
        digitalWrite(LEDPIN, LOW);

        // inner Tempregion
        if ((machineState == kPidNormal && (fabs(temperature - setPoint) < 0.5)) || (temperature > 115 && fabs(temperature - brewSetPoint) < 5))  {
            digitalWrite(LEDPIN, HIGH);
        }
    }
}

/**
 * @brief Set up internal WiFi hardware
 */
void wiFiSetup() {
    wm.setCleanConnect(true);
    wm.setConfigPortalTimeout(60); // sec Timeout for Portal
    wm.setConnectTimeout(10); // Try 10 sec to connect to WLAN, 5 sec too short!
    wm.setBreakAfterConfig(true);
    wm.setConnectRetries(3);
    //wm.setWiFiAutoReconnect(true);
    wm.setHostname(hostname);

    if (wm.autoConnect(hostname, pass)) {
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

const char sysVersion[] = (STR(FW_VERSION) "." STR(FW_SUBVERSION) "." STR(FW_HOTFIX) " " FW_BRANCH);

void setup() {
    editableVars =  {
        //#1
        {F("PID_ON"), "Enable PID Controller", false, "", kUInt8, 0, []{ return true; }, 0, 1, (void *)&pidON},

        //#2
        {F("START_USE_PONM"), F("Enable PonM"), true, F("Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>) while heating up the machine. Otherwise, just use the same PID values that are used later"), kUInt8, 0, []{ return true; }, 0, 1, (void *)&usePonM},

        //#3
        {F("START_KP"), F("Start Kp"), true, F("Proportional gain for cold start controller. This value is not used with the the error as usual but the absolute value of the temperature and counteracts the integral part as the temperature rises. Ideally, both parameters are set so that they balance each other out when the target temperature is reached."), kDouble, sPIDSection, []{ return true && usePonM; }, PID_KP_START_MIN, PID_KP_START_MAX, (void *)&startKp},

        //#4
        {F("START_TN"), F("Start Tn"), true, F("Integral gain for cold start controller (PonM mode, <a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>)"), kDouble, sPIDSection, []{ return true && usePonM; }, PID_TN_START_MIN, PID_TN_START_MAX, (void *)&startTn},

        //#5
        {F("PID_KP"), F("PID Kp"), true, F("Proportional gain (in Watts/C°) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The higher this value is, the higher is the output of the heater for a given temperature difference. E.g. 5°C difference will result in P*5 Watts of heater output."), kDouble, sPIDSection, []{ return true; }, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, (void *)&aggKp},

        //#6
        {F("PID_TN"), F("PID Tn (=Kp/Ki)"), true, F("Integral time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The larger this value is, the slower the integral part of the PID will increase (or decrease) if the process value remains above (or below) the setpoint in spite of proportional action. The smaller this value, the faster the integral term changes."), kDouble, sPIDSection, []{ return true; }, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, (void *)&aggTn},

        //#7
        {F("PID_TV"), F("PID Tv (=Kd/Kp)"), true, F("Differential time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). This value determines how far the PID equation projects the current trend into the future. The higher the value, the greater the dampening. Select it carefully, it can cause oscillations if it is set too high or too low."), kDouble, sPIDSection, []{ return true; }, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, (void *)&aggTv},

        //#8
        {F("PID_I_MAX"), F("PID Integrator Max"), true, F("Internal integrator limit to prevent windup (in Watts). This will allow the integrator to only grow to the specified value. This should be approximally equal to the output needed to hold the temperature after the setpoint has been reached and is depending on machine type and whether the boiler is insulated or not."), kDouble, sPIDSection, []{ return true; }, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, (void *)&aggIMax},

        //#9
        {F("STEAM_KP"), F("Steam Kp"), true, F("Proportional gain for the steaming mode (I or D are not used)"), kDouble, sPIDSection, []{ return true; }, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, (void *)&steamKp},

        //#10
        {F("STEAM_SET_POINT"), F("Steam Set point (°C)"), true, F("The temperature that the PID will use for steam mode"), kDouble, sTempSection, []{ return true; }, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, (void *)&steamSetPoint},

        //#11
        {F("TEMP"), F("Temperature"), false, "", kDouble, sPIDSection, []{ return false; }, 0, 200, (void *)&temperature},

        //#12
        {F("BREW_SET_POINT"), F("Set point (°C)"), true, F("The temperature that the PID will attempt to reach and hold"), kDouble, sTempSection, []{ return true; }, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, (void *)&brewSetPoint},

        //#13
        {F("BREW_TEMP_OFFSET"), F("Offset (°C)"), true, F("Optional offset that is added to the user-visible setpoint. Can be used to compensate sensor offsets and the average temperature loss between boiler and group so that the setpoint represents the approximate brew temperature."), kDouble, sTempSection, []{ return true; }, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, (void *)&brewTempOffset},

        //#14
        {F("BREW_TIME"), F("Brew Time (s)"), true, F("Stop brew after this time"), kDouble, sTempSection, []{ return true && ONLYPID == 0; }, BREW_TIME_MIN, BREW_TIME_MAX, (void *)&brewtime},

        //#15
        {F("BREW_PREINFUSIONPAUSE"), F("Preinfusion Pause Time (s)"), false, "", kDouble, sTempSection, []{ return true && ONLYPID == 0; }, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, (void *)&preinfusionpause},

        //#16
        {F("BREW_PREINFUSION"), F("Preinfusion Time (s)"), false, "", kDouble, sTempSection, []{ return true && ONLYPID == 0; }, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, (void *)&preinfusion},

        //#17
        {F("SCALE_WEIGHTSETPOINT"), F("Brew weight setpoint (g)"), true, F("Brew until this weight has been measured."), kDouble, sTempSection, []{ return true && (ONLYPIDSCALE == 1 || BREWMODE == 2); }, WEIGHTSETPOINT_MIN, WEIGHTSETPOINT_MAX, (void *)&weightSetpoint},

        //#18
        {F("PID_BD_ON"), F("Enable Brew PID"), true, F("Use separate PID parameters while brew is running"), kUInt8, sBDSection, []{ return true && BREWDETECTION > 0; }, 0, 1, (void *)&useBDPID},

        //#19
        {F("PID_BD_KP"), F("BD Kp"), true, F("Proportional gain (in Watts/°C) for the PID when brewing has been detected. Use this controller to either increase heating during the brew to counter temperature drop from fresh cold water in the boiler. Some machines, e.g. Rancilio Silvia, actually need to heat less not at all during the brew because of high temperature stability (<a href='https://www.kaffee-netz.de/threads/installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/#post-1453641' target='_blank'>Details<a>)"), kDouble, sBDSection, []{ return true && BREWDETECTION > 0 && useBDPID; }, PID_KP_BD_MIN, PID_KP_BD_MAX, (void *)&aggbKp},

        //#20
        {F("PID_BD_TN"), F("BD Tn (=Kp/Ki)"), true, F("Integral time constant (in seconds) for the PID when brewing has been detected."), kDouble, sBDSection, []{ return true && BREWDETECTION > 0 && useBDPID; }, PID_TN_BD_MIN, PID_TN_BD_MAX, (void *)&aggbTn},

        //#21
        {F("PID_BD_TV"), F("BD Tv (=Kd/Kp)"), true, F("Differential time constant (in seconds) for the PID when brewing has been detected."), kDouble, sBDSection, []{ return true && BREWDETECTION > 0 && useBDPID; }, PID_TV_BD_MIN, PID_TV_BD_MAX, (void *)&aggbTv},

        //#22
        {F("PID_BD_TIME"), F("PID BD Time (s)"), true, F("Fixed time in seconds for which the BD PID will stay enabled (also after Brew switch is inactive again)."), kDouble, sBDSection, []{ return true && BREWDETECTION > 0 && (useBDPID || BREWDETECTION == 1); }, BREW_SW_TIME_MIN, BREW_SW_TIME_MAX, (void *)&brewtimesoftware},

        //#23
        {F("PID_BD_BREWSENSITIVITY"), F("PID BD Sensitivity"), true, F("Software brew detection sensitivity that looks at average temperature, <a href='https://manual.rancilio-pid.de/de/customization/brueherkennung.html' target='_blank'>Details</a>. Needs to be &gt;0 also for Hardware switch detection."), kDouble, sBDSection, []{ return true && BREWDETECTION == 1; }, BD_THRESHOLD_MIN, BD_THRESHOLD_MAX, (void *)&brewSensitivity},

        //#24
        {F("STEAM_MODE"), F("Steam Mode"), false, "", kUInt8, sOtherSection, []{ return false; }, 0, 1, (void *)&steamON},

        //#25
        {F("BACKFLUSH_ON"), F("Backflush"), false, "", kUInt8, sOtherSection, []{ return false; }, 0, 1, (void *)&backflushON},

        //#26
        {F("VERSION"), F("Version"), false, "", kCString, sOtherSection, []{ return false; }, 0, 1, (void *)sysVersion},
    };
    //when adding parameters, update EDITABLE_VARS_LEN!

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

    if (TRIGGERRELAYTYPE) {
        relayETriggerON = HIGH;
        relayETriggerOFF = LOW;
    } else {
        relayETriggerON = LOW;
        relayETriggerOFF = HIGH;
    }
    if (VOLTAGESENSORTYPE) {
        VoltageSensorON = HIGH;
        VoltageSensorOFF = LOW;
    } else {
        VoltageSensorON = LOW;
        VoltageSensorOFF = HIGH;
    }

    // Initialize Pins
    pinMode(PINVALVE, OUTPUT);
    pinMode(PINPUMP, OUTPUT);
    pinMode(PINHEATER, OUTPUT);
    pinMode(PINSTEAMSWITCH, INPUT);
    digitalWrite(PINVALVE, relayOFF);
    digitalWrite(PINPUMP, relayOFF);
    digitalWrite(PINHEATER, LOW);

    // IF Etrigger selected
    if (ETRIGGER == 1) {
        pinMode(PINETRIGGER, OUTPUT);
        digitalWrite(PINETRIGGER, relayETriggerOFF);  // Set the E-Trigger OFF its,
                                                        // important for LOW Trigger Relais
    }

    // IF Voltage sensor selected
    if (BREWDETECTION == 3) {
        pinMode(PINVOLTAGESENSOR, PINMODEVOLTAGESENSOR);
    }

    // IF PINBREWSWITCH & Steam selected
    if (PINBREWSWITCH > 0) {
        #if (defined(ESP8266) && PINBREWSWITCH == 16)
            pinMode(PINBREWSWITCH, INPUT_PULLDOWN_16);
        #endif

        #if (defined(ESP8266) && PINBREWSWITCH == 15)
            pinMode(PINBREWSWITCH, INPUT);
        #endif

        #if defined(ESP32)
            pinMode(PINBREWSWITCH, INPUT_PULLDOWN);
            ;
        #endif
    }

    #if (defined(ESP8266) && PINSTEAMSWITCH == 16)
        pinMode(PINSTEAMSWITCH, INPUT_PULLDOWN_16);
    #endif

    #if (defined(ESP8266) && PINSTEAMSWITCH == 15)
        pinMode(PINSTEAMSWITCH, INPUT);
    #endif

    #if defined(ESP32)
        pinMode(PINSTEAMSWITCH, INPUT_PULLDOWN);
    #endif

    #if OLED_DISPLAY != 0
        u8g2.setI2CAddress(oled_i2c * 2);
        u8g2.begin();
        u8g2_prepare();
        displayLogo(String("Version ") + String(sysVersion), "");
       // delay(2000); // caused crash with wifi manager
    #endif

    // Init Scale by BREWMODE 2 or SHOTTIMER 2
    #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
        initScale();
    #endif

    // VL530L0x TOF sensor
    #if TOF == 1
        lox.begin(tof_i2c);  // initialize TOF sensor at I2C address
        lox.setMeasurementTimingBudgetMicroSeconds(2000000);
    #endif

    // Fallback offline
    if (connectmode == 1) {  // WiFi Mode
        wiFiSetup();
        websiteSetup();

        // OTA Updates
        if (ota && WiFi.status() == WL_CONNECTED) {
            ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
            ArduinoOTA.setPassword(OTApass);  //  Password for OTA
            ArduinoOTA.begin();
        }

        if (MQTT == 1) {
            snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "status");
            snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
            mqtt.setServer(mqtt_server_ip, mqtt_server_port);
            mqtt.setCallback(mqtt_callback);
            checkMQTT();
        }

        if (INFLUXDB == 1) {
            if (INFLUXDB_AUTH_TYPE == 1) {
                influxClient.setConnectionParams(INFLUXDB_URL, INFLUXDB_ORG_NAME, INFLUXDB_DB_NAME, INFLUXDB_API_TOKEN);
            }
            else if (INFLUXDB_AUTH_TYPE == 2 && (strlen(INFLUXDB_USER) > 0) && (strlen(INFLUXDB_PASSWORD) > 0)) {
                influxClient.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
            }
        }
    } else if (connectmode == 0) 
    { 
        wm.disconnect(); // no wm
        readSysParamsFromStorage(); // get values from stroage
        offlineMode = 1 ; //offline mode
        pidON = 1  ; //pid on
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

    //TSic 306 temp sensor
    #if TEMPSENSOR == 2
        //use old TSic library if connected to pin 16 of ESP8266
        #if (PINTEMPSENSOR == 16 && defined(ESP8266))
            uint16_t temp = 0;
            Sensor1.getTemperature(&temp);
            temperature = Sensor1.calc_Celsius(&temp);
        #endif

        //in all other cases, use ZACwire
        #if ((PINTEMPSENSOR != 16 && defined(ESP8266)) || defined(ESP32))
            temperature = Sensor2.getTemp();
        #endif
    #endif

    temperature -= brewTempOffset;

    // Initialisation MUST be at the very end of the init(), otherwise the
    // time comparision in loop() will have a big offset
    unsigned long currentTime = millis();
    previousMillistemp = currentTime;
    windowStartTime = currentTime;
    previousMillisDisplay = currentTime;
    previousMillisMQTT = currentTime;
    previousMillisInflux = currentTime;
    previousMillisETrigger = currentTime;
    previousMillisVoltagesensorreading = currentTime;
    lastMQTTConnectionAttempt = currentTime;

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
#if TOF == 1
    if (calibration_mode == 1) {
        loopcalibrate();
    } else {
        looppid();
    }
#else
    looppid();
#endif

    checkForRemoteSerialClients();
}

#if TOF == 1
/**
 * @brief ToF sensor calibration mode
 */
void loopcalibrate() {
    // Deactivate PID
    if (pidMode == 1) {
        pidMode = 0;
        bPID.SetMode(pidMode);
        pidOutput = 0;
    }

    digitalWrite(PINHEATER, LOW);   // Stop heating to be on the safe side ...

    unsigned long currentMillisTOF = millis();

    if (currentMillisTOF - previousMillisTOF >= intervalTOF) {
        previousMillisTOF = millis();
        VL53L0X_RangingMeasurementData_t measure;   // TOF Sensor measurement
        lox.rangingTest(&measure, false);           // pass in 'true' to get debug data printout!
        distance = measure.RangeMilliMeter;         // write new distence value to 'distance'
        debugPrintf("%d mm\n", distance);

        #if OLED_DISPLAY != 0
            displayDistance(distance);
        #endif
    }
}
#endif

void looppid() {
    // Only do Wifi stuff, if Wifi is connected
    if (WiFi.status() == WL_CONNECTED && offlineMode == 0) {
        if (MQTT == 1) {
            checkMQTT();
            writeSysParamsToMQTT();

            if (mqtt.connected() == 1) {
                mqtt.loop();
            }
        }

        ArduinoOTA.handle();  // For OTA

        // Disable interrupt if OTA is starting, otherwise it will not work
        ArduinoOTA.onStart([]() {
            disableTimer1();
            digitalWrite(PINHEATER, LOW);  // Stop heating
        });

        ArduinoOTA.onError([](ota_error_t error) { enableTimer1(); });

        // Enable interrupts if OTA is finished
        ArduinoOTA.onEnd([]() { enableTimer1(); });

        wifiReconnects = 0;  // reset wifi reconnects if connected
    } else {
        checkWifi();
    }

    #if TOF != 0
        unsigned long currentMillisTOF = millis();

        if (currentMillisTOF - previousMillisTOF >= intervalTOF) {
            previousMillisTOF = millis();
            VL53L0X_RangingMeasurementData_t measure;   // TOF Sensor measurement
            lox.rangingTest(&measure, false);           // pass in 'true' to get debug data printout!
            distance = measure.RangeMilliMeter;         // write new distence value to 'distance'

            if (distance <= 1000) {
                percentage = (100.00 / (water_empty - water_full)) * (water_empty - distance);  // calculate percentage of waterlevel
                debugPrintf("%d\n", percentage);
            }
        }
    #endif

    refreshTemp();        // update temperature values
    testEmergencyStop();  // test if temp is too high
    bPID.Compute();       // the variable pidOutput now has new values from PID (will be written to heater pin in ISR.cpp)

    if ((millis() - lastTempEvent) > tempEventInterval) {
        //send temperatures to website endpoint
        sendTempEvent(temperature, brewSetPoint, pidOutput);
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

    brew();                  // start brewing if button pressed
    checkSteamON();          // check for steam
    setEmergencyStopTemp();
    handleMachineState();      // update machineState
    tempLed();

    if (INFLUXDB == 1  && offlineMode == 0 ) {
        sendInflux();
    }

    if (ETRIGGER == 1) {  // E-Trigger active then void Etrigger()
        handleETrigger();
    }

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

    if (machineState == kPidOffline || machineState == kSensorError || machineState == kEmergencyStop || machineState == keepromError || brewPIDdisabled) {
        if (pidMode == 1) {
            // Force PID shutdown
            pidMode = 0;
            bPID.SetMode(pidMode);
            pidOutput = 0;
            digitalWrite(PINHEATER, LOW);  // Stop heating
        }
    } else {  // no sensorerror, no pid off or no Emergency Stop
        if (pidMode == 0) {
            pidMode = 1;
            bPID.SetMode(pidMode);
        }
    }

    // Set PID if first start of machine detected, and no steamON
    if ((machineState == kInit || machineState == kColdStart || machineState == kBelowSetPoint)) {
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
        if (BREWPID_DELAY > 0 && timeBrewed > 0 && timeBrewed < BREWPID_DELAY*1000) {
            //disable PID for BREWPID_DELAY seconds, enable PID again with new tunings after that
            if (!brewPIDdisabled) {
                brewPIDdisabled = true;
                bPID.SetMode(MANUAL);
                debugPrintf("disabled PID, waiting for %d seconds before enabling PID again\n", BREWPID_DELAY);
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
    if (sysParaPidKpStart.getStorage() != 0) return -1;
    if (sysParaPidTnStart.getStorage() != 0) return -1;
    if (sysParaPidKpReg.getStorage() != 0) return -1;
    if (sysParaPidTnReg.getStorage() != 0) return -1;
    if (sysParaPidTvReg.getStorage() != 0) return -1;
    if (sysParaPidIMaxReg.getStorage() != 0) return -1;
    if (sysParaPidKpBd.getStorage() != 0) return -1;
    if (sysParaPidTnBd.getStorage() != 0) return -1;
    if (sysParaPidTvBd.getStorage() != 0) return -1;
    if (sysParaBrewSetPoint.getStorage() != 0) return -1;
    if (sysParaTempOffset.getStorage() != 0) return -1;
    if (sysParaBrewTime.getStorage() != 0) return -1;
    if (sysParaBrewSwTime.getStorage() != 0) return -1;
    if (sysParaBrewThresh.getStorage() != 0) return -1;
    if (sysParaPreInfTime.getStorage() != 0) return -1;
    if (sysParaPreInfPause.getStorage() != 0) return -1;
    if (sysParaWeightSetPoint.getStorage() != 0) return -1;
    if (sysParaPidOn.getStorage() != 0) return -1;
    if (sysParaPidKpSteam.getStorage() != 0) return -1;
    if (sysParaSteamSetPoint.getStorage() != 0) return -1;
    if (sysParaUsePonM.getStorage() != 0) return -1;
    if (sysParaUseBDPID.getStorage() != 0) return -1;
    if (sysParaSteamSetPoint.getStorage() != 0) return -1;
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
    if (sysParaBrewSetPoint.setStorage() != 0) return -1;
    if (sysParaTempOffset.setStorage() != 0) return -1;
    if (sysParaBrewTime.setStorage() != 0) return -1;
    if (sysParaBrewSwTime.setStorage() != 0) return -1;
    if (sysParaBrewThresh.setStorage() != 0) return -1;
    if (sysParaPreInfTime.setStorage() != 0) return -1;
    if (sysParaPreInfPause.setStorage() != 0) return -1;
    if (sysParaWeightSetPoint.setStorage() != 0) return -1;
    if (sysParaPidKpSteam.setStorage() != 0) return -1;
    if (sysParaSteamSetPoint.setStorage() != 0) return -1;
    if (sysParaUseBDPID.setStorage() != 0) return -1;
    if (sysParaPidKpBd.setStorage() != 0) return -1;
    if (sysParaPidTnBd.setStorage() != 0) return -1;
    if (sysParaPidTvBd.setStorage() != 0) return -1;
    if (sysParaSteamSetPoint.setStorage() != 0) return -1;

    return storageCommit();
}


/**
 * @brief Send all current system parameter values to MQTT
 *
 * @return TODO 0 = success, < 0 = failure
 */
void writeSysParamsToMQTT(void) {
    unsigned long currentMillisMQTT = millis();
    if ((currentMillisMQTT - previousMillisMQTT >= intervalMQTT) && MQTT == 1) {
        previousMillisMQTT = currentMillisMQTT;

        if (mqtt.connected() == 1) {
            // status topic (will sets it to offline)
            mqtt_publish("status", (char *)"online");

            mqtt_publish("temperature", number2string(temperature));
            mqtt_publish("brewSetPoint", number2string(brewSetPoint));
            mqtt_publish("brewTempOffset", number2string(brewTempOffset));
            mqtt_publish("steamSetPoint", number2string(steamSetPoint));
            mqtt_publish("heaterPower", number2string(pidOutput));
            mqtt_publish("currentKp", number2string(bPID.GetKp()));
            mqtt_publish("currentKi", number2string(bPID.GetKi()));
            mqtt_publish("currentKd", number2string(bPID.GetKd()));
            mqtt_publish("pidON", number2string(pidON));
            mqtt_publish("brewtime", number2string(brewtime));
            mqtt_publish("preinfusionpause", number2string(preinfusionpause));
            mqtt_publish("preinfusion", number2string(preinfusion));
            mqtt_publish("steamON", number2string(steamON));
            mqtt_publish("backflushON", number2string(backflushON));

            // Normal PID
            mqtt_publish("aggKp", number2string(aggKp));
            mqtt_publish("aggTn", number2string(aggTn));
            mqtt_publish("aggTv", number2string(aggTv));
            mqtt_publish("aggIMax", number2string(aggIMax));

            // BD PID
            mqtt_publish("aggbKp", number2string(aggbKp));
            mqtt_publish("aggbTn", number2string(aggbTn));
            mqtt_publish("aggbTv", number2string(aggbTv));

            // Start PI
            mqtt_publish("startKp", number2string(startKp));
            mqtt_publish("startTn", number2string(startTn));

             // Steam P
            mqtt_publish("steamKp", number2string(steamKp));

            //BD Parameter
        #if BREWDETECTION == 1
            mqtt_publish("brewTimer", number2string(brewtimesoftware));
            mqtt_publish("brewLimit", number2string(brewSensitivity));
        #endif

        #if BREWMODE == 2
            mqtt_publish("weightSetpoint", number2string(weightSetpoint));
        #endif

        #if TOF == 1
            mqtt_publish("waterLevelDistance", number2string(distance));
            mqtt_publish("waterLevelPercentage", number2string(percentage));
        #endif
        }
    }
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
