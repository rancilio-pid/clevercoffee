#include "brewProfiles.h"
#include "Logger.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

BrewProfileInfo profileInfo[MAX_PROFILES]; // names only for now
BrewProfile currentProfile;

size_t profilesCount = 0;

const char* exitTypeStrs[] = {"none", "flow_under", "flow_over", "pressure_under", "pressure_over"};
const char* transitionStrs[] = {"none", "smooth", "fast", "hold"};
const char* pumpModeStrs[] = {"power", "pressure", "flow"};

extern const char* phaseName;

ExitType parseExitType(const char* str) {
    for (int i = 0; i < sizeof(exitTypeStrs) / sizeof(exitTypeStrs[0]); ++i) {
        if (strcmp(str, exitTypeStrs[i]) == 0) {
            return static_cast<ExitType>(i);
        }
    }

    LOGF(WARNING, "Unknown exit_type string: '%s'", str);
    return EXIT_TYPE_NONE;
}

TransitionType parseTransition(const char* str) {
    for (int i = 0; i < sizeof(transitionStrs) / sizeof(transitionStrs[0]); ++i) {
        if (strcmp(str, transitionStrs[i]) == 0) {
            return static_cast<TransitionType>(i);
        }
    }

    LOGF(WARNING, "Unknown transition string: '%s'", str);
    return TRANSITION_NONE;
}

PumpMode parsePumpMode(const char* str) {
    for (int i = 0; i < sizeof(pumpModeStrs) / sizeof(pumpModeStrs[0]); ++i) {
        if (strcmp(str, pumpModeStrs[i]) == 0) {
            return static_cast<PumpMode>(i);
        }
    }

    LOGF(WARNING, "Unknown pump mode string: '%s'", str);
    return POWER;
}

bool validatePhaseExitConditions(const BrewPhase& phase, const char* profileName, int phaseIndex) {
    if (phase.seconds < 1.0) {
        LOGF(WARNING, "Profile '%s' phase %d: requires 'seconds' >= 1.0", profileName, phaseIndex);
        return false;
    }

    switch (phase.exit_type) {
        case EXIT_TYPE_NONE:
            return true;

        case EXIT_TYPE_PRESSURE_OVER:
            if (phase.exit_pressure_over <= 0.0) {
                LOGF(WARNING, "Profile '%s' phase %d: EXIT_TYPE_PRESSURE_OVER requires 'exit_pressure_over' > 0.0", profileName, phaseIndex);
                return false;
            }
            return true;

        case EXIT_TYPE_PRESSURE_UNDER:
            if (phase.exit_pressure_under <= 0.0) {
                LOGF(WARNING, "Profile '%s' phase %d: EXIT_TYPE_PRESSURE_UNDER requires 'exit_pressure_under' > 0.0", profileName, phaseIndex);
                return false;
            }
            return true;

        case EXIT_TYPE_FLOW_OVER:
            if (phase.exit_flow_over <= 0.0) {
                LOGF(WARNING, "Profile '%s' phase %d: EXIT_TYPE_FLOW_OVER requires 'exit_flow_over' > 0.0", profileName, phaseIndex);
                return false;
            }
            return true;

        case EXIT_TYPE_FLOW_UNDER:
            if (phase.exit_flow_under <= 0.0) {
                LOGF(WARNING, "Profile '%s' phase %d: EXIT_TYPE_FLOW_UNDER requires 'exit_flow_under' > 0.0", profileName, phaseIndex);
                return false;
            }
            return true;
    }

    LOGF(WARNING, "Profile '%s' phase %d: Unknown exit type", profileName, phaseIndex);
    return false;
}

