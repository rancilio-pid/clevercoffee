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

// STL includes
#include <map>

// Libraries & Dependencies
#include "Logger.h"
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <PID_v1.h>  // for PID calculation
#include <U8g2lib.h> // i2c display
#include <WiFiManager.h>
#include <os.h>

// Includes
#include "Config.h"
#include "ParameterRegistry.h"

// Utilities
#include "utils/Timer.h"

// Hardware classes
#include "hardware/GPIOPin.h"
#include "hardware/IOSwitch.h"
#include "hardware/LED.h"
#include "hardware/Relay.h"
#include "hardware/StandardLED.h"
#include "hardware/Switch.h"
#include "hardware/TempSensorDallas.h"
#include "hardware/TempSensorTSIC.h"
#include "hardware/pinmapping.h"

// User configuration & defaults
#include "defaults.h"

hw_timer_t* timer = nullptr;

#include "hardware/pressureSensor.h"
#include <Wire.h>

#define HX711_ADC_config_h
#define SAMPLES                32
#define IGN_HIGH_SAMPLE        1
#define IGN_LOW_SAMPLE         1
#define SCK_DELAY              1
#define SCK_DISABLE_INTERRUPTS 0
#include <HX711_ADC.h>

#define HIGH_ACCURACY

Config config;

enum MachineState {
    kInit = 0,
    kPidNormal = 20,
    kBrew = 30,
    kManualFlush = 35,
    kSteam = 40,
    kBackflush = 50,
    kWaterTankEmpty = 70,
    kEmergencyStop = 80,
    kPidDisabled = 90,
    kStandby = 95,
    kSensorError = 100,
    kEepromError = 110,
};

MachineState machineState = kInit;
MachineState lastmachinestate = kInit;
int lastmachinestatepid = -1;

int offlineMode = 0;

// Display
U8G2* u8g2 = nullptr;

uint8_t featureFullscreenBrewTimer = 0;
uint8_t featureFullscreenManualFlushTimer = 0;
double postBrewTimerDuration = POST_BREW_TIMER_DURATION;
uint8_t featureHeatingLogo = 0;
uint8_t featurePidOffLogo = 0;

// WiFi
uint8_t wifiCredentialsSaved = 0;
WiFiManager wm;
constexpr unsigned long wifiConnectionDelay = WIFICONNECTIONDELAY;
constexpr unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;
String hostname;
auto pass = WM_PASS;
unsigned long lastWifiConnectionAttempt = millis();
unsigned int wifiReconnects = 0; // actual number of reconnects

// OTA
String otaPass;

// Pressure sensor
float inputPressure = 0;
float inputPressureFilter = 0;
const unsigned long intervalPressure = 100;
unsigned long previousMillisPressure; // initialisation at the end of init()

Switch* waterTankSensor = nullptr;

GPIOPin* statusLedPin = nullptr;
GPIOPin* brewLedPin = nullptr;
GPIOPin* steamLedPin = nullptr;

LED* statusLed = nullptr;
LED* brewLed = nullptr;
LED* steamLed = nullptr;

GPIOPin heaterRelayPin(PIN_HEATER, GPIOPin::OUT);
Relay* heaterRelay = nullptr;

GPIOPin pumpRelayPin(PIN_PUMP, GPIOPin::OUT);
Relay* pumpRelay = nullptr;

GPIOPin valveRelayPin(PIN_VALVE, GPIOPin::OUT);
Relay* valveRelay = nullptr;

Switch* powerSwitch = nullptr;
Switch* brewSwitch = nullptr;
Switch* steamSwitch = nullptr;

TempSensor* tempSensor = nullptr;

#include "isr.h"

// Method forward declarations
void setSteamMode(int steamMode);
void setBackflush(int backflush);
void setScaleTare(int tare);
void setScaleCalibration(int calibration);
void setPIDTunings(bool usePonM);
void setBDPIDTunings();
void setRuntimePidState(bool enabled);
void loopcalibrate();
void loopPid();
void loopLED();
void checkWaterTank();
void printMachineState();
char const* machinestateEnumToString(MachineState machineState);
char* number2string(double in);
char* number2string(float in);
char* number2string(int in);
char* number2string(unsigned int in);
float filterPressureValue(float input);
int writeSysParamsToMQTT(bool continueOnError);
void updateStandbyTimer();
void resetStandbyTimer();
void wiFiReset();

// system parameters
uint8_t pidON = 0;   // 1 = control loop in closed loop
uint8_t usePonM = 0; // 1 = use PonM for cold start PID, 0 = use normal PID for cold start
double brewSetpoint = SETPOINT;
double brewTempOffset = TEMPOFFSET;
double setpoint = brewSetpoint;
double steamSetpoint = STEAMSETPOINT;
double steamKp = STEAMKP;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double aggIMax = AGGIMAX;
double emaFactor = EMA_FACTOR;

