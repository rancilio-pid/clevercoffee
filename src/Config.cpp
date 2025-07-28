/**
 * @file Config.cpp
 * @brief Simplified unified configuration implementation
 */

#include "Config.h"

Config& config = Config::getInstance();

// Enum option arrays with explicit value mappings
const EnumOption switchTypeOptions[] = {{0, "Momentary"}, {1, "Toggle"}};

const EnumOption switchModeOptions[] = {{0, "Normally Open"}, {1, "Normally Closed"}};

const EnumOption relayTriggerOptions[] = {{0, "Low Trigger"}, {1, "High Trigger"}};

const EnumOption displayTemplateOpts[] = {{0, "Standard"}, {1, "Minimal"}, {2, "Temperature Only"}, {3, "Scale"}, {4, "Upright"}};

const EnumOption languageOpts[] = {{0, "English"}, {1, "German"}, {2, "Spanish"}};

const EnumOption oledTypeOpts[] = {{0, "SSD1306"}, {1, "SH1106"}};

const EnumOption oledAddressOpts[] = {{0, "0x3C"}, {1, "0x3D"}};

const EnumOption temperatureSensorOpts[] = {
    {0, "TSIC 306"},
    {1, "Dallas DS18B20"},
};

const EnumOption scaleTypeOpts[] = {{0, "HX711 (2 load cells)"}, {1, "HX711 (1 load cell)"}, {2, "Bluetooth"}};

const EnumOption logLevelOpts[] = {{0, "TRACE"}, {1, "DEBUG"}, {2, "INFO"}, {3, "WARNING"}, {4, "ERROR"}, {5, "FATAL"}, {6, "SILENT"}};

const EnumOption brewModeOptions[] = {
    {0, "Manual"},
    {1, "Automatic"},
};

