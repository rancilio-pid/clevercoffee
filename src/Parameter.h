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

        // For string parameters
        Parameter(
            const char* id,
            const char* displayName,
            EditableKind type,
            int section,
            int position,
            const std::function<String()>& stringGetter,
            const std::function<void(const String&)>& stringSetter,
            double maxLength, // Maximum string length
            bool hasHelpText = false,
            const char* helpText = "",
            const std::function<bool()>& showCondition = [] { return true; },
            void* globalVariablePointer = nullptr) :
            _id(id),
            _displayName(displayName),
            _type(type),
            _section(section),
            _position(position),
            _getter(nullptr),     // Not used for strings
            _setter(nullptr),     // Not used for strings
            _minValue(0),         // Not applicable for strings
            _maxValue(maxLength), // Used for string length validation
            _hasHelpText(hasHelpText),
            _helpText(helpText),
            _showCondition(showCondition),
            _stringGetter(stringGetter),
            _stringSetter(stringSetter),
            _globalVariablePointer(globalVariablePointer) {
        }

        // For numeric parameters (no string getter/setter)
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
            _stringGetter(nullptr), // Not used for numeric
            _stringSetter(nullptr), // Not used for numeric
            _globalVariablePointer(globalVariablePointer) {
        }

        // For boolean parameters (using kUInt8 type with 0/1 values)
        Parameter(
            const char* id,
            const char* displayName,
            EditableKind type,
            int section,
            int position,
            const std::function<bool()>& boolGetter,
            const std::function<void(bool)>& boolSetter,
            bool hasHelpText = false,
            const char* helpText = "",
            const std::function<bool()>& showCondition = [] { return true; },
            void* globalVariablePointer = nullptr) :
            _id(id),
            _displayName(displayName),
            _type(type),
            _section(section),
            _position(position),
            _getter([boolGetter]() { return boolGetter() ? 1.0 : 0.0; }),
            _setter([boolSetter](double val) { boolSetter(val > 0.5); }),
            _minValue(0),
            _maxValue(1),
            _hasHelpText(hasHelpText),
            _helpText(helpText),
            _showCondition(showCondition),
            _stringGetter(nullptr),
            _stringSetter(nullptr),
            _globalVariablePointer(globalVariablePointer) {
        }

        [[nodiscard]] const char* getId() const {
            return _id;
        }

        [[nodiscard]] const char* getDisplayName() const {
            return _displayName;
        }

        [[nodiscard]] EditableKind getType() const {
            return _type;
        }

        [[nodiscard]] int getSection() const {
            return _section;
        }

        [[nodiscard]] int getPosition() const {
            return _position;
        }

        [[nodiscard]] double getValue() const {
            return _getter();
        }

        void setValue(double value) const {
            _setter(value);
            syncToGlobalVariable(value);
        }

        [[nodiscard]] bool getBoolValue() const {
            return getValue() != 0.0;
        }

        [[nodiscard]] int getIntValue() const {
            return static_cast<int>(getValue());
        }

        [[nodiscard]] float getFloatValue() const {
            return static_cast<float>(getValue());
        }

        [[nodiscard]] uint8_t getUInt8Value() const {
            return static_cast<uint8_t>(getValue());
        }

        [[nodiscard]] String getStringValue() const {
            if (_stringGetter) {
                return _stringGetter();
            }
            return {};
        }

        void setStringValue(const String& value) {
            if (_stringSetter) {
                _stringSetter(value);
                syncToGlobalVariable(value);
            }
        }

        [[nodiscard]] double getMinValue() const {
            return _minValue;
        }

        [[nodiscard]] double getMaxValue() const {
            return _maxValue;
        }

        [[nodiscard]] bool hasHelpText() const {
            return _hasHelpText;
        }

        [[nodiscard]] const char* getHelpText() const {
            return _helpText;
        }

        [[nodiscard]] bool shouldShow() const {
            return _showCondition();
        }

        [[nodiscard]] String getFormattedValue() const {
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

        [[nodiscard]] void* getGlobalVariablePointer() const {
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
