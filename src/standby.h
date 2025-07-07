/**
 * @file standby.h
 *
 * @brief Standby mode
 */

#pragma once

#define TIME_TO_DISPLAY_OFF        10
#define TIME_TO_DISPLAY_OFF_MILLIS (TIME_TO_DISPLAY_OFF * 60 * 1000)

inline unsigned long standbyModeStartTimeMillis = millis();
inline unsigned long standbyModeRemainingTimeMillis = static_cast<long>(standbyModeTime) * 60 * 1000;
inline unsigned long standbyModeRemainingTimeDisplayOffMillis = TIME_TO_DISPLAY_OFF_MILLIS;
inline unsigned long lastStandbyTimeMillis = standbyModeStartTimeMillis;
inline unsigned long timeSinceStandbyMillis = 0;

inline unsigned long getStandbyTimeoutMillis() {
    return static_cast<unsigned long>(standbyModeTime * 60 * 1000);
}

/**
 * @brief Decrements the remaining standby time every second, counting down from the configured duration
 */
inline void updateStandbyTimer() {
    if (!standbyModeOn) {
        return;
    }

    const unsigned long currentTime = millis();

    // Update every second since last update
    if (currentTime - lastStandbyTimeMillis >= 1000) {
        lastStandbyTimeMillis = currentTime;

        if (standbyModeRemainingTimeMillis != 0) {
            const unsigned long standbyModeTimeMillis = getStandbyTimeoutMillis();
            const unsigned long elapsedTime = currentTime - standbyModeStartTimeMillis;

            if (standbyModeTimeMillis > elapsedTime) {
                standbyModeRemainingTimeMillis = standbyModeTimeMillis - elapsedTime;

                if (elapsedTime % 60000 < 1000) {
                    LOGF(INFO, "Standby time remaining: %i minutes", (standbyModeRemainingTimeMillis / 60000) + 1);
                }
            }
            else {
                standbyModeRemainingTimeMillis = 0;
                LOG(INFO, "Entering standby mode...");
            }
        }
        else if (standbyModeRemainingTimeDisplayOffMillis != 0) {
            const unsigned long standbyModeTimeMillis = getStandbyTimeoutMillis() + TIME_TO_DISPLAY_OFF_MILLIS;
            const unsigned long elapsedTime = currentTime - standbyModeStartTimeMillis;

            if (standbyModeTimeMillis > elapsedTime) {
                standbyModeRemainingTimeDisplayOffMillis = standbyModeTimeMillis - elapsedTime;

                if (elapsedTime % 60000 < 1000) {
                    LOGF(INFO, "Standby time until display is turned off: %i minutes", (standbyModeRemainingTimeDisplayOffMillis / 60000) + 1);
                }
            }
            else {
                standbyModeRemainingTimeDisplayOffMillis = 0;
                LOG(INFO, "Turning off display...");
            }
        }
    }
}

inline void resetStandbyTimer(const MachineState state) {
    standbyModeRemainingTimeMillis = getStandbyTimeoutMillis();
    standbyModeRemainingTimeDisplayOffMillis = TIME_TO_DISPLAY_OFF_MILLIS;
    standbyModeStartTimeMillis = millis();

    LOGF(INFO, "Resetting standby timer to %i minutes", static_cast<int>(standbyModeTime));
    LOGF(DEBUG, "New machine state: %s", machinestateEnumToString(state));
}
