#include "Timer.h"

#include "Arduino.h"
#include "Logger.h"

Timer::Timer(std::function<void()> func, unsigned long interval) :
    callback_(func), interval_(interval), next_(millis()) {};

void Timer::operator()() {
    if (millis() >= next_) {
        LOG(TRACE, "Timer expired, calling function");

        // Update timer
        next_ = millis() + interval_;

        // Call method:
        callback_();
    }
}

void Timer::reset() {
    next_ = millis();
}
