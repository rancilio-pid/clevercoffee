
/**
 * @file Config.h
 *
 * @brief Unified configuration management system
 */

#pragma once

#include "GlobalVariables.h"
#include "Logger.h"
#include "defaults.h"
#include "utils/helperUtils.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <cmath>
#include <functional>
#include <map>
#include <optional>

enum class ParamType {
    INT = 0,
    UINT8 = 1,
    DOUBLE = 2,
    FLOAT = 3,
    STRING = 4,
    ENUM = 5,
    BOOL = 6
};

// Structure for enum options with explicit value mapping
struct EnumOption {
        int value;
        const char* label;
};

struct ParamDef {
        ParamType type;
        double minValue = 0.0;
        double maxValue = 0.0;
        size_t maxLength = 0;
        void* globalVar = nullptr;
        std::function<bool()> showCondition = []() { return true; };
        const char* displayName = "";
        const char* helpText = "";
        int section = 0;
        int position = 0;

        // Default values stored as variants
        bool defaultBool = false;
        int defaultInt = 0;
        uint8_t defaultUInt8 = 0;
        double defaultDouble = 0.0;
        float defaultFloat = 0.0f;
        ::String defaultString = "";

        // Enum support
        const EnumOption* enumOptions = nullptr;
        size_t enumCount = 0;

        static ParamDef Bool(bool* var, bool defaultVal, const char* name = "", int sec = 0, int pos = 0, const char* help = "") {
            ParamDef def;
            def.type = ParamType::BOOL;
            def.minValue = 0.0;
            def.maxValue = 1.0;
            def.globalVar = var;
            def.defaultBool = defaultVal;
            def.displayName = name;
            def.helpText = help;
            def.section = sec;
            def.position = pos;
            return def;
        }

        static ParamDef Int(int* var, int defaultVal, int min, int max, const char* name = "", int sec = 0, int pos = 0, const char* help = "") {
            ParamDef def;
            def.type = ParamType::INT;
            def.minValue = min;
            def.maxValue = max;
            def.globalVar = var;
            def.defaultInt = defaultVal;
            def.displayName = name;
            def.helpText = help;
            def.section = sec;
            def.position = pos;
            return def;
        }

        static ParamDef Double(double* var, double defaultVal, double min, double max, const char* name = "", int sec = 0, int pos = 0, const char* help = "") {
            ParamDef def;
            def.type = ParamType::DOUBLE;
            def.minValue = min;
            def.maxValue = max;
            def.globalVar = var;
            def.defaultDouble = defaultVal;
            def.displayName = name;
            def.helpText = help;
            def.section = sec;
            def.position = pos;
            return def;
        }

        static ParamDef UInt8(uint8_t* var, uint8_t defaultVal, uint8_t min, uint8_t max, const char* name = "", int sec = 0, int pos = 0, const char* help = "") {
            ParamDef def;
            def.type = ParamType::UINT8;
            def.minValue = min;
            def.maxValue = max;
            def.globalVar = var;
            def.defaultUInt8 = defaultVal;
            def.displayName = name;
            def.helpText = help;
            def.section = sec;
            def.position = pos;
            return def;
        }

        static ParamDef Float(float* var, float defaultVal, float min, float max, const char* name = "", int sec = 0, int pos = 0, const char* help = "") {
            ParamDef def;
            def.type = ParamType::FLOAT;
            def.minValue = min;
            def.maxValue = max;
            def.globalVar = var;
            def.defaultFloat = defaultVal;
            def.displayName = name;
            def.helpText = help;
            def.section = sec;
            def.position = pos;
            return def;
        }

        static ParamDef String(::String* var, const ::String& defaultVal, size_t maxLen, const char* name = "", int sec = 0, int pos = 0, const char* help = "") {
            ParamDef def;
            def.type = ParamType::STRING;
            def.maxLength = maxLen;
            def.globalVar = var;
            def.defaultString = defaultVal;
            def.displayName = name;
            def.helpText = help;
            def.section = sec;
            def.position = pos;
            return def;
        }

        static ParamDef Enum(int* var, int defaultVal, const EnumOption* options, size_t optionCount, const char* name = "", int sec = 0, int pos = 0, const char* help = "") {
            ParamDef def;
            def.type = ParamType::ENUM;
            def.globalVar = var;
            def.defaultInt = defaultVal;
            def.enumOptions = options;
            def.enumCount = optionCount;
            def.displayName = name;
            def.helpText = help;
            def.section = sec;
            def.position = pos;
            return def;
        }

        // Method to add show condition
        ParamDef& condition(std::function<bool()> cond) {
            showCondition = cond;
            return *this;
        }

