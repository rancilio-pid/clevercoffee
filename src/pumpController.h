/**
 * @file pumpController.h
 *
 * @brief PID controller for pump when used with a dimmer
 * Includes function to read a profile and control the pump based on its parameters
 *
 */

/**
 * TODO
 * can set kPidDisabled during brew when saving parameters
 * deal with temperature changes in profiles
 * dynamic list of profiles, or block based on hardware config
 * lots of cleaning - check what variables are no longer needed etc
 */

unsigned long currentMillisPumpControl = 0;
unsigned long previousMillisPumpControl = 0;
unsigned long pumpControlInterval = 50;
unsigned long maxPumpControlInterval = 100;

float pumpdt = pumpControlInterval / 1000.0; // Time step in seconds

float flowPressureCeiling = 0;
float flowPressureRange = 0;
int loopIndex = 0;
int loopIndexPid = 0;
bool triggered = false;
int triggerCountdown = 0;

float setPressure = PUMP_PRESSURE_SETPOINT;
float setPumpFlowRate = PUMP_FLOW_SETPOINT;
PumpMode pumpControlMode = PRESSURE;
float dimmerPower = PUMP_POWER_SETPOINT;
float pressureKp = PSM_PRESSURE_KP;
float pressureKi = PSM_PRESSURE_KI;
float pressureKd = PSM_PRESSURE_KD;
float flowKp = PSM_FLOW_KP;
float flowKi = PSM_FLOW_KI;
float flowKd = PSM_FLOW_KD;
float pumpIntegral = 0.0;
float previousError = 0;
bool startProfile = true;
bool brewProfileComplete = false;
bool updateMetadata = false;

extern const char* dimmerModes[4];

float applySmoothOverride(float target, float input, float ceiling, float range, int curve = 1) {
    if (ceiling > 0 && range > 0 && input > ceiling) {
        float t = (input - ceiling) / range;
        t = constrain(t, 0.0f, 1.0f);

        if (curve == 2) {
            t = t * t;     // quadratic
        }
        else if (curve == 3) {
            t = t * t * t; // cubic
        }

        // reset integral to reduce windup, could be harsh
        // pumpIntegral = 0;

        return target * (1.0f - t);
    }

    return target;
}

void dimmerModeHandler() {
    pumpIntegral = 0;
    previousError = 0;

    if (config.get<int>("dimmer.mode") == PROFILE) {
        if (currentProfile.phaseCount > 0) {
            profileName = currentProfile.name;
            profileDescription = currentProfile.description;
            autoStop = (currentProfile.stop && config.get<bool>("hardware.sensors.scale.enabled")) || config.get<bool>("brew.by_weight.enabled") || config.get<bool>("brew.by_time.enabled");
            // lastBrewSetpoint = brewSetpoint;
            LOGF(INFO, "Profile Index: %i -- Profile Name: %s", currentProfileIndex, profileName);
        }
        else {
            LOG(WARNING, "Profile is null in dimmerModeHandler");
            profileName = "Invalid profile";
            profileDescription = "Invalid profile";
            autoStop = false;
        }
    }
    else {
        // if (lastBrewSetpoint > 0) {
        //     brewSetpoint = lastBrewSetpoint;
        // }
    }

    updateMetadata = true;
}

void dimmerTypeHandler() {
    if (pumpRelay) {
        if (pumpRelay->getType() == PumpControlType::DIMMER) {
            auto* dimmer = static_cast<PumpDimmer*>(pumpRelay.get());
            int type = config.get<int>("dimmer.type");
            PumpDimmer::ControlMethod method = (type == 1) ? PumpDimmer::ControlMethod::PHASE : (type == 2) ? PumpDimmer::ControlMethod::DAC_VELOFUSO : PumpDimmer::ControlMethod::PSM;
            dimmer->setControlMethod(method);
        }
    }
}

