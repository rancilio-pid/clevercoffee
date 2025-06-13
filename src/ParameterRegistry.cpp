
#include "ParameterRegistry.h"
#include "Logger.h"
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
extern double emaFactor;
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
extern int logLevel;
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
    auto addParam = [this](const std::shared_ptr<Parameter>& param) {
        _parameters.push_back(param);
        _parameterMap[param->getId()] = param;
    };

    // PID Section
    addParam(std::make_shared<Parameter>(
        "PID_ON", "Enable PID Controller", kUInt8, sPIDSection, 1,
        [&config]() -> bool {
            pidON = config.getPidEnabled();
            return pidON;
        },
        [&config](const bool val) {
            config.setPidEnabled(val);
            pidON = val;
        },
        false, "", [] { return true; }, &pidON));

    addParam(std::make_shared<Parameter>(
        "PID_USE_PONM", "Enable PonM", kUInt8, sPIDSection, 2,
        [&config]() -> bool {
            usePonM = config.getUsePonM();
            return usePonM;
        },
        [&config](const bool val) {
            config.setUsePonM(val);
            usePonM = val;
        },
        true, "Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>)", [] { return true; }, &usePonM));

    addParam(std::make_shared<Parameter>(
        "PID_EMA_FACTOR", "PID EMA Factor", kDouble, sPIDSection, 3,
        [&config] {
            emaFactor = config.getPidEmaFactor();
            return emaFactor;
        },
        [&config](const double val) {
            config.setPidEmaFactor(val);
            emaFactor = val;
        },
        PID_EMA_FACTOR_MIN, PID_EMA_FACTOR_MAX, true, "Smoothing of input that is used for Tv (derivative component of PID). Smaller means less smoothing but also less delay, 0 means no filtering", [] { return true; },
        &emaFactor));

    addParam(std::make_shared<Parameter>(
        "PID_KP", "PID Kp", kDouble, sPIDSection, 4,
        [&config] {
            aggKp = config.getPidKpRegular();
            return aggKp;
        },
        [&config](const double val) {
            config.setPidKpRegular(val);
            aggKp = val;
        },
        PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, true,
        "Proportional gain (in Watts/C°) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The higher this value is, the higher is the "
        "output of the heater for a given temperature difference. E.g. 5°C difference will result in P*5 Watts of heater output.",
        [] { return true; }, &aggKp));

    addParam(std::make_shared<Parameter>(
        "PID_TN", "PID Tn (=Kp/Ki)", kDouble, sPIDSection, 5,
        [&config] {
            aggTn = config.getPidTnRegular();
            return aggTn;
        },
        [&config](const double val) {
            config.setPidTnRegular(val);
            aggTn = val;
        },
        PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, true,
        "Integral time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The larger this value is, the slower the "
        "integral part of the PID will increase (or decrease) if the process value remains above (or below) the setpoint in spite of proportional action. The smaller this value, the faster the integral term changes.",
        [] { return true; }, &aggTn));

    addParam(std::make_shared<Parameter>(
        "PID_TV", "PID Tv (=Kd/Kp)", kDouble, sPIDSection, 6,
        [&config] {
            aggTv = config.getPidTvRegular();
            return aggTv;
        },
        [&config](const double val) {
            config.setPidTvRegular(val);
            aggTv = val;
        },
        PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, true,
        "Differential time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). This value determines how far the "
        "PID equation projects the current trend into the future. The higher the value, the greater the dampening. Select it carefully, it can cause oscillations if it is set too high or too low.",
        [] { return true; }, &aggTv));

    addParam(std::make_shared<Parameter>(
        "PID_I_MAX", "PID Integrator Max", kDouble, sPIDSection, 7,
        [&config] {
            aggIMax = config.getPidIMaxRegular();
            return aggIMax;
        },
        [&config](const double val) {
            config.setPidIMaxRegular(val);
            aggIMax = val;
        },
        PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, true,
        "Internal integrator limit to prevent windup (in Watts). This will allow the integrator to only grow to the specified value. This should be approximally equal to the output needed to hold the temperature after the "
        "setpoint has been reached and is depending on machine type and whether the boiler is insulated or not.",
        [] { return true; }, &aggIMax));

    addParam(std::make_shared<Parameter>(
        "STEAM_KP", "Steam Kp", kDouble, sPIDSection, 8,
        [&config] {
            steamKp = config.getPidKpSteam();
            return steamKp;
        },
        [&config](const double val) {
            config.setPidKpSteam(val);
            steamKp = val;
        },
        PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, true, "Proportional gain for the steaming mode (I or D are not used)", [] { return true; }, &steamKp));

    addParam(std::make_shared<Parameter>("TEMP", "Temperature", kDouble, sPIDSection, 9, [&] { return temperature; }, [](const double val) { temperature = val; }, 0.0, 200.0, false, "", [] { return false; }, &temperature));

    // Temperature Section
    addParam(std::make_shared<Parameter>(
        "BREW_SETPOINT", "Setpoint (°C)", kDouble, sTempSection, 10,
        [&config] {
            brewSetpoint = config.getBrewSetpoint();
            return brewSetpoint;
        },
        [&config](const double val) {
            config.setBrewSetpoint(val);
            brewSetpoint = val;
        },
        BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, true, "The temperature that the PID will attempt to reach and hold", [] { return true; }, &brewSetpoint));

    addParam(std::make_shared<Parameter>(
        "BREW_TEMP_OFFSET", "Offset (°C)", kDouble, sTempSection, 11,
        [&config] {
            brewTempOffset = config.getBrewTempOffset();
            return brewTempOffset;
        },
        [&config](const double val) {
            config.setBrewTempOffset(val);
            brewTempOffset = val;
        },
        BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, true,
        "Optional offset that is added to the user-visible setpoint. Can be used to compensate sensor offsets and the average temperature loss between boiler and group so that the setpoint represents the approximate brew "
        "temperature.",
        [] { return true; }, &brewTempOffset));

    addParam(std::make_shared<Parameter>(
        "STEAM_SETPOINT", "Steam Setpoint (°C)", kDouble, sTempSection, 12,
        [&config] {
            steamSetpoint = config.getSteamSetpoint();
            return steamSetpoint;
        },
        [&config](const double val) {
            config.setSteamSetpoint(val);
            steamSetpoint = val;
        },
        STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, true, "The temperature that the PID will use for steam mode", [] { return true; }, &steamSetpoint));

    // Brew Section
    addParam(std::make_shared<Parameter>(
        "BREWCONTROL", "Brew Control", kUInt8, sBrewSection, 13,
        [&config]() -> bool {
            featureBrewControl = config.getFeatureBrewControl();
            return featureBrewControl;
        },
        [&config](const bool val) {
            config.setFeatureBrewControl(val);
            featureBrewControl = val;
        },
        true, "Enables brew-by-time or brew-by-weight", [] { return FEATURE_BREWSWITCH == 1; }, &featureBrewControl));

    addParam(std::make_shared<Parameter>(
        "TARGET_BREW_TIME", "Target Brew Time (s)", kDouble, sBrewSection, 14,
        [&config] {
            targetBrewTime = config.getTargetBrewTime();
            return targetBrewTime;
        },
        [&config](const double val) {
            config.setTargetBrewTime(val);
            targetBrewTime = val;
        },
        TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX, true, "Stop brew after this time. Set to 0 to deactivate brew-by-time-feature.", [] { return featureBrewControl == 1; }, &targetBrewTime));

    addParam(std::make_shared<Parameter>(
        "BREW_PREINFUSIONPAUSE", "Preinfusion Pause Time (s)", kDouble, sBrewSection, 15,
        [&config] {
            preinfusionPause = config.getPreInfusionPause();
            return preinfusionPause;
        },
        [&config](const double val) {
            config.setPreInfusionPause(val);
            preinfusionPause = val;
        },
        PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, false, "", [] { return featureBrewControl == 1; }, &preinfusionPause));

    addParam(std::make_shared<Parameter>(
        "BREW_PREINFUSION", "Preinfusion Time (s)", kDouble, sBrewSection, 16,
        [&config] {
            preinfusion = config.getPreInfusionTime();
            return preinfusion;
        },
        [&config](const double val) {
            config.setPreInfusionTime(val);
            preinfusion = val;
        },
        PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, false, "", [] { return featureBrewControl == 1; }, &preinfusion));

    // Maintenance Section
    addParam(std::make_shared<Parameter>(
        "BACKFLUSH_CYCLES", "Backflush Cycles", kInteger, sMaintenanceSection, 17,
        [&config] {
            backflushCycles = config.getBackflushCycles();
            return backflushCycles;
        },
        [&config](const double val) {
            config.setBackflushCycles(static_cast<int>(val));
            backflushCycles = static_cast<int>(val);
        },
        BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX, true, "Number of cycles of filling and flushing during a backflush", [] { return featureBrewControl == 1; }, &backflushCycles));

    addParam(std::make_shared<Parameter>(
        "BACKFLUSH_FILL_TIME", "Backflush Fill Time (s)", kDouble, sMaintenanceSection, 18,
        [&config] {
            backflushFillTime = config.getBackflushFillTime();
            return backflushFillTime;
        },
        [&config](const double val) {
            config.setBackflushFillTime(val);
            backflushFillTime = val;
        },
        BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX, true, "Time in seconds the pump is running during one backflush cycle", [] { return featureBrewControl == 1; }, &backflushFillTime));

    addParam(std::make_shared<Parameter>(
        "BACKFLUSH_FLUSH_TIME", "Backflush Flush Time (s)", kDouble, sMaintenanceSection, 19,
        [&config] {
            backflushFlushTime = config.getBackflushFlushTime();
            return backflushFlushTime;
        },
        [&config](const double val) {
            config.setBackflushFlushTime(val);
            backflushFlushTime = val;
        },
        BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX, true, "Time in seconds the selenoid valve stays open during one backflush cycle", [] { return featureBrewControl == 1; }, &backflushFlushTime));