// Scale
float scaleCalibration = SCALE_CALIBRATION_FACTOR;
float scale2Calibration = SCALE_CALIBRATION_FACTOR;
float scaleKnownWeight = SCALE_KNOWN_WEIGHT;
double targetBrewWeight = TARGET_BREW_WEIGHT;

// PID - values for offline brew detection
uint8_t useBDPID = 0;
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;
double aggbKi = (aggbTn == 0) ? 0 : aggbKp / aggbTn;
double aggbKd = aggbTv * aggbKp;
double aggKi = (aggTn == 0) ? 0 : aggKp / aggTn;
double aggKd = aggTv * aggKp;

double brewPIDDelay = BREW_PID_DELAY; // Time PID will be disabled after brew started

uint8_t standbyModeOn = 0;
double standbyModeTime = STANDBY_MODE_TIME;

#include "standby.h"

// Variables to hold PID values (Temp input, Heater output)
double temperature, pidOutput;
int steamON = 0;
int steamFirstON = 0;

PID bPID(&temperature, &pidOutput, &setpoint, aggKp, aggKi, aggKd, 1, DIRECT);

#include "brewHandler.h"

// Other variables
boolean emergencyStop = false;                // Emergency stop if temperature is too high
constexpr double EmergencyStopTemp = 145;     // Temp EmergencyStopTemp
float inX = 0, inY = 0, inOld = 0, inSum = 0; // used for filterPressureValue()
boolean setupDone = false;

// Water tank sensor
boolean waterTankFull = true;
Timer loopWaterTank(&checkWaterTank, 200); // Check water tank level every 200 ms
int waterTankCheckConsecutiveReads = 0;    // Counter for consecutive readings of water tank sensor
constexpr int waterTankCountsNeeded = 3;   // Number of same readings to change water tank sensing

// PID controller
unsigned long previousMillistemp; // initialisation at the end of init()

double setpointTemp;
double previousInput = 0;

// Embedded HTTP Server
#include "embeddedWebserver.h"

struct cmp_str {
        bool operator()(char const* a, char const* b) const {
            return strcmp(a, b) < 0;
        }
};

// MQTT
#include "mqtt.h"

std::map<const char*, const char*, cmp_str> mqttVars;
std::map<const char*, std::function<double()>, cmp_str> mqttSensors = {};

unsigned long lastTempEvent = 0;
unsigned long tempEventInterval = 1000;

Timer hassioDiscoveryTimer(&sendHASSIODiscoveryMsg, 300000);

bool mqtt_was_connected = false;

/**
 * @brief Get Wifi signal strength and set signalBars for display
 */
int getSignalStrength() {
    if (offlineMode == 1) return 0;

    long rssi;

    if (WiFi.status() == WL_CONNECTED) {
        rssi = WiFi.RSSI();
    }
    else {
        rssi = -100;
    }

    if (rssi >= -50) {
        return 4;
    }
    else if (rssi < -50 && rssi >= -65) {
        return 3;
    }
    else if (rssi < -65 && rssi >= -75) {
        return 2;
    }
    else if (rssi < -75 && rssi >= -80) {
        return 1;
    }

    return 0;
}

void displayMessage(String text1, String text2, String text3, String text4, String text5, String text6);
void displayLogo(String displaymessagetext, String displaymessagetext2);
bool shouldDisplayBrewTimer();
void u8g2_prepare();

#include "display/displayTemplateManager.h"

Timer printDisplayTimer(&DisplayTemplateManager::printScreen, 100);

#include "powerHandler.h"
#include "scaleHandler.h"
#include "steamHandler.h"

// Emergency stop if temp is too high
void testEmergencyStop() {
    if (temperature > EmergencyStopTemp && emergencyStop == false) {
        emergencyStop = true;
    }
    else if (temperature < (brewSetpoint + 5) && emergencyStop == true) {
        emergencyStop = false;
    }
}

/**
 * @brief Switch to offline mode if maxWifiReconnects were exceeded during boot
 */
void initOfflineMode() {
    if (config.getOledEnabled()) {
        displayMessage("", "", "", "", "Begin Fallback,", "No Wifi");
    }

    LOG(INFO, "Start offline mode with eeprom values, no wifi :(");
    offlineMode = 1;
}

/**
 * @brief Check if Wifi is connected, if not reconnect abort function if offline, or brew is running
 */
