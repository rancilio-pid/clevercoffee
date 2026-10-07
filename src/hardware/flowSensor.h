/**
 * @file flowSensor.h
 *
 * @brief sensor sends a pulse to indicate flow rate. Only Digmesa Nano DM60 and its equivalents are supported
 */

#pragma once

#include "Logger.h"
#include "driver/pcnt.h"

volatile uint32_t pulseTime = 0;
uint32_t totalPulses = 0;           // used to allow pcnt to be reset, otherwise it could saturate at h_lim and stop counting
int16_t deltaCount = 0;
float pulses_per_ml = 48.0f;        // Nano DM60 datasheet states 48000 per litre. Other sensors have been 1.875f and 7.5f and require high flow rate to start
pcnt_unit_t flowUnit = PCNT_UNIT_1; // so far its only manual selection of the unit number, this may interfere with an encoder when added
bool debugFlow = false;
const uint32_t timeoutUS = 2000000;

int16_t deltaCountDebug[20] = {0};
uint16_t deltaCountDebugIndex = 0;

void IRAM_ATTR flowPulseISR() {
    pulseTime = micros();
}

void initFlowSensorPCNT(int16_t pin) {
    pcnt_config_t pcnt_config;
    memset(&pcnt_config, 0, sizeof(pcnt_config));

    pcnt_config.pulse_gpio_num = pin;
    pcnt_config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    pcnt_config.unit = flowUnit;
    pcnt_config.channel = PCNT_CHANNEL_0;
    pcnt_config.pos_mode = PCNT_COUNT_INC;
    pcnt_config.neg_mode = PCNT_COUNT_DIS;
    pcnt_config.lctrl_mode = PCNT_MODE_KEEP;
    pcnt_config.hctrl_mode = PCNT_MODE_KEEP;
    pcnt_config.counter_h_lim = 32767;
    pcnt_config.counter_l_lim = -32768;

    esp_err_t err = pcnt_unit_config(&pcnt_config);

    if (err != ESP_OK) {
        LOGF(ERROR, "PCNT config failed: %d", err);
    }

    pcnt_set_filter_value(flowUnit, 1000);
    pcnt_filter_enable(flowUnit);
    pcnt_counter_pause(flowUnit);
    pcnt_counter_clear(flowUnit);
    pcnt_counter_resume(flowUnit);
}

void initFlowSensor(GPIOPin& dataPin, bool debug = false, float calibration = 48.0f) {
    attachInterrupt(digitalPinToInterrupt(dataPin.getPin()), flowPulseISR, RISING);
    initFlowSensorPCNT(dataPin.getPin());
    debugFlow = debug;
    pulses_per_ml = calibration;
}

void resetFlowCounter() {
    pcnt_counter_pause(flowUnit);
    pcnt_counter_clear(flowUnit);
    pcnt_counter_resume(flowUnit);
}

int16_t readFlowPulses() {
    int16_t count;

    pcnt_get_counter_value(flowUnit, &count);

    return count;
}

float readFlowMLperSec() {
    static int16_t lastCount = 0;
    static uint32_t lastTime = 0;
    static uint32_t lastPulseTime = 0;
    static uint32_t lastPulseInterval = 0;
    int16_t count;
    static bool debugPrintFlow = false;
    static float currentFlowRate = 0.0f;

    pcnt_get_counter_value(flowUnit, &count);

    noInterrupts();
    uint32_t pulseTime1 = pulseTime; // latest pulse timestamp
    interrupts();

    uint32_t now = micros();
    uint32_t deltaTime = now - lastTime; // function time
    lastTime = now;
    deltaCount = count - lastCount;
    lastCount = count;
    totalPulses += deltaCount;

    if (lastPulseTime == 0) {
        lastPulseTime = pulseTime1;
    }

    uint32_t pulseInterval = pulseTime1 - lastPulseTime; // time since the last pulse when this function was last called

    if (count > 25000) {
        // reset occasionally to prevent saturation
        resetFlowCounter();
        lastCount = 0;
    }

    // see the last 20 delta counts
    if (debugFlow) {
        deltaCountDebug[deltaCountDebugIndex] = deltaCount;
        deltaCountDebugIndex = (deltaCountDebugIndex + 1) % 20;

        if (deltaCount > 0) {
            debugPrintFlow = true;
        }

        if (deltaCountDebugIndex == 0) {
            if (debugPrintFlow) {
                // only print if a pulse was detected
                debugPrintFlow = false;

                LOGF(DEBUG, "PCNT Total: %d", totalPulses);

                char buffer[512];
                int len = 0;

                len += snprintf(buffer + len, sizeof(buffer) - len, "Last 20 flow pulse counts: [");

                for (int i = 0; i < 20; i++) {
                    len += snprintf(buffer + len, sizeof(buffer) - len, "%d", deltaCountDebug[i]);

                    if (i < 20 - 1) {
                        len += snprintf(buffer + len, sizeof(buffer) - len, ", ");
                    }
                }

                len += snprintf(buffer + len, sizeof(buffer) - len, "]");
                LOGF(DEBUG, "%s", buffer);
            }
        }
    }

    uint32_t timeSinceLastPulse = now - pulseTime1;

    if (deltaCount > 0) {
        if (pulseInterval > 0) {
            float frequency = ((float)deltaCount * 1000000.0f) / pulseInterval;
            currentFlowRate = frequency / pulses_per_ml;
            lastPulseInterval = pulseInterval;
        }
    }
    else {
        // If slowing down, timeSinceLastPulse will start to exceed the last known interval.
        // Decay the flow rate if longer than last known, otherwise currentFlowRate will return old value
        if (timeSinceLastPulse > lastPulseInterval && lastPulseInterval > 0) {
            float frequency = 1000000.0f / (float)timeSinceLastPulse;
            currentFlowRate = frequency / pulses_per_ml;
        }
    }

    lastPulseTime = pulseTime1;

    if (deltaTime == 0 || timeSinceLastPulse > timeoutUS) {
        // no pulses in the last two seconds, return 0
        return 0;
    }

    return currentFlowRate;
}

float readPulseDelta() {
    return deltaCount;
}

float readTotalVolumeML() {
    return totalPulses / pulses_per_ml;
}