        // Method to convert parameter to JSON format
        JsonObject toJson(JsonObject& obj, const ::String& name) const {
            obj["name"] = name;
            obj["displayName"] = displayName;
            obj["section"] = section;
            obj["position"] = position;
            obj["hasHelpText"] = (helpText != nullptr && strlen(helpText) > 0);
            obj["show"] = showCondition();
            obj["type"] = static_cast<int>(type);

            // Add type-specific values and constraints
            switch (type) {
                case ParamType::BOOL:
                    obj["value"] = globalVar ? *static_cast<bool*>(globalVar) : false;
                    obj["min"] = 0;
                    obj["max"] = 1;
                    break;
                case ParamType::INT:
                    obj["value"] = globalVar ? *static_cast<int*>(globalVar) : 0;
                    obj["min"] = minValue;
                    obj["max"] = maxValue;
                    break;
                case ParamType::UINT8:
                    obj["value"] = globalVar ? *static_cast<uint8_t*>(globalVar) : 0;
                    obj["min"] = minValue;
                    obj["max"] = maxValue;
                    break;
                case ParamType::DOUBLE:
                    obj["value"] = globalVar ? round2(*static_cast<double*>(globalVar)) : 0.0;
                    obj["min"] = minValue;
                    obj["max"] = maxValue;
                    break;
                case ParamType::FLOAT:
                    obj["value"] = globalVar ? round2(*static_cast<float*>(globalVar)) : 0.0f;
                    obj["min"] = minValue;
                    obj["max"] = maxValue;
                    break;
                case ParamType::STRING:
                    obj["value"] = globalVar ? static_cast<::String*>(globalVar)->c_str() : "";
                    obj["maxLength"] = maxLength;
                    break;
                case ParamType::ENUM:
                    obj["value"] = globalVar ? *static_cast<int*>(globalVar) : 0;
                    // Add enum options with proper value/label structure
                    if (enumOptions != nullptr && enumCount > 0) {
                        JsonArray options = obj["options"].to<JsonArray>();
                        for (size_t i = 0; i < enumCount; i++) {
                            JsonObject option = options.add<JsonObject>();
                            option["value"] = enumOptions[i].value;
                            option["label"] = enumOptions[i].label;
                        }
                    }
                    break;
                default:
                    obj["value"] = 0;
                    break;
            }

            return obj;
        }
};

class Config {
    public:
        static Config& getInstance() {
            static Config instance;
            return instance;
        }

        bool begin() {
            initializeParams();
            loadFromNVS();
            LOG(INFO, "Configuration system initialized");
            return true;
        }

        // Type-safe parameter access
        template <typename T>
        T get(const ::String& path) const {
            auto it = _params.find(path.c_str());
            if (it != _params.end()) {
                const ParamDef& def = it->second;
                if (def.globalVar) {
                    if constexpr (std::is_same_v<T, bool>) {
                        return *static_cast<bool*>(def.globalVar);
                    }
                    else if constexpr (std::is_same_v<T, int>) {
                        return *static_cast<int*>(def.globalVar);
                    }
                    else if constexpr (std::is_same_v<T, uint8_t>) {
                        return *static_cast<uint8_t*>(def.globalVar);
                    }
                    else if constexpr (std::is_same_v<T, double>) {
                        return *static_cast<double*>(def.globalVar);
                    }
                    else if constexpr (std::is_same_v<T, float>) {
                        return *static_cast<float*>(def.globalVar);
                    }
                    else if constexpr (std::is_same_v<T, ::String>) {
                        return *static_cast<::String*>(def.globalVar);
                    }
                }
            }
            return T{};
        }

        // Type-safe parameter setting with validation and NVS save
        template <typename T>
        bool set(const ::String& path, const T& value) {
            auto it = _params.find(path.c_str());
            if (it == _params.end()) return false;

            const ParamDef& def = it->second;

            // Validate numeric ranges
            if constexpr (std::is_arithmetic_v<T>) {
                if (def.minValue != def.maxValue) { // Only validate if range is set
                    if (value < static_cast<T>(def.minValue) || value > static_cast<T>(def.maxValue)) return false;
                }
            }
            if constexpr (std::is_same_v<T, ::String>) {
                if (def.maxLength > 0 && value.length() > def.maxLength) return false;
            }

            // Update global variable
            if (def.globalVar) {
                LOGF(DEBUG, "Config::set(%s): Updating global var from current value to %s", path.c_str(), String(value).c_str());
                *static_cast<T*>(def.globalVar) = value;
                LOGF(DEBUG, "Config::set(%s): Global var updated, now saving to NVS", path.c_str());
                saveToNVS(path.c_str(), value);
                LOGF(DEBUG, "Config::set(%s): NVS save completed", path.c_str());
                return true;
            }
            return false;
        }

