
/**
 * @file Config.h
 *
 * @brief Centralized configuration management with JSON storage
 */

#pragma once

#include "ConfigDef.h"
#include "Logger.h"
#include "defaults.h"
#include "hardware/Relay.h"
#include "hardware/Switch.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <map>

class Config {
    public:
        /**
         * @brief Initialize the configuration system
         *
         * @return true if successful, false otherwise
         */
        bool begin() {
            if (!LittleFS.begin(true)) {
                LOG(ERROR, "Failed to initialize LittleFS");
                return false;
            }

            // Check if config file exists
            if (!LittleFS.exists(CONFIG_FILE)) {
                LOG(INFO, "Config file not found, creating from defaults");

                createDefaults();

                return save();
            }

            // Try to load existing config
            if (!load()) {
                LOG(WARNING, "Failed to load config, creating from defaults");

                createDefaults();

                return save();
            }

            initializeConfigDefs();

            return true;
        }

        /**
         * @brief Load configuration from file
         *
         * @return true if successful, false otherwise
         */
        bool load() {
            if (!LittleFS.exists(CONFIG_FILE)) {
                LOG(INFO, "Config file does not exist");

                return false;
            }

            File file = LittleFS.open(CONFIG_FILE, "r");

            if (!file) {
                LOG(ERROR, "Failed to open config file for reading");

                return false;
            }

            const DeserializationError error = deserializeJson(_doc, file);
            file.close();

            if (error) {
                LOG(ERROR, "Failed to parse config file");
                return false;
            }

            LOG(INFO, "Configuration loaded successfully");

            return true;
        }

        /**
         * @brief Save configuration to file
         *
         * @return true if successful, false otherwise
         */
        [[nodiscard]] bool save() const {
            File file = LittleFS.open(CONFIG_FILE, "w");

            if (!file) {
                LOG(ERROR, "Failed to open config file for writing");
                return false;
            }

            if (serializeJson(_doc, file) == 0) {
                LOG(ERROR, "Failed to write config to file");
                file.close();
                return false;
            }

            file.close();
            LOG(INFO, "Configuration saved successfully");

            return true;
        }

        bool validateAndApplyFromJson(const String& jsonString) {
            DynamicJsonDocument doc(4096);
            const DeserializationError error = deserializeJson(doc, jsonString);

            if (error) {
                LOGF(ERROR, "JSON parsing failed: %s", error.c_str());
                return false;
            }

            if (!validateAndApplyConfig(doc)) {
                return false;
            }

            return true;
        }

