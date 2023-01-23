/**
 * @file PeriodicTrigger.h
 *
 * @brief TODO
 *
 */

#pragma once

class PeriodicTrigger {
 public:
    PeriodicTrigger(unsigned long millisec);

    bool check();
    void reset();
    void reset(unsigned long millisec);

 private:
    unsigned long m_triggerInterval;
    unsigned long m_tref;
};

