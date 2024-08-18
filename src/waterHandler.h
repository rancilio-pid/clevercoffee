/**
 * @file waterSwitch.h
 *
 * @brief Handler for digital water switch
 */

uint8_t currStateWaterSwitch;

void checkWaterSwitch() {
    if (FEATURE_WATERSWITCH) {
        uint8_t waterSwitchReading = waterSwitch->isPressed();

        if (WATERSWITCH_TYPE == Switch::TOGGLE) {
            // Set hotWaterOn to 1 when waterswitch is HIGH
            if (waterSwitchReading == HIGH) {
                hotWaterOn = 1;
                pumpRelay.on();
            }

            if (waterSwitchReading == LOW && machineState == kHotWater) {
                hotWaterOn = 0;
                pumpRelay.off();
            }
        }
        else if (WATERSWITCH_TYPE == Switch::MOMENTARY) {
            if (waterSwitchReading != currStateWaterSwitch) {
                currStateWaterSwitch = waterSwitchReading;

                if (currStateWaterSwitch == HIGH) {
                    if (hotWaterOn == 0) {
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
