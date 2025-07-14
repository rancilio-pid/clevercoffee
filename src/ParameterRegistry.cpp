
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
extern double brewPidDelay;
extern bool useBDPID;
extern double aggbKp;
extern double aggbTn;
extern double aggbTv;
extern double emaFactor;
extern double steamSetpoint;
extern double targetBrewTime;
extern double preinfusion;
extern double preinfusionPause;
extern int backflushCycles;
extern double backflushFillTime;
extern double backflushFlushTime;
extern bool standbyModeOn;
extern double standbyModeTime;
extern bool featureFullscreenBrewTimer;
extern bool featureFullscreenManualFlushTimer;
extern bool featureFullscreenHotWaterTimer;
extern double postBrewTimerDuration;
extern bool featureHeatingLogo;
extern bool steamON;
extern bool backflushOn;
extern double temperature;
extern bool scaleTareOn;
extern bool scaleCalibrationOn;
extern int logLevel;
extern const char sysVersion[64];
extern bool includeDisplayInLogs;
extern bool timingDebugActive;

const char* switchTypes[2] = {"Momentary", "Toggle"};
const char* switchModes[2] = {"Normally Open", "Normally Closed"};
const char* relayTriggerTypes[2] = {"Low Trigger", "High Trigger"};
const char* dimmerTypes[2] = {"Pulse Skip Modulation", "Phase"};
const char* dimmerModes[4] = {"Power", "Pressure", "Flow", "Profile"};
const char* profileSelector[10] = {"Spring Lever", "Adaptive", "Londinium R24", "Londinium Vectis", "Light Roast", "Six Bar Espresso", "Gentle Bloom", "Pressurized Bloom", "Test Flow Step", "Test Flow Ramp"};