void Config::initializeParams() {
    // PID Parameters - using original hierarchical parameter IDs that website expects
    _params["pid.enabled"] = ParamDef::Bool(&pidON, false, "Enable PID Controller", 0, 101, "Enables or disables the PID temperature controller");
    _params["pid.use_ponm"] = ParamDef::Bool(&usePonM, false, "Enable PonM", 0, 102, "Use PonM mode (Proportional on Measurement)");
    _params["pid.ema_factor"] =
        ParamDef::Double(&emaFactor, EMA_FACTOR, PID_EMA_FACTOR_MIN, PID_EMA_FACTOR_MAX, "PID EMA Factor", 0, 111, "Smoothing of input for derivative component. Smaller = less smoothing but less delay");
    _params["pid.regular.kp"] = ParamDef::Double(&aggKp, AGGKP, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, "PID Kp", 0, 112, "Proportional gain (in Watts/°C) for the main PID controller");
    _params["pid.regular.tn"] = ParamDef::Double(&aggTn, AGGTN, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, "PID Tn", 0, 113, "Integral time constant (in seconds) for the main PID controller");
    _params["pid.regular.tv"] = ParamDef::Double(&aggTv, AGGTV, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, "PID Tv", 0, 114, "Differential time constant (in seconds) for the main PID controller");
    _params["pid.regular.i_max"] = ParamDef::Double(&aggIMax, AGGIMAX, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, "PID Integrator Max", 0, 115, "Internal integrator limit to prevent windup (in Watts)");
    _params["pid.steam.kp"] = ParamDef::Double(&steamKp, STEAMKP, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, "Steam Kp", 0, 116, "Proportional gain for the steaming mode");

    // Brew Detection PID
    _params["pid.bd.enabled"] = ParamDef::Bool(&useBDPID, false, "Enable Brew PID", 2, 701, "Use separate PID parameters while brew is running");
    _params["pid.bd.kp"] = ParamDef::Double(&aggbKp, AGGBKP, PID_KP_BD_MIN, PID_KP_BD_MAX, "BD Kp", 2, 712, "Proportional gain for PID when brewing has been detected");
    _params["pid.bd.tn"] = ParamDef::Double(&aggbTn, AGGBTN, PID_TN_BD_MIN, PID_TN_BD_MAX, "BD Tn", 2, 713, "Integral time constant for PID when brewing has been detected");
    _params["pid.bd.tv"] = ParamDef::Double(&aggbTv, AGGBTV, PID_TV_BD_MIN, PID_TV_BD_MAX, "BD Tv", 2, 714, "Differential time constant for PID when brewing has been detected");

    // Temperature Parameters
    _params["TEMP"] = ParamDef::Double(&temperature, 0.0, 0.0, 200.0, "Temperature", 1, 200, "Current temperature reading from sensor");
    _params["brew.setpoint"] = ParamDef::Double(&brewSetpoint, SETPOINT, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, "Setpoint (°C)", 1, 201, "The temperature that the PID will attempt to reach and hold");
    _params["brew.temp_offset"] =
        ParamDef::Double(&brewTempOffset, TEMPOFFSET, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, "Offset (°C)", 1, 202, "Optional offset added to the user-visible setpoint to compensate sensor offsets");
    _params["brew.pid_delay"] = ParamDef::Double(&brewPidDelay, BREW_PID_DELAY, BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, "Brew PID Delay (s)", 2, 711, "Delay time during which PID will be disabled once brew is detected");
    _params["steam.setpoint"] = ParamDef::Double(&steamSetpoint, STEAMSETPOINT, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, "Steam Setpoint (°C)", 1, 203, "The temperature that the PID will use for steam mode");

    // Brew Control Parameter
    _params["brew.by_time.enabled"] = ParamDef::Bool(&brewByTimeEnabled, false, "Brew by Time", 3, 311, "Enable brewing by time control");
    _params["brew.by_time.target_time"] = ParamDef::Double(&targetBrewTime, TARGET_BREW_TIME, TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX, "Target Brew Time (s)", 3, 301, "Target brew time in seconds"); // Legacy alias
    _params["brew.by_weight.enabled"] = ParamDef::Bool(&brewByWeightEnabled, false, "Brew by Weight", 3, 321, "Enable brewing by weight control");
    _params["brew.by_weight.auto_tare"] = ParamDef::Bool(&brewByWeightAutoTare, false, "Auto-tare", 3, 323, "Automatically tare scale before brewing");
    _params["brew.by_weight.target_weight"] =
        ParamDef::Double(&targetBrewWeight, TARGET_BREW_WEIGHT, TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX, "Target Brew Weight (g)", 3, 322, "Brew is running until this weight has been measured");
    _params["brew.pre_infusion.enabled"] = ParamDef::Bool(&preinfusionEnabled, false, "Pre-Infusion", 3, 304, "Enable pre-infusion phase");
    _params["brew.pre_infusion.time"] = ParamDef::Double(&preinfusion, PRE_INFUSION_TIME, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, "Preinfusion Time (s)", 3, 302, "Pre-infusion time in seconds");
    _params["brew.pre_infusion.pause"] = ParamDef::Double(&preinfusionPause, PRE_INFUSION_PAUSE_TIME, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, "Preinfusion Pause (s)", 3, 303, "Pre-infusion pause time in seconds");

    // Brew Mode Parameter (using local Config variable)
    _params["brew.mode"] = ParamDef::Enum(&_brewMode, 0, brewModeOptions, 2, "Brew Mode", 3, 310, "Brewing mode selection");

    // Backflush Parameters
    _params["backflush.cycles"] = ParamDef::Int(&backflushCycles, BACKFLUSH_CYCLES, BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX, "Backflush Cycles", 6, 401, "Number of backflush cycles to perform");
    _params["backflush.fill_time"] = ParamDef::Double(&backflushFillTime, BACKFLUSH_FILL_TIME, BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX, "Backflush Fill Time (s)", 6, 402, "Time to fill during backflush cycle");
    _params["backflush.flush_time"] =
        ParamDef::Double(&backflushFlushTime, BACKFLUSH_FLUSH_TIME, BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX, "Backflush Flush Time (s)", 6, 403, "Time to flush during backflush cycle");

    // System Parameters
    _params["system.hostname"] = ParamDef::String(&hostname, HOSTNAME, HOSTNAME_MAX_LENGTH, "Hostname", 9, 1101, "Hostname of your machine, changes require a restart");
    _params["system.ota_password"] = ParamDef::String(&otaPassword, OTAPASS, PASSWORD_MAX_LENGTH, "OTA Password", 9, 1102, "Password for over-the-air updates, changes require a restart");
    _params["system.offline_mode"] = ParamDef::Bool(&offlineMode, false, "Offline Mode", 9, 1103, "Run in offline mode without WiFi connection");
    _params["system.auth.enabled"] = ParamDef::Bool(&authEnabled, false, "Enable Authentication", 9, 1201, "Enables authentication for accessing certain parts of the website");
    _params["system.auth.username"] = ParamDef::String(&authUsername, AUTH_USERNAME, USERNAME_MAX_LENGTH, "Website Username", 9, 1202, "Username for accessing the website and authenticating web requests");
    _params["system.auth.password"] = ParamDef::String(&authPassword, AUTH_PASSWORD, PASSWORD_MAX_LENGTH, "Website Password", 9, 1203, "Password for accessing the website and authenticating web requests");
    _params["system.timing_debug.enabled"] = ParamDef::Bool(&timingDebugActive, false, "Loop timing in console", 9, 1301, "Enable or disable the process loop time debugging in console");
    _params["system.showdisplay.enabled"] = ParamDef::Bool(&includeDisplayInLogs, true, "Activate display recording", 9, 1303, "Enable or disable showing sendBuffer loops in debug logs");

    // Log Level (enum parameter)
    _params["system.log_level"] = ParamDef::Enum(&logLevel, 2, logLevelOpts, 7, "Log Level", 9, 1103, "Set the logging level for debug output");

    // Power Management
    _params["standby.enabled"] = ParamDef::Bool(&standbyModeOn, false, "Enable Standby Timer", 7, 801, "Turn heater off after standby time has elapsed");
    _params["standby.time"] = ParamDef::Double(&standbyModeTime, STANDBY_MODE_TIME, STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, "Standby Time", 7, 802, "Time in minutes until the heater is turned off");

    // MQTT Parameters
    _params["mqtt.enabled"] = ParamDef::Bool(&mqttEnabled, false, "MQTT Enabled", 8, 1001, "Enables MQTT, change requires a restart");
    _params["mqtt.broker"] = ParamDef::String(&mqttBroker, "", MQTT_BROKER_MAX_LENGTH, "MQTT Broker", 8, 1011, "IP address or hostname of your MQTT broker");
    _params["mqtt.port"] = ParamDef::Int(&mqttPort, 1883, 1, 65535, "MQTT Port", 8, 1012, "Port number of your MQTT broker");
    _params["mqtt.username"] = ParamDef::String(&mqttUsername, MQTT_USERNAME, USERNAME_MAX_LENGTH, "Username", 8, 1013, "Username for your MQTT broker");
    _params["mqtt.password"] = ParamDef::String(&mqttPassword, MQTT_PASSWORD, PASSWORD_MAX_LENGTH, "Password", 8, 1014, "Password for your MQTT broker");
    _params["mqtt.topic"] = ParamDef::String(&mqttTopic, MQTT_TOPIC, MQTT_TOPIC_MAX_LENGTH, "Topic Prefix", 8, 1015, "Custom MQTT topic prefix");
    _params["mqtt.hassio.enabled"] = ParamDef::Bool(&mqttHassioEnabled, false, "Hass.io enabled", 8, 1021, "Enables Home Assistant integration");
    _params["mqtt.hassio.prefix"] = ParamDef::String(&mqttHassioPrefix, MQTT_HASSIO_PREFIX, MQTT_HASSIO_PREFIX_MAX_LENGTH, "Hass.io Prefix", 8, 1022, "Custom MQTT topic prefix for Home Assistant");

    // Hardware - OLED
    _params["hardware.oled.enabled"] = ParamDef::Bool(&oledEnabled, true, "Enable OLED Display", 11, 2001, "Enable or disable the OLED display");
    _params["hardware.oled.type"] = ParamDef::Enum(&oledType, 0, oledTypeOpts, 2, "OLED Type", 11, 2002, "Select your OLED display type");
    _params["hardware.oled.address"] = ParamDef::Enum(&oledAddress, 0, oledAddressOpts, 2, "I2C Address", 11, 2003, "I2C address of the OLED display");

    // Hardware - Relays
    _params["hardware.relays.heater.trigger_type"] = ParamDef::Enum(&heaterTriggerType, 1, relayTriggerOptions, 2, "Heater Relay Trigger Type", 12, 2101, "Relay trigger type for heater control");
    _params["hardware.relays.valve.trigger_type"] = ParamDef::Enum(&valveTriggerType, 1, relayTriggerOptions, 2, "Valve Relay Trigger Type", 12, 2102, "Relay trigger type for valve control");
    _params["hardware.relays.pump.trigger_type"] = ParamDef::Enum(&pumpTriggerType, 1, relayTriggerOptions, 2, "Pump Relay Trigger Type", 12, 2103, "Relay trigger type for pump control");

    // Hardware - Switches
    _params["hardware.switches.brew.enabled"] = ParamDef::Bool(&brewSwitchEnabled, false, "Enable Brew Switch", 13, 2201, "Enable physical brew switch");
    _params["hardware.switches.brew.type"] = ParamDef::Enum(&brewSwitchType, 1, switchTypeOptions, 2, "Brew Switch Type", 13, 2202, "Type of brew switch connected");
    _params["hardware.switches.brew.mode"] = ParamDef::Enum(&brewSwitchMode, 0, switchModeOptions, 2, "Brew Switch Mode", 13, 2203, "Electrical configuration of brew switch");
    _params["hardware.switches.steam.enabled"] = ParamDef::Bool(&steamSwitchEnabled, false, "Enable Steam Switch", 13, 2211, "Enable physical steam switch");
    _params["hardware.switches.steam.type"] = ParamDef::Enum(&steamSwitchType, 1, switchTypeOptions, 2, "Steam Switch Type", 13, 2212, "Type of steam switch connected");
    _params["hardware.switches.steam.mode"] = ParamDef::Enum(&steamSwitchMode, 0, switchModeOptions, 2, "Steam Switch Mode", 13, 2213, "Electrical configuration of steam switch");
    _params["hardware.switches.power.enabled"] = ParamDef::Bool(&powerSwitchEnabled, false, "Enable Power Switch", 13, 2221, "Enable physical power switch");
    _params["hardware.switches.power.type"] = ParamDef::Enum(&powerSwitchType, 1, switchTypeOptions, 2, "Power Switch Type", 13, 2222, "Type of power switch connected");
    _params["hardware.switches.power.mode"] = ParamDef::Enum(&powerSwitchMode, 0, switchModeOptions, 2, "Power Switch Mode", 13, 2223, "Electrical configuration of power switch");
    _params["hardware.switches.hot_water.enabled"] = ParamDef::Bool(&hotWaterSwitchEnabled, false, "Enable Water Switch", 13, 2231, "Enable physical water switch");
    _params["hardware.switches.hot_water.type"] = ParamDef::Enum(&hotWaterSwitchType, 1, switchTypeOptions, 2, "Water Switch Type", 13, 2232, "Type of water switch connected");
    _params["hardware.switches.hot_water.mode"] = ParamDef::Enum(&hotWaterSwitchMode, 0, switchModeOptions, 2, "Water Switch Mode", 13, 2233, "Electrical configuration of water switch");

    // Hardware - LEDs
    _params["hardware.leds.status.enabled"] = ParamDef::Bool(&statusLedEnabled, false, "Enable Status LED", 4, 2301, "Enable status indicator LED");
    _params["hardware.leds.status.inverted"] = ParamDef::Bool(&statusLedInverted, false, "Invert Status LED", 4, 2302, "Invert the status LED logic (for common anode LEDs)");
    _params["hardware.leds.brew.enabled"] = ParamDef::Bool(&brewLedEnabled, false, "Enable Brew LED", 4, 2311, "Enable brew indicator LED");
    _params["hardware.leds.brew.inverted"] = ParamDef::Bool(&brewLedInverted, false, "Invert Brew LED", 4, 2312, "Invert the brew LED logic");
    _params["hardware.leds.steam.enabled"] = ParamDef::Bool(&steamLedEnabled, false, "Enable Steam LED", 4, 2321, "Enable steam indicator LED");
    _params["hardware.leds.steam.inverted"] = ParamDef::Bool(&steamLedInverted, false, "Invert Steam LED", 4, 2322, "Invert the steam LED logic");

    // Hardware - Sensors
    _params["hardware.sensors.temperature.type"] = ParamDef::Enum(&temperatureSensorType, 0, temperatureSensorOpts, 2, "Temperature Sensor Type", 15, 2401, "Type of temperature sensor connected");
    _params["hardware.sensors.pressure.enabled"] = ParamDef::Bool(&pressureSensorEnabled, false, "Enable Pressure Sensor", 4, 2411, "Enable pressure sensor functionality");
    _params["hardware.sensors.watertank.enabled"] = ParamDef::Bool(&waterTankSensorEnabled, false, "Enable Water Tank Sensor", 4, 2421, "Enable water tank level sensor");
    _params["hardware.sensors.watertank.mode"] = ParamDef::Enum(&waterTankSensorMode, 1, switchModeOptions, 2, "Water Tank Sensor Mode", 15, 2422, "Electrical configuration of water tank sensor");

    // Scale settings
    _params["hardware.sensors.scale.enabled"] = ParamDef::Bool(&scaleEnabled, false, "Enable Scale", 4, 2501, "Enable scale functionality");
    _params["hardware.sensors.scale.samples"] = ParamDef::Int(&scaleSamples, SCALE_SAMPLES, 1, 20, "Scale Samples", 4, 2502, "Number of samples used for calibration");
    _params["hardware.sensors.scale.type"] = ParamDef::Enum(&scaleType, 0, scaleTypeOpts, 3, "Scale Type", 15, 2503, "Integrated HX711-based scale with different load cell configurations or Bluetooth Low Energy scales");
    _params["hardware.sensors.scale.calibration"] =
        ParamDef::Double(&scaleCalibrationFactor, SCALE_CALIBRATION_FACTOR, SCALE_CALIBRATION_MIN, SCALE_CALIBRATION_MAX, "Scale Calibration", 4, 2504, "Raw data is divided by this value to convert to readable data");
    _params["hardware.sensors.scale.calibration2"] =
        ParamDef::Double(&scaleCalibrationFactor2, SCALE_CALIBRATION_FACTOR, SCALE_CALIBRATION_MIN, SCALE_CALIBRATION_MAX, "Scale Calibration 2", 4, 2505, "Second calibration factor for dual load cell scales");
    _params["hardware.sensors.scale.known_weight"] =
        ParamDef::Double(&scaleKnownWeight, SCALE_KNOWN_WEIGHT, SCALE_KNOWN_WEIGHT_MIN, SCALE_KNOWN_WEIGHT_MAX, "Scale Known Weight", 4, 2506, "Calibration weight for scale (weight of the tray)");

    // Display Parameters
    _params["display.template"] = ParamDef::Enum(&displayTemplate, 0, displayTemplateOpts, 5, "Display Template", 5, 901, "Set the display template, changes require a reboot");
    _params["display.inverted"] = ParamDef::Bool(&displayInverted, false, "Invert Display", 5, 902, "Set the display rotation, changes require a reboot");
    _params["display.language"] = ParamDef::Enum(&displayLanguage, 0, languageOpts, 3, "Display Language", 5, 903, "Set the language for the OLED display");
    _params["display.fullscreen_brew_timer"] = ParamDef::Bool(&featureFullscreenBrewTimer, false, "Enable Fullscreen Brew Timer", 3, 904, "Enable fullscreen overlay during brew");
    _params["display.fullscreen_manual_flush_timer"] = ParamDef::Bool(&featureFullscreenManualFlushTimer, false, "Enable Fullscreen Manual Flush Timer", 3, 905, "Enable fullscreen overlay during manual flush");
    _params["display.fullscreen_hot_water_timer"] = ParamDef::Bool(&featureFullscreenHotWaterTimer, false, "Enable Fullscreen Hot Water Timer", 3, 906, "Enable fullscreen overlay during hot water mode");
    _params["display.post_brew_timer_duration"] = ParamDef::Double(&postBrewTimerDuration, POST_BREW_TIMER_DURATION, POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX, "Post Brew Timer Duration (s)", 3, 907,
                                                                   "Time in seconds that brew timer will be shown after brew finished");
    _params["display.heating_logo"] = ParamDef::Bool(&featureHeatingLogo, true, "Enable Heating Logo", 3, 908, "Full screen logo will be shown if temperature is 5°C below setpoint");
    _params["display.pid_off_logo"] = ParamDef::Bool(&featurePidOffLogo, true, "Enable 'PID Disabled' Logo", 3, 909, "Full screen logo will be shown if PID is disabled");

    // Runtime action parameters (special functions)
    _params["STEAM_MODE"] = ParamDef::Bool(&steamON, false, "Steam Mode", 10, 503, "Toggle steam mode on/off");
    _params["BACKFLUSH_ON"] = ParamDef::Bool(&backflushOn, false, "Backflush", 10, 504, "Start/stop backflush cycle");
    _params["TARE_ON"] = ParamDef::Bool(&scaleTareOn, false, "Tare", 10, 501, "Trigger scale tare operation");
    _params["CALIBRATION_ON"] = ParamDef::Bool(&scaleCalibrationOn, false, "Calibration", 10, 502, "Start scale calibration process");

    // System version (read-only)
    _params["VERSION"] = ParamDef::String(&systemVersion, sysVersion, 64, "Version", 10, 7, "Firmware version string");
}

