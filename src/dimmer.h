#pragma once

// used to debounce the interrupts
unsigned long lastZCInterrupt = 0;
int currentPowerLevel = 0;
int power = 0;

/**
 * When starting a new shot, we have to reset the counter to clear any values left over from the last shot. 
*/
void resetDimmerCounter() {
    currentPowerLevel = 0;
}

/**
 * Power must be set as a percentage (i.e. between 0 and 100, inclusive).
*/
void setPower(int newPower) {
    // Illeagal value range, turn pump off to be on the safe side. 
    if (newPower < 0  || newPower > 100) {
        power = 0;
        return;
    }
    debugPrintf("Setting new power %d\n", newPower);
    power = newPower;
}

/*
Idea: each time we get a ZC interrupt after a full sine wave, we increment the currentPowerLevel by the set power percentage.
If we have reached 100%, we turn on the pump. 
This method should provide even distribution of "on" and "off" cycles without having to implement arrays or similar to keep track of previous on/off phases. 
*/
void handleZC() {
    unsigned long time = millis();
    if (time - lastZCInterrupt < 15) {
        return;
    }
    lastZCInterrupt = time;

    // increment power level by the set power level
    currentPowerLevel += power;

    // if the power level has reached 100, we turn the pump on
    if (currentPowerLevel >= 100) {
        digitalWrite(PIN_PUMP, HIGH);
        // Decrease power level by 100 to be ready for the next increments
        currentPowerLevel -= 100;
    } 
    // Power level is not yet 100 --> we wait for it to increase further and keep the pump off.
    else {
        digitalWrite(PIN_PUMP, LOW);
    }
}

void setupDimmer() {
    pinMode(PIN_ZC, INPUT_PULLUP); 
    attachInterrupt(digitalPinToInterrupt(PIN_ZC), handleZC, RISING); 
}