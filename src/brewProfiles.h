#pragma once

#include <Stream.h>
#include <vector>

constexpr int MAX_PROFILES = 10;
constexpr int MAX_PHASES = 6;
constexpr int MAX_NAME = 32;
constexpr int MAX_DESC = 256;

typedef enum {
    EXIT_TYPE_NONE,
    EXIT_TYPE_FLOW_UNDER,
    EXIT_TYPE_FLOW_OVER,
    EXIT_TYPE_PRESSURE_UNDER,
    EXIT_TYPE_PRESSURE_OVER,
} ExitType;

typedef enum {
    TRANSITION_NONE,
    TRANSITION_SMOOTH,
    TRANSITION_FAST,
    TRANSITION_HOLD, // not used yet
} TransitionType;

typedef enum {
    POWER,
    PRESSURE,
    FLOW,
    PROFILE,
} PumpMode;

typedef struct {
        char name[MAX_NAME];
        char description[MAX_DESC];
        float pressure, flow, volume, weight;
        float exit_flow_under, exit_flow_over;
        float exit_pressure_over, exit_pressure_under;
        float max_secondary, max_secondary_range;
        float seconds;
        ExitType exit_type;
        TransitionType transition;
        PumpMode pump;
} BrewPhase;

typedef struct {
        char name[MAX_NAME];
        char description[MAX_DESC];
        BrewPhase phases[MAX_PHASES];
        int phaseCount;
        float temperature;
        float time; // not used yet
        bool scales;
        bool flow;
        bool stop;
} BrewProfile;

struct BrewProfileInfo {
        char name[MAX_NAME];
        // char description;
};

extern size_t profilesCount;
extern BrewProfileInfo profileInfo[MAX_PROFILES];
extern BrewProfile currentProfile;

ExitType parseExitType(const char* str);
TransitionType parseTransition(const char* str);
PumpMode parsePump(const char* str);

void loadProfileMetadata();
void selectProfileByName(const char* name);
const char* getPhaseDescriptions(BrewProfile& currentProfile);

// bool loadProfile(const char* json, BrewPhase* phases, size_t maxPhases, size_t& outCount);
// void saveProfile(BrewPhase* phases, size_t count, Stream& out);