bool Config::loadFromJson(const ::String& jsonString) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        LOGF(ERROR, "JSON parsing failed: %s", error.c_str());
        return false;
    }

    for (JsonPairConst pair : doc.as<JsonObjectConst>()) {
        ::String path = pair.key().c_str();
        JsonVariantConst value = pair.value();

        auto it = _params.find(path.c_str());
        if (it == _params.end()) {
            LOGF(WARNING, "Unknown parameter: %s", path.c_str());
            continue;
        }

        const ParamDef& def = it->second;

        // Type-safe value setting
        switch (def.type) {
            case ParamType::BOOL:
                if (value.is<bool>()) {
                    set<bool>(path, value.as<bool>());
                }
                break;
            case ParamType::INT:
                if (value.is<int>()) {
                    set<int>(path, value.as<int>());
                }
                break;
            case ParamType::UINT8:
                if (value.is<uint8_t>()) {
                    set<uint8_t>(path, value.as<uint8_t>());
                }
                break;
            case ParamType::DOUBLE:
                if (value.is<double>()) {
                    set<double>(path, value.as<double>());
                }
                break;
            case ParamType::FLOAT:
                if (value.is<float>()) {
                    set<float>(path, value.as<float>());
                }
                break;
            case ParamType::STRING:
                if (value.is<const char*>()) {
                    set<::String>(path, ::String(value.as<const char*>()));
                }
                break;
            case ParamType::ENUM:
                if (value.is<int>()) {
                    set<int>(path, value.as<int>());
                }
                break;
        }
    }

    return true;
}

