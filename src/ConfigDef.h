
#pragma once

#include <Arduino.h>
#include <Logger.h>

struct ConfigDef {
        enum Type {
            BOOL,
            INT,
            DOUBLE,
            STRING
        };

        double minValue = 0.0;
        double maxValue = 0.0;
        size_t maxLength = 0;
        Type type = BOOL;
        bool boolVal = false;
        int intVal = 0;
        double doubleVal = 0.0;
        String stringVal;

        // Factory functions
        static ConfigDef forBool(const bool defaultVal) {
            ConfigDef def;
            def.minValue = 0;
            def.maxValue = 1;
            def.maxLength = 0;
            def.type = BOOL;
            def.boolVal = defaultVal;
            return def;
        }

        static ConfigDef forInt(const int defaultVal, const double min, const double max) {
            ConfigDef def;
            def.minValue = min;
            def.maxValue = max;
            def.maxLength = 0;
            def.type = INT;
            def.intVal = defaultVal;
            return def;
        }

        static ConfigDef forDouble(const double defaultVal, const double min, const double max) {
            ConfigDef def;
            def.minValue = min;
            def.maxValue = max;
            def.maxLength = 0;
            def.type = DOUBLE;
            def.doubleVal = defaultVal;
            return def;
        }

        static ConfigDef forString(const String& defaultVal, const size_t maxLen) {
            ConfigDef def;
            def.minValue = 0;
            def.maxValue = 0;
            def.maxLength = maxLen;
            def.type = STRING;
            def.stringVal = defaultVal;
            return def;
        }
};
