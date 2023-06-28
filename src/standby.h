/**
 * @file standby.h
 * 
 * @brief Standby mode
*/

#pragma once

unsigned long standbyModeStartTimeMillis = millis();
unsigned long standbyModeRemainingTimeMillis = standbyModeTime * 60 * 1000;
unsigned long lastStandbyTimeMillis = standbyModeStartTimeMillis;

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
                debugPrintf("Standby time remaining: %i minutes\n", standbyModeRemainingTimeMillis / 60000);
            }
        }
        else {
            standbyModeRemainingTimeMillis = 0;
        }
    }
}

void resetStandbyTimer(void) {
    standbyModeRemainingTimeMillis = standbyModeTime * 60 * 10000;
    standbyModeStartTimeMillis = millis();

    debugPrintf("Resetting standby timer to %i minutes\n",  (int)standbyModeTime);
}