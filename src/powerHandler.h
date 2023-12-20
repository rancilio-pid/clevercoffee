/**
 * @file powerSwitch.h
 *
 * @brief Handler for digital power switch
 */

uint8_t currStatePowerSwitch;                      // the current reading from the input pin

void checkPowerSwitch() {
    if (FEATURE_POWERSWITCH) {
        uint8_t powerSwitchReading = powerSwitch->isPressed();

        if (POWERSWITCH_TYPE == SWITCHTYPE_TOGGLE) {
            // Set pidON to 1 when powerswitch is HIGH
            if (powerSwitchReading == HIGH) {
                pidON = 1;
            }
            else {
                // Set pidON to 0 when powerswitch is not HIGH
                pidON = 0;
            }
        }
        else if (POWERSWITCH_TYPE == SWITCHTYPE_MOMENTARY) {
            // if the button state has changed:
            if (powerSwitchReading != currStatePowerSwitch) {
                currStatePowerSwitch = powerSwitchReading;

                // only toggle heating power if the new button state is HIGH
                if (currStatePowerSwitch == HIGH) {
                    if (pidON == 0) {
                        pidON = 1;
                    }
                    else {
                        pidON = 0;
                    }
                }
            }
        }
    }
}

