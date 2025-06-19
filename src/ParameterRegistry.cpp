
#include "ParameterRegistry.h"
#include "Logger.h"
#include <algorithm>

ParameterRegistry ParameterRegistry::_singleton;

// Global variables, needed for backwards compatibility
extern bool pidON;
extern bool usePonM;
extern double aggKp;
extern double aggTn;
extern double aggTv;
extern double aggIMax;
extern double steamKp;
extern double brewSetpoint;
extern double brewTempOffset;
extern double brewPIDDelay;
extern bool useBDPID;
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
extern bool standbyModeOn;
extern double standbyModeTime;
extern bool featureBrewControl;
extern bool featureFullscreenBrewTimer;
extern bool featureFullscreenManualFlushTimer;
extern double postBrewTimerDuration;
extern bool featureHeatingLogo;
extern bool featurePidOffLogo;
extern float scaleCalibration;
extern float scale2Calibration;
extern float scaleKnownWeight;
extern bool steamON;
extern bool backflushOn;
extern double temperature;
extern bool scaleTareOn;
extern bool scaleCalibrationOn;
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

    // clang-format off

    addBoolConfigParam(
        "pid.enabled",
        "Enable PID Controller",
        sPIDSection,
        1,
        &pidON,
        "Enables or disables the PID temperature controller"
    );

    addBoolConfigParam(
        "pid.use_ponm",
        "Enable PonM",
        sPIDSection,
        2,
        &usePonM,
        "Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>)"
    );

    addNumericConfigParam<double>(
        "pid.ema_factor",
        "PID EMA Factor",
        kDouble,
        sPIDSection,
        3,
        &emaFactor,
        PID_EMA_FACTOR_MIN,
        PID_EMA_FACTOR_MAX,
        "Smoothing of input that is used for Tv (derivative component of PID). Smaller means less smoothing but also less delay, 0 means no filtering"
    );

    addNumericConfigParam<double>(
        "pid.regular.kp",
        "PID Kp",
        kDouble,
        sPIDSection,
        4,
        &aggKp,
        PID_KP_REGULAR_MIN,
        PID_KP_REGULAR_MAX,
        "Proportional gain (in Watts/C°) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The higher this value is, the higher is the "
        "output of the heater for a given temperature difference. E.g. 5°C difference will result in P*5 Watts of heater output."
    );

    addNumericConfigParam<double>(
        "pid.regular.tn",
        "PID Tn (=Kp/Ki)",
        kDouble,
        sPIDSection,
        5,
        &aggTn,
        PID_TN_REGULAR_MIN,
        PID_TN_REGULAR_MAX,
        "Integral time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The larger this value is, the slower the "
        "integral part of the PID will increase (or decrease) if the process value remains above (or below) the setpoint in spite of proportional action. The smaller this value, the faster the integral term changes."
    );

    addNumericConfigParam<double>(
        "pid.regular.tv",
        "PID Tv (=Kd/Kp)",
        kDouble,
        sPIDSection,
        6,
        &aggTv,
        PID_TV_REGULAR_MIN,
        PID_TV_REGULAR_MAX,
        "Differential time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). This value determines how far the "
        "PID equation projects the current trend into the future. The higher the value, the greater the dampening. Select it carefully, it can cause oscillations if it is set too high or too low."
    );

    addNumericConfigParam<double>(
        "pid.regular.i_max",
        "PID Integrator Max",
        kDouble,
        sPIDSection,
        7,
        &aggIMax,
        PID_I_MAX_REGULAR_MIN,
        PID_I_MAX_REGULAR_MAX,
        "Internal integrator limit to prevent windup (in Watts). This will allow the integrator to only grow to the specified value. This should be approximally equal to the output needed to hold the temperature after the "
        "setpoint has been reached and is depending on machine type and whether the boiler is insulated or not."
    );

    addNumericConfigParam<double>(
        "pid.steam.kp",
        "Steam Kp",
        kDouble,
        sPIDSection,
        8,
        &steamKp,
        PID_KP_STEAM_MIN,
        PID_KP_STEAM_MAX,
        "Proportional gain for the steaming mode (I or D are not used)"
    );

    // Temperature Section
    addNumericConfigParam<double>(
        "TEMP",
        "Temperature",
        kDouble,
        sPIDSection,
        9,
        &temperature,
        0.0,
        200.0,
        "",
        [] { return false; }
    );

    addNumericConfigParam<double>(
        "brew.setpoint",
        "Setpoint (°C)",
        kDouble,
        sTempSection,
        10,
        &brewSetpoint,
        BREW_SETPOINT_MIN,
        BREW_SETPOINT_MAX,
        "The temperature that the PID will attempt to reach and hold"
    );

    addNumericConfigParam<double>(
        "brew.temp_offset",
        "Offset (°C)",
        kDouble,
        sTempSection,
        11,
        &brewTempOffset,
        BREW_TEMP_OFFSET_MIN,
        BREW_TEMP_OFFSET_MAX,
        "Optional offset that is added to the user-visible setpoint. Can be used to compensate sensor offsets and the average temperature loss between boiler and group so that the setpoint represents the approximate brew temperature."
    );

    addNumericConfigParam<double>(
        "steam.setpoint",
        "Steam Setpoint (°C)",
        kDouble,
        sTempSection,
        12,
        &steamSetpoint,
        STEAM_SETPOINT_MIN,
        STEAM_SETPOINT_MAX,
        "The temperature that the PID will use for steam mode"
    );

    // Brew Section
    addBoolConfigParam(
        "features.brew_control",
        "Brew Control",
        sBrewSection,
        13,
        &featureBrewControl,
        "Enables brew-by-time or brew-by-weight"
    );

    addNumericConfigParam<double>(
        "brew.target_time",
        "Target Brew Time (s)",
        kDouble,
        sBrewSection,
        14,
        &targetBrewTime,
        TARGET_BREW_TIME_MIN,
        TARGET_BREW_TIME_MAX,
        "Stop brew after this time. Set to 0 to deactivate brew-by-time-feature.",
        [] { return featureBrewControl; }
    );

    addNumericConfigParam<double>(
        "brew.pre_infusion.pause",
        "Preinfusion Pause Time (s)",
        kDouble,
        sBrewSection,
        15,
        &preinfusionPause,
        PRE_INFUSION_PAUSE_MIN,
        PRE_INFUSION_PAUSE_MAX,
        "",
        [] { return featureBrewControl; }
    );

    addNumericConfigParam<double>(
        "brew.pre_infusion.time",
        "Preinfusion Time (s)",
        kDouble,
        sBrewSection,
        16,
        &preinfusion,
        PRE_INFUSION_TIME_MIN,
        PRE_INFUSION_TIME_MAX,
        "",
        [] { return featureBrewControl; }
    );

    // Maintenance Section
    addNumericConfigParam<int>(
        "backflush.cycles",
        "Backflush Cycles",
        kInteger,
        sMaintenanceSection,
        17,
        &backflushCycles,
        BACKFLUSH_CYCLES_MIN,
        BACKFLUSH_CYCLES_MAX,
        "Number of cycles of filling and flushing during a backflush",
        [] { return featureBrewControl; }
    );

    addNumericConfigParam<double>(
        "backflush.fill_time",
        "Backflush Fill Time (s)",
        kDouble,
        sMaintenanceSection,
        18,
        &backflushFillTime,
        BACKFLUSH_FILL_TIME_MIN,
        BACKFLUSH_FILL_TIME_MAX,
        "Time in seconds the pump is running during one backflush cycle",
        [] { return featureBrewControl; }
    );

    addNumericConfigParam<double>(
        "backflush.flush_time",
        "Backflush Flush Time (s)",
        kDouble,
        sMaintenanceSection,
        19,
        &backflushFlushTime,
        BACKFLUSH_FLUSH_TIME_MIN,
        BACKFLUSH_FLUSH_TIME_MAX,
        "Time in seconds the selenoid valve stays open during one backflush cycle",
        [] { return featureBrewControl; }
    );

    if (config.get<bool>("hardware.sensors.scale.enabled")) {
        addNumericConfigParam<double>(
            "brew.target_weight",
            "Brew weight target (g)",
            kDouble,
            sBrewSection,
            20,
            &targetBrewWeight,
            TARGET_BREW_WEIGHT_MIN,
            TARGET_BREW_WEIGHT_MAX,
            "Brew is running until this weight has been measured. Set to 0 to deactivate brew-by-weight-feature.",
            [] { return featureBrewControl; }
        );

        addParam(std::make_shared<Parameter>(
            "TARE_ON",
            "Tare",
            kUInt8,
            sOtherSection,
            21,
            [&]() -> bool {
                return scaleTareOn;
            },
            [](const bool val) {
                scaleTareOn = val;
            },
            false,
            "",
            [] { return true; },
            &scaleTareOn
        ));

        addParam(std::make_shared<Parameter>(
            "CALIBRATION_ON",
            "Calibration",
            kUInt8,
            sOtherSection,
            22,
            [&]() -> bool {
                return scaleCalibrationOn;
            },
            [](const bool val) {
                scaleCalibrationOn = val;
            },
            false,
            "",
            [] { return true; },
            &scaleCalibrationOn
        ));

        addNumericConfigParam<float>(
            "hardware.sensors.scale.known_weight",
            "Known weight in g",
            kFloat,
            sScaleSection,
            23,
            &scaleKnownWeight,
            0.0,
            2000.0
        );

        addNumericConfigParam<float>(
            "hardware.sensors.scale.calibration",
            "Calibration factor scale 1",
            kFloat,
            sScaleSection,
            24,
            &scaleCalibration,
            -100000,
            100000
        );

        addNumericConfigParam<float>(
            "hardware.sensors.scale.calibration2",
            "Calibration factor scale 2",
            kFloat,
            sScaleSection,
            25,
            &scale2Calibration,
            -100000,
            100000,
            "",
            [this] { return _config->get<int>("hardware.sensors.scale.type") == 0; }
        );
    }

    // Brew PID Section
    addBoolConfigParam(
        "pid.bd.enabled",
        "Enable Brew PID",
        sBrewPidSection,
        26,
        &useBDPID,
        "Use separate PID parameters while brew is running"
    );

    addNumericConfigParam<double>(
        "brew.pid_delay",
        "Brew PID Delay (s)",
        kDouble,
        sBrewPidSection,
        27,
        &brewPIDDelay,
        BREW_PID_DELAY_MIN,
        BREW_PID_DELAY_MAX,
        "Delay time in seconds during which the PID will be disabled once a brew is detected. This prevents too high brew temperatures with boiler machines like Rancilio Silvia. Set to 0 for thermoblock machines."
    );

    addNumericConfigParam<double>(
        "pid.bd.kp",
        "BD Kp",
        kDouble,
        sBrewPidSection,
        28,
        &aggbKp,
        PID_KP_BD_MIN,
        PID_KP_BD_MAX,
        "Proportional gain (in Watts/°C) for the PID when brewing has been detected. Use this controller to either increase heating during the brew to counter temperature drop from fresh cold water in the boiler. Some machines, e.g. Rancilio Silvia, actually need to heat less or not at all during the brew because of high temperature stability (<a href='https://www.kaffee-netz.de/threads/installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/#post-1453641' target='_blank'>Details<a>)",
        [] { return useBDPID; }
    );

    addNumericConfigParam<double>(
        "pid.bd.tn",
        "BD Tn (=Kp/Ki)",
        kDouble,
        sBrewPidSection,
        29,
        &aggbTn,
        PID_TN_BD_MIN,
        PID_TN_BD_MAX,
        "Integral time constant (in seconds) for the PID when brewing has been detected.",
        [] { return useBDPID; }
    );

    addNumericConfigParam<double>(
        "pid.bd.tv",
        "BD Tv (=Kd/Kp)",
        kDouble,
        sBrewPidSection,
        30,
        &aggbTv,
        PID_TV_BD_MIN,
        PID_TV_BD_MAX,
        "Differential time constant (in seconds) for the PID when brewing has been detected.",
        [] { return useBDPID; }
    );

    // Other Section (special parameters, e.g. runtime-only toggles)
    addParam(std::make_shared<Parameter>(
        "STEAM_MODE",
        "Steam Mode",
        kUInt8,
        sOtherSection,
        31,
        [&]() -> bool {
            return steamON;
        },
        [](const bool val) {
            steamON = val;
        },
        false, "",
        [] { return true; },
        &steamON
    ));

    addParam(std::make_shared<Parameter>(
        "BACKFLUSH_ON",
        "Backflush",
        kUInt8,
        sOtherSection,
        32,
        [&]() -> bool {
            return backflushOn;
        },
        [](const bool val) {
            backflushOn = val;
        },
        false,
        "",
        [] { return true; },
        &backflushOn
    ));

    // Power Section
    addBoolConfigParam(
        "standby.enabled",
        "Enable Standby Timer",
        sPowerSection,
        33,
        &standbyModeOn,
        "Turn heater off after standby time has elapsed."
    );

    addNumericConfigParam<double>(
        "standby.time",
        "Standby Time",
        kDouble,
        sPowerSection,
        34,
        &standbyModeTime,
        STANDBY_MODE_TIME_MIN,
        STANDBY_MODE_TIME_MAX,
        "Time in minutes until the heater is turned off. Timer is reset by brew, manual flush, backflush and steam."
    );

    // Display Section
    addEnumConfigParam(
        "display.template",
        "Display Template",
        sDisplaySection,
        35,
        nullptr,
        (const char* const[]){"Standard", "Minimal", "Temp only", "Scale"},
        4,
        "Set the display template, changes require a reboot"
    );

    addEnumConfigParam(
        "display.language",
        "Display Language",
        sDisplaySection,
        36,
        nullptr,  // No global variable for this parameter
        (const char* const[]){"Deutsch", "English", "Español"},
        3,
        "Set the language for the OLED display, changes requre a reboot"
    );

    addBoolConfigParam(
        "display.fullscreen_brew_timer",
        "Enable Fullscreen Brew Timer",
        sDisplaySection,
        37,
        &featureFullscreenBrewTimer,
        "Enable fullscreen overlay during brew"
    );

    addBoolConfigParam(
        "display.fullscreen_manual_flush_timer",
        "Enable Fullscreen Manual Flush Timer",
        sDisplaySection,
        38,
        &featureFullscreenManualFlushTimer,
        "Enable fullscreen overlay during manual flush"
    );

    addNumericConfigParam(
        "display.post_brew_timer_duration",
        "Post Brew Timer Duration (s)",
        kDouble,
        sDisplaySection,
        39,
        &postBrewTimerDuration,
        POST_BREW_TIMER_DURATION_MIN,
        POST_BREW_TIMER_DURATION_MAX,
        "time in s that brew timer will be shown after brew finished"
    );

    addBoolConfigParam(
        "display.heating_logo",
        "Enable Heating Logo",
        sDisplaySection,
        40,
        &featureHeatingLogo,
        "full screen logo will be shown if temperature is 5°C below setpoint"
    );

    addBoolConfigParam(
        "display.pid_off_logo",
        "Enable 'PID Disabled' Logo",
        sDisplaySection,
        41,
        &featurePidOffLogo,
        "full screen logo will be shown if pid is disabled"
    );

    addBoolConfigParam(
        "mqtt.enabled",
        "MQTT enabled",
        sMqttSection,
        42,
        nullptr,
        "Enables MQTT, change requires a restart"
    );

    addStringConfigParam(
        "mqtt.broker",
        "Hostname",
        sMqttSection,
        43,
        nullptr,
        MQTT_BROKER_MAX_LENGTH,
        "IP addresss or hostname of your MQTT broker, changes require a restart"
    );

    addNumericConfigParam<int>(
        "mqtt.port",
        "Port",
        kInteger,
        sMqttSection,
        44,
        nullptr,
        1,
        65535,
        "Port number of your MQTT broker, changes require a restart"
    );

    addStringConfigParam(
        "mqtt.username",
        "Username",
        sMqttSection,
        45,
        nullptr,
        MQTT_USERNAME_MAX_LENGTH,
        "Username for your MQTT broker, changes require a restart"
    );

    addStringConfigParam(
        "mqtt.password",
        "Password",
        sMqttSection,
        46,
        nullptr,
        MQTT_PASSWORD_MAX_LENGTH,
        "Password for your MQTT broker, changes require a restart"
    );

    addStringConfigParam(
        "mqtt.topic",
        "Topic Prefix",
        sMqttSection,
        47,
        nullptr,
        MQTT_TOPIC_MAX_LENGTH,
        "Custom MQTT topic prefix, changes require a restart"
    );

    addBoolConfigParam(
        "mqtt.hassio.enabled",
        "Hass.io enabled",
        sMqttSection,
        48,
        nullptr,
        "Enables Home Assistant integration, requires a restart"
    );

    addStringConfigParam(
        "mqtt.hassio.prefix",
        "Hass.io Prefix",
        sMqttSection,
        49,
        nullptr,
        MQTT_HASSIO_PREFIX_MAX_LENGTH,
        "Custom MQTT topic prefix, changes require a restart"
    );

    addStringConfigParam(
        "system.hostname",
        "Hostname",
        sSystemSection,
        50,
        nullptr,
        HOSTNAME_MAX_LENGTH,
        "Hostname of your machine, changes require a restart"
    );

    addStringConfigParam(
        "system.ota_password",
        "OtA Password",
        sSystemSection,
        51,
        nullptr,
        OTAPASS_MAX_LENGTH,
        "Password for over-the-air updates, changes require a restart"
    );

    addEnumConfigParam(
        "system.log_level",
        "Log Level",
        sSystemSection,
        52,
        nullptr,
        (const char* const[]){"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "SILENT"},
        7,
        "Set the logging verbosity level"
    );

    // clang-format on

    addParam(std::make_shared<Parameter>("VERSION", "Version", kCString, sOtherSection, 53, [] { return sysVersion; }, nullptr, 64, false, "", [] { return false; }, nullptr));

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