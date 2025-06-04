#pragma once

#include <functional>
#include <utility>

enum EditableKind {
    kInteger,
    kUInt8,
    kDouble,
    kDoubletime,
    kCString,
    kFloat
};

class Parameter {
    public:
        Parameter(
            const char* id,
            const char* displayName,
            EditableKind type,
            int section,
            int position,
            std::function<double()> getter,
            std::function<void(double)> setter,
            double minValue,
            double maxValue,
            bool hasHelpText = false,
            const char* helpText = "",
            std::function<bool()> showCondition = [] { return true; },
            std::function<String()> stringGetter = nullptr,
            std::function<void(const String&)> stringSetter = nullptr,
            void* globalVariablePointer = nullptr) :
            _id(id),
            _displayName(displayName),
            _type(type),
            _section(section),
            _position(position),
            _getter(std::move(getter)),
            _setter(std::move(setter)),
            _minValue(minValue),
            _maxValue(maxValue),
            _hasHelpText(hasHelpText),
            _helpText(helpText),
            _showCondition(std::move(showCondition)),
            _stringGetter(std::move(stringGetter)),
            _stringSetter(std::move(stringSetter)),
            _globalVariablePointer(globalVariablePointer) {
        }

        const char* getId() const {
            return _id;
        }

        const char* getDisplayName() const {
            return _displayName;
        }

        EditableKind getType() const {
            return _type;
        }

        int getSection() const {
            return _section;
        }

        int getPosition() const {
            return _position;
        }

        double getValue() const {
            return _getter();
        }

        void setValue(double value) {
            _setter(value);
            syncToGlobalVariable(value);
        }

        bool getBoolValue() const {
            return getValue() != 0.0;
        }

        int getIntValue() const {
            return static_cast<int>(getValue());
        }

        float getFloatValue() const {
            return static_cast<float>(getValue());
        }

        uint8_t getUInt8Value() const {
            return static_cast<uint8_t>(getValue());
        }

        String getStringValue() const {
            if (_stringGetter) {
                return _stringGetter();
            }
            return String();
        }

        void setStringValue(const String& value) {
            if (_stringSetter) {
                _stringSetter(value);
                syncToGlobalVariable(value);
            }
        }

        double getMinValue() const {
            return _minValue;
        }

        double getMaxValue() const {
            return _maxValue;
        }

        bool hasHelpText() const {
            return _hasHelpText;
        }

        const char* getHelpText() const {
            return _helpText;
        }

        bool shouldShow() const {
            return _showCondition();
        }

        String getFormattedValue() const {
            switch (_type) {
                case kFloat:
                    return String(getFloatValue());
                case kDouble:
                case kDoubletime:
                    return String(getValue());
                case kInteger:
                    return String(getIntValue());
                case kUInt8:
                    return String(getUInt8Value());
                case kCString:
                    return getStringValue();
                default:
                    return "Unknown type";
            }
        }

        void* getGlobalVariablePointer() const {
            return _globalVariablePointer;
        }

        void setGlobalVariablePointer(void* ptr) {
            _globalVariablePointer = ptr;
        }

        void syncToGlobalVariable(double value) const {
            if (_globalVariablePointer == nullptr) return;

            switch (_type) {
                case kInteger:
                    *static_cast<int*>(_globalVariablePointer) = static_cast<int>(value);
                    break;

                case kUInt8:
                    *static_cast<uint8_t*>(_globalVariablePointer) = static_cast<uint8_t>(value);
                    break;

                case kDouble:
                case kDoubletime:
                    *static_cast<double*>(_globalVariablePointer) = value;
                    break;

                case kFloat:
                    *static_cast<float*>(_globalVariablePointer) = static_cast<float>(value);
                    break;

                case kCString:
                    break;
            }
        }

        void syncToGlobalVariable(const String& value) const {
            if (_globalVariablePointer == nullptr) {
                return;
            }

            if (_type == kCString) {
                *static_cast<String*>(_globalVariablePointer) = value;
            }
        }

    private:
        const char* _id;
        const char* _displayName;
        EditableKind _type;
        int _section;
        int _position;
        std::function<double()> _getter;
        std::function<void(double)> _setter;
        double _minValue;
        double _maxValue;
        bool _hasHelpText;
        const char* _helpText;
        std::function<bool()> _showCondition;
        std::function<String()> _stringGetter;
        std::function<void(const String&)> _stringSetter;
        void* _globalVariablePointer;
};