::String Config::exportToJson() const {
    JsonDocument doc;

    for (const auto& [path, def] : _params) {
        switch (def.type) {
            case ParamType::BOOL:
                doc[path] = get<bool>(String(path.c_str()));
                break;
            case ParamType::INT:
                doc[path] = get<int>(String(path.c_str()));
                break;
            case ParamType::UINT8:
                doc[path] = get<uint8_t>(String(path.c_str()));
                break;
            case ParamType::DOUBLE:
                doc[path] = get<double>(String(path.c_str()));
                break;
            case ParamType::FLOAT:
                doc[path] = get<float>(String(path.c_str()));
                break;
            case ParamType::STRING:
                doc[path] = get<::String>(::String(path.c_str()));
                break;
            case ParamType::ENUM:
                doc[path] = get<int>(String(path.c_str()));
                break;
        }
    }

    ::String output;
    serializeJson(doc, output);
    return output;
}

// TODO probably unused
JsonDocument Config::getParametersForAPI(const ::String& section) const {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (const auto& [path, def] : _params) {
        if (!def.showCondition()) continue;

        JsonObject paramObj = array.add<JsonObject>();
        paramObj["id"] = path;
        paramObj["name"] = def.displayName;
        paramObj["section"] = def.section;
        paramObj["position"] = def.position;
        paramObj["type"] = static_cast<int>(def.type);
        paramObj["help"] = def.helpText;

        // Add current value and constraints
        switch (def.type) {
            case ParamType::BOOL:
                paramObj["value"] = get<bool>(String(path.c_str()));
                paramObj["default"] = def.defaultBool;
                break;
            case ParamType::INT:
                paramObj["value"] = get<int>(String(path.c_str()));
                paramObj["default"] = def.defaultInt;
                paramObj["min"] = def.minValue;
                paramObj["max"] = def.maxValue;
                break;
            case ParamType::UINT8:
                paramObj["value"] = get<uint8_t>(String(path.c_str()));
                paramObj["default"] = def.defaultUInt8;
                paramObj["min"] = def.minValue;
                paramObj["max"] = def.maxValue;
                break;
            case ParamType::DOUBLE:
                paramObj["value"] = get<double>(String(path.c_str()));
                paramObj["default"] = def.defaultDouble;
                paramObj["min"] = def.minValue;
                paramObj["max"] = def.maxValue;
                break;
            case ParamType::FLOAT:
                paramObj["value"] = get<float>(String(path.c_str()));
                paramObj["default"] = def.defaultFloat;
                paramObj["min"] = def.minValue;
                paramObj["max"] = def.maxValue;
                break;
            case ParamType::STRING:
                paramObj["value"] = get<::String>(::String(path.c_str()));
                paramObj["default"] = def.defaultString;
                paramObj["maxLength"] = def.maxLength;
                break;
            case ParamType::ENUM:
                paramObj["value"] = get<int>(String(path.c_str()));
                paramObj["default"] = def.defaultInt;
                if (def.enumOptions && def.enumCount > 0) {
                    JsonArray options = paramObj["options"].to<JsonArray>();
                    for (size_t i = 0; i < def.enumCount; i++) {
                        JsonObject option = options.add<JsonObject>();
                        option["value"] = def.enumOptions[i].value;
                        option["label"] = def.enumOptions[i].label;
                    }
                }
                break;
        }
    }

    return doc;
}

