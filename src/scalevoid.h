/**
 * @file scalevoid.h
 *
 * @brief TODO
 */

#pragma once

#if (BREWMODE == 2 || ONLYPIDSCALE == 1)
/**
 * @brief Check measured weight
 */
void checkWeight() {
    static boolean newDataReady = 0;
    unsigned long currentMillisScale = millis();

    if (scaleFailure) {   // abort if scale is not working
        return;
    }

    if (currentMillisScale - previousMillisScale >= intervalWeight) {
        previousMillisScale = currentMillisScale;

        // check for new data/start next conversion:
        if (LoadCell.update()) {
            newDataReady = true;
        }

        // get smoothed value from the dataset:
        if (newDataReady) {
            weight = LoadCell.getData();
            newDataReady = 0;
        }
    }
}

/**
 * @brief Initialize scale
 */
void initScale() {
    LoadCell.begin(128);
    long stabilizingtime = 4000; // tare preciscion can be improved by adding a few seconds of stabilizing time
    boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
    u8g2.clearBuffer();
    u8g2.drawStr(0, 2, "Taring scale,");
    u8g2.drawStr(0, 12, "remove any load!");
    u8g2.drawStr(0, 22, "....");
    delay(2000);
    u8g2.sendBuffer();
    LoadCell.startMultiple(stabilizingtime, _tare);

    if (LoadCell.getTareTimeoutFlag()) {
        debugPrintln("Timeout, check MCU>HX711 wiring and pin designations");
        u8g2.drawStr(0, 32, "failed!");
        u8g2.drawStr(0, 42, "Scale not working...");    // scale timeout will most likely trigger after OTA update, but will still work after boot
        delay(5000);
        u8g2.sendBuffer();
    }
    else {
        if (scaleCalibration -1 < 0.00001) {
            u8g2.drawStr(0, 22, "Calibration coming up");
            u8g2.sendBuffer();
            LoadCell.setCalFactor(scaleCalibration);
            u8g2.clearBuffer();
            u8g2.drawStr(0, 2, "Calibration in progress.");
            u8g2.drawStr(0, 12, "Place known weight on");
            u8g2.drawStr(0, 22, "scale in next 10 seconds ");
            u8g2.drawStr(0, 32, number2string(scaleKnownWeight));
            u8g2.sendBuffer();
            LoadCell.tare();
            delay(10000);
            LoadCell.refreshDataSet();  
            scaleCalibration = LoadCell.getNewCalibration(scaleKnownWeight);
        }
        u8g2.drawStr(0, 32, "done.");
        u8g2.sendBuffer();
    }

    LoadCell.setCalFactor(scaleCalibration); // set calibration factor (float)
    LoadCell.setSamplesInUse(SCALE_SAMPLES);
}

/**
 * @brief Scale with shot timer
 */
void shottimerscale() {
    switch (shottimercounter)  {
        case 10:    // waiting step for brew switch turning on
            if (timeBrewed > 0) {
                weightPreBrew = weight;
                shottimercounter = 20;
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