void loadProfileMetadata() {
    File file = LittleFS.open("/profiles/defaultProfiles.json", "r");

    profilesCount = 0;

    if (!file) {
        LOG(ERROR, "Could not open profile metadata");
        return;
    }

    JsonDocument doc;

    if (deserializeJson(doc, file)) {
        file.close();
        return;
    }

    file.close();

    for (JsonObject profileJson : doc.as<JsonArray>()) {
        if (profilesCount >= MAX_PROFILES) {
            break;
        }

        const char* name = profileJson["name"] | "";
        // const char* desc = profileJson["description"] | "";

        strncpy(profileInfo[profilesCount].name, name, MAX_NAME);
        profileInfo[profilesCount].name[MAX_NAME - 1] = '\0';
        // strncpy(profileInfo[profilesCount].description, desc, MAX_DESC);
        // profileInfo[profilesCount].description[MAX_DESC-1] = '\0';

        profilesCount++;
    }

    LOGF(INFO, "Found %d brew profiles", profilesCount);
}

void loadProfileByName(const char* name) {
    File file = LittleFS.open("/profiles/defaultProfiles.json", "r");

    if (!file) {
        return;
    }

    JsonDocument doc;

    if (deserializeJson(doc, file)) {
        file.close();
        return;
    }

    file.close();

    for (JsonObject profileJson : doc.as<JsonArray>()) {
        const char* jsonName = profileJson["name"].as<const char*>();
        if (jsonName && strcmp(jsonName, name) == 0) {

            // Parse just this one
            strncpy(currentProfile.name, jsonName, MAX_NAME);
            currentProfile.name[MAX_NAME - 1] = '\0';

            const char* jsonDesc = profileJson["description"] | "";
            strncpy(currentProfile.description, jsonDesc, MAX_DESC);
            currentProfile.description[MAX_DESC - 1] = '\0';

            JsonArray phasesJson = profileJson["phases"];
            int phaseCount = min((int)phasesJson.size(), MAX_PHASES);
            currentProfile.phaseCount = phaseCount;

            for (int i = 0; i < phaseCount; i++) {
                JsonObject phaseJson = phasesJson[i];
                BrewPhase& p = currentProfile.phases[i];
                memset(&p, 0, sizeof(BrewPhase));

                const char* pname = phaseJson["name"] | "";
                const char* pdesc = phaseJson["description"] | "";

                strncpy(p.name, pname, MAX_NAME);
                p.name[MAX_NAME - 1] = '\0';
                strncpy(p.description, pdesc, MAX_DESC);
                p.description[MAX_DESC - 1] = '\0';

                p.pressure = phaseJson["pressure"] | 0.0;
                p.flow = phaseJson["flow"] | 0.0;
                p.volume = phaseJson["volume"] | 0.0;
                p.weight = phaseJson["weight"] | 0.0;
                p.exit_flow_under = phaseJson["exit_flow_under"] | 0.0;
                p.exit_flow_over = phaseJson["exit_flow_over"] | 0.0;
                p.exit_pressure_over = phaseJson["exit_pressure_over"] | 0.0;
                p.exit_pressure_under = phaseJson["exit_pressure_under"] | 0.0;
                p.max_secondary = phaseJson["max_secondary"] | 0.0;
                p.max_secondary_range = phaseJson["max_secondary_range"] | 0.0;
                p.seconds = phaseJson["seconds"] | 0.0;

                p.exit_type = parseExitType(phaseJson["exit_type"] | "none");
                p.transition = parseTransition(phaseJson["transition"] | "fast");
                p.pump = parsePumpMode(phaseJson["pump"] | "pressure");

                if (validatePhaseExitConditions(p, currentProfile.name, i)) {
                    LOGF(DEBUG, "Phase %d: %s validated", i, p.name);
                }
            }

            currentProfile.temperature = profileJson["temperature"] | 0.0;
            currentProfile.time = profileJson["time"] | 0.0;
            currentProfile.scales = profileJson["scales"] | false;
            currentProfile.flow = profileJson["flow"] | false;
            currentProfile.stop = profileJson["auto_stop"] | false;
        }
    }
}

void clearCurrentProfile() {
    memset(&currentProfile, 0, sizeof(BrewProfile));
    currentProfile.phaseCount = 0;
    currentProfile.temperature = 0;
    currentProfile.scales = false;
    currentProfile.flow = false;
    currentProfile.stop = false;
}