static constexpr const char* const brewModes[] = {"Manual", "Automatic"};
static constexpr const char* const displayTemplates[] = {"Standard", "Minimal", "Temp only", "Scale", "Upright"};
static constexpr const char* const displayLanguages[] = {"Deutsch", "English", "Español"};
static constexpr const char* const blinkingModes[] = {"Off", "Near Setpoint", "Away From Setpoint"};
static constexpr const char* const logLevels[] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "SILENT"};
static constexpr const char* const oledTypes[] = {"SH1106 (1.3\")", "SSD1306 (0.96\")"};
static constexpr const char* const oledAddresses[] = {"0x3C", "0x3D"};
static constexpr const char* const tempSensorTypes[] = {"TSIC306", "Dallas DS18B20"};
static constexpr const char* const scaleTypes[] = {"HX711 (2 load cell controllers)", "HX711 (1 load cell controller)", "Bluetooth"};

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
        101,
        &pidON,
        "Enables or disables the PID temperature controller"
    );

    addBoolConfigParam(
        "pid.use_ponm",
        "Enable PonM",
        sPIDSection,
        102,
        &usePonM,
        "Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>)"
    );

    addNumericConfigParam<double>(
        "pid.ema_factor",
        "PID EMA Factor",
        kDouble,
        sPIDSection,
        111,
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
        112,
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
        113,
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
        114,
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
        115,
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
        116,
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
        sTempSection,
        200,
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
        201,
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
        202,
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
        203,
        &steamSetpoint,
        STEAM_SETPOINT_MIN,
        STEAM_SETPOINT_MAX,
        "The temperature that the PID will use for steam mode"
    );

    // Brew Section
    if (config.get<bool>("hardware.switches.brew.enabled")) {
        addEnumConfigParam(
            "brew.mode",
            "Brew Mode",
            sBrewSection,
            301,
            nullptr,
            brewModes,
            2,
            "Manual mode gives you full control over the brew time while Automatic mode allows you to activate brew-by-time and/or brew-by-weight. The brew will then stop at whatever target is reached first."
        );

        addBoolConfigParam(
            "brew.by_time.enabled",
            "Brew by Time",
            sBrewSection,
            311,
            nullptr,
            "Enables brew by time, so the pump stops automatically when the target brew time is reached. Only available when Brew Mode is set to Automatic",
            [&config] { return config.get<int>("brew.mode") == 1; }
        );

        addNumericConfigParam<double>(
            "brew.by_time.target_time",
            "Target Brew Time (s)",
            kDouble,
            sBrewSection,
            312,
            &targetBrewTime,
            TARGET_BREW_TIME_MIN,
            TARGET_BREW_TIME_MAX,
            "Stop brew automatically after this amount of time",
            [&config] { return config.get<int>("brew.mode") == 1; }
        );

        if (config.get<bool>("hardware.sensors.scale.enabled")) {
            addBoolConfigParam(
                "brew.by_weight.enabled",
                "Brew by Weight",
                sBrewSection,
                321,
                nullptr,
                "Enables brew by weight, so the pump stops automatically when the target weight is reached. Only available when Brew Mode is set to Automatic",
                [&config] { return config.get<int>("brew.mode") == 1; }
            );

            addNumericConfigParam<double>(
                "brew.by_weight.target_weight",
                "Target Brew Weight (g)",
                kDouble,
                sBrewSection,
                322,
                nullptr,
                TARGET_BREW_WEIGHT_MIN,
                TARGET_BREW_WEIGHT_MAX,
                "Brew is running until this weight has been measured",
                [&config] { return config.get<int>("brew.mode") == 1; }
            );

            addBoolConfigParam(
                "brew.by_weight.auto_tare",
                "Auto-tare",
                sBrewSection,
                323,
                nullptr,
                "Enables auto-tare of a connected Bluetooth scale when a brew is started",
                [&config] { return config.get<int>("brew.mode") == 1 && config.get<int>("hardware.sensors.scale.type") == 2; }
            );
        }

        addBoolConfigParam(
            "brew.pre_infusion.enabled",
            "Pre-Infusion",
            sBrewSection,
            331,
            nullptr,
            "Enables pre-wetting of the coffee puck by turning on the pump for a configurable length of time."
        );

        addNumericConfigParam<double>(
            "brew.pre_infusion.time",
            "Pre-infusion Time (s)",
            kDouble,
            sBrewSection,
            332,
            &preinfusion,
            PRE_INFUSION_TIME_MIN,
            PRE_INFUSION_TIME_MAX,
            "Time in seconds the pump is running during the pre-infusion"
        );

        addNumericConfigParam<double>(
            "brew.pre_infusion.pause",
            "Pre-infusion Pause Time (s)",
            kDouble,
            sBrewSection,
            333,
            &preinfusionPause,
            PRE_INFUSION_PAUSE_MIN,
            PRE_INFUSION_PAUSE_MAX,
            "Pause to let the puck bloom after the initial pre-infusion while turning off the pump and leaving the 3-way valve open"
        );

        // Maintenance Section
        addNumericConfigParam<int>(
            "backflush.cycles",
            "Backflush Cycles",
            kInteger,
            sMaintenanceSection,
            401,
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
            402,
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
            403,
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
            501,
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
            502,
            [&]() -> bool {
                return scaleCalibrationOn;
            },
            [](const bool val) {
                scaleCalibrationOn = val;
            },
            false,
            "",
            [&config] { return config.get<int>("hardware.sensors.scale.type") < 2; },
            &scaleCalibrationOn
        ));
    }

    if (config.get<bool>("hardware.switches.brew.enabled")) {
        // Brew PID Section
        addBoolConfigParam(
            "pid.bd.enabled",
            "Enable Brew PID",
            sBrewPidSection,
            701,
            &useBDPID,
            "Use separate PID parameters while brew is running"
        );

        addNumericConfigParam<double>(
            "brew.pid_delay",
            "Brew PID Delay (s)",
            kDouble,
            sBrewPidSection,
            711,
            &brewPidDelay,
            BREW_PID_DELAY_MIN,
            BREW_PID_DELAY_MAX,
            "Delay time in seconds during which the PID will be disabled once a brew is detected. This prevents too high brew temperatures with boiler machines like Rancilio Silvia. Set to 0 for thermoblock machines."
        );

        addNumericConfigParam<double>(
            "pid.bd.kp",
            "BD Kp",
            kDouble,
            sBrewPidSection,
            712,
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
            713,
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
            714,
            &aggbTv,
            PID_TV_BD_MIN,
            PID_TV_BD_MAX,
            "Differential time constant (in seconds) for the PID when brewing has been detected."
        );
    }

    addBoolConfigParam(
        "dimmer.enabled",
        "Enable Pump Dimmer",
        sPumpPidSection,
        1401,
        nullptr,
        "Enable dimmer control of pump, requires hardware dimmer"
    );

    addBoolConfigParam(
        "dimmer.frequency",
        "Set 60hz",
        sPumpPidSection,
        1402,
        nullptr,
        "Enable if using 60hz, default 50hz",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addEnumConfigParam(
        "dimmer.type",
        "Dimmer Control Type",
        sPumpPidSection,
        1403,
        nullptr,
        dimmerTypes,
        2,
        "Software method of varying of dimmer. Pulse Skip has more accurate flow, while Phase is smoother but less accurate flow",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addEnumConfigParam(
        "dimmer.mode",
        "Dimmer Control Method",
        sPumpPidSection,
        1411,
        nullptr,
        dimmerModes,
        4,
        "Control setpoint the dimmer targets",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addEnumConfigParam(
        "dimmer.profile",
        "Dimmer Profile Selection",
        sPumpPidSection,
        1412,
        nullptr,
        profileSelector,
        10,
        "Profile to control the pump during brew",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.setpoint.pressure",
        "Pump Pressure Setpoint",
        kFloat,
        sPumpPidSection,
        1422,
        nullptr,
        PUMP_PRESSURE_SETPOINT_MIN,
        PUMP_PRESSURE_SETPOINT_MAX,
        "Pressure the PID controller will target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.setpoint.flow",
        "Pump Flow Setpoint",
        kFloat,
        sPumpPidSection,
        1423,
        nullptr,
        PUMP_FLOW_SETPOINT_MIN,
        PUMP_FLOW_SETPOINT_MAX,
        "Flow rate the PID controller will target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.setpoint.power",
        "Pump Power Setpoint",
        kFloat,
        sPumpPidSection,
        1421,
        nullptr,
        PUMP_POWER_SETPOINT_MIN,
        PUMP_POWER_SETPOINT_MAX,
        "Percent of output power the pump will run at",
        [&config] { return config.get<bool>("dimmer.enabled"); }
    );
        
    addNumericConfigParam<float>(
        "dimmer.psm.pressure.kp",
        "PSM Pressure Kp",
        kFloat,
        sPumpPidSection,
        1431,
        nullptr,
        PUMP_KP_MIN,
        PUMP_KP_MAX,
        "Proportional gain for Pulse Skip control with pressure target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.psm.pressure.ki",
        "PSM Pressure Ki",
        kFloat,
        sPumpPidSection,
        1432,
        nullptr,
        PUMP_KI_MIN,
        PUMP_KI_MAX,
        "Integral gain for Pulse Skip control with pressure target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.psm.pressure.kd",
        "PSM Pressure Kd",
        kFloat,
        sPumpPidSection,
        1433,
        nullptr,
        PUMP_KD_MIN,
        PUMP_KD_MAX,
        "Derivative gain for Pulse Skip control with pressure target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.psm.flow.kp",
        "PSM Flow Kp",
        kFloat,
        sPumpPidSection,
        1441,
        nullptr,
        PUMP_KP_MIN,
        PUMP_KP_MAX,
        "Proportional gain for Pulse Skip control with flow target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.psm.flow.ki",
        "PSM Flow Ki",
        kFloat,
        sPumpPidSection,
        1442,
        nullptr,
        PUMP_KI_MIN,
        PUMP_KI_MAX,
        "Integral gain for Pulse Skip control with flow target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.psm.flow.kd",
        "PSM Flow Kd",
        kFloat,
        sPumpPidSection,
        1443,
        nullptr,
        PUMP_KD_MIN,
        PUMP_KD_MAX,
        "Derivative gain for Pulse Skip control with flow target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.phase.pressure.kp",
        "Phase Pressure Kp",
        kFloat,
        sPumpPidSection,
        1451,
        nullptr,
        PUMP_KP_MIN,
        PUMP_KP_MAX,
        "Proportional gain for Phase control with pressure target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.phase.pressure.ki",
        "Phase Pressure Ki",
        kFloat,
        sPumpPidSection,
        1452,
        nullptr,
        PUMP_KI_MIN,
        PUMP_KI_MAX,
        "Integral gain for Phase control with pressure target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.phase.pressure.kd",
        "Phase Pressure Kd",
        kFloat,
        sPumpPidSection,
        1453,
        nullptr,
        PUMP_KD_MIN,
        PUMP_KD_MAX,
        "Derivative gain for Phase control with pressure target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.phase.flow.kp",
        "Phase Flow Kp",
        kFloat,
        sPumpPidSection,
        1461,
        nullptr,
        PUMP_KP_MIN,
        PUMP_KP_MAX,
        "Proportional gain for Phase control with flow target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.phase.flow.ki",
        "Phase Flow Ki",
        kFloat,
        sPumpPidSection,
        1462,
        nullptr,
        PUMP_KI_MIN,
        PUMP_KI_MAX,
        "Integral gain for Phase control with flow target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.phase.flow.kd",
        "Phase Flow Kd",
        kFloat,
        sPumpPidSection,
        1463,
        nullptr,
        PUMP_KD_MIN,
        PUMP_KD_MAX,
        "Derivative gain for Phase control with flow target",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.i_max",
        "Pump PID Integrator Max",
        kFloat,
        sPumpPidSection,
        1471,
        nullptr,
        PUMP_I_MAX_MIN,
        PUMP_I_MAX_MAX,
        "Limit on the integration accumulator",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.calibration.flow_rate1",
        "Flow rate calibration no pressure",
        kFloat,
        sPumpPidSection,
        1481,
        nullptr,
        PUMP_CALIBRATION_FLOW_MIN,
        PUMP_CALIBRATION_FLOW_MAX,
        "Water flow in 30s from group head, use brew or flush function. Requires a restart to apply",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.calibration.flow_rate2",
        "Flow rate calibration OPV pressure",
        kFloat,
        sPumpPidSection,
        1482,
        nullptr,
        PUMP_CALIBRATION_FLOW_MIN,
        PUMP_CALIBRATION_FLOW_MAX,
        "Water flow in 30s from return line, use water switch function. Requires a restart to apply",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );

    addNumericConfigParam<float>(
        "dimmer.calibration.opv_pressure",
        "OPV Pressure",
        kFloat,
        sPumpPidSection,
        1483,
        nullptr,
        PUMP_PRESSURE_SETPOINT_MIN, 
        PUMP_PRESSURE_SETPOINT_MAX,
        "Pressure sensor value when water switch is active and water is returning to the tank. Requires a restart to apply",
        [&config] { return config.get<bool>("dimmer.enabled") && config.get<bool>("hardware.sensors.pressure.enabled"); }
    );


    // Other Section (special parameters, e.g. runtime-only toggles)
    addParam(std::make_shared<Parameter>(
        "STEAM_MODE",
        "Steam Mode",
        kUInt8,
        sOtherSection,
        503,
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
            504,
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
        801,
        &standbyModeOn,
        "Turn heater off after standby time has elapsed."
    );

    addNumericConfigParam<double>(
        "standby.time",
        "Standby Time",
        kDouble,
        sPowerSection,
        802,
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
        901,
        nullptr,
        displayTemplates,
        5,
        "Set the display template",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "display.inverted",
        "Invert Display",
        sDisplaySection,
        902,
        nullptr,
        "Set the display rotation",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "display.language",
        "Display Language",
        sDisplaySection,
        903,
        nullptr,  // No global variable for this parameter
        displayLanguages,
        3,
        "Set the language for the OLED display",
        [] { return true; },
    true
    );

    addBoolConfigParam(
        "display.fullscreen_brew_timer",
        "Enable Fullscreen Brew Timer",
        sDisplaySection,
        904,
        &featureFullscreenBrewTimer,
        "Enable fullscreen overlay during brew"
    );

    addBoolConfigParam(
        "display.blescale_brew_timer",
        "Enable BLE Scale Brew Timer",
        sDisplaySection,
        905,
        nullptr,
        "Enable starting and stopping the brew timer on a connected BLE scale."
            "Note that there might be a certain delay between the command being sent and the timer on the scale actually starting."
            "Consider disabling the internal brew timer if you want to use this feature.",
        [&config] { return config.get<int>("hardware.sensors.scale.type") == 2; }
    );

    addBoolConfigParam(
        "display.fullscreen_manual_flush_timer",
        "Enable Fullscreen Manual Flush Timer",
        sDisplaySection,
        906,
        &featureFullscreenManualFlushTimer,
        "Enable fullscreen overlay during manual flush"
    );

    addBoolConfigParam(
        "display.fullscreen_hot_water_timer",
        "Enable Fullscreen Hot Water Timer",
        sDisplaySection,
        907,
        &featureFullscreenHotWaterTimer,
        "Enable fullscreen overlay during hot water mode"
    );

    addNumericConfigParam(
        "display.post_brew_timer_duration",
        "Post Brew Timer Duration (s)",
        kDouble,
        sDisplaySection,
        908,
        &postBrewTimerDuration,
        POST_BREW_TIMER_DURATION_MIN,
        POST_BREW_TIMER_DURATION_MAX,
        "time in s that brew timer will be shown after brew finished"
    );

    addBoolConfigParam(
        "display.heating_logo",
        "Enable Heating Logo",
        sDisplaySection,
        909,
        &featureHeatingLogo,
        "full screen logo will be shown if temperature is 5°C below setpoint"
    );

    addEnumConfigParam(
        "display.blinking.mode",
        "Set temperature display blinking",
        sDisplaySection,
        910,
        nullptr,
        blinkingModes,
        3,
        "Enable blinking of temperature based on distance to setpoint"
    );

    addNumericConfigParam<double>(
        "display.blinking.delta",
        "Delta to activate blinking",
        kDouble,
        sDisplaySection,
        911,
        nullptr,
        0.2,
        10,
        "Delta from setpoint for blinking temperature display"
    );

    // MQTT section
    addBoolConfigParam(
        "mqtt.enabled",
        "MQTT enabled",
        sMqttSection,
        1001,
        nullptr,
        "Enables MQTT connectivity",
        [] { return true; },
        true
    );

    addStringConfigParam(
        "mqtt.broker",
        "Hostname",
        sMqttSection,
        1011,
        nullptr,
        MQTT_BROKER_MAX_LENGTH,
        "IP addresss or hostname of your MQTT broker",
        [] { return true; },
        true
    );

    addNumericConfigParam<int>(
        "mqtt.port",
        "Port",
        kInteger,
        sMqttSection,
        1012,
        nullptr,
        1,
        65535,
        "Port number of your MQTT broker",
        [] { return true; },
        true
    );

    addStringConfigParam(
        "mqtt.username",
        "Username",
        sMqttSection,
        1013,
        nullptr,
        USERNAME_MAX_LENGTH,
        "Username for your MQTT broker",
        [] { return true; },
        true
    );

    addStringConfigParam(
        "mqtt.password",
        "Password",
        sMqttSection,
        1014,
        nullptr,
        PASSWORD_MAX_LENGTH,
        "Password for your MQTT broker",
        [] { return true; },
        true
    );

    addStringConfigParam(
        "mqtt.topic",
        "Topic Prefix",
        sMqttSection,
        1015,
        nullptr,
        MQTT_TOPIC_MAX_LENGTH,
        "Custom MQTT topic prefix",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "mqtt.hassio.enabled",
        "Hass.io enabled",
        sMqttSection,
        1021,
        nullptr,
        "Enables Home Assistant integration",
        [] { return true; },
        true
    );

    addStringConfigParam(
        "mqtt.hassio.prefix",
        "Hass.io Prefix",
        sMqttSection,
        1022,
        nullptr,
        MQTT_HASSIO_PREFIX_MAX_LENGTH,
        "Custom MQTT topic prefix",
        [] { return true; },
        true
    );

    addStringConfigParam(
        "system.hostname",
        "Hostname",
        sSystemSection,
        1101,
        nullptr,
        HOSTNAME_MAX_LENGTH,
        "Hostname of your machine",
        [] { return true; },
        true
    );

    addStringConfigParam(
        "system.ota_password",
        "OTA Password",
        sSystemSection,
        1102,
        nullptr,
        PASSWORD_MAX_LENGTH,
        "Password for over-the-air updates",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "system.log_level",
        "Log Level",
        sSystemSection,
        1103,
        &logLevel,
        logLevels,
        7,
        "Set the logging verbosity level"
    );

    addBoolConfigParam(
        "system.auth.enabled",
        "Enable Website Authentication",
        sSystemSection,
        1201,
        nullptr,
        "Enables authentication for accessing certain parts of the website and for web requests in general. "
            "This setting secures the calls to sensitive url endpoints, e.g. for config parameters, hardware settings, factory reset, etc."
    );

    addStringConfigParam(
        "system.auth.username",
        "Website Username",
        sSystemSection,
        1202,
        nullptr,
        USERNAME_MAX_LENGTH,
        "Username for accessing the website and authenticating web requests"
    );

    addStringConfigParam(
        "system.auth.password",
        "Website Password",
        sSystemSection,
        1203,
        nullptr,
        PASSWORD_MAX_LENGTH,
        "Password for accessing the website and authenticating web requests"
    );

    addBoolConfigParam(
        "system.offline_mode",
        "Offline Mode",
        sSystemSection,
        1204,
        nullptr,
        "Disable wifi and start an access point to display the website"
    );

    // Debugging Checkboxes
    addBoolConfigParam(
        "system.timing_debug.enabled",
        "Loop timing in console",
        sSystemSection,
        1301,
        &timingDebugActive,
        "Enable or disable the process loop time debugging in console.<br>"
        "r=draw display buffer<br>"
        "D=display refresh<br>"
        "W=website<br>"
        "M=MQTT<br>"
        "H=hassio<br>"
        "T=temperature",
        [&config] { return config.get<int>("system.log_level") == static_cast<int>(Logger::Level::DEBUG); }
    );

    addBoolConfigParam(
        "system.showdisplay.enabled",
        "Activate display recording in debug logs",
        sSystemSection,
        1303,
        &includeDisplayInLogs,
        "Enable or disable showing sendBuffer loops in debug logs",
        [&config] { return config.get<int>("system.log_level") == static_cast<int>(Logger::Level::DEBUG); }
    );
        
    addBoolConfigParam(
        "system.show_brewdata.enabled",
        "Enable brew data logs",
        sSystemSection,
        1304,
        nullptr,
        "Enable arrays of brew data in debug logs if dimmer is enabled",
        [&config] { return config.get<int>("system.log_level") == static_cast<int>(Logger::Level::DEBUG); }
    );

    // Hardware section

    // OLED
    addBoolConfigParam(
        "hardware.oled.enabled",
        "Enable OLED Display",
        sHardwareOledSection,
        2001,
        nullptr,
        "Enable or disable the OLED display",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.oled.type",
        "OLED Type",
        sHardwareOledSection,
        2002,
        nullptr,
        oledTypes,
        2,
        "Select your OLED display type",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.oled.address",
        "I2C Address",
        sHardwareOledSection,
        2003,
        nullptr,
        oledAddresses,
        2,
        "I2C address of the OLED display, should be 0x3C in most cases, if in doubt check the datasheet",
        [] { return true; },
        true
    );

    // Relays
    addEnumConfigParam(
        "hardware.relays.heater.trigger_type",
        "Heater Relay Trigger Type",
        sHardwareRelaySection,
        2101,
        nullptr,
        relayTriggerTypes,
        2,
        "Relay trigger type for heater control",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.relays.valve.trigger_type",
        "Valve Relay Trigger Type",
        sHardwareRelaySection,
        2102,
        nullptr,
        relayTriggerTypes,
        2,
        "Relay trigger type for valve control",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.relays.pump.trigger_type",
        "Pump Relay Trigger Type",
        sHardwareRelaySection,
        2103,
        nullptr,
        (const char* const[]){"Low Trigger", "High Trigger"},
        2,
        "Relay trigger type for pump control",
        [] { return true; },
        true
    );

    // Switches
    addBoolConfigParam(
        "hardware.switches.brew.enabled",
        "Enable Brew Switch",
        sHardwareSwitchSection,
        2201,
        nullptr,
        "Enable physical brew switch",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.switches.brew.type",
        "Brew Switch Type",
        sHardwareSwitchSection,
        2202,
        nullptr,
        switchTypes,
        2,
        "Type of brew switch connected",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.switches.brew.mode",
        "Brew Switch Mode",
        sHardwareSwitchSection,
        2203,
        nullptr,
        switchModes,
        2,
        "Electrical configuration of brew switch<br>Normally Open is active high<br>Normally Closed is active low",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.switches.steam.enabled",
        "Enable Steam Switch",
        sHardwareSwitchSection,
        2211,
        nullptr,
        "Enable physical steam switch",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.switches.steam.type",
        "Steam Switch Type",
        sHardwareSwitchSection,
        2212,
        nullptr,
        switchTypes,
        2,
        "Type of steam switch connected",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.switches.steam.mode",
        "Steam Switch Mode",
        sHardwareSwitchSection,
        2213,
        nullptr,
        switchModes,
        2,
        "Electrical configuration of steam switch<br>Normally Open is active high<br>Normally Closed is active low",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.switches.power.enabled",
        "Enable Power Switch",
        sHardwareSwitchSection,
        2221,
        nullptr,
        "Enable physical power switch",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.switches.power.type",
        "Power Switch Type",
        sHardwareSwitchSection,
        2222,
        nullptr,
        switchTypes,
        2,
        "Type of power switch connected",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.switches.power.mode",
        "Power Switch Mode",
        sHardwareSwitchSection,
        2223,
        nullptr,
        switchModes,
        2,
        "Electrical configuration of power switch<br>Normally Open is active high<br>Normally Closed is active low",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.switches.hot_water.enabled",
        "Enable Water Switch",
        sHardwareSwitchSection,
        2231,
        nullptr,
        "Enable physical water switch",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.switches.hot_water.type",
        "Water Switch Type",
        sHardwareSwitchSection,
        2232,
        nullptr,
        switchTypes,
        2,
        "Type of water switch connected",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.switches.hot_water.mode",
        "Water Switch Mode",
        sHardwareSwitchSection,
        2233,
        nullptr,
        switchModes,
        2,
        "Electrical configuration of water switch<br>Normally Open is active high<br>Normally Closed is active low",
        [] { return true; },
        true
    );

    // LEDs
    addBoolConfigParam(
        "hardware.leds.status.enabled",
        "Enable Status LED",
        sHardwareLedSection,
        2301,
        nullptr,
        "Enable status indicator LED",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.leds.status.inverted",
        "Invert Status LED",
        sHardwareLedSection,
        2302,
        nullptr,
        "Invert the status LED logic (for common anode LEDs)",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.leds.brew.enabled",
        "Enable Brew LED",
        sHardwareLedSection,
        2311,
        nullptr,
        "Enable brew indicator LED",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.leds.brew.inverted",
        "Invert Brew LED",
        sHardwareLedSection,
        2312,
        nullptr,
        "Invert the brew LED logic (for common anode LEDs)",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.leds.steam.enabled",
        "Enable Steam LED",
        sHardwareLedSection,
        2321,
        nullptr,
        "Enable steam indicator LED",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.leds.steam.inverted",
        "Invert Steam LED",
        sHardwareLedSection,
        2322,
        nullptr,
        "Invert the steam LED logic (for common anode LEDs)",
        [] { return true; },
        true
    );

    // Sensors
    addEnumConfigParam(
        "hardware.sensors.temperature.type",
        "Temperature Sensor Type",
        sHardwareSensorSection,
        2401,
        nullptr,
        tempSensorTypes,
        2,
        "Type of temperature sensor connected",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.sensors.pressure.enabled",
        "Enable Pressure Sensor",
        sHardwareSensorSection,
        2411,
        nullptr,
        "Enable pressure sensor for monitoring brew pressure",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.sensors.watertank.enabled",
        "Enable Water Tank Sensor",
        sHardwareSensorSection,
        2421,
        nullptr,
        "Enable water tank level sensor",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.sensors.watertank.mode",
        "Water Tank Sensor Mode",
        sHardwareSensorSection,
        2422,
        nullptr,
        switchModes,
        2,
        "Electrical configuration of water tank sensor",
        [] { return true; },
        true
    );

    addBoolConfigParam(
        "hardware.sensors.scale.enabled",
        "Enable Scale",
        sHardwareSensorSection,
        2431,
        nullptr,
        "Enable integrated scale for weight-based brewing",
        [] { return true; },
        true
    );

    addEnumConfigParam(
        "hardware.sensors.scale.type",
        "Scale Type",
        sHardwareSensorSection,
        2432,
        nullptr,
        scaleTypes,
        3,
        "Integrated HX711-based scale with different load cell configurations or Bluetooth Low Energy scales",
        [] { return true; },
        true
    );

    addNumericConfigParam<int>(
        "hardware.sensors.scale.samples",
        "Scale Samples",
        kInteger,
        sHardwareSensorSection,
        2433,
        nullptr,
        SCALE_SAMPLES_MIN, SCALE_SAMPLES_MAX,
        "Number of samples to average for scale readings (higher = more stable but slower)",
        [&config] { return config.get<int>("hardware.sensors.scale.type") < 2; },
        true
    );

    addNumericConfigParam<double>(
        "hardware.sensors.scale.calibration",
        "Scale Calibration Factor",
        kDouble,
        sHardwareSensorSection,
        2434,
        nullptr,
        SCALE_CALIBRATION_MIN, SCALE_CALIBRATION_MAX,
        "Primary scale calibration factor (adjust during calibration process)",
        [&config] { return config.get<int>("hardware.sensors.scale.type") < 2; }
    );

    addNumericConfigParam<double>(
        "hardware.sensors.scale.calibration2",
        "Scale Calibration Factor 2",
        kDouble,
        sHardwareSensorSection,
        2435,
        nullptr,
        SCALE_CALIBRATION_MIN, SCALE_CALIBRATION_MAX,
        "Secondary scale calibration factor (for dual load cell setups)",
        [&config] { return config.get<int>("hardware.sensors.scale.type") == 0; }
    );

    addNumericConfigParam<double>(
        "hardware.sensors.scale.known_weight",
        "Known Calibration Weight",
        kDouble,
        sHardwareSensorSection,
        2436,
        nullptr,
        SCALE_KNOWN_WEIGHT_MIN, SCALE_KNOWN_WEIGHT_MAX,
        "Weight in grams of the known calibration weight used for scale setup",
        [&config] { return config.get<int>("hardware.sensors.scale.type") < 2; }
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
