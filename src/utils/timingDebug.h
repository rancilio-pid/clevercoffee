/**
 * @file timingDebug.h
 *
 * @brief Stores and prints long duration processes to the console if DEBUG is enabled
 *
 */

#pragma once

enum ActivityType : uint16_t {
    ACT_DISPLAY_READY = 0x01,
    ACT_DISPLAY_RUNNING = 0x02,
    ACT_WEBSITE_RUNNING = 0x04,
    ACT_MQTT_RUNNING = 0x08,
    ACT_HASSIO_RUNNING = 0x10,
    ACT_TEMPERATURE_RUNNING = 0x20
};

/**
 * @brief print what has caused the long loop time
 * @return void
 */
void printActivityFlags(const uint16_t* activity, uint16_t size) {
    char activityBuffer[180];
    int len = 0;

    len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "Activity (short): [");

    for (int i = 0; i < size; ++i) {

        // Convert flags to short notation
        if (activity[i] == 0) {
            len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "_");
        }
        else {
            // append a short ID tag based on what processes happened in the loop
            if (activity[i] & ACT_DISPLAY_READY) {
                len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "r");
            }

            if (activity[i] & ACT_DISPLAY_RUNNING) {
                len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "D");
            }

            if (activity[i] & ACT_WEBSITE_RUNNING) {
                len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "W");
            }

            if (activity[i] & ACT_MQTT_RUNNING) {
                len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "M");
            }

            if (activity[i] & ACT_HASSIO_RUNNING) {
                len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "H");
            }

            if (activity[i] & ACT_TEMPERATURE_RUNNING) {
                len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "T");
            }
        }

        if (i < size - 1) {
            len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, ",");
        }
    }

    len += snprintf(activityBuffer + len, sizeof(activityBuffer) - len, "]");
    LOGF(DEBUG, "%s -- length %i", activityBuffer, len);
}

/**
 * @brief Print both timing and compact activity in one batch
 * @return void
 */
void printTimingAndActivityBatch(const uint16_t* timing, const uint16_t* activity, uint16_t size) {
    char timingBuffer[180];
    int tLen = 0;

    tLen += snprintf(timingBuffer + tLen, sizeof(timingBuffer) - tLen, "Loop timing (ms): [");

    for (int i = 0; i < size; ++i) {
        tLen += snprintf(timingBuffer + tLen, sizeof(timingBuffer) - tLen, "%u", timing[i]);
        if (i < size - 1) tLen += snprintf(timingBuffer + tLen, sizeof(timingBuffer) - tLen, ",");
    }
    tLen += snprintf(timingBuffer + tLen, sizeof(timingBuffer) - tLen, "]");

    // Only two log lines total
    LOGF(DEBUG, "%s -- length %i", timingBuffer, tLen);
    printActivityFlags(activity, size);
}

/**
 * @brief Store all long duration activities and their loop times, send when array is full
 * @return void
 */
void debugTimingLoop() {
    static const uint16_t LOOP_HISTORY_SIZE = 20;
    static uint16_t loopTiming[LOOP_HISTORY_SIZE];
    static uint16_t activityType[LOOP_HISTORY_SIZE];
    static unsigned long previousMillisDebug = millis();
    static unsigned long lastSendMillisDebug = millis();
    static uint16_t loopIndex = 0;
    static uint16_t loopCount = 0;
    static uint16_t maxLoop = 0;
    static unsigned long lastHeapSent = millis();

    if (millis() - lastHeapSent > 5000) {
        LOGF(DEBUG, "[Heap] Free: %u  MaxAlloc: %u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        lastHeapSent += 5000;
    }

    if (timingDebugActive) {
        loopCount += 1;
        uint16_t loopDuration = static_cast<uint16_t>(millis() - previousMillisDebug);
        previousMillisDebug = millis();

        // the loopDuration > 45 check is in case there is a long loop caused by something unknown
        // only record if one of these flags are set or loop has taken a long time
        if ((loopDuration > 45) || ((displayUpdateRunning && includeDisplayInLogs) || websiteUpdateRunning || mqttUpdateRunning || hassioUpdateRunning || temperatureUpdateRunning)) {

            if (loopDuration >= maxLoop) {
                maxLoop = loopDuration;
            }

            loopTiming[loopIndex] = loopDuration;
            activityType[loopIndex] = 0;

            // tag activityType with any activities that occured this loop
            if (displayBufferReady) {
                activityType[loopIndex] |= ACT_DISPLAY_READY;
            }

            if (displayUpdateRunning) {
                activityType[loopIndex] |= ACT_DISPLAY_RUNNING;
            }

            if (websiteUpdateRunning) {
                activityType[loopIndex] |= ACT_WEBSITE_RUNNING;
            }

            if (mqttUpdateRunning) {
                activityType[loopIndex] |= ACT_MQTT_RUNNING;
            }

            if (hassioUpdateRunning) {
                activityType[loopIndex] |= ACT_HASSIO_RUNNING;
            }

            if (temperatureUpdateRunning) {
                activityType[loopIndex] |= ACT_TEMPERATURE_RUNNING;
            }

            loopIndex = (loopIndex + 1) % LOOP_HISTORY_SIZE;

            // print to the last 20 entries to console
            if (loopIndex == 0) {
                printTimingAndActivityBatch(loopTiming, activityType, LOOP_HISTORY_SIZE);
                uint16_t reportTime = static_cast<uint16_t>(millis() - lastSendMillisDebug);
                float avgLoopMs = loopCount > 0 ? ((float)reportTime / loopCount) : 0;
                LOGF(DEBUG, "Max time %u (ms) -- %u entries report time %u (ms) -- average %0.2f (ms)", maxLoop, LOOP_HISTORY_SIZE, reportTime, avgLoopMs);
                lastSendMillisDebug = millis();
                loopCount = 0;
                maxLoop = 0;
            }
        }
    }
}