// only runs while in kBrew
void runProfile(int profileIndex) {
    if (profileIndex < 0 || profileIndex >= profilesCount) {
        return;
    }

    static float lastPressure = 0.0;
    static float lastSetPressure = 0.0;
    static float lastFlow = 0.0;
    static float lastSetFlow = 0.0;
    static float lastBrewWeight = 0.0;
    static float filteredWeight = 0.0;
    static bool phaseReset = false;

    // initialise all variables
    if (startProfile) {
        startProfile = false;
        brewProfileComplete = false;
        // targetBrewTime = profile->time;
        lastPressure = 0.0;
        lastSetPressure = 0.0;
        lastFlow = 0.0;
        lastSetFlow = 0.0;
        phaseTiming = 0;
        phaseReset = true;
        LOGF(DEBUG, "Running profile: %s\n", currentProfile.name);
        updateMetadata = true;
    }

    while (currentPhaseIndex < currentProfile.phaseCount) {
        BrewPhase& phase = currentProfile.phases[currentPhaseIndex];
        phaseName = phase.name;
        phaseDescription = phase.description;

        bool exitReached = false;

        // Transition to next phase if exit condition met
        switch (phase.exit_type) {
            case EXIT_TYPE_NONE:
                exitReached = (currBrewTime > phase.seconds * 1000 + phaseTiming);
                break;

            case EXIT_TYPE_PRESSURE_OVER:
                exitReached = (inputPressureFilter >= phase.exit_pressure_over);
                break;

            case EXIT_TYPE_PRESSURE_UNDER:
                exitReached = (inputPressureFilter <= phase.exit_pressure_under);
                break;

            case EXIT_TYPE_FLOW_OVER:
                exitReached = (pumpFlowRateFilter >= phase.exit_flow_over);
                break;

            case EXIT_TYPE_FLOW_UNDER:
                exitReached = (pumpFlowRateFilter <= phase.exit_flow_under);
                break;
        }

        if (config.get<bool>("hardware.sensors.scale.enabled") && phase.weight > 0 && currBrewWeight >= phase.weight) {
            exitReached = true;
        }

        if (currBrewState == kBrewFinished) {
            exitReached = true;
        }

        if (exitReached || (currBrewTime > phase.seconds * 1000 + phaseTiming)) {
            lastPressure = inputPressureFilter;
            lastSetPressure = phase.pressure;
            lastFlow = pumpFlowRateFilter;
            lastSetFlow = phase.flow;
            currentPhaseIndex += 1;
            phaseTiming = currBrewTime;
            phaseReset = true;
            lastBrewWeight = currBrewWeight;
            filteredWeight = lastBrewWeight;
            updateMetadata = true;

            if (currentPhaseIndex < currentProfile.phaseCount) {
                // use the profile->phase method as currentPhaseIndex has been incremented
                if (currBrewState != kBrewFinished) {
                    LOGF(DEBUG, "Moving to Phase %d: %s for %.1f seconds", currentPhaseIndex, currentProfile.phases[currentPhaseIndex].name, currentProfile.phases[currentPhaseIndex].seconds);
                }
            }
            else {
                brewProfileComplete = true;
                return;
            }
        }
        else {
            break; // Stay in current phase
        }
    }

    // Control logic based on phase settings
    // check if still in phases, otherwise skip control
    if (currentPhaseIndex >= currentProfile.phaseCount) {
        return;
    }

    BrewPhase& phase = currentProfile.phases[currentPhaseIndex];

    if (phaseReset) {
        LOGF(DEBUG, "Phase %s: exit_type=%d, flow_over=%.2f, pressure_over=%.2f, for %.1f seconds", phase.name, phase.exit_type, phase.exit_flow_over, phase.exit_pressure_over, phase.seconds);

        if ((phase.transition == TRANSITION_SMOOTH) && (phase.seconds < 1.0)) {
            LOGF(WARNING, "Phase '%s' duration (%.2f s) is less than 1 second for smooth transitions", phase.name, phase.seconds);
        }
    }

    flowPressureCeiling = phase.max_secondary;
    flowPressureRange = phase.max_secondary_range;

    if (currBrewWeight > filteredWeight) {
        filteredWeight = currBrewWeight;
    }

    if (phase.pump == FLOW) {
        if (phase.transition == TRANSITION_SMOOTH) {
            if (phaseReset) {
                if (pumpControlMode == PRESSURE) { // scale integral error if pumpControlMode was PRESSURE
                    pumpIntegral = pumpIntegral * (pressureKi / flowKi);
                    previousError = 0;
                    pumpControlMode = FLOW;

                    if (lastSetFlow > 0) { // if a flow rate was specified in a pressure phase, start a smooth transition from here
                        lastFlow = lastSetFlow;
                    }
                }
                else {
                    lastFlow = lastSetFlow; // if already in FLOW mode then continue from last requested flow rate, otherwise use last measured as starting point
                }

                phaseReset = false;
            }

            if (phase.exit_type == EXIT_TYPE_NONE && phase.weight > 0 && config.get<bool>("hardware.sensors.scale.enabled")) {
                float t = min((filteredWeight - lastBrewWeight) / (phase.weight - lastBrewWeight), 1.0f);
                setPumpFlowRate = lastFlow + (phase.flow - lastFlow) * t;
            }
            else {
                float elapsed = (currBrewTime - phaseTiming) / 1000.0;
                float t = min(elapsed / phase.seconds, 1.0f);
                setPumpFlowRate = lastFlow + (phase.flow - lastFlow) * t;
            }
        }
        else {
            if (phaseReset) {
                pumpIntegral = 0;
                previousError = 0;
                phaseReset = false;
            }

            pumpControlMode = FLOW;
            setPumpFlowRate = phase.flow;
        }

        setPressure = 0;
    }
    else if (phase.pump == PRESSURE) {
        if (phase.transition == TRANSITION_SMOOTH) {
            if (phaseReset) {
                if (pumpControlMode == FLOW) { // scale integral error if pumpControlMode was FLOW
                    pumpIntegral = pumpIntegral * (flowKi / pressureKi);
                    previousError = 0;
                    pumpControlMode = PRESSURE;

                    if (lastSetPressure > 0) { // if a pressure was specified in a flow phase, start a smooth transition from here
                        lastPressure = lastSetPressure;
                    }
                }
                else {
                    lastPressure = lastSetPressure; // if already in PRESSURE mode then continue from last requested pressure, otherwise use last measured as starting point
                }

                phaseReset = false;
            }

            if (phase.exit_type == EXIT_TYPE_NONE && phase.weight > 0 && config.get<bool>("hardware.sensors.scale.enabled")) {
                float t = min((filteredWeight - lastBrewWeight) / (phase.weight - lastBrewWeight), 1.0f);
                setPressure = lastPressure + (phase.pressure - lastPressure) * t;
            }
            else {
                float elapsed = (currBrewTime - phaseTiming) / 1000.0;
                float t = min(elapsed / phase.seconds, 1.0f);
                setPressure = lastPressure + (phase.pressure - lastPressure) * t;
            }
        }
        else {
            if (phaseReset) {
                pumpIntegral = 0;
                previousError = 0;
                phaseReset = false;
            }

            pumpControlMode = PRESSURE;
            setPressure = phase.pressure;
        }
        setPumpFlowRate = 0;
    }
    else {
        // Fallback: infer from pressure/flow, but shouldnt ever get here
        if (phase.pressure > 0) {
            pumpControlMode = PRESSURE;
            setPressure = phase.pressure;
            setPumpFlowRate = 0;
        }
        else {
            pumpControlMode = FLOW;
            setPumpFlowRate = phase.flow;
            setPressure = 0;
        }
    }
}

