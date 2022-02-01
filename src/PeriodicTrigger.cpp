/**
 * @file PeriodicTrigger.cpp
 *
 * @brief
 */

#include "PeriodicTrigger.h"

#include <Arduino.h>

PeriodicTrigger::PeriodicTrigger(unsigned long ms) {
    m_triggerInterval = ms;
    m_tref = millis();
}

bool PeriodicTrigger::check() {
    if ((millis() - m_tref) > m_triggerInterval) {
        m_tref += m_triggerInterval;
        return true;
    } else {
        return false;
    }
}

void PeriodicTrigger::reset() {
    m_tref = millis();
}

void PeriodicTrigger::reset(unsigned long ms) {
    m_triggerInterval = ms;
    m_tref = millis();
}