void checkWifi() {
    if (offlineMode == 1 || currBrewState > kBrewIdle) return;

    // There was no WIFI connection at boot -> connect and if it does not succeed, enter offline mode
    do {
        if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
            int statusTemp = WiFi.status();

            if (statusTemp != WL_CONNECTED) { // check WiFi connection status
                lastWifiConnectionAttempt = millis();
                wifiReconnects++;
                LOGF(INFO, "Attempting WIFI (re-)connection: %i", wifiReconnects);

                if (!setupDone) {
                    if (config.getOledEnabled()) {
                        displayMessage("", "", "", "", langstring_wifirecon, String(wifiReconnects));
                    }
                }

                wm.disconnect();
                wm.autoConnect();

                int count = 1;

                while (WiFi.status() != WL_CONNECTED && count <= 20) {
                    delay(100); // give WIFI some time to connect
                    count++;    // reconnect counter, maximum waiting time for reconnect = 20*100ms
                }
            }
        }

        yield(); // Prevent WDT trigger
    } while (!setupDone && wifiReconnects < maxWifiReconnects && WiFi.status() != WL_CONNECTED);

    if (wifiReconnects >= maxWifiReconnects && WiFi.status() != WL_CONNECTED) {
        // no wifi connection after trying connection, initiate offline mode
        initOfflineMode();
    }
    else {
        wifiReconnects = 0;
    }
}

char number2string_double[22];

char* number2string(const double in) {
    snprintf(number2string_double, sizeof(number2string_double), "%0.2f", in);

    return number2string_double;
}

char number2string_float[22];

char* number2string(const float in) {
    snprintf(number2string_float, sizeof(number2string_float), "%0.2f", in);

    return number2string_float;
}

char number2string_int[22];

char* number2string(const int in) {
    snprintf(number2string_int, sizeof(number2string_int), "%d", in);

    return number2string_int;
}

char number2string_uint[22];

char* number2string(const unsigned int in) {
    snprintf(number2string_uint, sizeof(number2string_uint), "%u", in);

    return number2string_uint;
}

/**
 * @brief Filter input value using exponential moving average filter (using fixed coefficients)
 *      After ~28 cycles the input is set to 99,66% if the real input value sum of inX and inY
 *      multiplier must be 1 increase inX multiplier to make the filter faster
 */
float filterPressureValue(const float input) {
    inX = static_cast<float>(input * 0.3);
    inY = static_cast<float>(inOld * 0.7);
    inSum = inX + inY;
    inOld = inSum;

    return inSum;
}

/**
 * @brief Handle the different states of the machine
 */
