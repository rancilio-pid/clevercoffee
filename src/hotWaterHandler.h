/**
 * @file hotWaterHandler.h
 *
 * @brief Handler for digital hot water switch
 */

uint8_t currStateHotWaterSwitch;

void checkHotWaterSwitch() {
    if (FEATURE_HOTWATERSWITCH) {
        uint8_t hotWaterSwitchReading = hotWaterSwitch->isPressed();

        if (HOTWATERSWITCH_TYPE == Switch::TOGGLE) {
            // Set hotWaterOn to 1 when waterswitch is HIGH
            if (hotWaterSwitchReading == HIGH && machineState != kWaterEmpty) {
                hotWaterOn = 1;
                pumpRelay.on();
            }

            if (hotWaterSwitchReading == LOW && machineState == kHotWater) {
                hotWaterOn = 0;
                pumpRelay.off();
            }
        }
        else if (HOTWATERSWITCH_TYPE == Switch::MOMENTARY) {
            if (hotWaterSwitchReading != currStateHotWaterSwitch) {
                currStateHotWaterSwitch = hotWaterSwitchReading;

                if (currStateHotWaterSwitch == HIGH && machineState != kWaterEmpty) {
                    if (hotWaterOn == 0) {
                        hotWaterOn = 1;
                        pumpRelay.on();
                    }
                    else {
                        if (machineState == kHotWater) {
                            hotWaterOn = 0;
                            pumpRelay.off();
                        }
                    }
                }
            }
        }
    }
}
