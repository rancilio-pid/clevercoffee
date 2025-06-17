
#pragma once

#include "defaults.h"
#include <Arduino.h>
#include <ArduinoJson.h>

struct ConfigDef {
        double minValue = 0.0;
        double maxValue = 0.0;
        size_t maxLength = 0;

        // Factory functions
        static ConfigDef forBool(bool defaultVal) {
            return {0, 1, 0, defaultVal};
        }

        static ConfigDef forInt(int defaultVal, double min, double max) {
            return {min, max, 0, defaultVal};
        }

        static ConfigDef forDouble(double defaultVal, double min, double max) {
            return {min, max, 0, defaultVal};
        }

        static ConfigDef forString(const char* defaultVal, size_t maxLen) {
            return {0, 0, maxLen, String(defaultVal)};
        }

        [[nodiscard]] JsonVariant getDefaultValue() const {
            return defaultValue;
        }

    private:
        JsonVariant defaultValue;

        template <typename T>
        ConfigDef(const double min, const double max, const size_t maxLen, const T& defaultVal) :
            minValue(min), maxValue(max), maxLength(maxLen) {
            defaultValue.set(defaultVal);
        }
};
