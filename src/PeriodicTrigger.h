/**
 * @file PeriodicTrigger.h
 *
 * @brief
 */

#ifndef PeriodicTrigger_h
#define PeriodicTrigger_h

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

#endif