void handleMachineState() {
    switch (machineState) {
        case kInit:
            if (!waterTankFull) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }
            else {
                machineState = kPidNormal;
            }

            break;

        case kPidNormal:

            if (brew()) {
                machineState = kBrew;

                if (standbyModeOn) {
                    resetStandbyTimer(machineState);
                }
            }

            if (manualFlush()) {
                machineState = kManualFlush;

                if (standbyModeOn) {
                    resetStandbyTimer(machineState);
                }
            }

            if (backflushOn) {
                machineState = kBackflush;

                if (standbyModeOn) {
                    resetStandbyTimer(machineState);
                }
            }

            if (steamON == 1) {
                machineState = kSteam;

                if (standbyModeOn) {
                    resetStandbyTimer(machineState);
                }
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (standbyModeOn && standbyModeRemainingTimeMillis == 0) {
                machineState = kStandby;
                setRuntimePidState(false);
            }

            if (pidON == 0 && machineState != kStandby) {
                machineState = kPidDisabled;
            }

            if (!waterTankFull) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }

            break;

        case kBrew:

            if (!brew()) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kManualFlush:

            if (!manualFlush()) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kSteam:
            if (steamON == 0) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (!waterTankFull) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kBackflush:

            backflush();

            if (backflushOn == 0) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (!waterTankFull && (currBackflushState == kBackflushIdle || currBackflushState == kBackflushFinished)) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kEmergencyStop:
            if (!emergencyStop) {
                machineState = kPidNormal;
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kWaterTankEmpty:
            if (waterTankFull) {
                machineState = kPidNormal;

                if (standbyModeOn) {
                    resetStandbyTimer(machineState);
                }
            }

            if (pidON == 0) {
                machineState = kPidDisabled;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kPidDisabled:
            if (pidON == 1) {
                machineState = kPidNormal;
            }

            if (tempSensor->hasError()) {
                machineState = kSensorError;
            }

            break;

        case kStandby:
            if (standbyModeRemainingTimeDisplayOffMillis == 0 && config.getOledEnabled()) {
                u8g2->setPowerSave(1);
            }

            if (pidON) {
                machineState = kPidNormal;
                resetStandbyTimer(machineState);

                if (config.getOledEnabled()) {
                    u8g2->setPowerSave(0);
                }
            }
            if (steamON) {
                setRuntimePidState(true);
                machineState = kSteam;
                resetStandbyTimer(machineState);

                if (config.getOledEnabled()) {
                    u8g2->setPowerSave(0);
                }
            }

            if (brew()) {
                setRuntimePidState(true);
                machineState = kBrew;
                resetStandbyTimer(machineState);

                if (config.getOledEnabled()) {
                    u8g2->setPowerSave(0);
                }
            }

            if (manualFlush()) {
                setRuntimePidState(true);
                machineState = kManualFlush;
                resetStandbyTimer(machineState);

                if (config.getOledEnabled()) {
                    u8g2->setPowerSave(0);
                }
            }

            if (backflushOn) {
                machineState = kBackflush;
                resetStandbyTimer(machineState);

                if (config.getOledEnabled()) {
                    u8g2->setPowerSave(0);
                }
            }

            if (tempSensor->hasError()) {
                if (config.getOledEnabled()) {
                    u8g2->setPowerSave(0);
                }

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
    LOGF(DEBUG, "new machineState: %s -> %s", machinestateEnumToString(lastmachinestate), machinestateEnumToString(machineState));
}

char const* machinestateEnumToString(const MachineState machineState) {
    switch (machineState) {
        case kInit:
            return "Init";
        case kPidNormal:
            return "PID Normal";
        case kBrew:
            return "Brew";
        case kManualFlush:
            return "Manual Flush";
        case kSteam:
            return "Steam";
        case kBackflush:
            return "Backflush";
        case kWaterTankEmpty:
            return "Water Tank Empty";
        case kEmergencyStop:
            return "Emergency Stop";
        case kPidDisabled:
            return "PID Disabled";
        case kStandby:
            return "Standby Mode";
        case kSensorError:
            return "Sensor Error";
        case kEepromError:
            return "EEPROM Error";
    }

    return "Unknown";
}

/**
 * @brief Set up internal WiFi hardware
 */
void wiFiSetup() {
    wm.setCleanConnect(true);
    wm.setConfigPortalTimeout(60); // sec timeout for captive portal
    wm.setConnectTimeout(10);      // using 10s to connect to WLAN, 5s is sometimes too short!
    wm.setBreakAfterConfig(true);
    wm.setConnectRetries(3);

    if (wm.getWiFiIsSaved()) {
        LOGF(INFO, "Connecting to WiFi: %s", hostname.c_str());

        if (config.getOledEnabled()) {
            displayLogo("Connecting to: ", hostname.c_str());
        }
    }

    wm.setHostname(hostname.c_str());

    if (wm.autoConnect(hostname.c_str(), pass)) {

        if (!config.save()) {
            LOG(ERROR, "Failed to save config to filesystem!");
        }

        LOGF(INFO, "WiFi connected - IP = %i.%i.%i.%i", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);

        byte mac[6];
        WiFi.macAddress(mac);
        const String macaddr0 = number2string(mac[0]);
        const String macaddr1 = number2string(mac[1]);
        const String macaddr2 = number2string(mac[2]);
        const String macaddr3 = number2string(mac[3]);
        const String macaddr4 = number2string(mac[4]);
        const String macaddr5 = number2string(mac[5]);
        const String completemac = macaddr0 + macaddr1 + macaddr2 + macaddr3 + macaddr4 + macaddr5;

        LOGF(DEBUG, "MAC-ADDRESS: %s", completemac.c_str());
    }
    else {
        LOG(INFO, "WiFi connection timed out...");

        if (config.getOledEnabled()) {
            displayLogo(langstring_nowifi[0], langstring_nowifi[1]);
        }

        wm.disconnect();
        delay(1000);

        offlineMode = 1;
    }

    if (config.getOledEnabled()) {
        displayLogo(langstring_connectwifi1, wm.getWiFiSSID(true));
    }
}

void wiFiReset() {
    wm.resetSettings();

    if (!config.save()) {
        LOG(ERROR, "Failed to save config to filesystem!");
    }

    delay(500);
    ESP.restart();
}

extern const char sysVersion[] = (STR(FW_VERSION) "." STR(FW_SUBVERSION) "." STR(FW_HOTFIX) " " FW_BRANCH " " AUTO_VERSION);

void setup() {
    // Start serial console
    Serial.begin(115200);

    // Initialize the logger
    Logger::init(23);

    if (!config.begin()) {
        LOG(ERROR, "Failed to load config from filesystem!");
    }

    ParameterRegistry::getInstance().initialize(config);

    if (!ParameterRegistry::getInstance().isReady()) {
        LOG(ERROR, "Failed to initialize ParameterRegistry!");
        // TODO Error handling
    }
    else {
        ParameterRegistry::getInstance().syncGlobalVariables();
        hostname = config.getHostname();
    }

    Wire.begin();

    if (config.getOledEnabled()) {
        switch (config.getOledType()) {
            case 0:
                u8g2 = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, PIN_I2CSCL, PIN_I2CSDA);  // e.g. 1.3"
                break;
            case 1:
                u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, PIN_I2CSCL, PIN_I2CSDA); // e.g. 0.96"
                break;
            default:
                break;
        }

        if (u8g2 != nullptr) {
            // TODO This line doesn't work for some reason
            // u8g2->setI2CAddress(0x3C);
            u8g2->begin();
            u8g2->clearBuffer();

            u8g2_prepare();

            initLangStrings(config);

            const int templateId = config.getDisplayTemplate();
            DisplayTemplateManager::initializeDisplay(templateId);

            displayLogo(String("Version "), String(sysVersion));
        }
        else {
            LOG(ERROR, "Error initializing the display!");
            config.setOledEnabled(false);
        }
    }

    // Calculate derived values
    aggKi = aggTn > 0 ? aggKp / aggTn : 0;
    aggKd = aggTv * aggKp;
    aggbKi = aggbTn > 0 ? aggbKp / aggbTn : 0;
    aggbKd = aggbTv * aggbKp;

    setupMqtt();

    if (mqtt_enabled) {
        // Editable values reported to MQTT
        mqttVars["pidON"] = "pid.enabled";
        mqttVars["brewSetpoint"] = "brew.setpoint";
        mqttVars["brewTempOffset"] = "brew.temp_offset";
        mqttVars["steamON"] = "STEAM_MODE";
        mqttVars["steamSetpoint"] = "steam.setpoint";
        mqttVars["pidUsePonM"] = "pid.use_ponm";
        mqttVars["aggKp"] = "pid.regular.kp";
        mqttVars["aggTn"] = "pid.regular.tn";
        mqttVars["aggTv"] = "pid.regular.tv";
        mqttVars["aggIMax"] = "pid.regular.i_max";
        mqttVars["steamKp"] = "pid.steam.kp";
        mqttVars["standbyModeOn"] = "standby.enabled";
        mqttVars["aggbKp"] = "pid.bd.kp";
        mqttVars["aggbTn"] = "pid.bd.tn";
        mqttVars["aggbTv"] = "pid.bd.tv";

        // Values reported to MQTT
        mqttSensors["temperature"] = [] { return temperature; };
        mqttSensors["heaterPower"] = [] { return pidOutput / 10; };
        mqttSensors["standbyModeTimeRemaining"] = [] { return standbyModeRemainingTimeMillis / 1000; };
        mqttSensors["currentKp"] = [] { return bPID.GetKp(); };
        mqttSensors["currentKi"] = [] { return bPID.GetKi(); };
        mqttSensors["currentKd"] = [] { return bPID.GetKd(); };
        mqttSensors["machineState"] = [] { return machineState; };

        if (config.getBrewSwitchEnabled()) {
            mqttVars["pidUseBD"] = "pid.bd.enabled";
            mqttVars["brewPidDelay"] = "brew.pid_delay";
            mqttSensors["currBrewTime"] = [] { return currBrewTime / 1000; };
            mqttVars["targetBrewTime"] = "brew.target_time";
            mqttVars["preinfusion"] = "brew.pre_infusion.time";
            mqttVars["preinfusionPause"] = "brew.pre_infusion.pause";
            mqttVars["backflushOn"] = "BACKFLUSH_ON";
            mqttVars["backflushCycles"] = "backflush.cycles";
            mqttVars["backflushFillTime"] = "backflush.fill_time";
            mqttVars["backflushFlushTime"] = "backflush.flush_time";
        }
    }

    if (config.getScaleEnabled()) {
        mqttVars["targetBrewWeight"] = "brew.target_weight";
        mqttVars["scaleCalibration"] = "hardware.sensors.scale.calibration";

        if (config.getScaleType() == 0) {
            mqttVars["scale2Calibration"] = "hardware.sensors.scale.calibration2";
        }

        mqttVars["scaleKnownWeight"] = "hardware.sensors.scale.known_weight";
        mqttVars["scaleTareOn"] = "TARE_ON";
        mqttVars["scaleCalibrationOn"] = "CALIBRATION_ON";

        mqttSensors["currReadingWeight"] = [] { return currReadingWeight; };
        mqttSensors["currBrewWeight"] = [] { return currBrewWeight; };
    }

    if (config.getPressureSensorEnabled()) {
        mqttSensors["pressure"] = [] { return inputPressureFilter; };
    }

    initTimer1();

    const auto heaterTriggerType = static_cast<Relay::TriggerType>(config.getHeaterRelayTriggerType());
    heaterRelay = new Relay(heaterRelayPin, heaterTriggerType);
    heaterRelay->off();

    const auto valvePumpTriggerType = static_cast<Relay::TriggerType>(config.getPumpValveRelayTriggerType());
    valveRelay = new Relay(valveRelayPin, valvePumpTriggerType);
    valveRelay->off();
    pumpRelay = new Relay(pumpRelayPin, valvePumpTriggerType);
    pumpRelay->off();

    if (config.getPowerSwitchEnabled()) {
        const auto type = static_cast<Switch::Type>(config.getPowerSwitchType());
        const auto mode = static_cast<Switch::Mode>(config.getPowerSwitchMode());
        powerSwitch = new IOSwitch(PIN_POWERSWITCH, GPIOPin::IN_HARDWARE, type, mode);
    }

    if (config.getSteamSwitchEnabled()) {
        const auto type = static_cast<Switch::Type>(config.getSteamSwitchType());
        const auto mode = static_cast<Switch::Mode>(config.getSteamSwitchMode());
        steamSwitch = new IOSwitch(PIN_STEAMSWITCH, GPIOPin::IN_HARDWARE, type, mode);
    }

    if (config.getBrewSwitchEnabled()) {
        const auto type = static_cast<Switch::Type>(config.getBrewSwitchType());
        const auto mode = static_cast<Switch::Mode>(config.getBrewSwitchMode());
        brewSwitch = new IOSwitch(PIN_BREWSWITCH, GPIOPin::IN_HARDWARE, type, mode);
    }

    if (config.getStatusLedEnabled()) {
        const bool inverted = config.getStatusLedInverted();
        statusLedPin = new GPIOPin(PIN_STATUSLED, GPIOPin::OUT);
        statusLed = new StandardLED(*statusLedPin, inverted);
        statusLed->turnOff();
    }

    if (config.getBrewLedEnabled()) {
        const bool inverted = config.getBrewLedInverted();
        brewLedPin = new GPIOPin(PIN_BREWLED, GPIOPin::OUT);
        brewLed = new StandardLED(*brewLedPin, inverted);
        brewLed->turnOff();
    }
    if (config.getSteamLedEnabled()) {
        const bool inverted = config.getSteamLedInverted();
        steamLedPin = new GPIOPin(PIN_STEAMLED, GPIOPin::OUT);
        steamLed = new StandardLED(*steamLedPin, inverted);
        steamLed->turnOff();
    }

    if (config.getWaterTankSensorEnabled()) {
        const auto mode = static_cast<Switch::Mode>(config.getWaterTankSensorMode());
        waterTankSensor = new IOSwitch(PIN_WATERTANKSENSOR, (mode == Switch::NORMALLY_OPEN ? GPIOPin::IN_PULLDOWN : GPIOPin::IN_PULLUP), Switch::TOGGLE, mode);
    }

    // Fallback offline
    if (!config.getOfflineModeEnabled()) { // WiFi Mode
        wiFiSetup();
        serverSetup();

        // OTA Updates
        if (WiFi.status() == WL_CONNECTED) {
            otaPass = config.getOtaPass();
            ArduinoOTA.setHostname(hostname.c_str()); //  Device name for OTA
            ArduinoOTA.setPassword(otaPass.c_str());  //  Password for OTA
            ArduinoOTA.begin();
        }

        if (mqtt_enabled) {
            snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix.c_str(), hostname.c_str(), "status");
            snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix.c_str(), hostname.c_str(), "set");
            mqtt.setServer(mqtt_server_ip.c_str(), mqtt_server_port);
            mqtt.setCallback(mqtt_callback);

            checkMQTT();

            if (mqtt_hassio_enabled) {
                sendHASSIODiscoveryMsg();
            }
        }
    }
    else {
        wm.disconnect();
        offlineMode = 1;
        setRuntimePidState(true);
    }

    // Start the logger
    Logger::begin();
    int level = ParameterRegistry::getInstance().getParameterById("system.log_level")->getValueAs<int>();
    Logger::setLevel(static_cast<Logger::Level>(level));

    // Initialize PID controller
    bPID.SetSampleTime(windowSize);
    bPID.SetOutputLimits(0, windowSize);
    bPID.SetIntegratorLimits(0, AGGIMAX);
    bPID.SetSmoothingFactor(emaFactor);
    bPID.SetMode(AUTOMATIC);

    if (config.getTempSensorType() == 0) {
        tempSensor = new TempSensorTSIC(PIN_TEMPSENSOR);
    }
    else if (config.getTempSensorType() == 1) {
        tempSensor = new TempSensorDallas(PIN_TEMPSENSOR);
    }

    temperature = tempSensor->getCurrentTemperature();
    temperature -= brewTempOffset;

    // Initialisation MUST be at the very end of the init(), otherwise the
    // time comparision in loop() will have a big offset
    unsigned long currentTime = millis();
    previousMillistemp = currentTime;
    windowStartTime = currentTime;
    previousMillisMQTT = currentTime;
    lastMQTTConnectionAttempt = currentTime;

    // Init Scale
    if (config.getScaleEnabled()) {
        initScale();
        previousMillisScale = currentTime;
    }

    if (config.getPressureSensorEnabled()) {
        previousMillisPressure = currentTime;
    }

    setupDone = true;

    enableTimer1();

    double fsUsage = ((double)LittleFS.usedBytes() / LittleFS.totalBytes()) * 100;
    LOGF(INFO, "LittleFS: %d%% (used %ld bytes from %ld bytes)", (int)ceil(fsUsage), LittleFS.usedBytes(), LittleFS.totalBytes());
}

