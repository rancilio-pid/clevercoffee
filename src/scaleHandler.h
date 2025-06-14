/**
 * @file scaleHandler.h
 *
 * @brief Implementation of scale initialization and weight measurement
 */

#pragma once

inline void scaleCalibrate(HX711_ADC loadCell, int pin, bool isSecondCell, float* calibration) {
    loadCell.setCalFactor(1.0);

    u8g2.clearBuffer();
    u8g2.drawStr(0, 22, "Calibration coming up");
    u8g2.drawStr(0, 32, "Empty scale ");
    u8g2.print(pin, 0);
    u8g2.sendBuffer();

    LOGF(INFO, "Taking scale %d to zero point", pin);

    loadCell.update();
    loadCell.tare();

    LOGF(INFO, "Put load on scale %d within the next 10 seconds", pin);

    float scaleKnownWeight = ParameterRegistry::getInstance().getParameterById("SCALE_KNOWN_WEIGHT")->getFloatValue();

    u8g2.clearBuffer();
    u8g2.drawStr(2, 2, "Calibration in progress.");
    u8g2.drawStr(2, 12, "Place known weight");
    u8g2.drawStr(2, 22, "on scale in next");
    u8g2.drawStr(2, 32, "10 seconds");
    u8g2.drawStr(2, 42, number2string(scaleKnownWeight));
    u8g2.sendBuffer();
    delay(10000);

    LOG(INFO, "Taking scale load point");

    // increase scale samples temporarily to ensure a stable reading
    loadCell.setSamplesInUse(128);
    loadCell.refreshDataSet();
    *calibration = loadCell.getNewCalibration(scaleKnownWeight);
    loadCell.setSamplesInUse(SCALE_SAMPLES);

    LOGF(INFO, "New calibration: %f", *calibration);

    u8g2.sendBuffer();

    if (isSecondCell) {
        ParameterRegistry::getInstance().setParameterValue("SCALE2_CALIBRATION", *calibration);
    }
    else {
        ParameterRegistry::getInstance().setParameterValue("SCALE_CALIBRATION", *calibration);
    }

    u8g2.clearBuffer();
    u8g2.drawStr(2, 2, "Calibration done!");
    u8g2.drawStr(2, 12, "New calibration:");
    u8g2.drawStr(2, 22, number2string(*calibration));
    u8g2.sendBuffer();

    delay(2000);
}

inline float w1 = 0.0;
inline float w2 = 0.0;

/**
 * @brief Check measured weight
 */
inline void checkWeight() {
    static boolean newDataReady = 0;
    unsigned long currentMillisScale = millis();

    if (scaleFailure) { // abort if scale is not working
        return;
    }

    // check for new data/start next conversion:
    if (LoadCell.update()) {
        newDataReady = true;
    }

    if (config.getScaleType() == 0) {
        // weirdly, the library examples do not check for updates on the second cell before getting the values...
        LoadCell2.update();
    }

    if (newDataReady) {
        if (currentMillisScale - previousMillisScale >= intervalWeight) {
            previousMillisScale = currentMillisScale;
            newDataReady = false;
            w1 = LoadCell.getData();

            if (config.getScaleType() == 0) {
                w2 = LoadCell2.getData();
            }
        }
    }

    if (config.getScaleType() == 0) {
        currReadingWeight = w1 + w2;
    }

    currReadingWeight = w1;

    if (scaleCalibrationOn) {
        scaleCalibrate(LoadCell, PIN_HXDAT, false, &scaleCalibration);

        if (config.getScaleType() == 0) {
            scaleCalibrate(LoadCell2, PIN_HXDAT2, true, &scale2Calibration);
        }

        scaleCalibrationOn = 0;
    }

    if (scaleTareOn) {
        scaleTareOn = 0;
        u8g2.clearBuffer();
        u8g2.drawStr(0, 2, "Taring scale,");
        u8g2.drawStr(0, 12, "remove any load!");
        u8g2.drawStr(0, 22, "....");
        delay(2000);
        u8g2.sendBuffer();
        LoadCell.tare();
        LoadCell.setCalFactor(scaleCalibration);

        if (config.getScaleType() == 0) {
            LoadCell2.setCalFactor(scale2Calibration);
            LoadCell2.tare();
        }

        u8g2.drawStr(0, 32, "done");
        u8g2.sendBuffer();
        delay(2000);
    }
}

void initScale() {
    boolean shouldCalibrate = scaleCalibrationOn;

    LoadCell.begin();

    if (config.getScaleType() == 0) {
        LoadCell2.begin();
    }

    unsigned long stabilizingtime = 5000; // tare preciscion can be improved by adding a few seconds of stabilizing time
    boolean _tare = true;                 // set this to false if you don't want tare to be performed in the next step

    if (config.getScaleType() == 1) {
        while (!LoadCell.startMultiple(stabilizingtime, _tare))
            ;
    }
    else {
        byte loadCellReady = 0;
        byte loadCell2Ready = 0;

        // run startup, stabilization and tare, both modules simultaniously
        // this parallel start seems to be the most important part to get accurate readings with two HX711s connected
        while ((loadCellReady + loadCell2Ready) < 2) {
            if (!loadCellReady) {
                loadCellReady = LoadCell.startMultiple(stabilizingtime, _tare);
            }

            if (!loadCell2Ready) {
                loadCell2Ready = LoadCell2.startMultiple(stabilizingtime, _tare);
            }
        }
    }

    if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
        LOG(ERROR, "Timeout, check MCU>HX711 wiring for scale");
        u8g2.clearBuffer();
        u8g2.drawStr(0, 32, "failed!");
        u8g2.drawStr(0, 42, "Scale not working..."); // scale timeout will most likely trigger after OTA update, but will still work after boot
        u8g2.sendBuffer();
        delay(5000);
        scaleFailure = true;
        return;
    }

    if (config.getScaleType() == 0) {
        if (LoadCell2.getTareTimeoutFlag() || LoadCell2.getSignalTimeoutFlag()) {
            LOG(ERROR, "Timeout, check MCU>HX711 wiring for scale 2");
            u8g2.clearBuffer();
            u8g2.drawStr(0, 32, "failed!");
            u8g2.drawStr(0, 42, "Scale not working..."); // scale timeout will most likely trigger after OTA update, but will still work after boot
            u8g2.sendBuffer();
            delay(5000);
            scaleFailure = true;
            return;
        }
    }

    LoadCell.setCalFactor(scaleCalibration);
    LoadCell.setSamplesInUse(SCALE_SAMPLES);

    if (config.getScaleType() == 0) {
        LoadCell2.setCalFactor(scale2Calibration);
        LoadCell2.setSamplesInUse(SCALE_SAMPLES);
    }

    scaleCalibrationOn = 0;
}

/**
 * @brief Scale with shot timer
 */
inline void shottimerscale() {
    switch (shottimerCounter) {
        case 10: // waiting step for brew switch turning on
            if (currBrewState != kBrewIdle) {
                prewBrewWeight = currReadingWeight;
                shottimerCounter = 20;
            }

            break;

        case 20:
            currBrewWeight = currReadingWeight - prewBrewWeight;

            if (currBrewState == kBrewIdle) {
                shottimerCounter = 10;
            }

            break;

        default:;
    }
}
