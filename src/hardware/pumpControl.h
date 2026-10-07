#pragma once

enum class PumpControlType {
    RELAY,
    DIMMER
};

class PumpControl {
    public:
        virtual void on() = 0;
        virtual void off() = 0;
        virtual bool getState() const = 0;
        virtual PumpControlType getType() const = 0;
        virtual ~PumpControl() = default;
};