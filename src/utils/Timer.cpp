#include "Timer.h"

#include "Arduino.h"
#include "Logger.h"

void Timer::operator()() {
    auto current = millis();

    if ((current - last_) >= interval_) {
        LOG(TRACE, "Timer expired, calling function");

        // Update timer
        last_ = current;

        // Call method:
        callback_();
    }
}