void loopPump() {
    if (!pumpRelay) {
        return;
    }

    if (!config.get<bool>("dimmer.enabled") || pumpRelay->getType() == PumpControlType::RELAY) {
        return;
    }

    if (machineState != kBrew) {
        startProfile = true;
        currentPhaseIndex = 0;
        flowPressureCeiling = 0.0;
        flowPressureRange = 0.0;
    }

    // continously refresh auto stop in case user changes standard auto settings
    autoStop = (currentProfile.stop && config.get<bool>("hardware.sensors.scale.enabled")) || config.get<bool>("brew.by_weight.enabled") || config.get<bool>("brew.by_time.enabled");

    if (config.get<bool>("dimmer.enabled")) {
        static float inputPID = 0.0;
        static float targetPID = 0.0;
        static float inputKp = 0.0;
        static float inputKi = 0.0;
        static float inputKd = 0.0;
        static int lastDimmerMode = config.get<int>("dimmer.mode");
        static int lastDimmerType = config.get<int>("dimmer.type");
        static float maxLoggedPressure = 0.0;

        // force back to POWER control if pressure sensor is disabled
        if (!config.get<bool>("hardware.sensors.pressure.enabled")) {
            config.set<int>("dimmer.mode", POWER);
        }
        else {
            if (config.get<int>("dimmer.profile") != currentProfileIndex) {
                currentProfileIndex = config.get<int>("dimmer.profile");
                selectProfileByName(profileInfo[currentProfileIndex].name);
                dimmerModeHandler();
            }
            else if (config.get<int>("dimmer.mode") != lastDimmerMode) {
                lastDimmerMode = config.get<int>("dimmer.mode");
                dimmerModeHandler();
            }
        }

        if (config.get<int>("dimmer.type") != lastDimmerType) {
            lastDimmerType = config.get<int>("dimmer.type");
            dimmerTypeHandler();
        }

        // override for flush and backflush
        if (machineState == kBackflush) {
            pumpControlMode = POWER;
            dimmerPower = 100;
        }
        else if (machineState == kManualFlush) {
            if (config.get<int>("dimmer.mode") == FLOW) {
                pumpControlMode = FLOW;
                setPumpFlowRate = config.get<float>("dimmer.setpoint.flow");
            }
            else if (config.get<int>("dimmer.mode") == POWER) {
                pumpControlMode = POWER;
                dimmerPower = config.get<float>("dimmer.setpoint.power");
            }
            else {
                pumpControlMode = POWER;
                dimmerPower = 100;
            }
        }
        else {
            switch (config.get<int>("dimmer.mode")) {
                case POWER:
                    dimmerPower = config.get<float>("dimmer.setpoint.power");
                    pumpControlMode = POWER;
                    break;

                case PRESSURE:
                    setPressure = config.get<float>("dimmer.setpoint.pressure");
                    pumpControlMode = PRESSURE;
                    break;

                case FLOW:
                    setPumpFlowRate = config.get<float>("dimmer.setpoint.flow");
                    pumpControlMode = FLOW;
                    flowPressureCeiling = 9.0; // pressure bar
                    flowPressureRange = 0.2;   // reduce to 0 output over 9.2bar
                    break;

                case PROFILE:
                    if (machineState == kBrew) {
                        runProfile(currentProfileIndex);
                    }
                    break;

                default:
                    break;
            }
        }

        if (pumpRelay->getState()) {
            currentMillisPumpControl = millis();

            if (currentMillisPumpControl - previousMillisPumpControl >= pumpControlInterval) {       // 50ms timing

                if (currentMillisPumpControl - previousMillisPumpControl > maxPumpControlInterval) { // if greater than 100ms, set pumpdt to 100ms to stop jumps due to an extended freeze
                    pumpdt = maxPumpControlInterval / 1000.0;
                }
                else {
                    pumpdt = (currentMillisPumpControl - previousMillisPumpControl) / 1000.0; // set to between 50ms and 100ms
                }

                // allow changes to PID parameters at any time
                if (config.get<int>("dimmer.type") == 1) {
                    pressureKp = config.get<float>("dimmer.phase.pressure.kp");
                    pressureKi = config.get<float>("dimmer.phase.pressure.ki");
                    pressureKd = config.get<float>("dimmer.phase.pressure.kd");
                    flowKp = config.get<float>("dimmer.phase.flow.kp");
                    flowKi = config.get<float>("dimmer.phase.flow.ki");
                    flowKd = config.get<float>("dimmer.phase.flow.kd");
                }
                else if (config.get<int>("dimmer.type") == 2) {
                    pressureKp = config.get<float>("dimmer.velo.pressure.kp");
                    pressureKi = config.get<float>("dimmer.velo.pressure.ki");
                    pressureKd = config.get<float>("dimmer.velo.pressure.kd");
                    flowKp = config.get<float>("dimmer.velo.flow.kp");
                    flowKi = config.get<float>("dimmer.velo.flow.ki");
                    flowKd = config.get<float>("dimmer.velo.flow.kd");
                }
                else {
                    pressureKp = config.get<float>("dimmer.psm.pressure.kp");
                    pressureKi = config.get<float>("dimmer.psm.pressure.ki");
                    pressureKd = config.get<float>("dimmer.psm.pressure.kd");
                    flowKp = config.get<float>("dimmer.psm.flow.kp");
                    flowKi = config.get<float>("dimmer.psm.flow.ki");
                    flowKd = config.get<float>("dimmer.psm.flow.kd");
                }

                PidResults[loopIndexPid][8] = currentMillisPumpControl - previousMillisPumpControl;
                previousMillisPumpControl = currentMillisPumpControl;

                PidResults[loopIndexPid][0] = inputPressure;
                PidResults[loopIndexPid][1] = setPressure;
                PidResults[loopIndexPid][2] = pumpFlowRate;
                PidResults[loopIndexPid][3] = setPumpFlowRate;
                PidResults[loopIndexPid][4] = currBrewWeight;

                if (pumpControlMode == POWER) {
                    inputPID = 0.0;
                    targetPID = 0.0;
                    inputKp = 0.0;
                    inputKi = 0.0;
                    inputKd = 0.0;
                    dimmerPower = constrain((int)dimmerPower, PUMP_POWER_SETPOINT_MIN, PUMP_POWER_SETPOINT_MAX);
                }
                else {
                    if (pumpControlMode == PRESSURE) {
                        inputPID = inputPressureFilter; // inputPressure;
                        targetPID = setPressure;
                        // Smooth flow override, doesnt work well in pressure
                        targetPID = applySmoothOverride(targetPID, pumpFlowRate, flowPressureCeiling, flowPressureRange, 2); // 1 is linear reduction, 2 quadratic, 3 cubic
                        inputKp = pressureKp;
                        inputKi = pressureKi;
                        inputKd = pressureKd;
                    }
                    else if (pumpControlMode == FLOW) {
                        inputPID = pumpFlowRateFilter;
                        targetPID = setPumpFlowRate;
                        // Smooth pressure override
                        targetPID = applySmoothOverride(targetPID, inputPressureFilter, flowPressureCeiling, flowPressureRange, 2); // 1 is linear reduction, 2 quadratic, 3 cubic
                        inputKp = flowKp;
                        inputKi = flowKi;
                        inputKd = flowKd;
                    }

                    float iMax = config.get<float>("dimmer.i_max") / inputKi;
                    float error = targetPID - inputPID;
                    pumpIntegral += error * pumpdt; // Integrate error
                    pumpIntegral = constrain(pumpIntegral, -iMax, iMax);
                    float pumpderivative = (error - previousError) / pumpdt;
                    previousError = error;

                    float output = (inputKp * error) + (inputKi * pumpIntegral) + (inputKd * pumpderivative);

                    PidResults[loopIndexPid][5] = inputKp * error;
                    PidResults[loopIndexPid][6] = inputKi * pumpIntegral;
                    PidResults[loopIndexPid][7] = inputKd * pumpderivative;

                    dimmerPower = constrain((int)output, PUMP_POWER_SETPOINT_MIN, PUMP_POWER_SETPOINT_MAX);
                }

                // Only update if power changed
                if (pumpRelay->getType() == PumpControlType::DIMMER) {
                    auto* dimmer = static_cast<PumpDimmer*>(pumpRelay.get());
                    dimmer->setPressure(inputPressureFilter);
                    dimmer->setPower(dimmerPower);
                }

                // DEBUGGING
                if (inputPressure > maxLoggedPressure) {
                    maxLoggedPressure = inputPressure;
                }

                loopIndexPid = (loopIndexPid + 1) % LOOP_HISTORY_SIZE;

                // Count down and log when ready
                if (loopIndexPid == 0) {
                    if (maxLoggedPressure > 0.10) {
                        if (config.get<bool>("system.show_brewdata.enabled")) {
                            printLoopPidAsList();
                        }
                    }

                    maxLoggedPressure = 0;
                }
            }
        }
        else {                                                          // Pump turned off
            pumpIntegral = 0;
            previousError = 0;
            previousMillisPumpControl = millis() - pumpControlInterval; // stops large spikes in log data timing
        }
    }
}