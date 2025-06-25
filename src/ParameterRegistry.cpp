
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
extern bool featureInvertDisplay;
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
        1,
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
        2,
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
        3,
        &steamSetpoint,
        STEAM_SETPOINT_MIN,
        STEAM_SETPOINT_MAX,
        "The temperature that the PID will use for steam mode"
    );

    // Brew Section
    if (config.get<bool>("hardware.switches.brew.enabled")) {
        addBoolConfigParam(
            "features.brew_control",
            "Brew Control",
            sBrewSection,
            1,
            &featureBrewControl,
            "Enables brew-by-time or brew-by-weight"
        );

        addNumericConfigParam<double>(
            "brew.target_time",
            "Target Brew Time (s)",
            kDouble,
            sBrewSection,
            2,
            &targetBrewTime,
            TARGET_BREW_TIME_MIN,
            TARGET_BREW_TIME_MAX,
            "Stop brew after this time. Set to 0 to deactivate brew-by-time-feature."
        );

        addNumericConfigParam<double>(
            "brew.pre_infusion.pause",
            "Preinfusion Pause Time (s)",
            kDouble,
            sBrewSection,
            3,
            &preinfusionPause,
            PRE_INFUSION_PAUSE_MIN,
            PRE_INFUSION_PAUSE_MAX,
            "Pause to let the puck bloom after the initial pre-infusion while turning off the pump and leaving the 3-way valve open"
        );

        addNumericConfigParam<double>(
            "brew.pre_infusion.time",
            "Preinfusion Time (s)",
            kDouble,
            sBrewSection,
            4,
            &preinfusion,
            PRE_INFUSION_TIME_MIN,
            PRE_INFUSION_TIME_MAX,
            "Time in seconds the pump is running during the pre-infusion"
        );

        if (config.get<bool>("hardware.sensors.scale.enabled")) {
            addNumericConfigParam<double>(
                "brew.target_weight",
                "Brew weight target (g)",
                kDouble,
                sBrewSection,
                5,
                &targetBrewWeight,
                TARGET_BREW_WEIGHT_MIN,
                TARGET_BREW_WEIGHT_MAX,
                "Brew is running until this weight has been measured. Set to 0 to deactivate brew-by-weight-feature.",
                [] { return featureBrewControl; }
            );
        }

        // Maintenance Section
        addNumericConfigParam<int>(
            "backflush.cycles",
            "Backflush Cycles",
            kInteger,
            sMaintenanceSection,
            1,
            &backflushCycles,
            BACKFLUSH_CYCLES_MIN,
            BACKFLUSH_CYCLES_MAX,
            "Number of cycles of filling and flushing during a backflush"
        );

        addNumericConfigParam<double>(
            "backflush.fill_time",
            "Backflush Fill Time (s)",
            kDouble,
            sMaintenanceSection,
            2,
            &backflushFillTime,
            BACKFLUSH_FILL_TIME_MIN,
            BACKFLUSH_FILL_TIME_MAX,
            "Time in seconds the pump is running during one backflush cycle"
        );

        addNumericConfigParam<double>(
            "backflush.flush_time",
            "Backflush Flush Time (s)",
            kDouble,
            sMaintenanceSection,
            3,
            &backflushFlushTime,
            BACKFLUSH_FLUSH_TIME_MIN,
            BACKFLUSH_FLUSH_TIME_MAX,
            "Time in seconds the selenoid valve stays open during one backflush cycle"
        );
    }

    if (config.get<bool>("hardware.sensors.scale.enabled")) {
        addParam(std::make_shared<Parameter>(
            "TARE_ON",
            "Tare",
            kUInt8,
            sOtherSection,
            3,
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
            4,
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
            1,
            &scaleKnownWeight,
            0.0,
            2000.0
        );

        addNumericConfigParam<float>(
            "hardware.sensors.scale.calibration",
            "Calibration factor scale 1",
            kFloat,
            sScaleSection,
            2,
            &scaleCalibration,
            -100000,
            100000
        );

        addNumericConfigParam<float>(
            "hardware.sensors.scale.calibration2",
            "Calibration factor scale 2",
            kFloat,
            sScaleSection,
            3,
            &scale2Calibration,
            -100000,
            100000,
            "",
            [this] { return _config->get<int>("hardware.sensors.scale.type") == 0; }
        );
    }

    if (config.get<bool>("hardware.switches.brew.enabled")) {
        // Brew PID Section
        addBoolConfigParam(
            "pid.bd.enabled",
            "Enable Brew PID",
            sBrewPidSection,
            1,
            &useBDPID,
            "Use separate PID parameters while brew is running"
        );

        addNumericConfigParam<double>(
            "brew.pid_delay",
            "Brew PID Delay (s)",
            kDouble,
            sBrewPidSection,
            2,
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
            3,
            &aggbKp,
            PID_KP_BD_MIN,
            PID_KP_BD_MAX,
            "Proportional gain (in Watts/°C) for the PID when brewing has been detected. Use this controller to either increase heating during the brew to counter temperature drop from fresh cold water in the boiler. Some machines, e.g. Rancilio Silvia, actually need to heat less or not at all during the brew because of high temperature stability (<a href='https://www.kaffee-netz.de/threads/installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/#post-1453641' target='_blank'>Details<a>)"
        );

        addNumericConfigParam<double>(
            "pid.bd.tn",
            "BD Tn (=Kp/Ki)",
            kDouble,
            sBrewPidSection,
            4,
            &aggbTn,
            PID_TN_BD_MIN,
            PID_TN_BD_MAX,
            "Integral time constant (in seconds) for the PID when brewing has been detected."
        );

        addNumericConfigParam<double>(
            "pid.bd.tv",
            "BD Tv (=Kd/Kp)",
            kDouble,
            sBrewPidSection,
            5,
            &aggbTv,
            PID_TV_BD_MIN,
            PID_TV_BD_MAX,
            "Differential time constant (in seconds) for the PID when brewing has been detected."
        );
    }

    // Other Section (special parameters, e.g. runtime-only toggles)
    addParam(std::make_shared<Parameter>(
        "STEAM_MODE",
        "Steam Mode",
        kUInt8,
        sOtherSection,
        5,
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

    if (config.get<bool>("hardware.switches.brew.enabled")) {
        addParam(std::make_shared<Parameter>(
            "BACKFLUSH_ON",
            "Backflush",
            kUInt8,
            sOtherSection,
            6,
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
    }

    // Power Section
    addBoolConfigParam(
        "standby.enabled",
        "Enable Standby Timer",
        sPowerSection,
        1,
        &standbyModeOn,
        "Turn heater off after standby time has elapsed."
    );

    addNumericConfigParam<double>(
        "standby.time",
        "Standby Time",
        kDouble,
        sPowerSection,
        2,
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
        1,
        nullptr,
        (const char* const[]){"Standard", "Minimal", "Temp only", "Scale", "Upright"},
        5,
        "Set the display template, changes require a reboot"
    );

    addBoolConfigParam(
        "display.inverted",
        "Invert Display",
        sDisplaySection,
        2,
        &featureInvertDisplay,
        "Set the display rotation, changes require a reboot"
    );

    addEnumConfigParam(
        "display.language",
        "Display Language",
        sDisplaySection,
        3,
        nullptr,  // No global variable for this parameter
        (const char* const[]){"Deutsch", "English", "Español"},
        3,
        "Set the language for the OLED display, changes requre a reboot"
    );

    addBoolConfigParam(
        "display.fullscreen_brew_timer",
        "Enable Fullscreen Brew Timer",
        sDisplaySection,
        4,
        &featureFullscreenBrewTimer,
        "Enable fullscreen overlay during brew"
    );

    addBoolConfigParam(
        "display.fullscreen_manual_flush_timer",
        "Enable Fullscreen Manual Flush Timer",
        sDisplaySection,
        5,
        &featureFullscreenManualFlushTimer,
        "Enable fullscreen overlay during manual flush"
    );

    addNumericConfigParam(
        "display.post_brew_timer_duration",
        "Post Brew Timer Duration (s)",
        kDouble,
        sDisplaySection,
        6,
        &postBrewTimerDuration,
        POST_BREW_TIMER_DURATION_MIN,
        POST_BREW_TIMER_DURATION_MAX,
        "time in s that brew timer will be shown after brew finished"
    );

    addBoolConfigParam(
        "display.heating_logo",
        "Enable Heating Logo",
        sDisplaySection,
        7,
        &featureHeatingLogo,
        "full screen logo will be shown if temperature is 5°C below setpoint"
    );

    addBoolConfigParam(
        "display.pid_off_logo",
        "Enable 'PID Disabled' Logo",
        sDisplaySection,
        8,
        &featurePidOffLogo,
        "full screen logo will be shown if pid is disabled"
    );

    // MQTT section
    addBoolConfigParam(
        "mqtt.enabled",
        "MQTT enabled",
        sMqttSection,
        1,
        nullptr,
        "Enables MQTT, change requires a restart"
    );

    addStringConfigParam(
        "mqtt.broker",
        "Hostname",
        sMqttSection,
        2,
        nullptr,
        MQTT_BROKER_MAX_LENGTH,
        "IP addresss or hostname of your MQTT broker, changes require a restart"
    );

    addNumericConfigParam<int>(
        "mqtt.port",
        "Port",
        kInteger,
        sMqttSection,
        3,
        nullptr,
        1,
        65535,
        "Port number of your MQTT broker, changes require a restart"
    );

    addStringConfigParam(
        "mqtt.username",
        "Username",
        sMqttSection,
        4,
        nullptr,
        MQTT_USERNAME_MAX_LENGTH,
        "Username for your MQTT broker, changes require a restart"
    );

    addStringConfigParam(
        "mqtt.password",
        "Password",
        sMqttSection,
        5,
        nullptr,
        MQTT_PASSWORD_MAX_LENGTH,
        "Password for your MQTT broker, changes require a restart"
    );

    addStringConfigParam(
        "mqtt.topic",
        "Topic Prefix",
        sMqttSection,
        6,
        nullptr,
        MQTT_TOPIC_MAX_LENGTH,
        "Custom MQTT topic prefix, changes require a restart"
    );

    addBoolConfigParam(
        "mqtt.hassio.enabled",
        "Hass.io enabled",
        sMqttSection,
        7,
        nullptr,
        "Enables Home Assistant integration, requires a restart"
    );

    addStringConfigParam(
        "mqtt.hassio.prefix",
        "Hass.io Prefix",
        sMqttSection,
        8,
        nullptr,
        MQTT_HASSIO_PREFIX_MAX_LENGTH,
        "Custom MQTT topic prefix, changes require a restart"
    );

    addStringConfigParam(
        "system.hostname",
        "Hostname",
        sSystemSection,
        1,
        nullptr,
        HOSTNAME_MAX_LENGTH,
        "Hostname of your machine, changes require a restart"
    );

    addStringConfigParam(
        "system.ota_password",
        "OtA Password",
        sSystemSection,
        2,
        nullptr,
        OTAPASS_MAX_LENGTH,
        "Password for over-the-air updates, changes require a restart"
    );

    addEnumConfigParam(
        "system.log_level",
        "Log Level",
        sSystemSection,
        3,
        nullptr,
        (const char* const[]){"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "SILENT"},
        7,
        "Set the logging verbosity level"
    );

    // Hardware section

    // OLED
    addBoolConfigParam(
        "hardware.oled.enabled",
        "Enable OLED Display",
        sHardwareOledSection,
        100,
        nullptr,
        "Enable or disable the OLED display"
    );

    addEnumConfigParam(
        "hardware.oled.type",
        "OLED Type",
        sHardwareOledSection,
        101,
        nullptr,
        (const char* const[]){"SSD1306", "SH1106"},
        2,
        "Select your OLED display type"
    );

    addNumericConfigParam<int>(
        "hardware.oled.address",
        "I2C Address",
        kInteger,
        sHardwareOledSection,
        102,
        nullptr,
        0x00, 0xFF,
        "I2C address of the OLED display (usually 0x3C or 0x3D)"
    );

    // Relays
    addEnumConfigParam(
        "hardware.relays.heater.trigger_type",
        "Heater Relay Trigger Type",
        sHardwareRelaySection,
        101,
        nullptr,
        (const char* const[]){"Low Trigger", "High Trigger"},
        2,
        "Relay trigger type for heater control"
    );

    addEnumConfigParam(
        "hardware.relays.pump_valve.trigger_type",
        "Pump/Valve Relay Trigger Type",
        sHardwareRelaySection,
        102,
        nullptr,
        (const char* const[]){"Low Trigger", "High Trigger"},
        2,
        "Relay trigger type for pump and valve control"
    );

    // Switches
    addBoolConfigParam(
        "hardware.switches.brew.enabled",
        "Enable Brew Switch",
        sHardwareSwitchSection,
        101,
        nullptr,
        "Enable physical brew switch"
    );

    addEnumConfigParam(
        "hardware.switches.brew.type",
        "Brew Switch Type",
        sHardwareSwitchSection,
        102,
        nullptr,
        (const char* const[]){"Momentary", "Toggle"},
        2,
        "Type of brew switch connected"
    );

    addEnumConfigParam(
        "hardware.switches.brew.mode",
        "Brew Switch Mode",
        sHardwareSwitchSection,
        103,
        nullptr,
        (const char* const[]){"Normally Open", "Normally Closed"},
        2,
        "Electrical configuration of brew switch"
    );

    addBoolConfigParam(
        "hardware.switches.steam.enabled",
        "Enable Steam Switch",
        sHardwareSwitchSection,
        104,
        nullptr,
        "Enable physical steam switch"
    );

    addEnumConfigParam(
        "hardware.switches.steam.type",
        "Steam Switch Type",
        sHardwareSwitchSection,
        105,
        nullptr,
        (const char* const[]){"Momentary", "Toggle"},
        2,
        "Type of steam switch connected"
    );

    addEnumConfigParam(
        "hardware.switches.steam.mode",
        "Steam Switch Mode",
        sHardwareSwitchSection,
        106,
        nullptr,
        (const char* const[]){"Normally Open", "Normally Closed"},
        2,
        "Electrical configuration of steam switch"
    );

    addBoolConfigParam(
        "hardware.switches.power.enabled",
        "Enable Power Switch",
        sHardwareSwitchSection,
        107,
        nullptr,
        "Enable physical power switch"
    );

    addEnumConfigParam(
        "hardware.switches.power.type",
        "Power Switch Type",
        sHardwareSwitchSection,
        108,
        nullptr,
        (const char* const[]){"Momentary", "Toggle"},
        3,
        "Type of power switch connected"
    );

    addEnumConfigParam(
        "hardware.switches.power.mode",
        "Power Switch Mode",
        sHardwareSwitchSection,
        109,
        nullptr,
        (const char* const[]){"Normally Open", "Normally Closed"},
        2,
        "Electrical configuration of power switch"
    );

    // LEDs
    addBoolConfigParam(
        "hardware.leds.status.enabled",
        "Enable Status LED",
        sHardwareLedSection,
        101,
        nullptr,
        "Enable status indicator LED"
    );

    addBoolConfigParam(
        "hardware.leds.status.inverted",
        "Invert Status LED",
        sHardwareLedSection,
        102,
        nullptr,
        "Invert the status LED logic (for common anode LEDs)"
    );

    addBoolConfigParam(
        "hardware.leds.brew.enabled",
        "Enable Brew LED",
        sHardwareLedSection,
        103,
        nullptr,
        "Enable brew indicator LED"
    );

    addBoolConfigParam(
        "hardware.leds.brew.inverted",
        "Invert Brew LED",
        sHardwareLedSection,
        104,
        nullptr,
        "Invert the brew LED logic (for common anode LEDs)"
    );

    addBoolConfigParam(
        "hardware.leds.steam.enabled",
        "Enable Steam LED",
        sHardwareLedSection,
        105,
        nullptr,
        "Enable steam indicator LED"
    );

    addBoolConfigParam(
        "hardware.leds.steam.inverted",
        "Invert Steam LED",
        sHardwareLedSection,
        106,
        nullptr,
        "Invert the steam LED logic (for common anode LEDs)"
    );

    // Sensors
    addEnumConfigParam(
        "hardware.sensors.temperature.type",
        "Temperature Sensor Type",
        sHardwareSensorSection,
        101,
        nullptr,
        (const char* const[]){"TSIC306", "Dallas DS18B20"},
        2,
        "Type of temperature sensor connected"
    );

    addBoolConfigParam(
        "hardware.sensors.pressure.enabled",
        "Enable Pressure Sensor",
        sHardwareSensorSection,
        102,
        nullptr,
        "Enable pressure sensor for monitoring brew pressure"
    );

    addBoolConfigParam(
        "hardware.sensors.watertank.enabled",
        "Enable Water Tank Sensor",
        sHardwareSensorSection,
        103,
        nullptr,
        "Enable water tank level sensor"
    );

    addEnumConfigParam(
        "hardware.sensors.watertank.mode",
        "Water Tank Sensor Mode",
        sHardwareSensorSection,
        104,
        nullptr,
        (const char* const[]){"Normally Open", "Normally Closed"},
        2,
        "Electrical configuration of water tank sensor"
    );

    addBoolConfigParam(
        "hardware.sensors.scale.enabled",
        "Enable Scale",
        sHardwareSensorSection,
        105,
        nullptr,
        "Enable integrated scale for weight-based brewing"
    );

    addEnumConfigParam(
        "hardware.sensors.scale.type",
        "Scale Setup Type",
        sHardwareSensorSection,
        106,
        nullptr,
        (const char* const[]){"2 load cells", "1 load cell"},
        2,
        "Scale load cell configuration"
    );

    addNumericConfigParam<int>(
        "hardware.sensors.scale.samples",
        "Scale Samples",
        kInteger,
        sHardwareSensorSection,
        107,
        nullptr,
        1, 20,
        "Number of samples to average for scale readings (higher = more stable but slower)"
    );

    addNumericConfigParam<double>(
        "hardware.sensors.scale.calibration",
        "Scale Calibration Factor",
        kDouble,
        sHardwareSensorSection,
        108,
        nullptr,
        -10000.0, 10000.0,
        "Primary scale calibration factor (adjust during calibration process)"
    );

    addNumericConfigParam<double>(
        "hardware.sensors.scale.calibration2",
        "Scale Calibration Factor 2",
        kDouble,
        sHardwareSensorSection,
        109,
        nullptr,
        -10000.0, 10000.0,
        "Secondary scale calibration factor (for dual load cell setups)"
    );

    addNumericConfigParam<double>(
        "hardware.sensors.scale.known_weight",
        "Known Calibration Weight",
        kDouble,
        sHardwareSensorSection,
        110,
        nullptr,
        1.0, 5000.0,
        "Weight in grams of the known calibration weight used for scale setup"
    );

    // clang-format on

    addParam(std::make_shared<Parameter>("VERSION", "Version", kCString, sOtherSection, 7, [] { return sysVersion; }, nullptr, 64, false, "", [] { return false; }, nullptr));

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