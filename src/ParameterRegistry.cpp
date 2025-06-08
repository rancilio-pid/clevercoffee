
#include "ParameterRegistry.h"
#include "userConfig.h"
#include <algorithm>

ParameterRegistry ParameterRegistry::_singleton;

// Global variables, needed for backwards compatibility
extern uint8_t pidON;
extern uint8_t usePonM;
extern double aggKp;
extern double aggTn;
extern double aggTv;
extern double aggIMax;
extern double steamKp;
extern double brewSetpoint;
extern double brewTempOffset;
extern double brewPIDDelay;
extern uint8_t useBDPID;
extern double aggbKp;
extern double aggbTn;
extern double aggbTv;
extern double steamSetpoint;
extern double targetBrewTime;
extern double preinfusion;
extern double preinfusionPause;
extern double targetBrewWeight;
extern int backflushCycles;
extern double backflushFillTime;
extern double backflushFlushTime;
extern uint8_t standbyModeOn;
extern double standbyModeTime;
extern uint8_t featureBrewControl;
extern uint8_t featureFullscreenBrewTimer;
extern uint8_t featureFullscreenManualFlushTimer;
extern double postBrewTimerDuration;
extern uint8_t featureHeatingLogo;
extern uint8_t featurePidOffLogo;
extern float scaleCalibration;
extern float scale2Calibration;
extern float scaleKnownWeight;
extern uint8_t steamON;
extern uint8_t backflushOn;
extern double temperature;
extern uint8_t scaleTareOn;
extern uint8_t scaleCalibrationOn;
extern const char sysVersion[64];