void loop() {
    // Accept potential connections for remote logging
    Logger::update();

    // Update water tank sensor
    loopWaterTank();

    // Update PID settings & machine state
    loopPid();

    // Update LED output based on machine state
    loopLED();

    // Handle automatic config save
    ParameterRegistry::getInstance().processPeriodicSave();
}

void loopPid() {
    // Only do Wifi stuff, if Wifi is connected
    if (WiFi.status() == WL_CONNECTED && offlineMode == 0) {
        if (mqtt_enabled) {
            checkMQTT();
            writeSysParamsToMQTT(true); // Continue on error

            if (mqtt.connected() == 1) {
                mqtt.loop();

                if (mqtt_hassio_enabled) {
                    hassioDiscoveryTimer();
                }

                mqtt_was_connected = true;
            }
            // Supress debug messages until we have a connection etablished
            else if (mqtt_was_connected) {
                LOG(INFO, "MQTT disconnected");
                mqtt_was_connected = false;
            }
        }

        ArduinoOTA.handle(); // For OTA

        // Disable interrupt if OTA is starting, otherwise it will not work
        ArduinoOTA.onStart([]() {
            disableTimer1();
            heaterRelay->off();
        });

        ArduinoOTA.onError([](ota_error_t error) { enableTimer1(); });

        // Enable interrupts if OTA is finished
        ArduinoOTA.onEnd([]() { enableTimer1(); });

        wifiReconnects = 0; // reset wifi reconnects if connected
    }
    else {
        checkWifi();
    }

    // Update the temperature:
    temperature = tempSensor->getCurrentTemperature();

    if (machineState != kSteam) {
        temperature -= brewTempOffset;
    }

    testEmergencyStop(); // test if temp is too high
    bPID.Compute();      // the variable pidOutput now has new values from PID (will be written to heater pin in ISR.cpp)

    if ((millis() - lastTempEvent) > tempEventInterval) {
        // send temperatures to website endpoint
        sendTempEvent(temperature, brewSetpoint, pidOutput / 10); // pidOutput is promill, so /10 to get percent value
        lastTempEvent = millis();

        if (pidON) {
            LOGF(TRACE, "Current PID mode: %s", bPID.GetPonE() ? "PonE" : "PonM");

            // P-Part
            LOGF(TRACE, "Current PID input error: %f", bPID.GetInputError());
            LOGF(TRACE, "Current PID P part: %f", bPID.GetLastPPart());
            LOGF(TRACE, "Current PID kP: %f", bPID.GetKp());
            // I-Part
            LOGF(TRACE, "Current PID I sum: %f", bPID.GetLastIPart());
            LOGF(TRACE, "Current PID kI: %f", bPID.GetKi());
            // D-Part
            LOGF(TRACE, "Current PID diff'd input: %f", bPID.GetDeltaInput());
            LOGF(TRACE, "Current PID D part: %f", bPID.GetLastDPart());
            LOGF(TRACE, "Current PID kD: %f", bPID.GetKd());
            // Combined PID output
            LOGF(TRACE, "Current PID Output: %f", pidOutput);
            LOGF(TRACE, "Current Machinestate: %s", machinestateEnumToString(machineState));
            // Brew
            LOGF(TRACE, "currBrewTime %f", currBrewTime);
            LOGF(TRACE, "Brew detected %i", brew());
            LOGF(TRACE, "brewPIDdisabled %i", brewPIDDisabled);
        }
    }

    if (config.getScaleEnabled()) {
        checkWeight();    // Check Weight Scale in the loop
        shotTimerScale(); // Calculation of weight of shot while brew is running
    }

    brew();
    manualFlush();

    if (config.getPressureSensorEnabled()) {
        unsigned long currentMillisPressure = millis();

        if (currentMillisPressure - previousMillisPressure >= intervalPressure) {
            previousMillisPressure = currentMillisPressure;
            inputPressure = measurePressure();
            inputPressureFilter = filterPressureValue(inputPressure);
        }
    }

    checkSteamSwitch();
    checkPowerSwitch();

    // set setpoint depending on steam or brew mode
    if (steamON == 1) {
        setpoint = steamSetpoint;
    }
    else if (steamON == 0) {
        setpoint = brewSetpoint;
    }

    updateStandbyTimer();
    handleMachineState();

    if (config.getBrewSwitchEnabled()) {
        shouldDisplayBrewTimer();
    }

    // Check if PID should run or not. If not, set to manual and force output to zero
    if (config.getOledEnabled()) {
        printDisplayTimer();
    }

    if (machineState == kPidDisabled || machineState == kWaterTankEmpty || machineState == kSensorError || machineState == kEmergencyStop || machineState == kEepromError || machineState == kStandby ||
        machineState == kBackflush || brewPIDDisabled) {
        if (bPID.GetMode() == 1) {
            // Force PID shutdown
            bPID.SetMode(0);
            pidOutput = 0;
            heaterRelay->off();
        }
    }
    else { // no sensorerror, no pid off or no Emergency Stop
        if (bPID.GetMode() == 0) {
            bPID.SetMode(1);
        }
    }

    // Regular PID operation
    if (machineState == kPidNormal) {
        setPIDTunings(usePonM);
    }

    // Brew PID
    if (machineState == kBrew) {
        if (brewPIDDelay > 0 && currBrewTime > 0 && currBrewTime < brewPIDDelay * 1000) {
            // disable PID for brewPIDDelay seconds, enable PID again with new tunings after that
            if (!brewPIDDisabled) {
                brewPIDDisabled = true;
                bPID.SetMode(MANUAL);
                pidOutput = 0;
                heaterRelay->off();
                LOGF(DEBUG, "disabled PID, waiting for %.0f seconds before enabling PID again", brewPIDDelay);
            }
        }
        else {
            if (brewPIDDisabled) {
                // enable PID again
                bPID.SetMode(AUTOMATIC);
                brewPIDDisabled = false;
                LOG(DEBUG, "Enabled PID again after delay");
            }

            if (useBDPID) {
                setBDPIDTunings();
            }
            else {
                setPIDTunings(usePonM);
            }
        }
    }
    // Reset brewPIDdisabled if brew was aborted
    if (machineState != kBrew && brewPIDDisabled) {
        // enable PID again
        bPID.SetMode(AUTOMATIC);
        brewPIDDisabled = false;
        LOG(DEBUG, "Enabled PID again after brew was manually stopped");
    }

    // Steam on
    if (machineState == kSteam) {
        if (lastmachinestatepid != machineState) {
            LOGF(DEBUG, "new PID-Values: P=%.1f  I=%.1f  D=%.1f", 150.0, 0.0, 0.0);
            lastmachinestatepid = machineState;
        }

        bPID.SetTunings(steamKp, 0, 0, 1);
    }
}

