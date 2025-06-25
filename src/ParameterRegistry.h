
#pragma once

#include "Config.h"
#include "Parameter.h"
#include <map>
#include <memory>
#include <vector>

enum ParameterSection {
    sPIDSection = 0,
    sTempSection = 1,
    sBrewPidSection = 2,
    sBrewSection = 3,
    sScaleSection = 4,
    sDisplaySection = 5,
    sMaintenanceSection = 6,
    sPowerSection = 7,
    sMqttSection = 8,
    sSystemSection = 9,
    sOtherSection = 10,
    sHardwareOledSection = 11,
    sHardwareRelaySection = 12,
    sHardwareSwitchSection = 13,
    sHardwareLedSection = 14,
    sHardwareSensorSection = 15
};

inline const char* getSectionName(const int sectionId) {
    switch (sectionId) {
        case sPIDSection:
            return "PID Controller";
        case sTempSection:
            return "Temperature";
        case sBrewSection:
            return "Brew Control";
        case sBrewPidSection:
            return "Brew PID";
        case sDisplaySection:
            return "Display";
        case sPowerSection:
            return "Power Management";
        case sScaleSection:
            return "Scale";
        case sMaintenanceSection:
            return "Maintenance";
        case sMqttSection:
            return "MQTT";
        case sSystemSection:
            return "System";
        case sOtherSection:
            return "Other";
        case sHardwareOledSection:
            return "OLED";
        case sHardwareRelaySection:
            return "Relays";
        case sHardwareSwitchSection:
            return "Switches";
        case sHardwareLedSection:
            return "LEDs";
        case sHardwareSensorSection:
            return "Sensors";
        default:
            return "Unknown Section";
    }
}

class ParameterRegistry {
    private:
        ParameterRegistry() :
            _ready(false), _config(nullptr), _pendingChanges(false), _lastChangeTime(0) {
        }

        static ParameterRegistry _singleton;

        bool _ready;

        std::vector<std::shared_ptr<Parameter>> _parameters;
        std::map<std::string, std::shared_ptr<Parameter>> _parameterMap;
        Config* _config;
        bool _pendingChanges;
        unsigned long _lastChangeTime;
        static constexpr unsigned long SAVE_DELAY_MS = 2000;

        void addParam(const std::shared_ptr<Parameter>& param) {
            _parameters.push_back(param);
            _parameterMap[param->getId()] = param;
        }

    public:
        static ParameterRegistry& getInstance() {
            return _singleton;
        }

        [[nodiscard]] bool isReady() const {
            return _ready;
        }

        void initialize(Config& config);

        [[nodiscard]] const std::vector<std::shared_ptr<Parameter>>& getParameters() const {
            return _parameters;
        }

        void syncGlobalVariables() const;

        std::shared_ptr<Parameter> getParameterById(const char* id);

        template <typename T>
        bool setParameterValue(const char* id, const T& value) {
            const auto param = getParameterById(id);

            if (!param) {
                return false;
            }

            if constexpr (std::is_same_v<T, String> || std::is_same_v<T, std::string>) {
                // Handle string parameters
                if (param->getType() == kCString) {
                    param->setStringValue(value);
                }
                else {
                    const double numericValue = value.toDouble();
                    param->setValue(numericValue);
                }
            }
            else if constexpr (std::is_same_v<T, bool>) {
                // Handle boolean parameters
                param->setValue(value ? 1.0 : 0.0);
            }
            else {
                // Handle all numeric types (int, float, double, uint8_t, etc.)
                param->setValue(static_cast<double>(value));
            }

            markChanged();
            return true;
        }

        // Persistence management
        void processPeriodicSave() {
            if (!_config || !_pendingChanges) {
                return;
            }

            // Check if enough time has passed since last change
            if (millis() - _lastChangeTime > SAVE_DELAY_MS) {
                if (_config->save()) {
                    _pendingChanges = false;
                    LOG(INFO, "Configuration automatically saved to filesystem");
                }
            }
        }

        void forceSave() {
            if (!_config || !_pendingChanges) {
                LOG(INFO, "No pending changes, configuration not written to filesystem");
                return;
            }

            if (_config->save()) {
                _pendingChanges = false;
                LOG(INFO, "Configuration forcibly saved to filesystem");
            }
        }

        void markChanged() {
            _pendingChanges = true;
            _lastChangeTime = millis();
        }

        // Convenience method for adding string config parameters
        void addStringConfigParam(
            const char* configPath, const char* displayName, int section, int position, String* globalVar, double maxLength, const char* helpText = "", const std::function<bool()>& showCondition = [] { return true; }) {

            const auto param = std::make_shared<Parameter>(
                configPath, displayName, kCString, section, position, [this, configPath]() -> String { return _config->get<String>(configPath); },
                [this, configPath, globalVar](const String& val) {
                    _config->set<String>(configPath, val);
                    if (globalVar) *globalVar = val;
                },
                maxLength, !String(helpText).isEmpty(), helpText, showCondition, globalVar);

            addParam(param);
        }

        // Convenience method for adding boolean config parameters
        void addBoolConfigParam(const char* configPath, const char* displayName, int section, int position, bool* globalVar, const char* helpText = "", const std::function<bool()>& showCondition = [] { return true; }) {

            const auto param = std::make_shared<Parameter>(
                configPath, displayName, kUInt8, section, position, [this, configPath]() -> bool { return _config->get<bool>(configPath); },
                [this, configPath, globalVar](const bool val) {
                    _config->set<bool>(configPath, val);
                    if (globalVar) *globalVar = val;
                },
                !String(helpText).isEmpty(), helpText, showCondition, globalVar);

            addParam(param);
        }

        // Convenience method for adding numeric config parameters
        template <typename T>
        void addNumericConfigParam(
            const char* configPath, const char* displayName, EditableKind type, int section, int position, T* globalVar, double minValue, double maxValue, const char* helpText = "", std::function<bool()> showCondition = [] {
                return true;
            }) {

            auto param = std::make_shared<Parameter>(
                configPath, displayName, type, section, position, [this, configPath]() -> double { return static_cast<double>(_config->get<T>(configPath)); },
                [this, configPath, globalVar](const double val) {
                    T typedVal = static_cast<T>(val);
                    _config->set<T>(configPath, typedVal);
                    if (globalVar) *globalVar = typedVal;
                },
                minValue, maxValue, !String(helpText).isEmpty(), helpText, showCondition, globalVar);

            addParam(param);
        }

        // Convenience method for adding enum config parameters
        void addEnumConfigParam(
            const char* configPath,
            const char* displayName,
            int section,
            int position,
            int* globalVar,
            const char* const options[],
            int optionCount,
            const char* helpText = "",
            const std::function<bool()>& showCondition = [] { return true; }) {

            const auto param = std::make_shared<Parameter>(
                configPath, displayName, kEnum, section, position, [this, configPath]() -> double { return _config->get<int>(configPath); },
                [this, configPath, globalVar](const double val) {
                    const int intVal = static_cast<int>(val);
                    _config->set<int>(configPath, intVal);
                    if (globalVar) *globalVar = intVal;
                },
                options, optionCount, !String(helpText).isEmpty(), helpText, showCondition, globalVar);

            addParam(param);
        }
};