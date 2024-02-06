/**
 * @file PeriodicTrigger.h
 *
 * @brief Programmable timer interval
 * 
 */

#pragma once

class PeriodicTrigger {
 public:
    PeriodicTrigger(unsigned long millisec);

    /**
     * @brief Check if the interval period has elapsed
     * @return Returns true until the timer is reset
    */
    bool check();

    /**
     * @brief Reset timer interval to currently configured value
    */
    void reset();

    /**
     * @brief Reset timer with new timer interval
    */
    void reset(unsigned long millisec);

 private:
    unsigned long m_triggerInterval;
    unsigned long m_tref;
};