void Config::loadFromNVS() {
    LOGF(INFO, "Loading configuration from NVS namespace: %s", STORAGE_NAMESPACE);
    _prefs.begin(STORAGE_NAMESPACE, true); // Read-only mode

    int totalParams = 0;
    int loadedParams = 0;

    for (const auto& [path, def] : _params) {
        if (!def.globalVar) continue;
        totalParams++;

        // Generate hashed NVS key for the parameter path
        String nvsKey = generateNvsKey(path.c_str());

        // Only update global variable if a value exists in NVS
        if (_prefs.isKey(nvsKey.c_str())) {
            LOGF(INFO, "Config::loadFromNVS(%s): Found key '%s' in NVS", path.c_str(), nvsKey.c_str());
            loadedParams++;
            switch (def.type) {
                case ParamType::BOOL:
                    {
                        bool value = _prefs.getBool(nvsKey.c_str());
                        LOGF(DEBUG, "Config::loadFromNVS(%s): getBool(%s) = %s", path.c_str(), nvsKey.c_str(), value ? "true" : "false");
                        *static_cast<bool*>(def.globalVar) = value;
                        LOGF(INFO, "Config::loadFromNVS(%s): Set global variable to %s", path.c_str(), value ? "true" : "false");
                        break;
                    }
                case ParamType::INT:
                    {
                        int value = _prefs.getInt(nvsKey.c_str());
                        LOGF(DEBUG, "Config::loadFromNVS(%s): getInt(%s) = %d", path.c_str(), nvsKey.c_str(), value);
                        *static_cast<int*>(def.globalVar) = value;
                        LOGF(INFO, "Config::loadFromNVS(%s): Set global variable to %d", path.c_str(), value);
                        break;
                    }
                case ParamType::UINT8:
                    {
                        uint8_t value = _prefs.getUChar(nvsKey.c_str());
                        LOGF(DEBUG, "Config::loadFromNVS(%s): getUChar(%s) = %d", path.c_str(), nvsKey.c_str(), value);
                        *static_cast<uint8_t*>(def.globalVar) = value;
                        LOGF(INFO, "Config::loadFromNVS(%s): Set global variable to %d", path.c_str(), value);
                        break;
                    }
                case ParamType::DOUBLE:
                    {
                        double value = _prefs.getDouble(nvsKey.c_str());
                        LOGF(DEBUG, "Config::loadFromNVS(%s): getDouble(%s) = %.6f", path.c_str(), nvsKey.c_str(), value);
                        *static_cast<double*>(def.globalVar) = value;
                        LOGF(INFO, "Config::loadFromNVS(%s): Set global variable to %.6f", path.c_str(), value);
                        break;
                    }
                case ParamType::FLOAT:
                    {
                        float value = _prefs.getFloat(nvsKey.c_str());
                        LOGF(DEBUG, "Config::loadFromNVS(%s): getFloat(%s) = %.6f", path.c_str(), nvsKey.c_str(), value);
                        *static_cast<float*>(def.globalVar) = value;
                        LOGF(INFO, "Config::loadFromNVS(%s): Set global variable to %.6f", path.c_str(), value);
                        break;
                    }
                case ParamType::STRING:
                    {
                        ::String value = _prefs.getString(nvsKey.c_str());
                        LOGF(DEBUG, "Config::loadFromNVS(%s): getString(%s) = '%s'", path.c_str(), nvsKey.c_str(), value.c_str());
                        *static_cast<::String*>(def.globalVar) = value;
                        LOGF(INFO, "Config::loadFromNVS(%s): Set global variable to '%s'", path.c_str(), value.c_str());
                        break;
                    }
                case ParamType::ENUM:
                    {
                        int value = _prefs.getInt(nvsKey.c_str());
                        LOGF(DEBUG, "Config::loadFromNVS(%s): getInt(%s) = %d (enum)", path.c_str(), nvsKey.c_str(), value);
                        *static_cast<int*>(def.globalVar) = value;
                        LOGF(INFO, "Config::loadFromNVS(%s): Set global variable to %d (enum)", path.c_str(), value);
                        break;
                    }
            }
        }
        else {
            LOGF(DEBUG, "Config::loadFromNVS(%s): Key '%s' not found in NVS, keeping default value", path.c_str(), nvsKey.c_str());
        }
        // If no key exists in NVS, global variable keeps its default value
    }

    _prefs.end();
    LOGF(INFO, "Loaded %d/%d parameters from NVS", loadedParams, totalParams);
}