void loopLED() {
    if (config.getStatusLedEnabled() && statusLed != nullptr) {
        if ((machineState == kPidNormal && (fabs(temperature - setpoint) < 0.3)) || (temperature > 115 && fabs(temperature - setpoint) < 5)) {
            statusLed->turnOn();
        }
        else {
            statusLed->turnOff();
        }
    }

    if (config.getBrewLedEnabled() && brewLed != nullptr) {
        brewLed->setGPIOState(machineState == kBrew);
    }

    if (config.getSteamLedEnabled() && steamLed != nullptr) {
        steamLed->setGPIOState(machineState == kSteam);
    }
}

void checkWaterTank() {
    if (!config.getWaterTankSensorEnabled()) {
        return;
    }

    if (bool isWaterDetected = waterTankSensor->isPressed(); isWaterDetected && !waterTankFull) {
        waterTankFull = true;
        LOG(INFO, "Water tank full");
    }
    else if (!isWaterDetected && waterTankFull) {
        waterTankFull = false;
        LOG(WARNING, "Water tank empty");
    }
}

void setBackflush(const int backflush) {
    backflushOn = backflush;
}

void setScaleCalibration(int calibration) {
    scaleCalibrationOn = calibration;
}

void setScaleTare(int tare) {
    scaleTareOn = tare;
}

