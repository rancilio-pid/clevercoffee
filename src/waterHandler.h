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
            // Set waterON to 1 when steamswitch is HIGH
            if (waterSwitchReading == HIGH) {
                waterON = 1;
                pumpRelay.on();
            }

            // if activated via web interface then steamFirstON == 1, prevent override
            if (waterSwitchReading == LOW && machineState == kWater) {
                waterON = 0;
                pumpRelay.off();
            }

        }
        else if (WATERSWITCH_TYPE == Switch::MOMENTARY) {
            if (waterSwitchReading != currStateWaterSwitch) {
                currStateWaterSwitch = waterSwitchReading;

                // only toggle heating power if the new button state is HIGH
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