void selectProfileByName(const char* name) {
    if (!name || name[0] == '\0') {
        LOGF(WARNING, "selectProfileByName called with empty name");
        return;
    }

    clearCurrentProfile();
    loadProfileByName(name);

    if (currentProfile.phaseCount == 0) {
        LOGF(WARNING, "Failed to load profile: %s", name);
        return;
    }

    phaseName = currentProfile.phases[0].name;
    LOGF(INFO, "Loaded profile: %s", currentProfile.name);
}

const char* getPhaseDescriptions(BrewProfile& currentProfile) {
    static char buf[512];
    buf[0] = '\0';

    if (currentProfile.phaseCount == 0) {
        strcpy(buf, "No profile loaded");
        return buf;
    }

    for (int i = 0; i < currentProfile.phaseCount; i++) {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Phase %d: %s\n%s\n\n", i + 1, currentProfile.phases[i].name, currentProfile.phases[i].description);
    }

    LOGF(INFO, "Profile Description Length: %u", strlen(buf));

    return buf;
}

/*bool loadProfile(const char* json, BrewPhase* phases, size_t maxPhases, size_t& outCount) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    size_t count = 0;

    for (JsonObject obj : arr) {
        if (count >= maxPhases) {
            break;
        }

        BrewPhase& p = phases[count];
        p = {};
        p.name = obj["name"] | "";
        p.description = obj["description"] | "";
        p.pressure = obj["pressure"] | 0.0;
        p.flow = obj["flow"] | 0.0;
        p.volume = obj["volume"] | 0.0;
        p.weight = obj["weight"] | 0.0;
        p.exit_flow_under = obj["exit_flow_under"] | 0.0;
        p.exit_flow_over = obj["exit_flow_over"] | 0.0;
        p.exit_pressure_over = obj["exit_pressure_over"] | 0.0;
        p.exit_pressure_under = obj["exit_pressure_under"] | 0.0;
        p.max_secondary = obj["max_secondary"] | 0.0;
        p.max_secondary_range = obj["max_secondary_range"] | 0.0;
        p.seconds = obj["seconds"] | 0.0;
        p.exit_type = parseExitType(obj["exit_type"] | "none");
        p.transition = parseTransition(obj["transition"] | "fast");
        p.pump = parsePump(obj["pump"] | "pressure");

        count++;
    }

    outCount = count;
    return true;
}

void saveProfile(BrewPhase* phases, size_t count, Stream& out) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (size_t i = 0; i < count; ++i) {
        BrewPhase& p = phases[i];
        JsonObject o = arr.add<JsonObject>();

        o["name"] = p.name;
        o["description"] = p.description;
        if (p.pressure != 0.0) o["pressure"] = p.pressure;
        if (p.flow != 0.0) o["flow"] = p.flow;
        if (p.volume != 0.0) o["volume"] = p.volume;
        if (p.weight != 0.0) o["weight"] = p.weight;
        if (p.exit_flow_under != 0.0) o["exit_flow_under"] = p.exit_flow_under;
        if (p.exit_flow_over != 0.0) o["exit_flow_over"] = p.exit_flow_over;
        if (p.exit_pressure_under != 0.0) o["exit_pressure_under"] = p.exit_pressure_under;
        if (p.exit_pressure_over != 0.0) o["exit_pressure_over"] = p.exit_pressure_over;
        if (p.max_secondary != 0.0) o["max_secondary"] = p.max_secondary;
        if (p.max_secondary_range != 0.0) o["max_secondary_range"] = p.max_secondary_range;
        if (p.seconds != 0.0) o["seconds"] = p.seconds;
        o["exit_type"] = exitTypeStrs[p.exit_type];
        o["transition"] = transitionStrs[p.transition];
        o["pump"] = pumpModeStrs[p.pump];
    }

    serializeJsonPretty(doc, out);
}
*/