void Config::saveToNVS() {
    for (const auto& [path, def] : _params) {
        if (!def.globalVar) continue;

        switch (def.type) {
            case ParamType::BOOL:
                saveToNVS(path.c_str(), *static_cast<bool*>(def.globalVar));
                break;
            case ParamType::INT:
                saveToNVS(path.c_str(), *static_cast<int*>(def.globalVar));
                break;
            case ParamType::UINT8:
                saveToNVS(path.c_str(), *static_cast<uint8_t*>(def.globalVar));
                break;
            case ParamType::DOUBLE:
                saveToNVS(path.c_str(), *static_cast<double*>(def.globalVar));
                break;
            case ParamType::FLOAT:
                saveToNVS(path.c_str(), *static_cast<float*>(def.globalVar));
                break;
            case ParamType::STRING:
                saveToNVS(path.c_str(), *static_cast<::String*>(def.globalVar));
                break;
            case ParamType::ENUM:
                saveToNVS(path.c_str(), *static_cast<int*>(def.globalVar));
                break;
        }
    }
}

bool Config::resetToDefault(const ::String& path) {
    auto it = _params.find(path.c_str());
    if (it == _params.end()) return false;

    const ParamDef& def = it->second;

    _prefs.begin(STORAGE_NAMESPACE, false);

    _prefs.remove(path.c_str());
    _prefs.end();

    return true;

    // switch (def.type) {
    //     case ParamType::BOOL:
    //         return set<bool>(path, def.defaultBool);
    //     case ParamType::INT:
    //         return set<int>(path, def.defaultInt);
    //     case ParamType::UINT8:
    //         return set<uint8_t>(path, def.defaultUInt8);
    //     case ParamType::DOUBLE:
    //         return set<double>(path, def.defaultDouble);
    //     case ParamType::FLOAT:
    //         return set<float>(path, def.defaultFloat);
    //     case ParamType::STRING:
    //         return set<::String>(path, def.defaultString);
    //     case ParamType::ENUM:
    //         return set<int>(path, def.defaultInt);
    // }

    // return false;
}

void Config::resetAllToDefaults() {
    LOGF(INFO, "Resetting all parameters to default values");

    _prefs.begin(STORAGE_NAMESPACE, false);
    _prefs.clear();
    _prefs.end();

    // for (const auto& [path, def] : _params) {
    //     resetToDefault(::String(path.c_str()));
    // }
    LOGF(INFO, "All parameters reset to defaults");
}
