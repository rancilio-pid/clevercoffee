/**
 * @file Timer.h
 *
 * @brief A helper class to call a method in regular intervals
 */

#pragma once

#include <functional>

class Timer {
    public:
        Timer() = delete;
        Timer(std::function<void()> func, unsigned long interval) :
            callback_(func), interval_(interval){};
        void operator()();

    private:
        // Callback to be executed when timer runs out:
        std::function<void()> callback_;

        // Interval of timer execution
        unsigned long interval_;

        // Last time the timer ran out
        unsigned long last_{0};
};
