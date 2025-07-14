#pragma once

#include "GPIOPin.h"
#include "pumpControl.h"

class PumpDimmer : public PumpControl {
    public:
        enum class ControlMethod {
            PHASE,
            PSM
        };

        PumpDimmer(GPIOPin& outputPin, GPIOPin& zeroCrossPin, int timerNum, bool hz);

        void begin();
        int getInterpolatedDelay(float powerPercent);
        void setPower(int power);
        void setPressure(float pressure);
        int getPower() const;
        void on();
        void off();
        bool getState() const;
        float getFrequency() const;
        float getFlow(float pressure) const;
        void setCalibration(float flowRate1, float flowRate2, float opvPressure);

        void setControlMethod(ControlMethod method);
        ControlMethod getControlMethod() const;
        PumpControlType getType() const override;

    private:
        GPIOPin& _out;
        GPIOPin& _zc;
        int _timerNum;
        int _power;
        float _scaledPower;
        int _psmAccumulated;
        float _pressure;
        bool _60hz = false;
        int _maxDelay = 5660;
        int _minDelay = 200;
        bool _state;
        uint32_t _delayMicros = 200;
        unsigned long _lastZC = 0;
        ControlMethod _method;
        hw_timer_t* _timer;
        float _flowRate1 = 292.4f;    // g in 30s using flush
        float _flowRate2 = 124.4f;    // g in 30s using water switch
        float _deltaFlow = -5.6f;     // difference in flow in mL/s
        float _opvPressureInv = 0.1f; // 10 Bars at OPV

        enum class TimerPhase {
            DELAY,
            RESET
        }; // Nested inside the class
        TimerPhase _phaseState;                         // Class member for phase dimmer state

        void resetPSMCounter();                         // PSM
        void handlePSMZeroCross();                      // PSM
        static void IRAM_ATTR onZeroCrossPSMStatic();   // PSM

        void handlePhaseZeroCross();                    // Phase
        static void IRAM_ATTR onZeroCrossPhaseStatic(); // Phase

        static PumpDimmer* instance;
};