#include "Dimmers.h"

unsigned int delayLowLut[9] = {5695, 5343, 4992, 4673, 4365, 4030, 3607, 3149, 2630};
unsigned int delayHighLut[21] = {2630, 2578, 2514, 2440, 2367, 2293, 2219, 2146, 2072, 1998, 1924, 1851, 1777, 1641, 1498, 1355, 1213, 1070, 853, 526, 200};

const char* controlMethodToString(PumpDimmer::ControlMethod method) {
    switch (method) {
        case PumpDimmer::ControlMethod::PSM:
            return "PSM";

        case PumpDimmer::ControlMethod::PHASE:
            return "PHASE";

        default:
            return "Unknown";
    }
}

PumpDimmer* PumpDimmer::instance = nullptr;

PumpDimmer::PumpDimmer(GPIOPin& outputPin, GPIOPin& zeroCrossPin, int timerNum, bool hz) :
    _out(outputPin), _zc(zeroCrossPin), _timerNum(timerNum), _60hz(hz), _power(0), _psmAccumulated(0), _state(false), _lastZC(0), _method(ControlMethod::PSM), _timer(nullptr) {
    instance = this;
}

void PumpDimmer::begin() {
    _out.write(LOW);

    static bool adjusted = false; // flag to ensure scaling only happens once

    if (_60hz && adjusted == false) {

        for (int i = 0; i < 9; i++) {
            delayLowLut[i] = (unsigned int)(delayLowLut[i] * 0.8333f);
        }

        for (int i = 0; i < 21; i++) {
            delayHighLut[i] = (unsigned int)(delayHighLut[i] * 0.8333f);
        }

        adjusted = true;
    }

    _timer = timerBegin(_timerNum, 80, true); // 80 prescaler = 1 µs ticks (assuming 80 MHz APB clock)

    timerAttachInterrupt(
        _timer,
        []() {
            if (instance->_phaseState == TimerPhase::DELAY) {
                instance->_out.write(HIGH);
                instance->_phaseState = TimerPhase::RESET;
                timerWrite(instance->_timer, 0);
                timerAlarmWrite(instance->_timer, 100, false); // Reset in 100 µs
                timerAlarmEnable(instance->_timer);
            }
            else {
                instance->_out.write(LOW);
                timerAlarmDisable(instance->_timer); // Done for this cycle
            }
        },
        true);

    if (_method == ControlMethod::PHASE) {
        attachInterrupt(_zc.getPin(), onZeroCrossPhaseStatic, RISING);
    }
    else {
        attachInterrupt(_zc.getPin(), onZeroCrossPSMStatic, RISING);
    }
}

int PumpDimmer::getInterpolatedDelay(float powerPercent) {
    if (powerPercent >= 100) {
        return delayHighLut[20];
    }

    if (powerPercent <= 0) {
        return delayLowLut[0];
    }

    if (powerPercent < 80.0f) {
        // Use coarse LUT
        float stepSize = 10.0f;
        int index = powerPercent / 10;
        float fraction = (powerPercent - (index * stepSize)) / stepSize;
        float delayLow = delayLowLut[index];
        float delayHigh = delayLowLut[index + 1];

        return delayLow + fraction * (delayHigh - delayLow);
    }
    else {
        // Use fine LUT
        float finePower = powerPercent - 80.0f; // 0 to 20
        int index = finePower;                  // Integer percent (0–20)
        float fraction = finePower - index;
        float delayLow = delayHighLut[index];
        float delayHigh = delayHighLut[index + 1];

        return delayLow + fraction * (delayHigh - delayLow);
    }
}

void PumpDimmer::setPower(int power) {
    _power = constrain((int)power, 0, 100);

    if (_method == ControlMethod::PHASE) {
        float pressureScaler = _pressure * 6.0f;
        _scaledPower = pressureScaler + (100 - pressureScaler) * (_power * 0.01f);
        _delayMicros = instance->getInterpolatedDelay(_scaledPower); // Lower delay = more power (fired earlier)
    }
}

void PumpDimmer::setPressure(float pressure) {
    _pressure = pressure;
}

int PumpDimmer::getPower() const {
    return _power;
}

void PumpDimmer::on() {
    if (!_state && _method == ControlMethod::PSM) {
        resetPSMCounter();
    }

    _state = true;
}

void PumpDimmer::off() {
    _state = false;
    _out.write(LOW);
}

bool PumpDimmer::getState() const {
    return _state;
}

float PumpDimmer::getFrequency() const {
    return _60hz ? 60 : 50;
}

void PumpDimmer::setCalibration(float flowRate1, float flowRate2, float opvPressure) {
    _opvPressureInv = 1.0f / opvPressure;
    _flowRate1 = flowRate1 * 0.03333333f;
    _flowRate2 = flowRate2 * 0.03333333f;
    _deltaFlow = _flowRate2 - _flowRate1;
}

float PumpDimmer::getFlow(float pressure) const {
    float result = 0.0f;

    if (_method == ControlMethod::PSM) {
        float powerMultiplier = _state ? float(_power) * 0.01f : 0.0f;
        result = powerMultiplier * (_deltaFlow * _opvPressureInv * pressure + _flowRate1);
    }
    else {
        float powerMultiplier = _state ? _scaledPower * 0.01f : 0.0f;
        result = powerMultiplier * _flowRate1 - 0.06f * (1 - powerMultiplier) * pressure * _flowRate1 + pressure * _deltaFlow * _opvPressureInv;
    }

    return result > 0.0f ? result : 0.0f;
}

void PumpDimmer::setControlMethod(ControlMethod method) {
    if (_method == method) {
        return;
    }

    detachInterrupt(_zc.getPin());
    _method = method;

    if (_method == ControlMethod::PHASE) {
        attachInterrupt(_zc.getPin(), onZeroCrossPhaseStatic, RISING);
    }
    else {
        attachInterrupt(_zc.getPin(), onZeroCrossPSMStatic, RISING);
    }
}

PumpDimmer::ControlMethod PumpDimmer::getControlMethod() const {
    return _method;
}

// PSM method
void PumpDimmer::resetPSMCounter() {
    _psmAccumulated = 0;
}

void PumpDimmer::handlePSMZeroCross() {
    unsigned long now = millis();

    if (now - _lastZC < 15) {
        return; // Debounce
    }

    _lastZC = now;

    if (_power <= 0 || !_state) {
        _out.write(LOW);
        return;
    }

    _psmAccumulated += _power;

    if (_psmAccumulated >= 100) {
        _out.write(HIGH);
        _psmAccumulated -= 100;
    }
    else {
        _out.write(LOW);
    }
}

void IRAM_ATTR PumpDimmer::onZeroCrossPSMStatic() {
    if (instance) {
        instance->handlePSMZeroCross();
    }
}

// Phase method
void PumpDimmer::handlePhaseZeroCross() {
    if (_power <= 0 || !_state) {
        _out.write(LOW);
        return;
    }

    _phaseState = TimerPhase::DELAY;
    timerWrite(_timer, 0);
    timerAlarmDisable(_timer);
    timerAlarmWrite(_timer, _delayMicros, false);
    timerAlarmEnable(_timer);
}

void IRAM_ATTR PumpDimmer::onZeroCrossPhaseStatic() {
    if (instance) {
        instance->handlePhaseZeroCross();
    }
}

PumpControlType PumpDimmer::getType() const {
    return PumpControlType::DIMMER;
}