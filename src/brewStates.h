/**
 * @file brewStates.h
 *
 * @brief Enums for brew state machine
 */
#pragma once

enum BrewSwitchState {
    kBrewSwitchIdle = 10,
    kBrewSwitchPressed = 20,
    kBrewSwitchShortPressed = 30,
    kBrewSwitchLongPressed = 40,
    kBrewSwitchWaitForRelease = 50
};

enum BrewState {
    kBrewIdle = 10,
    kPreinfusion = 20,
    kPreinfusionPause = 30,
    kBrewRunning = 40,
    kBrewFinished = 50,
};

enum ManualFlushState {
    kManualFlushIdle = 10,
    kManualFlushRunning = 20,
};

enum BackflushState {
    kBackflushIdle = 10,
    kBackflushFilling = 20,
    kBackflushFlushing = 30,
    kBackflushEnding = 40,
    kBackflushFinished = 50
};