void setRuntimePidState(const bool enabled) {
    pidON = enabled ? 1 : 0;
    config.setPidEnabled(enabled);
}

void setSteamMode(const int steamMode) {
    steamON = steamMode;

    if (steamON == 1) {
        steamFirstON = 1;
    }

    if (steamON == 0) {
        steamFirstON = 0;
    }
}

void setPIDTunings(const bool usePonM) {
    // Prevent overwriting of brewdetection values
    // calc ki, kd
    if (aggTn != 0) {
        aggKi = aggKp / aggTn;
    }
    else {
        aggKi = 0;
    }

    aggKd = aggTv * aggKp;

    bPID.SetIntegratorLimits(0, aggIMax);

    if (lastmachinestatepid != machineState) {
        LOGF(DEBUG, "new PID-Values: P=%.1f  I=%.1f  D=%.1f", aggKp, aggKi, aggKd);
        lastmachinestatepid = machineState;
    }

    if (usePonM) {
        bPID.SetTunings(aggbKp, aggbKi, aggbKd, P_ON_M);
    }
    else {
        bPID.SetTunings(aggKp, aggKi, aggKd, 1);
    }
}

void setBDPIDTunings() {
    // calc ki, kd
    if (aggbTn != 0) {
        aggbKi = aggbKp / aggbTn;
    }
    else {
        aggbKi = 0;
    }

    aggbKd = aggbTv * aggbKp;

    if (lastmachinestatepid != machineState) {
        LOGF(DEBUG, "new PID-Values: P=%.1f  I=%.1f  D=%.1f", aggbKp, aggbKi, aggbKd);
        lastmachinestatepid = machineState;
    }

    bPID.SetTunings(aggbKp, aggbKi, aggbKd, 1);
}
