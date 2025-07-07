/**
 * @file main.cpp
 *
 * @brief Main sketch
 *
 * @version 4.0.0 Master
 */

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
    kHotWater = 40,
    kSteam = 50,
    kBackflush = 60,
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

bool offlineMode = false;
int displayOffline = 0;

inline bool systemInitialized = false;

// Display
U8G2* u8g2 = nullptr;

bool featureFullscreenBrewTimer = false;
bool featureFullscreenManualFlushTimer = false;
bool featureFullscreenHotWaterTimer = false;
double postBrewTimerDuration = POST_BREW_TIMER_DURATION;
bool featureHeatingLogo = false;
bool featurePidOffLogo = false;

// WiFi
WiFiManager wm;
constexpr unsigned long wifiConnectionDelay = WIFICONNECTIONDELAY;
constexpr unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;
String hostname = "silvia";
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

// timing flags
bool timingDebugActive = false;
bool includeDisplayInLogs = false;
bool displayBufferReady = false;
bool displayUpdateRunning = false;
bool websiteUpdateRunning = false;
bool mqttUpdateRunning = false;
bool hassioUpdateRunning = false;
bool temperatureUpdateRunning = false;

#include "utils/timingDebug.h"

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
Switch* hotWaterSwitch = nullptr;

TempSensor* tempSensor = nullptr;

#include "isr.h"

// Method forward declarations
void setSteamMode(bool steamMode);
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

// debugging water pump actions
String hotWaterStateDebug = "off";
String lastHotWaterStateDebug = "off";

// system parameters
bool pidON = false;
bool usePonM = false;
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
float scale2Calibration = SCALE2_CALIBRATION_FACTOR;
float scaleKnownWeight = SCALE_KNOWN_WEIGHT;
double targetBrewWeight = TARGET_BREW_WEIGHT;

// PID - values for offline brew detection
bool useBDPID = false;
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;
double aggbKi = (aggbTn == 0) ? 0 : aggbKp / aggbTn;
double aggbKd = aggbTv * aggbKp;
double aggKi = (aggTn == 0) ? 0 : aggKp / aggTn;
double aggKd = aggTv * aggKp;

double brewPidDelay = BREW_PID_DELAY; // Time PID will be disabled after brew started

bool standbyModeOn = false;
double standbyModeTime = STANDBY_MODE_TIME;

#include "standby.h"

// Variables to hold PID values (Temp input, Heater output)
double temperature, pidOutput;
bool steamON = false;
bool steamFirstON = false;

PID bPID(&temperature, &pidOutput, &setpoint, aggKp, aggKi, aggKd, 1, DIRECT);

#include "brewHandler.h"
#include "hotWaterHandler.h"

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
    if (offlineMode) return 0;

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

void displayMessage(const String& text1, const String& text2, const String& text3, const String& text4, const String& text5, const String& text6);
void displayLogo(const String& displaymessagetext, const String& displaymessagetext2);
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
    if (config.get<bool>("hardware.oled.enabled")) {
        displayOffline = 1;
    }

    LOG(INFO, "Start offline mode with eeprom values, no wifi :(");
    offlineMode = true;
}

/**
 * @brief Check if Wifi is connected, if not reconnect abort function if offline, or brew is running
 */
