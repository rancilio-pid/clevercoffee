/**
 * @file scalevoid.h
 *
 * @brief TODO
 */

#pragma once

#if (BREWMODE == 2 || ONLYPIDSCALE == 1)

void calibrate() {
    calibrateON = 0;
    LoadCell.setCalFactor(1.0);
    u8g2.clearBuffer();
    u8g2.drawStr(0, 22, "Calibration coming up");
    u8g2.drawStr(0, 32, "Empty the scale");
    u8g2.sendBuffer();
    LoadCell.update();
    LoadCell.tare();
    delay(2000);
    u8g2.clearBuffer();
    u8g2.drawStr(2, 2, "Calibration in progress.");
    u8g2.drawStr(2, 12, "Place known weight");
    u8g2.drawStr(2, 22, "on scale in next");
    u8g2.drawStr(2, 32," 10 seconds");
    u8g2.drawStr(2, 42, number2string(scaleKnownWeight));
    u8g2.sendBuffer();
    delay(10000);
    LoadCell.refreshDataSet();  
    scaleCalibration = LoadCell.getNewCalibration(scaleKnownWeight);
    LoadCell.setCalFactor(scaleCalibration);
    u8g2.sendBuffer();
    writeSysParamsToStorage();
    delay(2000);
    LoadCell.tare();

}

/**
 * @brief Check measured weight
 */
void checkWeight() {
    static boolean newDataReady = 0;
    unsigned long currentMillisScale = millis();

    if (scaleFailure) {   // abort if scale is not working
        return;
    }

    if (LoadCell.update()) newDataReady = true;

    if (newDataReady) {
        if (currentMillisScale - previousMillisScale >= intervalWeight) {
        previousMillisScale = currentMillisScale;
            weight = LoadCell.getData();
            newDataReady = 0;
        }
    }

    if (calibrateON) {
        while (!LoadCell.update());
        calibrate();
    }

    if (tareON) {
        tareON = 0;
        u8g2.clearBuffer();
        u8g2.drawStr(0, 2, "Taring scale,");
        u8g2.drawStr(0, 12, "remove any load!");
        u8g2.drawStr(0, 22, "....");
        delay(2000);
        u8g2.sendBuffer();
        LoadCell.tare();
        u8g2.drawStr(0, 32, "done");
        u8g2.sendBuffer();
        delay(2000);
    }
    
}

/**
 * @brief Initialize scale
 */
void initScale() {
    readSysParamsFromStorage();
    LoadCell.begin(128);
    long stabilizingtime = 5000; // tare preciscion can be improved by adding a few seconds of stabilizing time
    boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
    u8g2.clearBuffer();
    u8g2.drawStr(0, 2, "Taring scale,");
    u8g2.drawStr(0, 12, "remove any load!");
    u8g2.drawStr(0, 22, "....");
    u8g2.sendBuffer();
    delay(2000);
    LoadCell.startMultiple(stabilizingtime, _tare);
    delay(5000);

    if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag() ) {
        debugPrintf("Timeout, check MCU>HX711 wiring and pin designations");
        u8g2.drawStr(0, 32, "failed!");
        u8g2.drawStr(0, 42, "Scale not working...");    // scale timeout will most likely trigger after OTA update, but will still work after boot
        u8g2.sendBuffer();
        scaleFailure = true;
        delay(5000);
    }
    else {
        if (calibrateON) {
            LoadCell.setCalFactor(1.0);
            while (!LoadCell.update());
            calibrate();
            LoadCell.setSamplesInUse(SCALE_SAMPLES);
            u8g2.drawStr(0, 52, "done.");
            delay(2000);
        }
        else {
            LoadCell.setCalFactor(scaleCalibration); // set calibration factor (float)
            LoadCell.setSamplesInUse(SCALE_SAMPLES);
            u8g2.drawStr(0, 42, number2string(scaleCalibration));
            u8g2.drawStr(0, 52, "done.");
            u8g2.sendBuffer();
            delay(2000);
        }
    }

}

/**
 * @brief Scale with shot timer
 */
void shottimerscale() {
    switch (shottimercounter)  {
        case 10:    // waiting step for brew switch turning on
        if (preinfusionpause == 0 || preinfusion == 0) {
            if (timeBrewed > 0) {
                weightPreBrew = weight;
                shottimercounter = 20;
            }
        } else {
            if (timeBrewed > preinfusion*1000) {
                weightPreBrew = weight;
                shottimercounter = 20;
            }
        }


            break;

        case 20:
            weightBrew = weight - weightPreBrew;

            if (timeBrewed == 0) {
                shottimercounter = 10;
            }

            break;
    }
}
#endif