        // JSON import/export
        bool loadFromJson(const ::String& jsonString);
        ::String exportToJson() const;

        // API support
        JsonDocument getParametersForAPI(const ::String& section = "") const;

        // Compatibility methods for existing webserver
        bool validateAndApplyFromJson(const ::String& jsonString) {
            return loadFromJson(jsonString);
        }
        void syncGlobalVariables() { /* Already handled by direct binding */
        }
        ::String generateJsonConfig() const {
            return exportToJson();
        }

        // NVS operations
        void loadFromNVS();
        void saveToNVS();

        // Reset parameter to default value
        bool resetToDefault(const ::String& path);

        // Reset all parameters to defaults
        void resetAllToDefaults();

        // Check if parameter exists
        bool hasParameter(const ::String& key) const {
            return _params.find(key.c_str()) != _params.end();
        }

        // Try to get parameter value, returns false if parameter doesn't exist or type mismatch
        template <typename T>
        bool tryGet(const ::String& key, T& value) const {
            auto it = _params.find(key.c_str());
            if (it == _params.end()) {
                return false;
            }

            const ParamDef& def = it->second;
            if (!def.globalVar) {
                return false;
            }

            try {
                if constexpr (std::is_same_v<T, bool>) {
                    if (def.type == ParamType::BOOL) {
                        value = *static_cast<bool*>(def.globalVar);
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, int>) {
                    if (def.type == ParamType::INT) {
                        value = *static_cast<int*>(def.globalVar);
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, uint8_t>) {
                    if (def.type == ParamType::UINT8) {
                        value = *static_cast<uint8_t*>(def.globalVar);
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, double>) {
                    if (def.type == ParamType::DOUBLE) {
                        value = *static_cast<double*>(def.globalVar);
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, float>) {
                    if (def.type == ParamType::FLOAT) {
                        value = *static_cast<float*>(def.globalVar);
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, ::String>) {
                    if (def.type == ParamType::STRING) {
                        value = *static_cast<::String*>(def.globalVar);
                        return true;
                    }
                }
                return false;
            } catch (...) {
                return false;
            }
        }

        // Get parameters map for webserver compatibility
        const std::map<std::string, ParamDef>& getParameters() const {
            return _params;
        }

        // Generate a short key for NVS storage (max 15 chars) - public for webserver use
        String generateNvsKey(const char* path) const {
            // Simple hash-based approach to create short unique keys
            uint32_t hash = 0;
            const char* str = path;
            while (*str) {
                hash = hash * 31 + *str++;
            }

            // Create a key with prefix + hash (max 15 chars)
            String key = "c" + String(hash, HEX);
            if (key.length() > 15) {
                key = key.substring(0, 15);
            }

            LOGF(DEBUG, "Config::generateNvsKey(%s): Using NVS key '%s'", path, key.c_str());
            return key;
        }

    private:
        std::map<std::string, ParamDef> _params;
        Preferences _prefs;

        // Local storage for parameters without global variables
        // if NVS value exist will be overwritten during boot
        int _brewMode = 0; // Default value for brew.mode

        void initializeParams();

        template <typename T>
        void saveToNVS(const char* path, const T& value) {
            // Look up the parameter definition to get the correct type
            auto it = _params.find(path);
            if (it == _params.end()) {
                LOGF(ERROR, "Config::saveToNVS(%s): Parameter not found in definition", path);
                return;
            }
            const ParamDef& def = it->second;

            String nvsKey = generateNvsKey(path);
            LOGF(INFO, "Config::saveToNVS(%s): Saving value '%s' with NVS key '%s'", path, String(value).c_str(), nvsKey.c_str());
            _prefs.begin(STORAGE_NAMESPACE, false);
            bool success = false;

            // Use parameter definition type, not template type, for storage decisions
            // But only cast when types are compatible
            switch (def.type) {
                case ParamType::BOOL:
                    if constexpr (std::is_same_v<T, bool>) {
                        success = _prefs.putBool(nvsKey.c_str(), value);
                        LOGF(DEBUG, "Config::saveToNVS(%s): putBool(%s, %s) = %s", path, nvsKey.c_str(), value ? "true" : "false", success ? "success" : "failed");
                    }
                    else if constexpr (std::is_arithmetic_v<T>) {
                        success = _prefs.putBool(nvsKey.c_str(), static_cast<bool>(value));
                        LOGF(DEBUG, "Config::saveToNVS(%s): putBool(%s, %s) = %s", path, nvsKey.c_str(), static_cast<bool>(value) ? "true" : "false", success ? "success" : "failed");
                    }
                    else {
                        LOGF(ERROR, "Config::saveToNVS(%s): Cannot convert non-arithmetic type to bool", path);
                    }
                    break;
                case ParamType::INT:
                case ParamType::ENUM: // ENUMs are stored as integers
                    if constexpr (std::is_same_v<T, int>) {
                        success = _prefs.putInt(nvsKey.c_str(), value);
                        LOGF(DEBUG, "Config::saveToNVS(%s): putInt(%s, %d) = %s", path, nvsKey.c_str(), value, success ? "success" : "failed");
                    }
                    else if constexpr (std::is_arithmetic_v<T>) {
                        success = _prefs.putInt(nvsKey.c_str(), static_cast<int>(value));
                        LOGF(DEBUG, "Config::saveToNVS(%s): putInt(%s, %d) = %s", path, nvsKey.c_str(), static_cast<int>(value), success ? "success" : "failed");
                    }
                    else {
                        LOGF(ERROR, "Config::saveToNVS(%s): Cannot convert non-arithmetic type to int", path);
                    }
                    break;
                case ParamType::UINT8:
                    if constexpr (std::is_same_v<T, uint8_t>) {
                        success = _prefs.putUChar(nvsKey.c_str(), value);
                        LOGF(DEBUG, "Config::saveToNVS(%s): putUChar(%s, %d) = %s", path, nvsKey.c_str(), value, success ? "success" : "failed");
                    }
                    else if constexpr (std::is_arithmetic_v<T>) {
                        success = _prefs.putUChar(nvsKey.c_str(), static_cast<uint8_t>(value));
                        LOGF(DEBUG, "Config::saveToNVS(%s): putUChar(%s, %d) = %s", path, nvsKey.c_str(), static_cast<uint8_t>(value), success ? "success" : "failed");
                    }
                    else {
                        LOGF(ERROR, "Config::saveToNVS(%s): Cannot convert non-arithmetic type to uint8_t", path);
                    }
                    break;
                case ParamType::DOUBLE:
                    if constexpr (std::is_same_v<T, double>) {
                        success = _prefs.putDouble(nvsKey.c_str(), value);
                        LOGF(DEBUG, "Config::saveToNVS(%s): putDouble(%s, %.6f) = %s", path, nvsKey.c_str(), value, success ? "success" : "failed");
                    }
                    else if constexpr (std::is_arithmetic_v<T>) {
                        success = _prefs.putDouble(nvsKey.c_str(), static_cast<double>(value));
                        LOGF(DEBUG, "Config::saveToNVS(%s): putDouble(%s, %.6f) = %s", path, nvsKey.c_str(), static_cast<double>(value), success ? "success" : "failed");
                    }
                    else {
                        LOGF(ERROR, "Config::saveToNVS(%s): Cannot convert non-arithmetic type to double", path);
                    }
                    break;
                case ParamType::FLOAT:
                    if constexpr (std::is_same_v<T, float>) {
                        success = _prefs.putFloat(nvsKey.c_str(), value);
                        LOGF(DEBUG, "Config::saveToNVS(%s): putFloat(%s, %.6f) = %s", path, nvsKey.c_str(), value, success ? "success" : "failed");
                    }
                    else if constexpr (std::is_arithmetic_v<T>) {
                        success = _prefs.putFloat(nvsKey.c_str(), static_cast<float>(value));
                        LOGF(DEBUG, "Config::saveToNVS(%s): putFloat(%s, %.6f) = %s", path, nvsKey.c_str(), static_cast<float>(value), success ? "success" : "failed");
                    }
                    else {
                        LOGF(ERROR, "Config::saveToNVS(%s): Cannot convert non-arithmetic type to float", path);
                    }
                    break;
                case ParamType::STRING:
                    if constexpr (std::is_same_v<T, ::String>) {
                        success = _prefs.putString(nvsKey.c_str(), value);
                        LOGF(DEBUG, "Config::saveToNVS(%s): putString(%s, '%s') = %s", path, nvsKey.c_str(), value.c_str(), success ? "success" : "failed");
                    }
                    else {
                        String strValue = String(value);
                        success = _prefs.putString(nvsKey.c_str(), strValue);
                        LOGF(DEBUG, "Config::saveToNVS(%s): putString(%s, '%s') = %s", path, nvsKey.c_str(), strValue.c_str(), success ? "success" : "failed");
                    }
                    break;
                default:
                    LOGF(ERROR, "Config::saveToNVS(%s): Unknown parameter type %d", path, static_cast<int>(def.type));
                    break;
            }
            _prefs.end();

            if (!success) {
                LOGF(ERROR, "Failed to save parameter '%s' to NVS (key: %s)", path, nvsKey.c_str());
            }
            else {
                LOGF(INFO, "Successfully saved parameter '%s' to NVS (key: %s)", path, nvsKey.c_str());
            }
        }
};

extern Config& config;
