/**
 * @file flowSensor.h
 *
 * @brief sensor sends a pulse to indicate flow rate. Only Digmesa Nano DM60 and its equivalents are supported
 */

#pragma once

#include "Logger.h"
#include "driver/pcnt.h"

volatile uint32_t pulseTime[2] = {0};
volatile bool pulseIdx = 0;
uint32_t totalPulses = 0;           // used to allow pcnt to be reset, otherwise it would saturate at h_lim and stop counting
int16_t deltaCount = 0;
const float pulses_per_ml = 48.0f;  // Nano DM60 datasheet states 48000 per litre. Other sensors have been 1.875f and 7.5f and require high flow rate to start
pcnt_unit_t flowUnit = PCNT_UNIT_0; // so far its only manual selection of the unit number, this may interfere with an encoder when added
bool debugFlow = false;

int16_t deltaCountDebug[20] = {0};
uint16_t deltaCountDebugIndex = 0;

void IRAM_ATTR flowPulseISR() {
    uint32_t current_time = micros();

    if (current_time - pulseTime[pulseIdx] > 200) { // debounce of 200 microseconds
        pulseIdx = !pulseIdx;
        pulseTime[pulseIdx] = current_time;
    }
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

void initFlowSensor(GPIOPin& dataPin, bool debug = false) {
    attachInterrupt(digitalPinToInterrupt(dataPin.getPin()), flowPulseISR, RISING);
    initFlowSensorPCNT(dataPin.getPin());
    debugFlow = debug;
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
    int16_t count;
    static bool debugPrintFlow = false;

    pcnt_get_counter_value(flowUnit, &count);

    deltaCount = count - lastCount;
    lastCount = count;
    totalPulses += deltaCount;

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

    uint32_t now = micros();
    uint32_t deltaTime = now - lastTime;

    noInterrupts();
    uint32_t pulseTime1 = pulseTime[pulseIdx];        // latest pulse timestamp
    uint32_t pulseTime2 = pulseTime[!pulseIdx];       // previous pulse timestamp
    interrupts();

    uint32_t pulseInterval = pulseTime1 - pulseTime2; // last pulse interval
    lastTime = now;

    if (now - pulseTime1 > pulseInterval) {
        // if no pulses at the same rate as last pulse, use time since last pulse to indicate declining flow rate
        pulseInterval = now - pulseTime1;
    }

    if (deltaTime == 0 || pulseInterval > 2000000) {
        // if no pulses in the last two seconds, return 0 to avoid returning very high flow rates due to noise
        return 0;
    }

    float frequency = (float)deltaCount * 1000000.0f / deltaTime;

    if (deltaCount < 4) {
        // if we have less than 4 pulses in the interval, use the last pulse interval to calculate frequency.
        // It is assumed the flow rate between pulses will be fairly constant, but there is a large difference in calculated flow between 2 pulses and 3 pulses
        if (pulseInterval > 0) {
            frequency = 1000000.0f / pulseInterval;
        }
    }

    return frequency / pulses_per_ml;
}

float readPulseDelta() {
    return deltaCount;
}

float readTotalVolumeML() {
    return totalPulses / pulses_per_ml;
}