#if (FEATURE_SCALE == 1)
    addParam(std::make_shared<Parameter>(
        "SCALE_TARGET_BREW_WEIGHT", "Brew weight target (g)", kDouble, sBrewSection, 20,
        [&config] {
            targetBrewWeight = config.getTargetBrewWeight();
            return targetBrewWeight;
        },
        [&config](const double val) {
            config.setTargetBrewWeight(val);
            targetBrewWeight = val;
        },
        TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX, true, "Brew is running until this weight has been measured. Set to 0 to deactivate brew-by-weight-feature.", [] { return featureBrewControl == 1; }, &targetBrewWeight));

    addParam(std::make_shared<Parameter>("TARE_ON", "Tare", kUInt8, sScaleSection, 21, [&]() -> bool { return scaleTareOn; }, [](const bool val) { scaleTareOn = val; }, false, "", [] { return false; }, &scaleTareOn));

    addParam(std::make_shared<Parameter>(
        "CALIBRATION_ON", "Calibration", kUInt8, sScaleSection, 22, [&]() -> bool { return scaleCalibrationOn; }, [](const bool val) { scaleCalibrationOn = val; }, false, "", [] { return false; }, &scaleCalibrationOn));

    addParam(std::make_shared<Parameter>(
        "SCALE_KNOWN_WEIGHT", "Known weight in g", kFloat, sScaleSection, 23,
        [&config] {
            scaleKnownWeight = config.getScaleKnownWeight();
            return scaleKnownWeight;
        },
        [&config](const double val) {
            config.setScaleKnownWeight(static_cast<float>(val));
            scaleKnownWeight = static_cast<float>(val);
        },
        0.0, 2000.0, false, "", [] { return true; }, &scaleKnownWeight));

    addParam(std::make_shared<Parameter>(
        "SCALE_CALIBRATION", "Calibration factor scale 1", kFloat, sScaleSection, 24,
        [&config] {
            scaleCalibration = config.getScaleCalibration();
            return scaleCalibration;
        },
        [&config](const double val) {
            config.setScaleCalibration(static_cast<float>(val));
            scaleCalibration = static_cast<float>(val);
        },
        -100000, 100000, false, "", [] { return true; }, &scaleCalibration));

    addParam(std::make_shared<Parameter>(
        "SCALE2_CALIBRATION", "Calibration factor scale 2", kFloat, sScaleSection, 25,
        [&config] {
            scale2Calibration = config.getScale2Calibration();
            return scale2Calibration;
        },
        [&config](const double val) {
            config.setScale2Calibration(static_cast<float>(val));
            scale2Calibration = static_cast<float>(val);
        },
        -100000, 100000, false, "", [] { return SCALE_TYPE == 0; }, &scale2Calibration));
