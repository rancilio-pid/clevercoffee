/**
 * @file standby.h
 * 
 * @brief Standby mode
*/

#pragma once

#define TIME_TO_DISPLAY_OFF 10

unsigned long standbyModeStartTimeMillis = millis();
unsigned long standbyModeRemainingTimeMillis = standbyModeTime * 60 * 1000;
unsigned long standbyModeRemainingTimeDisplayOffMillis = TIME_TO_DISPLAY_OFF * 60 * 1000;
unsigned long lastStandbyTimeMillis = standbyModeStartTimeMillis;
unsigned long timeSinceStandbyMillis = 0;

/**
 * @brief Decrements the remaining standby time every second, counting down from the configured duration 
 */
void updateStandbyTimer(void) {
    unsigned long currentTime = millis();

    if ((standbyModeRemainingTimeMillis != 0) && ((currentTime % 1000) == 0) && (currentTime != lastStandbyTimeMillis)) {
        unsigned long standbyModeTimeMillis = standbyModeTime * 60 * 1000;
        long elapsedTime = currentTime - standbyModeStartTimeMillis;
        lastStandbyTimeMillis = currentTime;

        if (standbyModeTimeMillis > elapsedTime) {
            standbyModeRemainingTimeMillis = standbyModeTimeMillis - elapsedTime;

            if ((currentTime % 60000) == 0) {
                LOGF(INFO, "Standby time remaining: %i minutes", standbyModeRemainingTimeMillis / 60000);
            }
        }
        else {
            standbyModeRemainingTimeMillis = 0;
            LOG(INFO, "Entering standby mode...");
        }
    }
    else if ((standbyModeRemainingTimeDisplayOffMillis != 0) && ((currentTime % 1000) == 0) && (currentTime != lastStandbyTimeMillis)) {
        unsigned long standbyModeTimeMillis = (standbyModeTime * 60 * 1000) + (TIME_TO_DISPLAY_OFF * 60 * 1000);
        long elapsedTime = currentTime - standbyModeStartTimeMillis;
        lastStandbyTimeMillis = currentTime;

        if (standbyModeTimeMillis > elapsedTime) {
            standbyModeRemainingTimeDisplayOffMillis = standbyModeTimeMillis - elapsedTime;

            if ((currentTime % 60000) == 0) {
                LOGF(INFO, "Standby time until display is turned off: %i minutes", standbyModeRemainingTimeDisplayOffMillis / 60000);
            }
        }
        else {
            standbyModeRemainingTimeDisplayOffMillis = 0;
            LOG(INFO, "Turning off display...");
        }
    }
}

void resetStandbyTimer(void) {
    standbyModeRemainingTimeMillis = standbyModeTime * 60 * 1000;
    standbyModeRemainingTimeDisplayOffMillis = TIME_TO_DISPLAY_OFF * 60 * 1000;
    standbyModeStartTimeMillis = millis();

    LOGF(INFO, "Resetting standby timer to %i minutes",  (int)standbyModeTime);
}