        template <typename T>
        T get(const String& path) const {
            auto current = _doc.as<JsonVariantConst>();
            int startIndex = 0;
            int dotIndex;

            while ((dotIndex = path.indexOf('.', startIndex)) != -1) {
                char segment[64]; // Stack-allocated buffer
                const int segmentLen = dotIndex - startIndex;

                if (segmentLen >= 64) {
                    return T{};
                }

                path.substring(startIndex, dotIndex).toCharArray(segment, segmentLen + 1);
                segment[segmentLen] = '\0';

                if (!current[segment].isNull()) {
                    current = current[segment];
                }
                else {
                    return T{};
                }

                startIndex = dotIndex + 1;
            }

            char finalSegment[64];
            const unsigned int pathLen = path.length();
            const unsigned int finalLen = pathLen - static_cast<unsigned int>(startIndex);

            if (finalLen >= 64) {
                return T{};
            }

            path.substring(startIndex).toCharArray(finalSegment, finalLen + 1);
            finalSegment[finalLen] = '\0';

            current = current[finalSegment];

            if constexpr (std::is_same_v<T, bool>) {
                return current.as<bool>();
            }
            else if constexpr (std::is_same_v<T, int>) {
                return current.as<int>();
            }
            else if constexpr (std::is_same_v<T, uint8_t>) {
                return current.as<uint8_t>();
            }
            else if constexpr (std::is_same_v<T, float>) {
                return current.as<float>();
            }
            else if constexpr (std::is_same_v<T, double>) {
                return current.as<double>();
            }
            else if constexpr (std::is_same_v<T, String>) {
                return current.as<String>();
            }
            else {
                static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, String>, "Type must be arithmetic or String");
                return current.as<T>();
            }
        }

        template <typename T>
        void set(const String& path, const T& value) {
            auto current = _doc.as<JsonVariant>();
            int startIndex = 0;
            int dotIndex;

            // Navigate to the parent object
            while ((dotIndex = path.indexOf('.', startIndex)) != -1) {
                char segment[64]; // Stack-allocated buffer
                const int segmentLen = dotIndex - startIndex;

                if (segmentLen >= 64) {
                    return;
                }

                path.substring(startIndex, dotIndex).toCharArray(segment, segmentLen + 1);
                segment[segmentLen] = '\0';

                if (!current[segment].is<JsonObject>()) {
                    current[segment] = current.createNestedObject();
                }
                current = current[segment];

                startIndex = dotIndex + 1;
            }

            char finalSegment[64];
            const unsigned int pathLen = path.length();
            const unsigned int finalLen = pathLen - static_cast<unsigned int>(startIndex);

            if (finalLen >= 64) {
                return;
            }

            path.substring(startIndex).toCharArray(finalSegment, finalLen + 1);
            finalSegment[finalLen] = '\0';

            current[finalSegment] = value;
        }

    private:
        inline static auto CONFIG_FILE = "/config.json";

        StaticJsonDocument<4096> _doc;

        std::map<std::string, ConfigDef> _configDefs;

        void initializeConfigDefs() {
            _configDefs.clear();

            // PID general
            _configDefs.emplace("pid.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("pid.use_ponm", ConfigDef::forBool(false));
            _configDefs.emplace("pid.ema_factor", ConfigDef::forDouble(EMA_FACTOR, PID_EMA_FACTOR_MIN, PID_EMA_FACTOR_MAX));

            // PID regular
            _configDefs.emplace("pid.regular.kp", ConfigDef::forDouble(AGGKP, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX));
            _configDefs.emplace("pid.regular.tn", ConfigDef::forDouble(AGGTN, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX));
            _configDefs.emplace("pid.regular.tv", ConfigDef::forDouble(AGGTV, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX));
            _configDefs.emplace("pid.regular.i_max", ConfigDef::forDouble(AGGIMAX, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX));

            // PID brew detection
            _configDefs.emplace("pid.bd.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("pid.bd.kp", ConfigDef::forDouble(AGGBKP, PID_KP_BD_MIN, PID_KP_BD_MAX));
            _configDefs.emplace("pid.bd.tn", ConfigDef::forDouble(AGGBTN, PID_TN_BD_MIN, PID_TN_BD_MAX));
            _configDefs.emplace("pid.bd.tv", ConfigDef::forDouble(AGGBTV, PID_TV_BD_MIN, PID_TV_BD_MAX));

            // PID steam
            _configDefs.emplace("pid.steam.kp", ConfigDef::forDouble(STEAMKP, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX));

            // Brew settings
            _configDefs.emplace("brew.setpoint", ConfigDef::forDouble(SETPOINT, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX));
            _configDefs.emplace("brew.temp_offset", ConfigDef::forDouble(TEMPOFFSET, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX));
            _configDefs.emplace("brew.pid_delay", ConfigDef::forDouble(BREW_PID_DELAY, BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX));
            _configDefs.emplace("brew.target_time", ConfigDef::forDouble(TARGET_BREW_TIME, TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX));
            _configDefs.emplace("brew.target_weight", ConfigDef::forDouble(TARGET_BREW_WEIGHT, TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX));

            // Pre-infusion
            _configDefs.emplace("brew.pre_infusion.time", ConfigDef::forDouble(PRE_INFUSION_TIME, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX));
            _configDefs.emplace("brew.pre_infusion.pause", ConfigDef::forDouble(PRE_INFUSION_PAUSE_TIME, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX));

            // Steam
            _configDefs.emplace("steam.setpoint", ConfigDef::forDouble(STEAMSETPOINT, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX));

            // Backflushing
            _configDefs.emplace("backflush.cycles", ConfigDef::forInt(BACKFLUSH_CYCLES, BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX));
            _configDefs.emplace("backflush.fill_time", ConfigDef::forDouble(BACKFLUSH_FILL_TIME, BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX));
            _configDefs.emplace("backflush.flush_time", ConfigDef::forDouble(BACKFLUSH_FLUSH_TIME, BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX));

            // Standby
            _configDefs.emplace("standby.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("standby.time", ConfigDef::forDouble(STANDBY_MODE_TIME, STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX));

            // Features
            _configDefs.emplace("features.brew_control", ConfigDef::forBool(false));

            // MQTT
            _configDefs.emplace("mqtt.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("mqtt.broker", ConfigDef::forString("", MQTT_BROKER_MAX_LENGTH));
            _configDefs.emplace("mqtt.port", ConfigDef::forInt(1883, 1, 65535));
            _configDefs.emplace("mqtt.username", ConfigDef::forString(MQTT_USERNAME, MQTT_USERNAME_MAX_LENGTH));
            _configDefs.emplace("mqtt.password", ConfigDef::forString(MQTT_PASSWORD, MQTT_PASSWORD_MAX_LENGTH));
            _configDefs.emplace("mqtt.topic", ConfigDef::forString(MQTT_TOPIC, MQTT_TOPIC_MAX_LENGTH));
            _configDefs.emplace("mqtt.hassio.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("mqtt.hassio.prefix", ConfigDef::forString(MQTT_HASSIO_PREFIX, MQTT_HASSIO_PREFIX_MAX_LENGTH));

            // System
            _configDefs.emplace("system.hostname", ConfigDef::forString(HOSTNAME, HOSTNAME_MAX_LENGTH));
            _configDefs.emplace("system.ota_password", ConfigDef::forString(OTAPASS, OTAPASS_MAX_LENGTH));
            _configDefs.emplace("system.offline_mode", ConfigDef::forBool(false));
            _configDefs.emplace("system.log_level", ConfigDef::forInt(static_cast<int>(Logger::Level::INFO), 0, 5));

            // Display
            _configDefs.emplace("display.template", ConfigDef::forInt(0, 0, 4));
            _configDefs.emplace("display.language", ConfigDef::forInt(0, 0, 2));
            _configDefs.emplace("display.fullscreen_brew_timer", ConfigDef::forBool(false));
            _configDefs.emplace("display.fullscreen_manual_flush_timer", ConfigDef::forBool(false));
            _configDefs.emplace("display.post_brew_timer_duration", ConfigDef::forDouble(POST_BREW_TIMER_DURATION, POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX));
            _configDefs.emplace("display.heating_logo", ConfigDef::forBool(true));
            _configDefs.emplace("display.pid_off_logo", ConfigDef::forBool(true));

            // Hardware - OLED
            _configDefs.emplace("hardware.oled.enabled", ConfigDef::forBool(true));
            _configDefs.emplace("hardware.oled.rotation", ConfigDef::forInt(0, 0, 3));
            _configDefs.emplace("hardware.oled.type", ConfigDef::forInt(0, 0, 1));
            _configDefs.emplace("hardware.oled.address", ConfigDef::forInt(0x3C, 0x00, 0xFF));

            // Hardware - Relays
            _configDefs.emplace("hardware.relays.heater.trigger_type", ConfigDef::forInt(Relay::HIGH_TRIGGER, 0, 1));
            _configDefs.emplace("hardware.relays.pump_valve.trigger_type", ConfigDef::forInt(Relay::HIGH_TRIGGER, 0, 1));

            // Hardware - Switches
            _configDefs.emplace("hardware.switches.brew.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.switches.brew.type", ConfigDef::forInt(Switch::TOGGLE, 0, 2));
            _configDefs.emplace("hardware.switches.brew.mode", ConfigDef::forInt(Switch::NORMALLY_OPEN, 0, 1));
            _configDefs.emplace("hardware.switches.steam.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.switches.steam.type", ConfigDef::forInt(Switch::TOGGLE, 0, 2));
            _configDefs.emplace("hardware.switches.steam.mode", ConfigDef::forInt(Switch::NORMALLY_OPEN, 0, 1));
            _configDefs.emplace("hardware.switches.power.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.switches.power.type", ConfigDef::forInt(Switch::TOGGLE, 0, 2));
            _configDefs.emplace("hardware.switches.power.mode", ConfigDef::forInt(Switch::NORMALLY_OPEN, 0, 1));

            // Hardware - LEDs
            _configDefs.emplace("hardware.leds.status.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.leds.status.inverted", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.leds.brew.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.leds.brew.inverted", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.leds.steam.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.leds.steam.inverted", ConfigDef::forBool(false));

            // Hardware - Sensors
            _configDefs.emplace("hardware.sensors.temperature.type", ConfigDef::forInt(0, 0, 1));
            _configDefs.emplace("hardware.sensors.pressure.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.sensors.watertank.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.sensors.watertank.mode", ConfigDef::forInt(Switch::NORMALLY_CLOSED, 0, 1));

            // Scale
            _configDefs.emplace("hardware.sensors.scale.enabled", ConfigDef::forBool(false));
            _configDefs.emplace("hardware.sensors.scale.samples", ConfigDef::forInt(SCALE_SAMPLES, 1, 20));
            _configDefs.emplace("hardware.sensors.scale.type", ConfigDef::forInt(0, 0, 5));
            _configDefs.emplace("hardware.sensors.scale.calibration", ConfigDef::forDouble(SCALE_CALIBRATION_FACTOR, SCALE_CALIBRATION_MIN, SCALE_CALIBRATION_MAX));
            _configDefs.emplace("hardware.sensors.scale.calibration2", ConfigDef::forDouble(SCALE2_CALIBRATION_FACTOR, SCALE2_CALIBRATION_MIN, SCALE2_CALIBRATION_MAX));
            _configDefs.emplace("hardware.sensors.scale.known_weight", ConfigDef::forDouble(SCALE_KNOWN_WEIGHT, SCALE_KNOWN_WEIGHT_MIN, SCALE_KNOWN_WEIGHT_MAX));
        }

        /**
         * @brief Set a value in the JSON document using a dot-separated path
         */
        template <typename T>
        static bool setJsonValue(JsonDocument& doc, const String& path, const T& value) {
            if (path.isEmpty()) {
                LOGF(ERROR, "Empty path provided to setJsonValue");
                return false;
            }

            if (!doc.is<JsonObject>()) {
                doc.to<JsonObject>();
            }

            auto current = doc.as<JsonObject>();

            if (current.isNull()) {
                LOGF(ERROR, "Failed to get root object");
                return false;
            }

            // Split the path into segments
            int startIndex = 0;
            int dotIndex;

            // Navigate through all segments except the last one
            while ((dotIndex = path.indexOf('.', startIndex)) != -1) {
                String segment = path.substring(startIndex, dotIndex);

                if (segment.isEmpty()) {
                    LOGF(ERROR, "Empty segment in path: %s", path.c_str());
                    return false;
                }

                // Get or create the nested object for this segment
                if (!current.containsKey(segment)) {
                    // Create new nested object
                    current = current.createNestedObject(segment);
                    if (current.isNull()) {
                        LOGF(ERROR, "Failed to create nested object for segment: %s", segment.c_str());
                        return false;
                    }
                }
                else if (current[segment].is<JsonObject>()) {
                    // Use existing object
                    current = current[segment];
                }
                else {
                    // Existing value is not an object - need to replace it
                    current.remove(segment);
                    current = current.createNestedObject(segment);
                    if (current.isNull()) {
                        LOGF(ERROR, "Failed to replace value with nested object for segment: %s", segment.c_str());
                        return false;
                    }
                }

                startIndex = dotIndex + 1;
            }

            // Set the final value using the last segment as the key
            String leafKey = path.substring(startIndex);
            if (leafKey.isEmpty()) {
                LOGF(ERROR, "Empty leaf key in path: %s", path.c_str());
                return false;
            }

            current[leafKey] = value;

            LOGF(TRACE, "Successfully set %s = %s", path.c_str(), String(value).c_str());

            return true;
        }

        /**
         * @brief Create a new configuration with default values
         */
        void createDefaults() {
            LOGF(INFO, "Starting createDefaults");

            initializeConfigDefs();
            _doc.clear();

            LOGF(INFO, "Processing %d config definitions", _configDefs.size());

            int successCount = 0;
            for (const auto& [path, configDef] : _configDefs) {
                const auto pathStr = String(path.c_str());

                LOGF(DEBUG, "Processing path: '%s'", pathStr.c_str());

                bool success = false;

                switch (configDef.type) {
                    case ConfigDef::BOOL:
                        LOGF(DEBUG, "Setting bool %s = %s", pathStr.c_str(), configDef.boolVal ? "true" : "false");
                        success = setJsonValue(_doc, pathStr, configDef.boolVal);
                        break;

                    case ConfigDef::INT:
                        LOGF(DEBUG, "Setting int %s = %d", pathStr.c_str(), configDef.intVal);
                        success = setJsonValue(_doc, pathStr, configDef.intVal);
                        break;

                    case ConfigDef::DOUBLE:
                        LOGF(DEBUG, "Setting double %s = %f", pathStr.c_str(), configDef.doubleVal);
                        success = setJsonValue(_doc, pathStr, configDef.doubleVal);
                        break;

                    case ConfigDef::STRING:
                        LOGF(DEBUG, "Setting string %s = '%s'", pathStr.c_str(), configDef.stringVal.c_str());
                        success = setJsonValue(_doc, pathStr, configDef.stringVal);
                        break;

                    default:
                        LOGF(ERROR, "Unknown config type for path: %s", pathStr.c_str());
                        continue;
                }

                if (success) {
                    successCount++;
                    LOGF(DEBUG, "Successfully set value for %s", pathStr.c_str());
                }
                else {
                    LOGF(ERROR, "Failed to set value for %s", pathStr.c_str());
                }

                // Add a small delay to prevent overwhelming the system
                delay(1);
            }

            LOGF(INFO, "createDefaults completed. Successfully set %d/%d values", successCount, _configDefs.size());

            String jsonStr;
            serializeJsonPretty(_doc, jsonStr);
            LOGF(DEBUG, "Final JSON structure:\n%s", jsonStr.c_str());
        }

        bool validateAndApplyConfig(const JsonDocument& doc) {
            LOGF(INFO, "Validating and applying configuration with %d parameters", _configDefs.size());

            // Helper function to recursively extract all paths from JSON
            std::function<void(JsonVariantConst, const String&, std::vector<std::pair<String, JsonVariantConst>>&)> extractPaths = [&](JsonVariantConst obj, const String& prefix,
                                                                                                                                       std::vector<std::pair<String, JsonVariantConst>>& paths) {
                if (obj.is<JsonObjectConst>()) {
                    for (JsonPairConst pair : obj.as<JsonObjectConst>()) {
                        String newPath = prefix.isEmpty() ? String(pair.key().c_str()) : prefix + "." + pair.key().c_str();
                        extractPaths(pair.value(), newPath, paths);
                    }
                }
                else {
                    // Leaf value
                    paths.emplace_back(prefix, obj);
                }
            };

            // Extract all paths from the document
            std::vector<std::pair<String, JsonVariantConst>> docPaths;
            extractPaths(doc.as<JsonVariantConst>(), "", docPaths);

            LOGF(DEBUG, "Found %d parameters in uploaded config", docPaths.size());

            // Validate each path against _configDefs
            for (const auto& [path, value] : docPaths) {
                // Check if this path exists in our config definitions
                auto it = _configDefs.find(path.c_str());
                if (it == _configDefs.end()) {
                    LOGF(WARNING, "Unknown parameter in config: %s - skipping", path.c_str());
                    continue;
                }

                const ConfigDef& def = it->second;

                // Validate and apply based on type
                bool validationSuccess = false;
                switch (def.type) {
                    case ConfigDef::BOOL:
                        {
                            if (value.is<bool>()) {
                                bool boolVal = value.as<bool>();
                                set<bool>(path.c_str(), boolVal);
                                validationSuccess = true;
                                LOGF(TRACE, "Applied bool %s = %s", path.c_str(), boolVal ? "true" : "false");
                            }
                            else {
                                LOGF(ERROR, "Invalid type for boolean parameter %s", path.c_str());
                            }
                            break;
                        }

                    case ConfigDef::INT:
                        {
                            if (value.is<int>()) {
                                int intVal = value.as<int>();
                                if (intVal >= def.minValue && intVal <= def.maxValue) {
                                    set<int>(path.c_str(), intVal);
                                    validationSuccess = true;
                                    LOGF(TRACE, "Applied int %s = %d", path.c_str(), intVal);
                                }
                                else {
                                    LOGF(ERROR, "Value %d for %s outside range [%.2f, %.2f]", intVal, path.c_str(), def.minValue, def.maxValue);
                                }
                            }
                            else {
                                LOGF(ERROR, "Invalid type for integer parameter %s", path.c_str());
                            }
                            break;
                        }

                    case ConfigDef::DOUBLE:
                        {
                            if (value.is<double>() || value.is<float>()) {
                                double doubleVal = value.as<double>();
                                if (doubleVal >= def.minValue && doubleVal <= def.maxValue) {
                                    set<double>(path.c_str(), doubleVal);
                                    validationSuccess = true;
                                    LOGF(TRACE, "Applied double %s = %.4f", path.c_str(), doubleVal);
                                }
                                else {
                                    LOGF(ERROR, "Value %.4f for %s outside range [%.2f, %.2f]", doubleVal, path.c_str(), def.minValue, def.maxValue);
                                }
                            }
                            else {
                                LOGF(ERROR, "Invalid type for double parameter %s", path.c_str());
                            }
                            break;
                        }

                    case ConfigDef::STRING:
                        {
                            if (value.is<const char*>() || value.is<String>()) {
                                String stringVal = value.as<String>();
                                if (stringVal.length() <= def.maxLength) {
                                    set<String>(path.c_str(), stringVal);
                                    validationSuccess = true;
                                    LOGF(TRACE, "Applied string %s = %s", path.c_str(), stringVal.c_str());
                                }
                                else {
                                    LOGF(ERROR, "String value for %s too long: %d > %d", path.c_str(), stringVal.length(), def.maxLength);
                                }
                            }
                            else {
                                LOGF(ERROR, "Invalid type for string parameter %s", path.c_str());
                            }
                            break;
                        }
                }

                if (!validationSuccess) {
                    LOGF(ERROR, "Failed to validate parameter: %s", path.c_str());
                    return false;
                }
            }

            LOGF(INFO, "Successfully validated and applied all configuration parameters");

            return save();
        }

        template <typename T>
        static bool validateParameterRange(const char* paramName, T value, T min, T max) {
            if (value < min || value > max) {
                LOGF(ERROR, "Parameter %s value %.2f out of range [%.2f, %.2f]", paramName, static_cast<double>(value), static_cast<double>(min), static_cast<double>(max));
                return false;
            }
            return true;
        }

        static String constrainStringParameter(const String& value, const size_t maxLength, const char* paramName = nullptr) {
            if (value.length() <= maxLength) {
                return value;
            }

            LOGF(WARNING, "Parameter '%s' truncated from %d to %d characters", paramName, value.length(), maxLength);

            return value.substring(0, maxLength);
        }
};
