#include "Timer.h"

#include "Arduino.h"
#include "Logger.h"

Timer::Timer(std::function<void()> func, unsigned long interval, bool start_paused) :
    callback_(func), interval_(interval), next_(millis()), running_(!start_paused) {};

void Timer::pause() {
    running_ = false;
}

void Timer::resume() {
    running_ = true;
}

void Timer::operator()() {
    if (running_ && millis() >= next_) {
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
