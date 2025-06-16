#include "Parameter.h"

// Constructor for general parameters with string getters/setters
Parameter::Parameter(const char* id,
                     const char* displayName,
                     EditableKind type,
                     int section,
                     int position,
                     std::function<double()> getter,
                     std::function<void(double)> setter,
                     double minValue,
                     double maxValue,
                     bool hasHelpText,
                     const char* helpText,
                     std::function<bool()> showCondition,
                     std::function<String()> stringGetter,
                     std::function<void(const String&)> stringSetter,
                     void* globalVariablePointer) :
    _id(id),
    _displayName(displayName),
    _type(type),
    _section(section),
    _position(position),
    _getter(std::move(getter)),
    _setter(std::move(setter)),
    _enumOptions(nullptr),
    _enumCount(0),
    _minValue(minValue),
    _maxValue(maxValue),
    _hasHelpText(hasHelpText),
    _helpText(helpText),
    _showCondition(std::move(showCondition)),
    _stringGetter(std::move(stringGetter)),
    _stringSetter(std::move(stringSetter)),
    _globalVariablePointer(globalVariablePointer) {
}

// Constructor for string parameters
Parameter::Parameter(const char* id,
                     const char* displayName,
                     EditableKind type,
                     int section,
                     int position,
                     const std::function<String()>& stringGetter,
                     const std::function<void(const String&)>& stringSetter,
                     double maxLength,
                     bool hasHelpText,
                     const char* helpText,
                     const std::function<bool()>& showCondition,
                     void* globalVariablePointer) :
    _id(id),
    _displayName(displayName),
    _type(type),
    _section(section),
    _position(position),
    _getter(nullptr),
    _setter(nullptr),
    _enumOptions(nullptr),
    _enumCount(0),
    _minValue(0),
    _maxValue(maxLength),
    _hasHelpText(hasHelpText),
    _helpText(helpText),
    _showCondition(showCondition),
    _stringGetter(stringGetter),
    _stringSetter(stringSetter),
    _globalVariablePointer(globalVariablePointer) {
}

// Constructor for numeric parameters (no string getter/setter)
Parameter::Parameter(const char* id,
                     const char* displayName,
                     EditableKind type,
                     int section,
                     int position,
                     std::function<double()> getter,
                     std::function<void(double)> setter,
                     double minValue,
                     double maxValue,
                     bool hasHelpText,
                     const char* helpText,
                     std::function<bool()> showCondition,
                     void* globalVariablePointer) :
    _id(id),
    _displayName(displayName),
    _type(type),
    _section(section),
    _position(position),
    _getter(std::move(getter)),
    _setter(std::move(setter)),
    _enumOptions(nullptr),
    _enumCount(0),
    _minValue(minValue),
    _maxValue(maxValue),
    _hasHelpText(hasHelpText),
    _helpText(helpText),
    _showCondition(std::move(showCondition)),
    _stringGetter(nullptr),
    _stringSetter(nullptr),
    _globalVariablePointer(globalVariablePointer) {
}

// Constructor for boolean parameters (using kUInt8 type with 0/1 values)
Parameter::Parameter(const char* id,
                     const char* displayName,
                     EditableKind type,
                     int section,
                     int position,
                     const std::function<bool()>& boolGetter,
                     const std::function<void(bool)>& boolSetter,
                     bool hasHelpText,
                     const char* helpText,
                     const std::function<bool()>& showCondition,
                     void* globalVariablePointer) :
    _id(id),
    _displayName(displayName),
    _type(type),
    _section(section),
    _position(position),
    _getter([boolGetter] { return boolGetter() ? 1.0 : 0.0; }),
    _setter([boolSetter](const double val) { boolSetter(val > 0.5); }),
    _enumOptions(nullptr),
    _enumCount(0),
    _minValue(0),
    _maxValue(1),
    _hasHelpText(hasHelpText),
    _helpText(helpText),
    _showCondition(showCondition),
    _stringGetter(nullptr),
    _stringSetter(nullptr),
    _globalVariablePointer(globalVariablePointer) {
}

