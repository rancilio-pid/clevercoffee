
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
    sOtherSection = 10
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
        static const unsigned long SAVE_DELAY_MS = 2000;

    public:
        static ParameterRegistry& getInstance() {
            return _singleton;
        }

        bool isReady() const {
            return _ready;
        }

        void initialize(Config& config);

        const std::vector<std::shared_ptr<Parameter>>& getParameters() const {
            return _parameters;
        }

        void syncGlobalVariables() const;

        std::shared_ptr<Parameter> getParameterById(const char* id);

        bool setParameterValue(const char* id, double value) {
            auto param = getParameterById(id);

            if (!param) {
                return false;
            }

            param->setValue(value);
            markChanged();

            return true;
        }

        bool setParameterBoolValue(const char* id, bool value) {
            auto param = getParameterById(id);

            if (!param) {
                return false;
            }

            param->setValue(value ? 1.0 : 0.0);
            markChanged();

            return true;
        }

        bool setParameterIntValue(const char* id, int value) {
            auto param = getParameterById(id);

            if (!param) {
                return false;
            }

            param->setValue(static_cast<double>(value));
            markChanged();

            return true;
        }

        bool setParameterFloatValue(const char* id, float value) {
            auto param = getParameterById(id);

            if (!param) {
                return false;
            }

            param->setValue(static_cast<double>(value));
            markChanged();

            return true;
        }

        bool setParameterDoubleValue(const char* id, double value) {
            auto param = getParameterById(id);

            if (!param) {
                return false;
            }

            param->setValue(value);
            markChanged();

            return true;
        }

        bool setParameterUInt8Value(const char* id, uint8_t value) {
            auto param = getParameterById(id);

            if (!param) {
                return false;
            }

            param->setValue(static_cast<double>(value));
            markChanged();

            return true;
        }

        bool setParameterStringValue(const char* id, const String& value) {
            auto param = getParameterById(id);

            if (!param) {
                return false;
            }

            if (param->getType() == kCString) {
                param->setStringValue(value);
            }
            else {
                double numericValue = value.toDouble();
                param->setValue(numericValue);
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
            if ((millis() - _lastChangeTime) > SAVE_DELAY_MS) {
                if (_config->save()) {
                    _pendingChanges = false;
                    LOG(INFO, "Configuration automatically saved to filesystem");
                }
            }
        }

        void forceSave() {
            if (!_config || !_pendingChanges) {
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
};