void ParameterRegistry::initialize(Config& config) {
    if (_ready) {
        return;
    }

    _config = &config;

    _parameters.clear();
    _parameterMap.clear();
    _pendingChanges = false;
    _lastChangeTime = 0;

    // Add all parameters
    auto addParam = [this](std::shared_ptr<Parameter> param) {
        _parameters.push_back(param);
        _parameterMap[param->getId()] = param;
    };

    // PID Section
    addParam(std::make_shared<Parameter>(
        "PID_ON", "Enable PID Controller", kUInt8, sPIDSection, 1,
        [&config]() {
            pidON = config.getPidEnabled() ? 1 : 0;
            return pidON;
        },
        [&config](double val) {
            config.setPidEnabled(val != 0);
            pidON = val != 0 ? 1 : 0;
        },
        0, 1, false, "", []() { return true; }, nullptr, nullptr, &pidON));

    addParam(std::make_shared<Parameter>(
        "PID_USE_PONM", "Enable PonM", kUInt8, sPIDSection, 2,
        [&config]() {
            usePonM = config.getUsePonM() ? 1 : 0;
            return usePonM;
        },
        [&config](double val) {
            config.setUsePonM(val != 0);
            usePonM = val != 0 ? 1 : 0;
        },
        0, 1, true, "Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>)", []() { return true; }, nullptr, nullptr, &usePonM));

    addParam(std::make_shared<Parameter>(
        "PID_KP", "PID Kp", kDouble, sPIDSection, 3,
        [&config]() {
            aggKp = config.getPidKpRegular();
            return aggKp;
        },
        [&config](double val) {
            config.setPidKpRegular(val);
            aggKp = val;
        },
        PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, true,
        "Proportional gain (in Watts/C°) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The higher this value is, the higher is the "
        "output of the heater for a given temperature difference. E.g. 5°C difference will result in P*5 Watts of heater output.",
        []() { return true; }, nullptr, nullptr, &aggKp));

    addParam(std::make_shared<Parameter>(
        "PID_TN", "PID Tn (=Kp/Ki)", kDouble, sPIDSection, 4,
        [&config]() {
            aggTn = config.getPidTnRegular();
            return aggTn;
        },
        [&config](double val) {
            config.setPidTnRegular(val);
            aggTn = val;
        },
        PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, true,
        "Integral time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The larger this value is, the slower the "
        "integral part of the PID will increase (or decrease) if the process value remains above (or below) the setpoint in spite of proportional action. The smaller this value, the faster the integral term changes.",
        []() { return true; }, nullptr, nullptr, &aggTn));

    addParam(std::make_shared<Parameter>(
        "PID_TV", "PID Tv (=Kd/Kp)", kDouble, sPIDSection, 5,
        [&config]() {
            aggTv = config.getPidTvRegular();
            return aggTv;
        },
        [&config](double val) {
            config.setPidTvRegular(val);
            aggTv = val;
        },
        PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, true,
        "Differential time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). This value determines how far the "
        "PID equation projects the current trend into the future. The higher the value, the greater the dampening. Select it carefully, it can cause oscillations if it is set too high or too low.",
        []() { return true; }, nullptr, nullptr, &aggTv));

    addParam(std::make_shared<Parameter>(
        "PID_I_MAX", "PID Integrator Max", kDouble, sPIDSection, 6,
        [&config]() {
            aggIMax = config.getPidIMaxRegular();
            return aggIMax;
        },
        [&config](double val) {
            config.setPidIMaxRegular(val);
            aggIMax = val;
        },
        PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, true,
        "Internal integrator limit to prevent windup (in Watts). This will allow the integrator to only grow to the specified value. This should be approximally equal to the output needed to hold the temperature after the "
        "setpoint has been reached and is depending on machine type and whether the boiler is insulated or not.",
        []() { return true; }, nullptr, nullptr, &aggIMax));

    addParam(std::make_shared<Parameter>(
        "STEAM_KP", "Steam Kp", kDouble, sPIDSection, 7,
        [&config]() {
            steamKp = config.getPidKpSteam();
            return steamKp;
        },
        [&config](double val) {
            config.setPidKpSteam(val);
            steamKp = val;
        },
        PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, true, "Proportional gain for the steaming mode (I or D are not used)", []() { return true; }, nullptr, nullptr, &steamKp));

    addParam(std::make_shared<Parameter>(
        "TEMP", "Temperature", kDouble, sPIDSection, 8, [&]() { return temperature; }, [](double val) { temperature = val; }, 0, 200, false, "", []() { return false; }, nullptr, nullptr, &temperature));

    // Temperature Section
    addParam(std::make_shared<Parameter>(
        "BREW_SETPOINT", "Set point (°C)", kDouble, sTempSection, 9,
        [&config]() {
            brewSetpoint = config.getBrewSetpoint();
            return brewSetpoint;
        },
        [&config](double val) {
            config.setBrewSetpoint(val);
            brewSetpoint = val;
        },
        BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, true, "The temperature that the PID will attempt to reach and hold", []() { return true; }, nullptr, nullptr, &brewSetpoint));

    addParam(std::make_shared<Parameter>(
        "BREW_TEMP_OFFSET", "Offset (°C)", kDouble, sTempSection, 10,
        [&config]() {
            brewTempOffset = config.getBrewTempOffset();
            return brewTempOffset;
        },
        [&config](double val) {
            config.setBrewTempOffset(val);
            brewTempOffset = val;
        },
        BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, true,
        "Optional offset that is added to the user-visible setpoint. Can be used to compensate sensor offsets and the average temperature loss between boiler and group so that the setpoint represents the approximate brew "
        "temperature.",
        []() { return true; }, nullptr, nullptr, &brewTempOffset));

    addParam(std::make_shared<Parameter>(
        "STEAM_SETPOINT", "Steam Set point (°C)", kDouble, sTempSection, 11,
        [&config]() {
            steamSetpoint = config.getSteamSetpoint();
            return steamSetpoint;
        },
        [&config](double val) {
            config.setSteamSetpoint(val);
            steamSetpoint = val;
        },
        STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, true, "The temperature that the PID will use for steam mode", []() { return true; }, nullptr, nullptr, &steamSetpoint));

    // Brew Section
    addParam(std::make_shared<Parameter>(
        "BREWCONTROL", "Brew Control", kUInt8, sBrewSection, 12,
        [&config]() {
            featureBrewControl = config.getFeatureBrewControl() ? 1 : 0;
            return featureBrewControl;
        },
        [&config](double val) {
            config.setFeatureBrewControl(val != 0);
            featureBrewControl = val != 0 ? 1 : 0;
        },
        0, 1, true, "Enables brew-by-time or brew-by-weight", []() { return FEATURE_BREWSWITCH == 1; }, nullptr, nullptr, &featureBrewControl));

    addParam(std::make_shared<Parameter>(
        "TARGET_BREW_TIME", "Target Brew Time (s)", kDouble, sBrewSection, 13,
        [&config]() {
            targetBrewTime = config.getTargetBrewTime();
            return targetBrewTime;
        },
        [&config](double val) {
            config.setTargetBrewTime(val);
            targetBrewTime = val;
        },
        TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX, true, "Stop brew after this time. Set to 0 to deactivate brew-by-time-feature.", []() { return featureBrewControl == 1; }, nullptr, nullptr, &targetBrewTime));

    addParam(std::make_shared<Parameter>(
        "BREW_PREINFUSIONPAUSE", "Preinfusion Pause Time (s)", kDouble, sBrewSection, 14,
        [&config]() {
            preinfusionPause = config.getPreInfusionPause();
            return preinfusionPause;
        },
        [&config](double val) {
            config.setPreInfusionPause(val);
            preinfusionPause = val;
        },
        PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, false, "", []() { return featureBrewControl == 1; }, nullptr, nullptr, &preinfusionPause));

    addParam(std::make_shared<Parameter>(
        "BREW_PREINFUSION", "Preinfusion Time (s)", kDouble, sBrewSection, 15,
        [&config]() {
            preinfusion = config.getPreInfusionTime();
            return preinfusion;
        },
        [&config](double val) {
            config.setPreInfusionTime(val);
            preinfusion = val;
        },
        PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, false, "", []() { return featureBrewControl == 1; }, nullptr, nullptr, &preinfusion));

    // Maintenance Section
    addParam(std::make_shared<Parameter>(
        "BACKFLUSH_CYCLES", "Backflush Cycles", kInteger, sMaintenanceSection, 16,
        [&config]() {
            backflushCycles = config.getBackflushCycles();
            return backflushCycles;
        },
        [&config](double val) {
            config.setBackflushCycles(static_cast<int>(val));
            backflushCycles = static_cast<int>(val);
        },
        BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX, true, "Number of cycles of filling and flushing during a backflush", []() { return featureBrewControl == 1; }, nullptr, nullptr, &backflushCycles));

    addParam(std::make_shared<Parameter>(
        "BACKFLUSH_FILL_TIME", "Backflush Fill Time (s)", kDouble, sMaintenanceSection, 17,
        [&config]() {
            backflushFillTime = config.getBackflushFillTime();
            return backflushFillTime;
        },
        [&config](double val) {
            config.setBackflushFillTime(val);
            backflushFillTime = val;
        },
        BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX, true, "Time in seconds the pump is running during one backflush cycle", []() { return featureBrewControl == 1; }, nullptr, nullptr, &backflushFillTime));

    addParam(std::make_shared<Parameter>(
        "BACKFLUSH_FLUSH_TIME", "Backflush Flush Time (s)", kDouble, sMaintenanceSection, 18,
        [&config]() {
            backflushFlushTime = config.getBackflushFlushTime();
            return backflushFlushTime;
        },
        [&config](double val) {
            config.setBackflushFlushTime(val);
            backflushFlushTime = val;
        },
        BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX, true, "Time in seconds the selenoid valve stays open during one backflush cycle", []() { return featureBrewControl == 1; }, nullptr, nullptr, &backflushFlushTime));

#if (FEATURE_SCALE == 1)
    addParam(std::make_shared<Parameter>(
        "SCALE_TARGET_BREW_WEIGHT", "Brew weight target (g)", kDouble, sBrewSection, 19,
        [&config]() {
            targetBrewWeight = config.getTargetBrewWeight();
            return targetBrewWeight;
        },
        [&config](double val) {
            config.setTargetBrewWeight(val);
            targetBrewWeight = val;
        },
        TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX, true, "Brew is running until this weight has been measured. Set to 0 to deactivate brew-by-weight-feature.",
        []() { return FEATURE_SCALE == 1 && featureBrewControl == 1; }, nullptr, nullptr, &targetBrewWeight));

    addParam(std::make_shared<Parameter>(
        "TARE_ON", "Tare", kUInt8, sScaleSection, 30, [&]() { return scaleTareOn; }, [](double val) { scaleTareOn = val != 0 ? 1 : 0; }, 0, 1, false, "", []() { return false; }, nullptr, nullptr, &scaleTareOn));

    addParam(std::make_shared<Parameter>(
        "CALIBRATION_ON", "Calibration", kUInt8, sScaleSection, 31, [&]() { return scaleCalibrationOn; }, [](double val) { scaleCalibrationOn = val != 0 ? 1 : 0; }, 0, 1, false, "", []() { return false; }, nullptr, nullptr,
        &scaleCalibrationOn));

    addParam(std::make_shared<Parameter>(
        "SCALE_KNOWN_WEIGHT", "Known weight in g", kFloat, sScaleSection, 32,
        [&config]() {
            scaleKnownWeight = config.getScaleKnownWeight();
            return scaleKnownWeight;
        },
        [&config](double val) {
            config.setScaleKnownWeight(static_cast<float>(val));
            scaleKnownWeight = static_cast<float>(val);
        },
        0, 2000, false, "", []() { return true; }, nullptr, nullptr, &scaleKnownWeight));

    addParam(std::make_shared<Parameter>(
        "SCALE_CALIBRATION", "Calibration factor scale 1", kFloat, sScaleSection, 33,
        [&config]() {
            scaleCalibration = config.getScaleCalibration();
            return scaleCalibration;
        },
        [&config](double val) {
            config.setScaleCalibration(static_cast<float>(val));
            scaleCalibration = static_cast<float>(val);
        },
        -100000, 100000, false, "", []() { return true; }, nullptr, nullptr, &scaleCalibration));

    addParam(std::make_shared<Parameter>(
        "SCALE2_CALIBRATION", "Calibration factor scale 2", kFloat, sScaleSection, 34,
        [&config]() {
            scale2Calibration = config.getScale2Calibration();
            return scale2Calibration;
        },
        [&config](double val) {
            config.setScale2Calibration(static_cast<float>(val));
            scale2Calibration = static_cast<float>(val);
        },
        -100000, 100000, false, "", []() { return SCALE_TYPE == 0; }, nullptr, nullptr, &scale2Calibration));
#endif

    // Brew PID Section
    addParam(std::make_shared<Parameter>(
        "PID_BD_DELAY", "Brew PID Delay (s)", kDouble, sBrewPidSection, 20,
        [&config]() {
            brewPIDDelay = config.getBrewPIDDelay();
            return brewPIDDelay;
        },
        [&config](double val) {
            config.setBrewPIDDelay(val);
            brewPIDDelay = val;
        },
        BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, true,
        "Delay time in seconds during which the PID will be disabled once a brew is detected. This prevents too high brew temperatures with boiler machines like Rancilio Silvia. Set to 0 for thermoblock machines.",
        []() { return true; }, nullptr, nullptr, &brewPIDDelay));

    addParam(std::make_shared<Parameter>(
        "PID_BD_ON", "Enable Brew PID", kUInt8, sBrewPidSection, 21,
        [&config]() {
            useBDPID = config.getUseBDPID() ? 1 : 0;
            return useBDPID;
        },
        [&config](double val) {
            config.setUseBDPID(val != 0);
            useBDPID = val != 0 ? 1 : 0;
        },
        0, 1, true, "Use separate PID parameters while brew is running", []() { return FEATURE_BREWSWITCH == 1; }, nullptr, nullptr, &useBDPID));

    addParam(std::make_shared<Parameter>(
        "PID_BD_KP", "BD Kp", kDouble, sBrewPidSection, 22,
        [&config]() {
            aggbKp = config.getPidKpBD();
            return aggbKp;
        },
        [&config](double val) {
            config.setPidKpBD(val);
            aggbKp = val;
        },
        PID_KP_BD_MIN, PID_KP_BD_MAX, true,
        "Proportional gain (in Watts/°C) for the PID when brewing has been detected. Use this controller to either increase heating during the brew to counter temperature drop from fresh cold water in the boiler. Some "
        "machines, e.g. Rancilio Silvia, actually need to heat less or not at all during the brew because of high temperature stability (<a "
        "href='https://www.kaffee-netz.de/threads/installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/#post-1453641' target='_blank'>Details<a>)",
        []() { return FEATURE_BREWSWITCH == 1 && useBDPID == 1; }, nullptr, nullptr, &aggbKp));

    addParam(std::make_shared<Parameter>(
        "PID_BD_TN", "BD Tn (=Kp/Ki)", kDouble, sBrewPidSection, 23,
        [&config]() {
            aggbTn = config.getPidTnBD();
            return aggbTn;
        },
        [&config](double val) {
            config.setPidTnBD(val);
            aggbTn = val;
        },
        PID_TN_BD_MIN, PID_TN_BD_MAX, true, "Integral time constant (in seconds) for the PID when brewing has been detected.", []() { return FEATURE_BREWSWITCH == 1 && useBDPID == 1; }, nullptr, nullptr, &aggbTn));

    addParam(std::make_shared<Parameter>(
        "PID_BD_TV", "BD Tv (=Kd/Kp)", kDouble, sBrewPidSection, 24,
        [&config]() {
            aggbTv = config.getPidTvBD();
            return aggbTv;
        },
        [&config](double val) {
            config.setPidTvBD(val);
            aggbTv = val;
        },
        PID_TV_BD_MIN, PID_TV_BD_MAX, true, "Differential time constant (in seconds) for the PID when brewing has been detected.", []() { return FEATURE_BREWSWITCH == 1 && useBDPID == 1; }, nullptr, nullptr, &aggbTv));

    // Other Section (special parameters, e.g. runtime-only toggles)
    addParam(std::make_shared<Parameter>(
        "STEAM_MODE", "Steam Mode", kUInt8, sOtherSection, 25, [&]() { return steamON; }, [](double val) { steamON = val != 0 ? 1 : 0; }, 0, 1, false, "", []() { return true; }, nullptr, nullptr, &steamON));

    addParam(std::make_shared<Parameter>(
        "BACKFLUSH_ON", "Backflush", kUInt8, sOtherSection, 26, [&]() { return backflushOn; }, [](double val) { backflushOn = val != 0 ? 1 : 0; }, 0, 1, false, "", []() { return true; }, nullptr, nullptr, &backflushOn));

    // Power Section
    addParam(std::make_shared<Parameter>(
        "STANDBY_MODE_ON", "Enable Standby Timer", kUInt8, sPowerSection, 27,
        [&config]() {
            standbyModeOn = config.getStandbyModeOn() ? 1 : 0;
            return standbyModeOn;
        },
        [&config](double val) {
            config.setStandbyModeOn(val != 0);
            standbyModeOn = val != 0 ? 1 : 0;
        },
        0, 1, true, "Turn heater off after standby time has elapsed.", []() { return true; }, nullptr, nullptr, &standbyModeOn));

    addParam(std::make_shared<Parameter>(
        "STANDBY_MODE_TIMER", "Standby Time", kDouble, sPowerSection, 28,
        [&config]() {
            standbyModeTime = config.getStandbyModeTime();
            return standbyModeTime;
        },
        [&config](double val) {
            config.setStandbyModeTime(val);
            standbyModeTime = val;
        },
        STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, true, "Time in minutes until the heater is turned off. Timer is reset by brew, manual flush, backflush and steam.", []() { return true; }, nullptr, nullptr,
        &standbyModeTime));

    // Display Section
    addParam(std::make_shared<Parameter>(
        "FULLSCREEN_BREW_TIMER", "Enable Fullscreen Brew Timer", kUInt8, sDisplaySection, 35,
        [&config]() {
            featureFullscreenBrewTimer = config.getFeatureFullscreenBrewTimer() ? 1 : 0;
            return featureFullscreenBrewTimer;
        },
        [&config](double val) {
            config.setFeatureFullscreenBrewTimer(val != 0);
            featureFullscreenBrewTimer = val != 0 ? 1 : 0;
        },
        0, 1, true, "Enable fullscreen overlay during brew", []() { return true; }, nullptr, nullptr, &featureFullscreenBrewTimer));

    addParam(std::make_shared<Parameter>(
        "FULLSCREEN_MANUAL_FLUSH_TIMER", "Enable Fullscreen Manual Flush Timer", kUInt8, sDisplaySection, 36,
        [&config]() {
            featureFullscreenManualFlushTimer = config.getFeatureFullscreenManualFlushTimer() ? 1 : 0;
            return featureFullscreenManualFlushTimer;
        },
        [&config](double val) {
            config.setFeatureFullscreenManualFlushTimer(val != 0);
            featureFullscreenManualFlushTimer = val != 0 ? 1 : 0;
        },
        0, 1, true, "Enable fullscreen overlay during manual flush", []() { return true; }, nullptr, nullptr, &featureFullscreenManualFlushTimer));

    addParam(std::make_shared<Parameter>(
        "POST_BREW_TIMER_DURATION", "Post Brew Timer Duration (s)", kDouble, sDisplaySection, 37,
        [&config]() {
            postBrewTimerDuration = config.getPostBrewTimerDuration();
            return postBrewTimerDuration;
        },
        [&config](double val) {
            config.setPostBrewTimerDuration(val);
            postBrewTimerDuration = val;
        },
        POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX, true, "time in s that brew timer will be shown after brew finished", []() { return true; }, nullptr, nullptr, &postBrewTimerDuration));

    addParam(std::make_shared<Parameter>(
        "HEATING_LOGO", "Enable Heating Logo", kUInt8, sDisplaySection, 38,
        [&config]() {
            featureHeatingLogo = config.getFeatureHeatingLogo() ? 1 : 0;
            return featureHeatingLogo;
        },
        [&config](double val) {
            config.setFeatureHeatingLogo(val != 0);
            featureHeatingLogo = val != 0 ? 1 : 0;
        },
        0, 1, true, "full screen logo will be shown if temperature is 5°C below setpoint", []() { return true; }, nullptr, nullptr, &featureHeatingLogo));

    addParam(std::make_shared<Parameter>(
        "PID_OFF_LOGO", "Enable 'PID Disabled' Logo", kUInt8, sDisplaySection, 39,
        [&config]() {
            featurePidOffLogo = config.getFeaturePidOffLogo() ? 1 : 0;
            return featurePidOffLogo;
        },
        [&config](double val) {
            config.setFeaturePidOffLogo(val != 0);
            featurePidOffLogo = val != 0 ? 1 : 0;
        },
        0, 1, true, "full screen logo will be shown if pid is disabled", []() { return true; }, nullptr, nullptr, &featurePidOffLogo));

    addParam(std::make_shared<Parameter>(
        "MQTT_ENABLED", "MQTT enabled", kUInt8, sMqttSection, 41, [&config]() { return config.getMqttEnabled(); }, [&config](const double val) { config.setMqttEnabled(val != 0); }, 0, 1, true,
        "Enables MQTT, requires a restart", []() -> bool { return true; }, nullptr, nullptr, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_BROKER", "Hostname", kCString, sMqttSection, 42, nullptr, nullptr, 0, MQTT_BROKER_MAX_LENGTH, true, "IP addresss or hostname of your MQTT broker, changes require a restart", []() -> bool { return true; },
        [&config]() { return config.getMqttBroker(); }, [&config](const String& val) { config.setMqttBroker(val); }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_PORT", "Port", kInteger, sMqttSection, 43, [&config]() { return config.getMqttPort(); }, [&config](const double val) { config.setMqttPort(val); }, 0, 99999, true,
        "Port number of your MQTT broker, changes require a restart", []() -> bool { return true; }, nullptr, nullptr, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_USERNAME", "Username", kCString, sMqttSection, 44, nullptr, nullptr, 0, MQTT_USERNAME_MAX_LENGTH, true, "Username for your MQTT broker, changes require a restart", []() -> bool { return true; },
        [&config]() { return config.getMqttUsername(); }, [&config](const String& val) { config.setMqttUsername(val); }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_PASSWORD", "Password", kCString, sMqttSection, 45, nullptr, nullptr, 0, MQTT_PASSWORD_MAX_LENGTH, true, "Password for your MQTT broker, changes require a restart", []() -> bool { return true; },
        [&config]() { return config.getMqttPassword(); }, [&config](const String& val) { config.setMqttPassword(val); }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_TOPIC", "Topic Prefix", kCString, sMqttSection, 45, nullptr, nullptr, 0, MQTT_TOPIC_MAX_LENGTH, true, "Custom MQTT topic prefix, changes require a restart", []() -> bool { return true; },
        [&config]() { return config.getMqttTopic(); }, [&config](const String& val) { config.setMqttTopic(val); }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_HASSIO_ENABLED", "Hass.io enabled", kUInt8, sMqttSection, 46, [&config]() { return config.getMqttHassioEnabled(); }, [&config](const double val) { config.setMqttHassioEnabled(val != 0); }, 0, 1, true,
        "Enables Home Assistant integration, requires a restart", []() -> bool { return true; }, nullptr, nullptr, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_HASSIO_PREFIX", "Hass.io Prefix", kCString, sMqttSection, 47, nullptr, nullptr, 0, MQTT_HASSIO_PREFIX_MAX_LENGTH, true, "Custom MQTT topic prefix, changes require a restart", []() -> bool { return true; },
        [&config]() { return config.getMqttHassioPrefix(); }, [&config](const String& val) { config.setMqttHassioPrefix(val); }, nullptr));

    addParam(std::make_shared<Parameter>("VERSION", "Version", kCString, sOtherSection, 48, [&]() { return 0; }, [](double val) {}, 0, 1, false, "", []() { return false; }, []() { return sysVersion; }, nullptr, nullptr));

    std::sort(_parameters.begin(), _parameters.end(), [](const std::shared_ptr<Parameter>& a, const std::shared_ptr<Parameter>& b) { return a->getPosition() < b->getPosition(); });

    _ready = true;
}

std::shared_ptr<Parameter> ParameterRegistry::getParameterById(const char* id) {
    auto it = _parameterMap.find(id);

    if (it != _parameterMap.end()) {
        return it->second;
    }

    return nullptr;
}

void ParameterRegistry::syncGlobalVariables() {
    for (const auto& param : _parameters) {
        if (param && param->getGlobalVariablePointer()) {
            if (param->getType() == kCString) {
                param->syncToGlobalVariable(param->getStringValue());
            }
            else {
                param->syncToGlobalVariable(param->getValue());
            }
        }
    }
}