void checkWifi() {
    static int wifiConnectCounter = 1;
    static bool wifiConnectedHandled = false;
    if (offlineMode || checkBrewActive()) return;

    // Try to connect and if it does not succeed, enter offline mode
    if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
        if (WiFi.status() != WL_CONNECTED) { // check WiFi connection status
            wifiConnectedHandled = false;

            if (wifiConnectCounter == 1) {
                wifiReconnects++;
                LOGF(INFO, "Attempting WIFI (re-)connection: %i", wifiReconnects);
                wm.disconnect();
                WiFi.begin();
            }

            delay(20);                // give WIFI some time to connect

            if (WiFi.status() != WL_CONNECTED && wifiConnectCounter < 100) {
                wifiConnectCounter++; // reconnect counter, maximum waiting time for reconnect = 20*100ms plus loop times
            }
            else {
                if (wifiConnectCounter == 100) {
                    LOGF(INFO, "Wifi Reconnection failed - %i loops", wifiConnectCounter);
                    lastWifiConnectionAttempt = millis();
                    wifiConnectCounter = 1;
                }
            }
        }
        else {
            if (wifiConnectedHandled == false) {
                LOGF(INFO, "Wifi Reconnected - %i loops", wifiConnectCounter);
                wifiConnectedHandled = true;
                wifiConnectCounter = 1;
            }
        }
    }

    if (wifiReconnects >= maxWifiReconnects && WiFi.status() != WL_CONNECTED) {
        // no wifi connection after trying connection, initiate offline mode
        initOfflineMode();
    }
    else {
        if (WiFi.status() == WL_CONNECTED) {
            wifiReconnects = 0;
        }
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

            if (tempSensor != nullptr && tempSensor->hasError()) {
                machineState = kSensorError;
            }

            if (!pidON) {
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

            if (steamON) {
                machineState = kSteam;

                if (standbyModeOn) {
                    resetStandbyTimer(machineState);
                }
            }

            if (checkHotWaterStates()) {
                machineState = kHotWater;

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

            if (!pidON && machineState != kStandby) {
                machineState = kPidDisabled;
            }

            if (!waterTankFull) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor != nullptr && tempSensor->hasError()) {
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

            if (!pidON) {
                machineState = kPidDisabled;
            }

            if (tempSensor != nullptr && tempSensor->hasError()) {
                machineState = kSensorError;
            }

            if (machineState != kBrew) {
                MQTTReCnctCount = 0; // allow MQTT to try to reconnect if exiting brew mode
            }

            break;

        case kManualFlush:
            if (!manualFlush()) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (!pidON) {
                machineState = kPidDisabled;
            }

            if (tempSensor != nullptr && tempSensor->hasError()) {
                machineState = kSensorError;
            }
            break;

        case kHotWater:
            if (!checkHotWaterStates()) {
                machineState = kPidNormal;
            }

            if (steamON) {
                machineState = kSteam;

                if (standbyModeOn) {
                    resetStandbyTimer(machineState);
                }
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (!pidON) {
                machineState = kPidDisabled;
            }

            if (tempSensor != nullptr && tempSensor->hasError()) {
                machineState = kSensorError;
            }

            break;

        case kSteam:
            if (!steamON) {
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

        case kBackflush:
            backflush();

            if (!backflushOn) {
                machineState = kPidNormal;
            }

            if (emergencyStop) {
                machineState = kEmergencyStop;
            }

            if (!pidON) {
                machineState = kPidDisabled;
            }

            if (!waterTankFull && (currBackflushState == kBackflushIdle || currBackflushState == kBackflushFinished)) {
                machineState = kWaterTankEmpty;
            }

            if (tempSensor != nullptr && tempSensor->hasError()) {
                machineState = kSensorError;
            }

            break;

        case kEmergencyStop:
            if (!emergencyStop) {
                machineState = kPidNormal;
            }

            if (!pidON) {
                machineState = kPidDisabled;
            }

            if (tempSensor != nullptr && tempSensor->hasError()) {
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

            if (!pidON) {
                machineState = kPidDisabled;
            }

            if (tempSensor != nullptr && tempSensor->hasError()) {
                machineState = kSensorError;
            }

            break;

        case kPidDisabled:
            if (pidON) {
                machineState = kPidNormal;
            }

            if (tempSensor != nullptr && tempSensor->hasError()) {
                machineState = kSensorError;
            }

            break;

        case kStandby:
            {
                bool oledEnabled = config.get<bool>("hardware.oled.enabled");

                if (standbyModeRemainingTimeDisplayOffMillis == 0 && oledEnabled) {
                    u8g2->setPowerSave(1);
                }

                if (pidON) {
                    machineState = kPidNormal;
                    resetStandbyTimer(machineState);

                    if (oledEnabled) {
                        u8g2->setPowerSave(0);
                    }
                }
                if (steamON) {
                    setRuntimePidState(true);
                    machineState = kSteam;
                    resetStandbyTimer(machineState);

                    if (oledEnabled) {
                        u8g2->setPowerSave(0);
                    }
                }

                if (checkHotWaterStates()) {
                    setRuntimePidState(true);
                    machineState = kHotWater;
                    resetStandbyTimer(machineState);

                    if (oledEnabled) {
                        u8g2->setPowerSave(0);
                    }
                }

                if (brew()) {
                    setRuntimePidState(true);
                    machineState = kBrew;
                    resetStandbyTimer(machineState);

                    if (oledEnabled) {
                        u8g2->setPowerSave(0);
                    }
                }

                if (manualFlush()) {
                    setRuntimePidState(true);
                    machineState = kManualFlush;
                    resetStandbyTimer(machineState);

                    if (oledEnabled) {
                        u8g2->setPowerSave(0);
                    }
                }

                if (backflushOn) {
                    machineState = kBackflush;
                    resetStandbyTimer(machineState);

                    if (oledEnabled) {
                        u8g2->setPowerSave(0);
                    }
                }

                if (tempSensor != nullptr && tempSensor->hasError()) {
                    if (oledEnabled) {
                        u8g2->setPowerSave(0);
                    }

                    machineState = kSensorError;
                }

                if (machineState != kStandby) {
                    MQTTReCnctCount = 0; // allow MQTT to try to reconnect if exiting standby
                }

                break;
            }

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
        case kHotWater:
            return "Hot Water";
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

    bool oledEnabled = config.get<bool>("hardware.oled.enabled");

    if (wm.getWiFiIsSaved()) {
        LOG(INFO, "Connecting to WiFi");
    }
    wm.setHostname(hostname.c_str());
    wm.setEnableConfigPortal(false); // doesnt start config portal within autoconnect
    wm.setDisableConfigPortal(true); // disables config portal on wifi save
    bool wifiConnected = wm.autoConnect(hostname.c_str(), pass);
    if (!wifiConnected) {
        if (oledEnabled) {
            displayLogo("Starting Portal AP", hostname.c_str());
        }
        wifiConnected = wm.startConfigPortal(hostname.c_str(), pass);
    }
    if (wifiConnected) {
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
        if (oledEnabled) {
            displayLogo(langstring_connectwifi1, wm.getWiFiSSID(true));
        }
    }
    else {
        LOG(INFO, "WiFi connection timed out...");

        if (oledEnabled) {
            displayLogo(langstring_nowifi[0], langstring_nowifi[1]);
        }

        wm.disconnect();
        delay(1000);

        offlineMode = true;
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

extern const char sysVersion[] = STR(AUTO_VERSION);

void setup() {
    // Start serial console
    Serial.begin(115200);

    // Initialize the logger
    Logger::init(23);

    if (!config.begin()) {
        LOG(ERROR, "Failed to load config from filesystem!");
    }

    hostname = config.get<String>("system.hostname");

    ParameterRegistry::getInstance().initialize(config);

    if (!ParameterRegistry::getInstance().isReady()) {
        LOG(ERROR, "Failed to initialize ParameterRegistry!");
        // TODO Error handling
    }
    else {
        ParameterRegistry::getInstance().syncGlobalVariables();
    }

    Wire.begin();

    if (config.get<bool>("hardware.oled.enabled")) {
        switch (config.get<int>("hardware.oled.type")) {
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
            if (const int i2cAddress = config.get<int>("hardware.oled.i2c_address"); i2cAddress == 0) {
                u8g2->setI2CAddress(0x3C * 2);
            }
            else {
                u8g2->setI2CAddress(0x3D * 2);
            }

            u8g2->begin();
            u8g2->clearBuffer();

            u8g2_prepare();

            initLangStrings(config);

            const int templateId = config.get<int>("display.template");
            DisplayTemplateManager::initializeDisplay(templateId);

            displayLogo(String("Version "), String(sysVersion));
        }
        else {
            LOG(ERROR, "Error initializing the display!");
            config.set<bool>("hardware.oled.enabled", false);
        }
    }

    // Calculate derived values
    aggKi = aggTn > 0 ? aggKp / aggTn : 0;
    aggKd = aggTv * aggKp;
    aggbKi = aggbTn > 0 ? aggbKp / aggbTn : 0;
    aggbKd = aggbTv * aggbKp;

    initTimer1();

    const auto heaterTriggerType = static_cast<Relay::TriggerType>(config.get<int>("hardware.relays.heater.trigger_type"));
    heaterRelay = new Relay(heaterRelayPin, heaterTriggerType);
    heaterRelay->off();

    const auto valveTriggerType = static_cast<Relay::TriggerType>(config.get<int>("hardware.relays.valve.trigger_type"));
    valveRelay = new Relay(valveRelayPin, valveTriggerType);
    valveRelay->off();

    const auto pumpTriggerType = static_cast<Relay::TriggerType>(config.get<int>("hardware.relays.pump.trigger_type"));
    pumpRelay = new Relay(pumpRelayPin, pumpTriggerType);
    pumpRelay->off();

    if (config.get<bool>("hardware.switches.power.enabled")) {
        const auto type = static_cast<Switch::Type>(config.get<int>("hardware.switches.power.type"));
        const auto mode = static_cast<Switch::Mode>(config.get<int>("hardware.switches.power.mode"));
        powerSwitch = new IOSwitch(PIN_POWERSWITCH, GPIOPin::IN_HARDWARE, type, mode, mode);
    }

    if (config.get<bool>("hardware.switches.steam.enabled")) {
        const auto type = static_cast<Switch::Type>(config.get<int>("hardware.switches.steam.type"));
        const auto mode = static_cast<Switch::Mode>(config.get<int>("hardware.switches.steam.mode"));
        steamSwitch = new IOSwitch(PIN_STEAMSWITCH, GPIOPin::IN_HARDWARE, type, mode, mode);
    }

    if (config.get<bool>("hardware.switches.brew.enabled")) {
        const auto type = static_cast<Switch::Type>(config.get<int>("hardware.switches.brew.type"));
        const auto mode = static_cast<Switch::Mode>(config.get<int>("hardware.switches.brew.mode"));
        brewSwitch = new IOSwitch(PIN_BREWSWITCH, GPIOPin::IN_HARDWARE, type, mode, mode);
    }

    if (config.get<bool>("hardware.switches.hot_water.enabled")) {
        const auto type = static_cast<Switch::Type>(config.get<int>("hardware.switches.hot_water.type"));
        const auto mode = static_cast<Switch::Mode>(config.get<int>("hardware.switches.hot_water.mode"));
        hotWaterSwitch = new IOSwitch(PIN_WATERSWITCH, GPIOPin::IN_HARDWARE, type, mode, mode);
    }

    if (config.get<bool>("hardware.leds.status.enabled")) {
        const bool inverted = config.get<bool>("hardware.leds.status.inverted");
        statusLedPin = new GPIOPin(PIN_STATUSLED, GPIOPin::OUT);
        statusLed = new StandardLED(*statusLedPin, inverted);
        statusLed->turnOff();
    }

    if (config.get<bool>("hardware.leds.brew.enabled")) {
        const bool inverted = config.get<bool>("hardware.leds.brew.inverted");
        brewLedPin = new GPIOPin(PIN_BREWLED, GPIOPin::OUT);
        brewLed = new StandardLED(*brewLedPin, inverted);
        brewLed->turnOff();
    }
    if (config.get<bool>("hardware.leds.steam.enabled")) {
        const bool inverted = config.get<bool>("hardware.leds.steam.inverted");
        steamLedPin = new GPIOPin(PIN_STEAMLED, GPIOPin::OUT);
        steamLed = new StandardLED(*steamLedPin, inverted);
        steamLed->turnOff();
    }

    if (config.get<bool>("hardware.sensors.watertank.enabled")) {
        const auto mode = static_cast<Switch::Mode>(config.get<int>("hardware.sensors.watertank.mode"));
        waterTankSensor = new IOSwitch(PIN_WATERTANKSENSOR, (mode == Switch::NORMALLY_OPEN ? GPIOPin::IN_PULLDOWN : GPIOPin::IN_PULLUP), Switch::TOGGLE, mode, mode);
    }

    if (!config.get<bool>("system.offline_mode")) { // WiFi Mode
        wiFiSetup();
        serverSetup();

        // OTA Updates
        if (WiFi.status() == WL_CONNECTED) {
            otaPass = config.get<String>("system.ota_password");
            ArduinoOTA.setHostname(hostname.c_str()); //  Device name for OTA
            ArduinoOTA.setPassword(otaPass.c_str());  //  Password for OTA
            ArduinoOTA.begin();
        }

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

            // Values reported to MQTT
            mqttSensors["temperature"] = [] { return temperature; };
            mqttSensors["heaterPower"] = [] { return pidOutput / 10; };
            mqttSensors["standbyModeTimeRemaining"] = [] { return standbyModeRemainingTimeMillis / 1000; };
            mqttSensors["currentKp"] = [] { return bPID.GetKp(); };
            mqttSensors["currentKi"] = [] { return bPID.GetKi(); };
            mqttSensors["currentKd"] = [] { return bPID.GetKd(); };
            mqttSensors["machineState"] = [] { return machineState; };

            if (config.get<bool>("hardware.switches.brew.enabled")) {
                mqttVars["aggbKp"] = "pid.bd.kp";
                mqttVars["aggbTn"] = "pid.bd.tn";
                mqttVars["aggbTv"] = "pid.bd.tv";
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

            if (config.get<bool>("hardware.sensors.scale.enabled")) {
                mqttVars["targetBrewWeight"] = "brew.target_weight";
                mqttVars["scaleCalibration"] = "hardware.sensors.scale.calibration";

                if (config.get<int>("hardware.sensors.scale.type") == 0) {
                    mqttVars["scale2Calibration"] = "hardware.sensors.scale.calibration2";
                }

                mqttVars["scaleKnownWeight"] = "hardware.sensors.scale.known_weight";
                mqttVars["scaleTareOn"] = "TARE_ON";
                mqttVars["scaleCalibrationOn"] = "CALIBRATION_ON";

                mqttSensors["currReadingWeight"] = [] { return currReadingWeight; };
                mqttSensors["currBrewWeight"] = [] { return currBrewWeight; };
            }

            if (config.get<bool>("hardware.sensors.pressure.enabled")) {
                mqttSensors["pressure"] = [] { return inputPressureFilter; };
            }

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
        offlineMode = true;
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

    const int tempSensorType = config.get<int>("hardware.sensors.temperature.type");

    if (tempSensorType == 0) {
        tempSensor = new TempSensorTSIC(PIN_TEMPSENSOR);
    }
    else if (tempSensorType == 1) {
        tempSensor = new TempSensorDallas(PIN_TEMPSENSOR);
    }

    if (tempSensor != nullptr) {
        temperature = tempSensor->getCurrentTemperature();
        temperature -= brewTempOffset;
    }

    // Initialisation MUST be at the very end of the init(), otherwise the
    // time comparision in loop() will have a big offset
    unsigned long currentTime = millis();
    previousMillistemp = currentTime;
    windowStartTime = currentTime;
    previousMillisMQTT = currentTime;
    lastMQTTConnectionAttempt = currentTime;

    // Init Scale
    if (config.get<bool>("hardware.sensors.scale.enabled")) {
        initScale();
    }

    if (config.get<bool>("hardware.sensors.pressure.enabled")) {
        previousMillisPressure = currentTime;
    }

    setupDone = true;

    enableTimer1();

    double fsUsage = ((double)LittleFS.usedBytes() / LittleFS.totalBytes()) * 100;
    LOGF(INFO, "LittleFS: %d%% (used %ld bytes from %ld bytes)", (int)ceil(fsUsage), LittleFS.usedBytes(), LittleFS.totalBytes());

    systemInitialized = true;

    // For momentary switches, start in normal operation mode
    if (config.get<bool>("hardware.switches.power.enabled") && config.get<int>("hardware.switches.power.type") == Switch::MOMENTARY) {
        machineState = kPidNormal;
        setRuntimePidState(true);
    }
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

    // print timing related data to check what is causing stutters
    debugTimingLoop();

    // Handle automatic config save
    ParameterRegistry::getInstance().processPeriodicSave();
}

void loopPid() {

    // Update the temperature:
    temperatureUpdateRunning = false;

    if (tempSensor != nullptr) {
        temperature = tempSensor->getCurrentTemperature();

        if (machineState != kSteam) {
            temperature -= brewTempOffset;
        }
    }

    static bool wifiWasConnected = false;

    // Only do Wifi stuff, if Wifi is connected
    if (WiFi.status() == WL_CONNECTED && !offlineMode) {

        if (wifiWasConnected == false) {
            LOG(INFO, "WiFi Connected");
            wifiWasConnected = true;
        }

        if (mqtt_enabled) {
            mqttUpdateRunning = false;

            if (getSignalStrength() > 1) {
                checkMQTT();

                // if screen is ready to refresh wait for next loop
                if (!displayBufferReady && !temperatureUpdateRunning) {
                    writeSysParamsToMQTT(true); // Continue on error
                }
            }

            hassioUpdateRunning = false;

            if (mqtt.connected() == 1) {
                mqtt.loop();
                previousMqttConnection = millis();

                if (mqtt_hassio_enabled) {
                    // resend discovery messages if not during a main function and MQTT has been disconnected but has now reconnected
                    // this could mean mqtt_was_connected stays false for up to 5 mins but mqtt retains old HASSIO messages
                    if (!(machineState >= kBrew && machineState <= kBackflush) && (!mqtt_was_connected && !displayBufferReady && !temperatureUpdateRunning)) {
                        hassioDiscoveryTimer();
                    }
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
        wifiWasConnected = false;
        checkWifi();
    }

    testEmergencyStop(); // test if temp is too high
    bPID.Compute();      // the variable pidOutput now has new values from PID (will be written to heater pin in ISR.cpp)

    websiteUpdateRunning = false;

    // refresh website if loop does not have anoth long running process already
    if (((millis() - lastTempEvent) > tempEventInterval) && (!mqttUpdateRunning && !hassioUpdateRunning && !displayBufferReady && !temperatureUpdateRunning)) {
        websiteUpdateRunning = true;

        // send temperatures to website endpoint
        if (WiFi.status() == WL_CONNECTED && !offlineMode) {
            sendTempEvent(temperature, brewSetpoint, pidOutput / 10); // pidOutput is promill, so /10 to get percent value
        }

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
            LOGF(TRACE, "Brew detected %i", checkBrewActive());
            LOGF(TRACE, "brewPidDisabled %i", brewPidDisabled);
        }
    }

    if (config.get<bool>("hardware.sensors.scale.enabled")) {
        checkWeight();    // Check Weight Scale in the loop
        shotTimerScale(); // Calculation of weight of shot while brew is running
    }

    if (config.get<bool>("hardware.sensors.pressure.enabled")) {
        if (const unsigned long currentMillisPressure = millis(); currentMillisPressure - previousMillisPressure >= intervalPressure) {
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
    hotWaterHandler();
    valveSafetyShutdownCheck();

    if (config.get<bool>("hardware.switches.brew.enabled")) {
        shouldDisplayBrewTimer();
    }

    displayUpdateRunning = false;

    if (config.get<bool>("hardware.oled.enabled")) {

        // update display on loops that have not had other major tasks running, if blocked it will send in the next loop (average 0.5ms)
        if (!websiteUpdateRunning && !mqttUpdateRunning && !hassioUpdateRunning && !temperatureUpdateRunning && (standbyModeRemainingTimeDisplayOffMillis > 0)) {

            // displayUpdateRunning currently doesn't block anything as it is near the end of the loop, but if this code block moves it can be used to block other processes
            // sendBuffer() takes around 35ms so it flags that it has happened
            if (displayBufferReady) {
                u8g2->sendBuffer();
                displayBufferReady = false;
                displayUpdateRunning = true;
            }
            else {
                printDisplayTimer();
            }
        }
    }

    // Check if PID should run or not. If not, set to manual and force output to zero
    if (machineState == kPidDisabled || machineState == kWaterTankEmpty || machineState == kSensorError || machineState == kEmergencyStop || machineState == kEepromError || machineState == kStandby ||
        machineState == kBackflush || brewPidDisabled) {
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
        if (brewPidDelay > 0 && currBrewTime > 0 && currBrewTime < brewPidDelay * 1000) {
            // disable PID for brewPidDelay seconds, enable PID again with new tunings after that
            if (!brewPidDisabled) {
                brewPidDisabled = true;
                bPID.SetMode(MANUAL);
                pidOutput = 0;
                heaterRelay->off();
                LOGF(DEBUG, "disabled PID, waiting for %.0f seconds before enabling PID again", brewPidDelay);
            }
        }
        else {
            if (brewPidDisabled) {
                // enable PID again
                bPID.SetMode(AUTOMATIC);
                brewPidDisabled = false;
                LOGF(DEBUG, "Enabled PID again after %.0f seconds of brew pid delay", brewPidDelay);
            }

            if (useBDPID) {
                setBDPIDTunings();
            }
            else {
                setPIDTunings(usePonM);
            }
        }
    }
    // Reset brewPidDisabled if brew was aborted
    if (machineState != kBrew && brewPidDisabled) {
        // enable PID again
        bPID.SetMode(AUTOMATIC);
        brewPidDisabled = false;
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
    if (config.get<bool>("hardware.leds.status.enabled") && statusLed != nullptr) {
        if ((machineState == kPidNormal && (fabs(temperature - setpoint) < 0.3)) || (temperature > 115 && fabs(temperature - setpoint) < 5)) {
            statusLed->turnOn();
        }
        else {
            statusLed->turnOff();
        }
    }

    if (config.get<bool>("hardware.leds.brew.enabled") && brewLed != nullptr) {
        brewLed->setGPIOState(machineState == kBrew);
    }

    if (config.get<bool>("hardware.leds.steam.enabled") && steamLed != nullptr) {
        steamLed->setGPIOState(machineState == kSteam);
    }
}

void checkWaterTank() {
    if (!config.get<bool>("hardware.sensors.watertank.enabled") || waterTankSensor == nullptr) {
        return;
    }

    if (const bool isWaterDetected = waterTankSensor->isPressed(); isWaterDetected && !waterTankFull) {
        waterTankFull = true;
        LOG(INFO, "Water tank full");
    }
    else if (!isWaterDetected && waterTankFull) {
        waterTankFull = false;
        LOG(WARNING, "Water tank empty");
    }
}

void setRuntimePidState(const bool enabled) {
    pidON = enabled ? 1 : 0;
    config.set<bool>("pid.enabled", enabled);
}

void setSteamMode(bool steamMode) {
    steamON = steamMode;

    if (steamON) {
        steamFirstON = true;
    }

    if (!steamON) {
        steamFirstON = false;
    }
}

/**
 * @brief Safely shutdown machine operations
 * Turns off pump, valve, and heater regardless of current state
 */
void performSafeShutdown() {
    setRuntimePidState(false);

    heaterRelay->off();
    pumpRelay->off();
    valveRelay->off();

    // Reset all brew-related states
    if (currBrewState != kBrewIdle) {
        LOG(INFO, "Stopping active brew");
        currBrewState = kBrewIdle;
        currBrewSwitchState = kBrewSwitchIdle;
        currBrewTime = 0;
        startingTime = 0;
        brewSwitchWasOff = false;
    }

    // Reset manual flush states
    if (currManualFlushState != kManualFlushIdle) {
        LOG(INFO, "Stopping manual group head flush");
        currManualFlushState = kManualFlushIdle;
        currBrewSwitchState = kBrewSwitchIdle;
        currBrewTime = 0;
        startingTime = 0;
    }

    // Reset backflush state
    if (currBackflushState != kBackflushIdle) {
        LOG(INFO, "Stopping active backflush");
        currBackflushState = kBackflushIdle;
        currBackflushCycles = 1;
    }

    // Reset hot water state
    if (currHotWaterState != kHotWaterIdle) {
        LOG(INFO, "Stopping hot water draw");
        currHotWaterState = kHotWaterIdle;
        currHotWaterSwitchState = kHotWaterSwitchIdle;
        currPumpOnTime = 0;
        pumpStartingTime = 0;
    }

    // Turn off steam mode if active
    if (steamON) {
        LOG(INFO, "Disabling steam mode");
        steamON = false;
        steamFirstON = false;
    }

    LOG(INFO, "Safe shutdown, all relays turned off");
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