#endif

    // Brew PID Section
    addParam(std::make_shared<Parameter>(
        "PID_BD_ON", "Enable Brew PID", kUInt8, sBrewPidSection, 26,
        [&config]() -> bool {
            useBDPID = config.getUseBDPID();
            return useBDPID;
        },
        [&config](const bool val) {
            config.setUseBDPID(val);
            useBDPID = val;
        },
        true, "Use separate PID parameters while brew is running", [] { return FEATURE_BREWSWITCH == 1; }, &useBDPID));

    addParam(std::make_shared<Parameter>(
        "PID_BD_DELAY", "Brew PID Delay (s)", kDouble, sBrewPidSection, 27,
        [&config] {
            brewPIDDelay = config.getBrewPIDDelay();
            return brewPIDDelay;
        },
        [&config](const double val) {
            config.setBrewPIDDelay(val);
            brewPIDDelay = val;
        },
        BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, true,
        "Delay time in seconds during which the PID will be disabled once a brew is detected. This prevents too high brew temperatures with boiler machines like Rancilio Silvia. Set to 0 for thermoblock machines.",
        [] { return true; }, &brewPIDDelay));

    addParam(std::make_shared<Parameter>(
        "PID_BD_KP", "BD Kp", kDouble, sBrewPidSection, 28,
        [&config] {
            aggbKp = config.getPidKpBD();
            return aggbKp;
        },
        [&config](const double val) {
            config.setPidKpBD(val);
            aggbKp = val;
        },
        PID_KP_BD_MIN, PID_KP_BD_MAX, true,
        "Proportional gain (in Watts/°C) for the PID when brewing has been detected. Use this controller to either increase heating during the brew to counter temperature drop from fresh cold water in the boiler. Some "
        "machines, e.g. Rancilio Silvia, actually need to heat less or not at all during the brew because of high temperature stability (<a "
        "href='https://www.kaffee-netz.de/threads/installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/#post-1453641' target='_blank'>Details<a>)",
        [] { return useBDPID == 1; }, &aggbKp));

    addParam(std::make_shared<Parameter>(
        "PID_BD_TN", "BD Tn (=Kp/Ki)", kDouble, sBrewPidSection, 29,
        [&config] {
            aggbTn = config.getPidTnBD();
            return aggbTn;
        },
        [&config](const double val) {
            config.setPidTnBD(val);
            aggbTn = val;
        },
        PID_TN_BD_MIN, PID_TN_BD_MAX, true, "Integral time constant (in seconds) for the PID when brewing has been detected.", [] { return useBDPID == 1; }, &aggbTn));

    addParam(std::make_shared<Parameter>(
        "PID_BD_TV", "BD Tv (=Kd/Kp)", kDouble, sBrewPidSection, 30,
        [&config] {
            aggbTv = config.getPidTvBD();
            return aggbTv;
        },
        [&config](const double val) {
            config.setPidTvBD(val);
            aggbTv = val;
        },
        PID_TV_BD_MIN, PID_TV_BD_MAX, true, "Differential time constant (in seconds) for the PID when brewing has been detected.", [] { return useBDPID == 1; }, &aggbTv));

    // Other Section (special parameters, e.g. runtime-only toggles)
    addParam(std::make_shared<Parameter>("STEAM_MODE", "Steam Mode", kUInt8, sOtherSection, 31, [&]() -> bool { return steamON; }, [](const bool val) { steamON = val; }, false, "", [] { return true; }, &steamON));

    addParam(
        std::make_shared<Parameter>("BACKFLUSH_ON", "Backflush", kUInt8, sOtherSection, 32, [&]() -> bool { return backflushOn; }, [](const bool val) { backflushOn = val; }, false, "", [] { return true; }, &backflushOn));

    // Power Section
    addParam(std::make_shared<Parameter>(
        "STANDBY_MODE_ON", "Enable Standby Timer", kUInt8, sPowerSection, 33,
        [&config]() -> bool {
            standbyModeOn = config.getStandbyModeOn();
            return standbyModeOn;
        },
        [&config](const bool val) {
            config.setStandbyModeOn(val);
            standbyModeOn = val;
        },
        true, "Turn heater off after standby time has elapsed.", [] { return true; }, &standbyModeOn));

    addParam(std::make_shared<Parameter>(
        "STANDBY_MODE_TIMER", "Standby Time", kDouble, sPowerSection, 34,
        [&config] {
            standbyModeTime = config.getStandbyModeTime();
            return standbyModeTime;
        },
        [&config](const double val) {
            config.setStandbyModeTime(val);
            standbyModeTime = val;
        },
        STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, true, "Time in minutes until the heater is turned off. Timer is reset by brew, manual flush, backflush and steam.", [] { return true; }, &standbyModeTime));

    // Display Section
    addParam(std::make_shared<Parameter>(
        "DISPLAY_TEMPLATE", "Display Template", kEnum, sDisplaySection, 35, [&config] { return config.getDisplayTemplate(); }, [&config](const double val) { config.setDisplayTemplate(val); },
        (const char* const[]){"Standard", "Minimal", "Temp only", "Scale"}, 4, true, "Set the display template, changes requre a reboot", [] { return true; }));

    addParam(std::make_shared<Parameter>(
        "FULLSCREEN_BREW_TIMER", "Enable Fullscreen Brew Timer", kUInt8, sDisplaySection, 36,
        [&config]() -> bool {
            featureFullscreenBrewTimer = config.getFeatureFullscreenBrewTimer();
            return featureFullscreenBrewTimer;
        },
        [&config](const bool val) {
            config.setFeatureFullscreenBrewTimer(val);
            featureFullscreenBrewTimer = val;
        },
        true, "Enable fullscreen overlay during brew", [] { return true; }, &featureFullscreenBrewTimer));

    addParam(std::make_shared<Parameter>(
        "FULLSCREEN_MANUAL_FLUSH_TIMER", "Enable Fullscreen Manual Flush Timer", kUInt8, sDisplaySection, 37,
        [&config]() -> bool {
            featureFullscreenManualFlushTimer = config.getFeatureFullscreenManualFlushTimer();
            return featureFullscreenManualFlushTimer;
        },
        [&config](const bool val) {
            config.setFeatureFullscreenManualFlushTimer(val);
            featureFullscreenManualFlushTimer = val;
        },
        true, "Enable fullscreen overlay during manual flush", [] { return true; }, &featureFullscreenManualFlushTimer));

    addParam(std::make_shared<Parameter>(
        "POST_BREW_TIMER_DURATION", "Post Brew Timer Duration (s)", kDouble, sDisplaySection, 38,
        [&config] {
            postBrewTimerDuration = config.getPostBrewTimerDuration();
            return postBrewTimerDuration;
        },
        [&config](const double val) {
            config.setPostBrewTimerDuration(val);
            postBrewTimerDuration = val;
        },
        POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX, true, "time in s that brew timer will be shown after brew finished", [] { return true; }, &postBrewTimerDuration));

    addParam(std::make_shared<Parameter>(
        "HEATING_LOGO", "Enable Heating Logo", kUInt8, sDisplaySection, 39,
        [&config]() -> bool {
            featureHeatingLogo = config.getFeatureHeatingLogo();
            return featureHeatingLogo;
        },
        [&config](const bool val) {
            config.setFeatureHeatingLogo(val);
            featureHeatingLogo = val;
        },
        true, "full screen logo will be shown if temperature is 5°C below setpoint", [] { return true; }, &featureHeatingLogo));

    addParam(std::make_shared<Parameter>(
        "PID_OFF_LOGO", "Enable 'PID Disabled' Logo", kUInt8, sDisplaySection, 40,
        [&config]() -> bool {
            featurePidOffLogo = config.getFeaturePidOffLogo();
            return featurePidOffLogo;
        },
        [&config](const bool val) {
            config.setFeaturePidOffLogo(val);
            featurePidOffLogo = val != 0;
        },
        true, "full screen logo will be shown if pid is disabled", [] { return true; }, &featurePidOffLogo));

    addParam(std::make_shared<Parameter>(
        "MQTT_ENABLED", "MQTT enabled", kUInt8, sMqttSection, 41, [&config]() -> bool { return config.getMqttEnabled(); }, [&config](const bool val) { config.setMqttEnabled(val); }, true, "Enables MQTT, requires a restart",
        []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_BROKER", "Hostname", kCString, sMqttSection, 42, [&config] { return config.getMqttBroker(); }, [&config](const String& val) { config.setMqttBroker(val); }, MQTT_BROKER_MAX_LENGTH, true,
        "IP addresss or hostname of your MQTT broker, changes require a restart", []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_PORT", "Port", kInteger, sMqttSection, 43, [&config] { return static_cast<double>(config.getMqttPort()); }, [&config](const double val) { config.setMqttPort(static_cast<int>(val)); }, 0.0, 99999.0, true,
        "Port number of your MQTT broker, changes require a restart", []() -> bool { return true; }, static_cast<void*>(nullptr)));

    addParam(std::make_shared<Parameter>(
        "MQTT_USERNAME", "Username", kCString, sMqttSection, 44, [&config] { return config.getMqttUsername(); }, [&config](const String& val) { config.setMqttUsername(val); }, MQTT_USERNAME_MAX_LENGTH, true,
        "Username for your MQTT broker, changes require a restart", []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_PASSWORD", "Password", kCString, sMqttSection, 45, [&config] { return config.getMqttPassword(); }, [&config](const String& val) { config.setMqttPassword(val); }, MQTT_PASSWORD_MAX_LENGTH, true,
        "Password for your MQTT broker, changes require a restart", []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_TOPIC", "Topic Prefix", kCString, sMqttSection, 46, [&config] { return config.getMqttTopic(); }, [&config](const String& val) { config.setMqttTopic(val); }, MQTT_TOPIC_MAX_LENGTH, true,
        "Custom MQTT topic prefix, changes require a restart", []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_HASSIO_ENABLED", "Hass.io enabled", kUInt8, sMqttSection, 47, [&config]() -> bool { return config.getMqttHassioEnabled(); }, [&config](const bool val) { config.setMqttHassioEnabled(val); }, true,
        "Enables Home Assistant integration, requires a restart", []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "MQTT_HASSIO_PREFIX", "Hass.io Prefix", kCString, sMqttSection, 48, [&config] { return config.getMqttHassioPrefix(); }, [&config](const String& val) { config.setMqttHassioPrefix(val); }, MQTT_HASSIO_PREFIX_MAX_LENGTH,
        true, "Custom MQTT topic prefix, changes require a restart", []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "HOSTNAME", "Hostname", kCString, sSystemSection, 49, [&config] { return config.getHostname(); }, [&config](const String& val) { config.setHostname(val); }, HOSTNAME_MAX_LENGTH, true,
        "Hostname of your machine, changes require a restart", []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "OTA_PASSWORD", "OtA Password", kCString, sSystemSection, 50, [&config] { return config.getOtaPass(); }, [&config](const String& val) { config.setOtaPass(val); }, OTAPASS_MAX_LENGTH, true,
        "Password for over-the-air updates, changes require a restart", []() -> bool { return true; }, nullptr));

    addParam(std::make_shared<Parameter>(
        "LOG_LEVEL", "Log Level", kEnum, sSystemSection, 51, [&config] { return config.getLogLevel(); },
        [&config](const double val) {
            config.setLogLevel(val);
            Logger::setLevel(static_cast<Logger::Level>(val));
        },
        (const char* const[]){"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "SILENT"}, 7, true, "Set the logging verbosity level", [] { return true; }));

    addParam(std::make_shared<Parameter>("VERSION", "Version", kCString, sOtherSection, 52, [] { return sysVersion; }, nullptr, 64, false, "", [] { return false; }, nullptr));

    std::sort(_parameters.begin(), _parameters.end(), [](const std::shared_ptr<Parameter>& a, const std::shared_ptr<Parameter>& b) { return a->getPosition() < b->getPosition(); });

    _ready = true;
}

std::shared_ptr<Parameter> ParameterRegistry::getParameterById(const char* id) {
    if (const auto it = _parameterMap.find(id); it != _parameterMap.end()) {
        return it->second;
    }

    return nullptr;
}

void ParameterRegistry::syncGlobalVariables() const {
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