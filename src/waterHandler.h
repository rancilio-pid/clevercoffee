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
            // Set waterON to 1 when waterswitch is HIGH
            if (waterSwitchReading == HIGH) {
                waterON = 1;
                pumpRelay.on();
            }

            if (waterSwitchReading == LOW && machineState == kWater) {
                waterON = 0;
                pumpRelay.off();
            }
        }
        else if (WATERSWITCH_TYPE == Switch::MOMENTARY) {
            if (waterSwitchReading != currStateWaterSwitch) {
                currStateWaterSwitch = waterSwitchReading;

                if (currStateWaterSwitch == HIGH) {
                    if (waterON == 0) {
                        waterON = 1;
                        pumpRelay.on();
                    }
                    else {
                        waterON = 0;
                        pumpRelay.off();
                    }
                }
            }
        }
    }
}
