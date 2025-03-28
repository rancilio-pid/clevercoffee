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

            if (hotWaterSwitchReading == LOW) {
                hotWaterOn = 0;
                pumpRelay.off();
            }
        }
        else if (HOTWATERSWITCH_TYPE == Switch::MOMENTARY) {
            if (hotWaterSwitchReading != currStateHotWaterSwitch) {
                currStateHotWaterSwitch = hotWaterSwitchReading;

                if (currStateHotWaterSwitch == HIGH) {
                    if (hotWaterOn == 0 && machineState != kWaterEmpty) {
                        hotWaterOn = 1;
                        pumpRelay.on();
                    }
                    else {
                        hotWaterOn = 0;
                        pumpRelay.off();
                    }
                }
            }
        }
    }
}
