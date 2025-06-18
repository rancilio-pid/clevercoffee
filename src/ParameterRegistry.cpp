
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

    // Add all parameters
    auto addParam = [this](const std::shared_ptr<Parameter>& param) {
        _parameters.push_back(param);
        _parameterMap[param->getId()] = param;
    };

    // PID Section
    auto pid_enabled = "pid.enabled";
    addParam(std::make_shared<Parameter>(
        pid_enabled, "Enable PID Controller", kUInt8, sPIDSection, 1,
        [&config, pid_enabled]() -> bool {
            pidON = config.get<bool>(pid_enabled);
            return pidON;
        },
        [&config, pid_enabled](const bool val) {
            config.set<bool>(pid_enabled, val);
            pidON = val;
        },
        false, "", [] { return true; }, &pidON));

    auto pid_use_ponm = "pid.use_ponm";
    addParam(std::make_shared<Parameter>(
        pid_use_ponm, "Enable PonM", kUInt8, sPIDSection, 2,
        [&config, pid_use_ponm]() -> bool {
            usePonM = config.get<bool>(pid_use_ponm);
            return usePonM;
        },
        [&config, pid_use_ponm](const bool val) {
            config.set<bool>(pid_use_ponm, val);
            usePonM = val;
        },
        true, "Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>)", [] { return true; }, &usePonM));

    auto pid_ema_factor = "pid.ema_factor";
    addParam(std::make_shared<Parameter>(
        pid_ema_factor, "PID EMA Factor", kDouble, sPIDSection, 3,
        [&config, pid_ema_factor] {
            emaFactor = config.get<double>(pid_ema_factor);
            return emaFactor;
        },
        [&config, pid_ema_factor](const double val) {
            config.set<double>(pid_ema_factor, val);
            emaFactor = val;
        },
        PID_EMA_FACTOR_MIN, PID_EMA_FACTOR_MAX, true, "Smoothing of input that is used for Tv (derivative component of PID). Smaller means less smoothing but also less delay, 0 means no filtering", [] { return true; },
        &emaFactor));

    auto pid_regular_kp = "pid.regular.kp";
    addParam(std::make_shared<Parameter>(
        pid_regular_kp, "PID Kp", kDouble, sPIDSection, 4,
        [&config, pid_regular_kp] {
            aggKp = config.get<double>(pid_regular_kp);
            return aggKp;
        },
        [&config, pid_regular_kp](const double val) {
            config.set<double>(pid_regular_kp, val);
            aggKp = val;
        },
        PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, true,
        "Proportional gain (in Watts/C°) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The higher this value is, the higher is the "
        "output of the heater for a given temperature difference. E.g. 5°C difference will result in P*5 Watts of heater output.",
        [] { return true; }, &aggKp));

    auto pid_regular_tn = "pid.regular.tn";
    addParam(std::make_shared<Parameter>(
        pid_regular_tn, "PID Tn (=Kp/Ki)", kDouble, sPIDSection, 5,
        [&config, pid_regular_tn] {
            aggTn = config.get<double>(pid_regular_tn);
            return aggTn;
        },
        [&config, pid_regular_tn](const double val) {
            config.set<double>(pid_regular_tn, val);
            aggTn = val;
        },
        PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, true,
        "Integral time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The larger this value is, the slower the "
        "integral part of the PID will increase (or decrease) if the process value remains above (or below) the setpoint in spite of proportional action. The smaller this value, the faster the integral term changes.",
        [] { return true; }, &aggTn));

    auto pid_regular_tv = "pid.regular.tv";
    addParam(std::make_shared<Parameter>(
        pid_regular_tv, "PID Tv (=Kd/Kp)", kDouble, sPIDSection, 6,
        [&config, pid_regular_tv] {
            aggTv = config.get<double>(pid_regular_tv);
            return aggTv;
        },
        [&config, pid_regular_tv](const double val) {
            config.set<double>(pid_regular_tv, val);
            aggTv = val;
        },
        PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, true,
        "Differential time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). This value determines how far the "
        "PID equation projects the current trend into the future. The higher the value, the greater the dampening. Select it carefully, it can cause oscillations if it is set too high or too low.",
        [] { return true; }, &aggTv));

    auto pid_regular_i_max = "pid.regular.i_max";
    addParam(std::make_shared<Parameter>(
        pid_regular_i_max, "PID Integrator Max", kDouble, sPIDSection, 7,
        [&config, pid_regular_i_max] {
            aggIMax = config.get<double>(pid_regular_i_max);
            return aggIMax;
        },
        [&config, pid_regular_i_max](const double val) {
            config.set<double>(pid_regular_i_max, val);
            aggIMax = val;
        },
        PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, true,
        "Internal integrator limit to prevent windup (in Watts). This will allow the integrator to only grow to the specified value. This should be approximally equal to the output needed to hold the temperature after the "
        "setpoint has been reached and is depending on machine type and whether the boiler is insulated or not.",
        [] { return true; }, &aggIMax));

    auto pid_steam_kp = "pid.steam.kp";
    addParam(std::make_shared<Parameter>(
        pid_steam_kp, "Steam Kp", kDouble, sPIDSection, 8,
        [&config, pid_steam_kp] {
            steamKp = config.get<double>(pid_steam_kp);
            return steamKp;
        },
        [&config, pid_steam_kp](const double val) {
            config.set<double>(pid_steam_kp, val);
            steamKp = val;
        },
        PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, true, "Proportional gain for the steaming mode (I or D are not used)", [] { return true; }, &steamKp));

    addParam(std::make_shared<Parameter>("TEMP", "Temperature", kDouble, sPIDSection, 9, [&] { return temperature; }, [](const double val) { temperature = val; }, 0.0, 200.0, false, "", [] { return false; }, &temperature));

    // Temperature Section
    auto brew_setpoint = "brew.setpoint";
    addParam(std::make_shared<Parameter>(
        brew_setpoint, "Setpoint (°C)", kDouble, sTempSection, 10,
        [&config, brew_setpoint] {
            brewSetpoint = config.get<double>(brew_setpoint);
            return brewSetpoint;
        },
        [&config, brew_setpoint](const double val) {
            config.set<double>(brew_setpoint, val);
            brewSetpoint = val;
        },
        BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, true, "The temperature that the PID will attempt to reach and hold", [] { return true; }, &brewSetpoint));

    auto brew_temp_offset = "brew.temp_offset";
    addParam(std::make_shared<Parameter>(
        brew_temp_offset, "Offset (°C)", kDouble, sTempSection, 11,
        [&config, brew_temp_offset] {
            brewTempOffset = config.get<double>(brew_temp_offset);
            return brewTempOffset;
        },
        [&config, brew_temp_offset](const double val) {
            config.set<double>(brew_temp_offset, val);
            brewTempOffset = val;
        },
        BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, true,
        "Optional offset that is added to the user-visible setpoint. Can be used to compensate sensor offsets and the average temperature loss between boiler and group so that the setpoint represents the approximate brew "
        "temperature.",
        [] { return true; }, &brewTempOffset));

    auto steam_setpoint = "steam.setpoint";
    addParam(std::make_shared<Parameter>(
        "steam.setpoint", "Steam Setpoint (°C)", kDouble, sTempSection, 12,
        [&config, steam_setpoint] {
            steamSetpoint = config.get<double>(steam_setpoint);
            return steamSetpoint;
        },
        [&config, steam_setpoint](const double val) {
            config.set<double>(steam_setpoint, val);
            steamSetpoint = val;
        },
        STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, true, "The temperature that the PID will use for steam mode", [] { return true; }, &steamSetpoint));

    // Brew Section
    auto features_brew_control = "features.brew_control";
    addParam(std::make_shared<Parameter>(
        "features.brew_control", "Brew Control", kUInt8, sBrewSection, 13,
        [&config, features_brew_control]() -> bool {
            featureBrewControl = config.get<bool>(features_brew_control);
            return featureBrewControl;
        },
        [&config, features_brew_control](const bool val) {
            config.set<bool>(features_brew_control, val);
            featureBrewControl = val;
        },
        true, "Enables brew-by-time or brew-by-weight", [] { return true; }, &featureBrewControl));

    auto brew_target_time = "brew.target_time";
    addParam(std::make_shared<Parameter>(
        brew_target_time, "Target Brew Time (s)", kDouble, sBrewSection, 14,
        [&config, brew_target_time] {
            targetBrewTime = config.get<double>(brew_target_time);
            return targetBrewTime;
        },
        [&config, brew_target_time](const double val) {
            config.set<double>(brew_target_time, val);
            targetBrewTime = val;
        },
        TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX, true, "Stop brew after this time. Set to 0 to deactivate brew-by-time-feature.", [] { return featureBrewControl == 1; }, &targetBrewTime));

    auto brew_pre_infusion_pause = "brew.pre_infusion.pause";
    addParam(std::make_shared<Parameter>(
        brew_pre_infusion_pause, "Preinfusion Pause Time (s)", kDouble, sBrewSection, 15,
        [&config, brew_pre_infusion_pause] {
            preinfusionPause = config.get<double>(brew_pre_infusion_pause);
            return preinfusionPause;
        },
        [&config, brew_pre_infusion_pause](const double val) {
            config.set<double>(brew_pre_infusion_pause, val);
            preinfusionPause = val;
        },
        PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, false, "", [] { return featureBrewControl == 1; }, &preinfusionPause));

    auto brew_pre_infusion_time = "brew.pre_infusion.time";
    addParam(std::make_shared<Parameter>(
        brew_pre_infusion_time, "Preinfusion Time (s)", kDouble, sBrewSection, 16,
        [&config, brew_pre_infusion_time] {
            preinfusion = config.get<double>(brew_pre_infusion_time);
            return preinfusion;
        },
        [&config, brew_pre_infusion_time](const double val) {
            config.set<double>(brew_pre_infusion_time, val);
            preinfusion = val;
        },
        PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, false, "", [] { return featureBrewControl == 1; }, &preinfusion));

    // Maintenance Section
    auto backflush_cycles = "backflush.cycles";
    addParam(std::make_shared<Parameter>(
        backflush_cycles, "Backflush Cycles", kInteger, sMaintenanceSection, 17,
        [&config, backflush_cycles] {
            backflushCycles = config.get<int>(backflush_cycles);
            return backflushCycles;
        },
        [&config, backflush_cycles](const double val) {
            config.set<int>(backflush_cycles, static_cast<int>(val));
            backflushCycles = static_cast<int>(val);
        },
        BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX, true, "Number of cycles of filling and flushing during a backflush", [] { return featureBrewControl == 1; }, &backflushCycles));

    auto backflush_fill_time = "backflush.fill_time";
    addParam(std::make_shared<Parameter>(
        backflush_fill_time, "Backflush Fill Time (s)", kDouble, sMaintenanceSection, 18,
        [&config, backflush_fill_time] {
            backflushFillTime = config.get<double>(backflush_fill_time);
            return backflushFillTime;
        },
        [&config, backflush_fill_time](const double val) {
            config.set<double>(backflush_fill_time, val);
            backflushFillTime = val;
        },
        BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX, true, "Time in seconds the pump is running during one backflush cycle", [] { return featureBrewControl == 1; }, &backflushFillTime));

    auto backflush_flush_time = "backflush.flush_time";
    addParam(std::make_shared<Parameter>(
        backflush_flush_time, "Backflush Flush Time (s)", kDouble, sMaintenanceSection, 19,
        [&config, backflush_flush_time] {
            backflushFlushTime = config.get<double>(backflush_flush_time);
            return backflushFlushTime;
        },
        [&config, backflush_flush_time](const double val) {
            config.set<double>(backflush_flush_time, val);
            backflushFlushTime = val;
        },
        BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX, true, "Time in seconds the selenoid valve stays open during one backflush cycle", [] { return featureBrewControl == 1; }, &backflushFlushTime));

    if (config.get<bool>("hardware.sensors.scale.enabled")) {
        auto brew_target_weight = "brew.target_weight";
        addParam(std::make_shared<Parameter>(
            brew_target_weight, "Brew weight target (g)", kDouble, sBrewSection, 20,
            [&config, brew_target_weight] {
                targetBrewWeight = config.get<double>(brew_target_weight);
                return targetBrewWeight;
            },
            [&config, brew_target_weight](const double val) {
                config.set<double>(brew_target_weight, val);
                targetBrewWeight = val;
            },
            TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX, true, "Brew is running until this weight has been measured. Set to 0 to deactivate brew-by-weight-feature.", [] { return featureBrewControl == 1; },
            &targetBrewWeight));

        addParam(std::make_shared<Parameter>("TARE_ON", "Tare", kUInt8, sOtherSection, 21, [&]() -> bool { return scaleTareOn; }, [](const bool val) { scaleTareOn = val; }, false, "", [] { return true; }, &scaleTareOn));

        addParam(std::make_shared<Parameter>(
            "CALIBRATION_ON", "Calibration", kUInt8, sOtherSection, 22, [&]() -> bool { return scaleCalibrationOn; }, [](const bool val) { scaleCalibrationOn = val; }, false, "", [] { return true; }, &scaleCalibrationOn));

        auto hardware_sensors_scale_known_weight = "hardware.sensors.scale.known_weight";
        addParam(std::make_shared<Parameter>(
            hardware_sensors_scale_known_weight, "Known weight in g", kFloat, sScaleSection, 23,
            [&config, hardware_sensors_scale_known_weight] {
                scaleKnownWeight = config.get<float>(hardware_sensors_scale_known_weight);
                return scaleKnownWeight;
            },
            [&config, hardware_sensors_scale_known_weight](const double val) {
                config.set<float>(hardware_sensors_scale_known_weight, static_cast<float>(val));
                scaleKnownWeight = static_cast<float>(val);
            },
            0.0, 2000.0, false, "", [] { return true; }, &scaleKnownWeight));

        auto hardware_sensors_scale_calibration = "hardware.sensors.scale.calibration";
        addParam(std::make_shared<Parameter>(
            hardware_sensors_scale_calibration, "Calibration factor scale 1", kFloat, sScaleSection, 24,
            [&config, hardware_sensors_scale_calibration] {
                scaleCalibration = config.get<float>(hardware_sensors_scale_calibration);
                return scaleCalibration;
            },
            [&config, hardware_sensors_scale_calibration](const double val) {
                config.set<float>(hardware_sensors_scale_calibration, static_cast<float>(val));
                scaleCalibration = static_cast<float>(val);
            },
            -100000, 100000, false, "", [] { return true; }, &scaleCalibration));

        auto hardware_sensors_scale_calibration2 = "hardware.sensors.scale.calibration2";
        addParam(std::make_shared<Parameter>(
            hardware_sensors_scale_calibration2, "Calibration factor scale 2", kFloat, sScaleSection, 25,
            [&config, hardware_sensors_scale_calibration2] {
                scale2Calibration = config.get<float>(hardware_sensors_scale_calibration2);
                return scale2Calibration;
            },
            [&config, hardware_sensors_scale_calibration2](const double val) {
                config.set<float>(hardware_sensors_scale_calibration2, static_cast<float>(val));
                scale2Calibration = static_cast<float>(val);
            },
            -100000, 100000, false, "", [&config] { return config.get<int>("hardware.sensors.scale.type") == 0; }, &scale2Calibration));
    }

    // Brew PID Section
    auto pid_bd_enabled = "pid.bd.enabled";
    addParam(std::make_shared<Parameter>(
        pid_bd_enabled, "Enable Brew PID", kUInt8, sBrewPidSection, 26,
        [&config, pid_bd_enabled]() -> bool {
            useBDPID = config.get<bool>(pid_bd_enabled);
            return useBDPID;
        },
        [&config, pid_bd_enabled](const bool val) {
            config.set<bool>(pid_bd_enabled, val);
            useBDPID = val;
        },
        true, "Use separate PID parameters while brew is running", [] { return true; }, &useBDPID));

    auto brew_pid_delay = "brew.pid_delay";
    addParam(std::make_shared<Parameter>(
        brew_pid_delay, "Brew PID Delay (s)", kDouble, sBrewPidSection, 27,
        [&config, brew_pid_delay] {
            brewPIDDelay = config.get<double>(brew_pid_delay);
            return brewPIDDelay;
        },
        [&config, brew_pid_delay](const double val) {
            config.set<double>(brew_pid_delay, val);
            brewPIDDelay = val;
        },
        BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, true,
        "Delay time in seconds during which the PID will be disabled once a brew is detected. This prevents too high brew temperatures with boiler machines like Rancilio Silvia. Set to 0 for thermoblock machines.",
        [] { return true; }, &brewPIDDelay));

    auto pid_bd_kp = "pid.bd.kp";
    addParam(std::make_shared<Parameter>(
        pid_bd_kp, "BD Kp", kDouble, sBrewPidSection, 28,
        [&config, pid_bd_kp] {
            aggbKp = config.get<double>(pid_bd_kp);
            return aggbKp;
        },
        [&config, pid_bd_kp](const double val) {
            config.set<double>(pid_bd_kp, val);
            aggbKp = val;
        },
        PID_KP_BD_MIN, PID_KP_BD_MAX, true,
        "Proportional gain (in Watts/°C) for the PID when brewing has been detected. Use this controller to either increase heating during the brew to counter temperature drop from fresh cold water in the boiler. Some "
        "machines, e.g. Rancilio Silvia, actually need to heat less or not at all during the brew because of high temperature stability (<a "
        "href='https://www.kaffee-netz.de/threads/installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/#post-1453641' target='_blank'>Details<a>)",
        [] { return useBDPID == 1; }, &aggbKp));

    auto pid_bd_tn = "pid.bd.tn";
    addParam(std::make_shared<Parameter>(
        pid_bd_tn, "BD Tn (=Kp/Ki)", kDouble, sBrewPidSection, 29,
        [&config, pid_bd_tn] {
            aggbTn = config.get<double>(pid_bd_tn);
            return aggbTn;
        },
        [&config, pid_bd_tn](const double val) {
            config.set<double>(pid_bd_tn, val);
            aggbTn = val;
        },
        PID_TN_BD_MIN, PID_TN_BD_MAX, true, "Integral time constant (in seconds) for the PID when brewing has been detected.", [] { return useBDPID == 1; }, &aggbTn));

    auto pid_bd_tv = "pid.bd.tv";
    addParam(std::make_shared<Parameter>(
        pid_bd_tv, "BD Tv (=Kd/Kp)", kDouble, sBrewPidSection, 30,
        [&config, pid_bd_tv] {
            aggbTv = config.get<double>(pid_bd_tv);
            return aggbTv;
        },
        [&config, pid_bd_tv](const double val) {
            config.set<double>(pid_bd_tv, val);
            aggbTv = val;
        },
        PID_TV_BD_MIN, PID_TV_BD_MAX, true, "Differential time constant (in seconds) for the PID when brewing has been detected.", [] { return useBDPID == 1; }, &aggbTv));

    // Other Section (special parameters, e.g. runtime-only toggles)
    addParam(std::make_shared<Parameter>("STEAM_MODE", "Steam Mode", kUInt8, sOtherSection, 31, [&]() -> bool { return steamON; }, [](const bool val) { steamON = val; }, false, "", [] { return true; }, &steamON));

    addParam(
        std::make_shared<Parameter>("BACKFLUSH_ON", "Backflush", kUInt8, sOtherSection, 32, [&]() -> bool { return backflushOn; }, [](const bool val) { backflushOn = val; }, false, "", [] { return true; }, &backflushOn));

    // Power Section
    auto standby_enabled = "standby.enabled";
    addParam(std::make_shared<Parameter>(
        standby_enabled, "Enable Standby Timer", kUInt8, sPowerSection, 33,
        [&config, standby_enabled]() -> bool {
            standbyModeOn = config.get<bool>(standby_enabled);
            return standbyModeOn;
        },
        [&config, standby_enabled](const bool val) {
            config.set<bool>(standby_enabled, val);
            standbyModeOn = val;
        },
        true, "Turn heater off after standby time has elapsed.", [] { return true; }, &standbyModeOn));

    auto standby_time = "standby.time";
    addParam(std::make_shared<Parameter>(
        standby_time, "Standby Time", kDouble, sPowerSection, 34,
        [&config, standby_time] {
            standbyModeTime = config.get<double>(standby_time);
            return standbyModeTime;
        },
        [&config, standby_time](const double val) {
            config.set<double>(standby_time, val);
            standbyModeTime = val;
        },
        STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, true, "Time in minutes until the heater is turned off. Timer is reset by brew, manual flush, backflush and steam.", [] { return true; }, &standbyModeTime));

    // Display Section
    auto display_template = "display.template";
    addParam(std::make_shared<Parameter>(
        display_template, "Display Template", kEnum, sDisplaySection, 35, [&config, display_template] { return config.get<int>(display_template); },
        [&config, display_template](const double val) { config.set<int>(display_template, static_cast<int>(val)); }, (const char* const[]){"Standard", "Minimal", "Temp only", "Scale"}, 4, true,
        "Set the display template, changes requre a reboot", [] { return true; }));

    auto display_language = "display.language";
    addParam(std::make_shared<Parameter>(
        display_language, "Display Language", kEnum, sDisplaySection, 36, [&config, display_language] { return config.get<int>(display_language); },
        [&config, display_language](const double val) { config.set<int>(display_language, static_cast<int>(val)); }, (const char* const[]){"Deutsch", "English", "Español"}, 3, true,
        "Set the language for the OLED display, changes requre a reboot", [] { return true; }));

    auto display_fullscreen_brew_timer = "display.fullscreen_brew_timer";
    addParam(std::make_shared<Parameter>(
        display_fullscreen_brew_timer, "Enable Fullscreen Brew Timer", kUInt8, sDisplaySection, 37,
        [&config, display_fullscreen_brew_timer]() -> bool {
            featureFullscreenBrewTimer = config.get<bool>(display_fullscreen_brew_timer);
            return featureFullscreenBrewTimer;
        },
        [&config, display_fullscreen_brew_timer](const bool val) {
            config.set<bool>(display_fullscreen_brew_timer, val);
            featureFullscreenBrewTimer = val;
        },
        true, "Enable fullscreen overlay during brew", [] { return true; }, &featureFullscreenBrewTimer));

    auto display_fullscreen_manual_flush_timer = "display.fullscreen_manual_flush_timer";
    addParam(std::make_shared<Parameter>(
        display_fullscreen_manual_flush_timer, "Enable Fullscreen Manual Flush Timer", kUInt8, sDisplaySection, 38,
        [&config, display_fullscreen_manual_flush_timer]() -> bool {
            featureFullscreenManualFlushTimer = config.get<bool>(display_fullscreen_manual_flush_timer);
            return featureFullscreenManualFlushTimer;
        },
        [&config, display_fullscreen_manual_flush_timer](const bool val) {
            config.set<bool>(display_fullscreen_manual_flush_timer, val);
            featureFullscreenManualFlushTimer = val;
        },
        true, "Enable fullscreen overlay during manual flush", [] { return true; }, &featureFullscreenManualFlushTimer));

    auto display_post_brew_timer_duration = "display.post_brew_timer_duration";
    addParam(std::make_shared<Parameter>(
        display_post_brew_timer_duration, "Post Brew Timer Duration (s)", kDouble, sDisplaySection, 39,
        [&config, display_post_brew_timer_duration] {
            postBrewTimerDuration = config.get<double>(display_post_brew_timer_duration);
            return postBrewTimerDuration;
        },
        [&config, display_post_brew_timer_duration](const double val) {
            config.set<double>(display_post_brew_timer_duration, val);
            postBrewTimerDuration = val;
        },
        POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX, true, "time in s that brew timer will be shown after brew finished", [] { return true; }, &postBrewTimerDuration));

    auto display_heating_logo = "display.heating_logo";
    addParam(std::make_shared<Parameter>(
        display_heating_logo, "Enable Heating Logo", kUInt8, sDisplaySection, 40,
        [&config, display_heating_logo]() -> bool {
            featureHeatingLogo = config.get<bool>(display_heating_logo);
            return featureHeatingLogo;
        },
        [&config, display_heating_logo](const bool val) {
            config.set<bool>(display_heating_logo, val);
            featureHeatingLogo = val;
        },
        true, "full screen logo will be shown if temperature is 5°C below setpoint", [] { return true; }, &featureHeatingLogo));

    auto display_pid_off_logo = "display.pid_off_logo";
    addParam(std::make_shared<Parameter>(
        display_pid_off_logo, "Enable 'PID Disabled' Logo", kUInt8, sDisplaySection, 41,
        [&config, display_pid_off_logo]() -> bool {
            featurePidOffLogo = config.get<bool>(display_pid_off_logo);
            return featurePidOffLogo;
        },
        [&config, display_pid_off_logo](const bool val) {
            config.set<bool>(display_pid_off_logo, val);
            featurePidOffLogo = val != 0;
        },
        true, "full screen logo will be shown if pid is disabled", [] { return true; }, &featurePidOffLogo));

    auto mqtt_enabled = "mqtt.enabled";
    addParam(std::make_shared<Parameter>(
        mqtt_enabled, "MQTT enabled", kUInt8, sMqttSection, 42, [&config, mqtt_enabled]() -> bool { return config.get<bool>(mqtt_enabled); }, [&config, mqtt_enabled](const bool val) { config.set<bool>(mqtt_enabled, val); },
        true, "Enables MQTT, requires a restart", []() -> bool { return true; }, nullptr));

    auto mqtt_broker = "mqtt.broker";
    addParam(std::make_shared<Parameter>(
        mqtt_broker, "Hostname", kCString, sMqttSection, 43, [&config, mqtt_broker] { return config.get<String>(mqtt_broker); }, [&config, mqtt_broker](const String& val) { config.set<String>(mqtt_broker, val); },
        MQTT_BROKER_MAX_LENGTH, true, "IP addresss or hostname of your MQTT broker, changes require a restart", []() -> bool { return true; }, nullptr));

    auto mqtt_port = "mqtt.port";
    addParam(std::make_shared<Parameter>(
        mqtt_port, "Port", kInteger, sMqttSection, 44, [&config, mqtt_port]() -> int { return config.get<int>(mqtt_port); }, [&config, mqtt_port](const double val) { config.set<int>(mqtt_port, static_cast<int>(val)); }, 1,
        65535, true, "Port number of your MQTT broker, changes require a restart", []() -> bool { return true; }, static_cast<void*>(nullptr)));

    auto mqtt_username = "mqtt.username";
    addParam(std::make_shared<Parameter>(
        mqtt_username, "Username", kCString, sMqttSection, 45, [&config, mqtt_username] { return config.get<String>(mqtt_username); }, [&config, mqtt_username](const String& val) { config.set<String>(mqtt_username, val); },
        MQTT_USERNAME_MAX_LENGTH, true, "Username for your MQTT broker, changes require a restart", []() -> bool { return true; }, nullptr));

    auto mqtt_password = "mqtt.password";
    addParam(std::make_shared<Parameter>(
        mqtt_password, "Password", kCString, sMqttSection, 46, [&config, mqtt_password] { return config.get<String>(mqtt_password); }, [&config, mqtt_password](const String& val) { config.set<String>(mqtt_password, val); },
        MQTT_PASSWORD_MAX_LENGTH, true, "Password for your MQTT broker, changes require a restart", []() -> bool { return true; }, nullptr));

    auto mqtt_topic = "mqtt.topic";
    addParam(std::make_shared<Parameter>(
        mqtt_topic, "Topic Prefix", kCString, sMqttSection, 47, [&config, mqtt_topic] { return config.get<String>(mqtt_topic); }, [&config, mqtt_topic](const String& val) { config.set<String>(mqtt_topic, val); },
        MQTT_TOPIC_MAX_LENGTH, true, "Custom MQTT topic prefix, changes require a restart", []() -> bool { return true; }, nullptr));

    auto mqtt_hassio_enabled = "mqtt.hassio.enabled";
    addParam(std::make_shared<Parameter>(
        mqtt_hassio_enabled, "Hass.io enabled", kUInt8, sMqttSection, 48, [&config, mqtt_hassio_enabled]() -> bool { return config.get<bool>(mqtt_hassio_enabled); },
        [&config, mqtt_hassio_enabled](const bool val) { config.set<bool>(mqtt_hassio_enabled, val); }, true, "Enables Home Assistant integration, requires a restart", []() -> bool { return true; }, nullptr));

    auto mqtt_hassio_prefix = "mqtt.hassio.prefix";
    addParam(std::make_shared<Parameter>(
        mqtt_hassio_prefix, "Hass.io Prefix", kCString, sMqttSection, 49, [&config, mqtt_hassio_prefix] { return config.get<String>(mqtt_hassio_prefix); },
        [&config, mqtt_hassio_prefix](const String& val) { config.set<String>(mqtt_hassio_prefix, val); }, MQTT_HASSIO_PREFIX_MAX_LENGTH, true, "Custom MQTT topic prefix, changes require a restart",
        []() -> bool { return true; }, nullptr));

    auto system_hostname = "system.hostname";
    addParam(std::make_shared<Parameter>(
        system_hostname, "Hostname", kCString, sSystemSection, 50, [&config, system_hostname] { return config.get<String>(system_hostname); },
        [&config, system_hostname](const String& val) { config.set<String>(system_hostname, val); }, HOSTNAME_MAX_LENGTH, true, "Hostname of your machine, changes require a restart", []() -> bool { return true; }, nullptr));

    auto system_ota_password = "system.ota_password";
    addParam(std::make_shared<Parameter>(
        system_ota_password, "OtA Password", kCString, sSystemSection, 51, [&config, system_ota_password] { return config.get<String>(system_ota_password); },
        [&config, system_ota_password](const String& val) { config.set<String>(system_ota_password, val); }, OTAPASS_MAX_LENGTH, true, "Password for over-the-air updates, changes require a restart",
        []() -> bool { return true; }, nullptr));

    auto system_log_level = "system.log_level";
    addParam(std::make_shared<Parameter>(
        system_log_level, "Log Level", kEnum, sSystemSection, 52, [&config, system_log_level]() -> int { return config.get<int>(system_log_level); },
        [&config, system_log_level](const double val) {
            config.set<int>(system_log_level, static_cast<int>(val));
            Logger::setLevel(static_cast<Logger::Level>(val));
        },
        (const char* const[]){"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "SILENT"}, 7, true, "Set the logging verbosity level", [] { return true; }));

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