// Constructor for enum parameters
Parameter::Parameter(const char* id,
                     const char* displayName,
                     EditableKind type,
                     int section,
                     int position,
                     const std::function<double()>& getter,
                     const std::function<void(double)>& setter,
                     const char* const* enumOptions,
                     size_t enumCount,
                     bool hasHelpText,
                     const char* helpText,
                     const std::function<bool()>& showCondition,
                     void* globalVariablePointer) :
    _id(id),
    _displayName(displayName),
    _type(type),
    _section(section),
    _position(position),
    _getter(getter),
    _setter(setter),
    _enumOptions(enumOptions),
    _enumCount(enumCount),
    _minValue(0),
    _maxValue(static_cast<double>(enumCount - 1)),
    _hasHelpText(hasHelpText),
    _helpText(helpText),
    _showCondition(showCondition),
    _stringGetter(nullptr),
    _stringSetter(nullptr),
    _globalVariablePointer(globalVariablePointer) {
}

const char* Parameter::getId() const {
    return _id;
}

const char* Parameter::getDisplayName() const {
    return _displayName;
}

EditableKind Parameter::getType() const {
    return _type;
}

int Parameter::getSection() const {
    return _section;
}

int Parameter::getPosition() const {
    return _position;
}

double Parameter::getValue() const {
    return _getter();
}

void Parameter::setValue(const double value) const {
    _setter(value);
    syncToGlobalVariable(value);
}

bool Parameter::getBoolValue() const {
    return getValue() != 0.0;
}

int Parameter::getIntValue() const {
    return static_cast<int>(getValue());
}

float Parameter::getFloatValue() const {
    return static_cast<float>(getValue());
}

uint8_t Parameter::getUInt8Value() const {
    return static_cast<uint8_t>(getValue());
}

String Parameter::getStringValue() const {
    if (_stringGetter) {
        return _stringGetter();
    }
    return {};
}

void Parameter::setStringValue(const String& value) const {
    if (_stringSetter) {
        _stringSetter(value);
        syncToGlobalVariable(value);
    }
}

double Parameter::getMinValue() const {
    return _minValue;
}

double Parameter::getMaxValue() const {
    return _maxValue;
}

bool Parameter::hasHelpText() const {
    return _hasHelpText;
}

const char* Parameter::getHelpText() const {
    return _helpText;
}

bool Parameter::shouldShow() const {
    return _showCondition();
}

String Parameter::getFormattedValue() const {
    switch (_type) {
        case kFloat:
            return String(getFloatValue());

        case kDouble:
            return String(getValue());

        case kInteger:
            return String(getIntValue());

        case kUInt8:
            return String(getUInt8Value());

        case kCString:
            return getStringValue();

        case kEnum:
            return getEnumDisplayValue();

        default:
            return "Unknown type";
    }
}

const char* const* Parameter::getEnumOptions() const {
    return _enumOptions;
}

size_t Parameter::getEnumCount() const {
    return _enumCount;
}

bool Parameter::isEnum() const {
    return _type == kEnum;
}

String Parameter::getEnumDisplayValue() const {
    if (!isEnum() || _enumOptions == nullptr) {
        return "";
    }

    const int index = static_cast<int>(getValue());

    return index >= 0 && index < static_cast<int>(_enumCount) ? String(_enumOptions[index]) : "";
}

void* Parameter::getGlobalVariablePointer() const {
    return _globalVariablePointer;
}

void Parameter::setGlobalVariablePointer(void* ptr) {
    _globalVariablePointer = ptr;
}

void Parameter::syncToGlobalVariable(const double value) const {
    if (_globalVariablePointer == nullptr) return;

    switch (_type) {
        case kInteger:
            *static_cast<int*>(_globalVariablePointer) = static_cast<int>(value);
            break;

        case kUInt8:
            *static_cast<uint8_t*>(_globalVariablePointer) = static_cast<uint8_t>(value);
            break;

        case kDouble:
            *static_cast<double*>(_globalVariablePointer) = value;
            break;

        case kFloat:
            *static_cast<float*>(_globalVariablePointer) = static_cast<float>(value);
            break;

        case kEnum:
            *static_cast<int*>(_globalVariablePointer) = static_cast<int>(value);
            break;

        case kCString:
            break;
    }
}

void Parameter::syncToGlobalVariable(const String& value) const {
    if (_globalVariablePointer == nullptr) {
        return;
    }

    if (_type == kCString) {
        *static_cast<String*>(_globalVariablePointer) = value;
    }
}