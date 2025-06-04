/**
 * @file embeddedWebserver.h
 *
 * @brief Embedded webserver
 *
 */

#pragma once

#include <Arduino.h>

#include "FS.h"
#include <AsyncTCP.h>
#include <WiFi.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include "LittleFS.h"
#include <functional>

inline AsyncWebServer server(80);
inline AsyncEventSource events("/events");

inline double curTemp = 0.0;
inline double tTemp = 0.0;
inline double hPower = 0.0;

#define HISTORY_LENGTH 600 // 30 mins of values (20 vals/min * 60 min) = 600 (7,2kb)

static float tempHistory[3][HISTORY_LENGTH] = {0};
inline int historyCurrentIndex = 0;
inline int historyValueCount = 0;

void serverSetup();

inline uint8_t flipUintValue(uint8_t value) {
    return (value + 3) % 2;
}

inline String getTempString() {
    StaticJsonDocument<96> doc;

    doc["currentTemp"] = curTemp;
    doc["targetTemp"] = tTemp;
    doc["heaterPower"] = hPower;

    String jsonTemps;
    serializeJson(doc, jsonTemps);

    return jsonTemps;
}

// proper modulo function (% is remainder, so will return negatives)
inline int mod(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

// rounds a number to 2 decimal places
// example: round(3.14159) -> 3.14
// (less characters when serialized to json)
inline double round2(double value) {
    return (int)(value * 100 + 0.5) / 100.0;
}

inline String getValue(const String& varName) {
    try {
        std::shared_ptr<Parameter> e = ParameterRegistry::getInstance().getParameterById(varName.c_str());

        if (e == nullptr) {
            return "(unknown variable " + varName + ")";
        }

        return e->getFormattedValue();
    } catch (const std::out_of_range& exc) {
        return "(unknown variable " + varName + ")";
    }
}

inline void paramToJson(const String& name, const std::shared_ptr<Parameter>& param, DynamicJsonDocument& doc) {
    JsonObject paramObj = doc.createNestedObject();
    paramObj["type"] = param->getType();
    paramObj["name"] = name;
    paramObj["displayName"] = param->getDisplayName();
    paramObj["section"] = param->getSection();
    paramObj["position"] = param->getPosition();
    paramObj["hasHelpText"] = param->hasHelpText();
    paramObj["show"] = param->shouldShow();

    // set parameter value using the appropriate method based on type
    if (param->getType() == kInteger) {
        paramObj["value"] = static_cast<int>(param->getValue());
    }
    else if (param->getType() == kUInt8) {
        paramObj["value"] = static_cast<uint8_t>(param->getValue());
    }
    else if (param->getType() == kDouble || param->getType() == kDoubletime) {
        paramObj["value"] = round2(param->getValue());
    }
    else if (param->getType() == kFloat) {
        paramObj["value"] = round2(static_cast<float>(param->getValue()));
    }
    else if (param->getType() == kCString) {
        paramObj["value"] = param->getStringValue();
    }

    paramObj["min"] = param->getMinValue();
    paramObj["max"] = param->getMaxValue();
}

// Use libraries for the webinterface from the internet (0) or from the local filesystem (1). 0 has slightly faster load times
#define NOINTERNET 1

// hash strings at compile time to use in switch statement
// (from https://stackoverflow.com/questions/2111667/compile-time-string-hashing)
constexpr unsigned int str2int(const char* str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

inline String getHeader(const String& varName) {
    switch (str2int(varName.c_str())) {
        case (str2int("FONTAWESOME")):
#if NOINTERNET == 1
            return F("<link href=\"/css/fontawesome-6.2.1.min.css\" rel=\"stylesheet\">");
#else
            return F("<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.2.1/css/all.min.css\">");
#endif
        case (str2int("BOOTSTRAP")):
#if NOINTERNET == 1
            return F("<link href=\"/css/bootstrap-5.2.3.min.css\" rel=\"stylesheet\">");
#else
            return F("<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-rbsA2VBKQhggwzxH7pPCaAqO46MgnOM80zW1RWuH61DGLwZJEdK2Kadq2F9CUG65\" "
                     "crossorigin=\"anonymous\">");
#endif
        case (str2int("BOOTSTRAP_BUNDLE")):
#if NOINTERNET == 1
            return F("<script src=\"/js/bootstrap.bundle.5.2.3.min.js\"></script>");
#else
            return F("<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-kenU1KFdBIe4zVF0s0G1M5b4hcpxyD9F7jL+jjXkk+Q2h455rYXK/7HAuoJl+0I4\" "
                     "crossorigin=\"anonymous\"></script>");
#endif
        case (str2int("VUEJS")):
#if NOINTERNET == 1
            return F("<script src=\"/js/vue.3.2.47.min.js\"></script>");
#else
            return F("<script src=\"https://cdn.jsdelivr.net/npm/vue@3.2.47/dist/vue.global.prod.min.js\"></script>");
#endif
        case (str2int("VUE_NUMBER_INPUT")):
#if NOINTERNET == 1
            return F("<script src=\"/js/vue-number-input.min.js\"></script>");
#else
            return F("<script src=\"https://unpkg.com/@chenfengyuan/vue-number-input@2.0.1/dist/vue-number-input.min.js\"></script>");
#endif
        case (str2int("UPLOT")):
#if NOINTERNET == 1
            return F("<script src=\"/js/uPlot.1.6.28.min.js\"></script><link rel=\"stylesheet\" href=\"/css/uPlot.min.css\">");
#else
            return F("<script src=\"https://cdn.jsdelivr.net/npm/uplot@1.6.28/dist/uPlot.iife.min.js\"></script><link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/uplot@1.6.24/dist/uPlot.min.css\">");
#endif
    }
    return "";
}

inline String staticProcessor(const String& var) {
    // try replacing var for variables in ParameterRegistry
    if (var.startsWith("VAR_SHOW_")) {
        return getValue(var.substring(9));   // cut off "VAR_SHOW_"
    }
    else if (var.startsWith("VAR_HEADER_")) {
        return getHeader(var.substring(11)); // cut off "VAR_HEADER_"
    }

    // var didn't start with above names, try opening var as fragment file and use contents if it exists
    // TODO: this seems to consume too much heap in some cases, probably better to remove fragment loading and only use one SPA in the long term (or only support ESP32 which has more RAM)
    String varLower(var);
    varLower.toLowerCase();

    File file = LittleFS.open("/html_fragments/" + varLower + ".html", "r");

    if (file) {
        if (file.size() * 2 < ESP.getFreeHeap()) {
            String ret = file.readString();
            file.close();
            return ret;
        }
        else {
            LOGF(DEBUG, "Can't open file %s, not enough memory available", file.name());
        }
    }
    else {
        LOGF(DEBUG, "Fragment %s not found", varLower.c_str());
    }

    // didn't find a value for the var, replace var with empty string
    return {};
}

inline void serverSetup() {
    // set up dynamic routes (endpoints)

    server.on("/toggleSteam", HTTP_POST, [](AsyncWebServerRequest* request) {
        int steam = flipUintValue(steamON);

        setSteamMode(steam);
        LOGF(DEBUG, "Toggle steam mode: %i", steam);

        request->redirect("/");
    });

    server.on("/togglePid", HTTP_POST, [](AsyncWebServerRequest* request) {
        LOGF(DEBUG, "/togglePid requested, method: %d", request->method());

        auto pidParam = ParameterRegistry::getInstance().getParameterById("PID_ON");
        bool newPidState = !pidParam->getBoolValue();
        ParameterRegistry::getInstance().setParameterBoolValue("PID_ON", newPidState);

        // Update global variable for immediate effect
        pidON = newPidState ? 1 : 0;

        LOGF(DEBUG, "Toggle PID state: %d\n", newPidState);

        request->redirect("/");
    });

    server.on("/toggleBackflush", HTTP_POST, [](AsyncWebServerRequest* request) {
        int backflush = flipUintValue(backflushOn);

        setBackflush(backflush);
        LOGF(DEBUG, "Toggle backflush mode: %i", backflush);

        request->redirect("/");
    });

#if FEATURE_SCALE == 1
    server.on("/toggleTareScale", HTTP_POST, [](AsyncWebServerRequest* request) {
        int tare = flipUintValue(scaleTareOn);

        setScaleTare(tare);
        LOGF(DEBUG, "Toggle scale tare mode: %i", tare);

        request->redirect("/");
    });

    server.on("/toggleScaleCalibration", HTTP_POST, [](AsyncWebServerRequest* request) {
        int scaleCalibrate = flipUintValue(scaleCalibrationOn);

        setScaleCalibration(scaleCalibrate);
        LOGF(DEBUG, "Toggle scale calibration mode: %i", scaleCalibrate);

        request->redirect("/");
    });
#endif

    server.on("/parameters", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest* request) {
        auto& registry = ParameterRegistry::getInstance();

        size_t webVisibleCount = 0;
        const auto& parameters = registry.getParameters();

        for (const auto& param : parameters) {
            if (param->shouldShow()) {
                webVisibleCount++;
            }
        }

        // Calculate document size dynamically based on actual parameter count
        size_t docSize = JSON_ARRAY_SIZE(webVisibleCount) + (webVisibleCount * JSON_OBJECT_SIZE(9)) + (webVisibleCount * JSON_STRING_SIZE(55)); // Estimated string sizes for name + displayName

        DynamicJsonDocument doc(docSize);

        if (request->method() == 2) { // HTTP_POST - Update parameters
            int requestParams = request->params();

            for (int i = 0; i < requestParams; i++) {
                auto* p = request->getParam(i);
                String varName;

                if (p->name().startsWith("var")) {
                    varName = p->name().substring(3);
                }
                else {
                    varName = p->name();
                }

                try {
                    std::shared_ptr<Parameter> param = registry.getParameterById(varName.c_str());

                    if (param == nullptr || !param->shouldShow()) {
                        continue;
                    }

                    if (param->getType() == kCString) {
                        if (registry.setParameterStringValue(varName.c_str(), p->value())) {
                            paramToJson(varName, param, doc);
                        }
                    }
                    else {
                        double newVal = 0.0;

                        if (param->getType() == kInteger || param->getType() == kUInt8) {
                            newVal = static_cast<double>(atoi(p->value().c_str()));
                        }
                        else {
                            newVal = atof(p->value().c_str());
                        }

                        if (registry.setParameterValue(varName.c_str(), newVal)) {
                            paramToJson(varName, param, doc);
                        }
                    }
                } catch (const std::out_of_range& exc) {
                    continue;
                }
            }

            registry.forceSave();

            String paramsJson;
            serializeJson(doc, paramsJson);
            request->send(200, "application/json", paramsJson);

            // Write the new values to MQTT
            writeSysParamsToMQTT(true);
        }
        else if (request->method() == 1) { // HTTP_GET - Retrieve parameters
            const size_t paramCount = request->params();
            String paramId = paramCount > 0 ? request->getParam(0)->value() : "";

            if (!paramId.isEmpty()) {
                // Get specific parameter
                std::shared_ptr<Parameter> param = registry.getParameterById(paramId.c_str());

                if (param != nullptr && param->shouldShow()) {
                    paramToJson(paramId, param, doc);
                }
            }
            else {
                // Get all web-visible parameters
                for (const auto& param : parameters) {
                    if (param->shouldShow()) {
                        paramToJson(param->getId(), param, doc);
                    }
                }
            }
        }

        if (doc.size() == 0) {
            request->send(404, "application/json",
                          F("{ \"code\": 404, \"message\": "
                            "\"Parameter not found\"}"));
            return;
        }

        String paramsJson;
        serializeJson(doc, paramsJson);
        request->send(200, "application/json", paramsJson);
    });

    server.on("/parameterHelp", HTTP_GET, [](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(1024);
        auto* p = request->getParam(0);

        if (p == nullptr) {
            request->send(422, "text/plain", "parameter is missing");
            return;
        }

        const String& varValue = p->value();

        std::shared_ptr<Parameter> param = ParameterRegistry::getInstance().getParameterById(varValue.c_str());

        if (param == nullptr) {
            request->send(404, "application/json", "parameter not found");
            return;
        }

        doc["name"] = varValue;
        doc["helpText"] = param->getHelpText();

        String helpJson;
        serializeJson(doc, helpJson);
        request->send(200, "application/json", helpJson);
    });

    server.on("/temperatures", HTTP_GET, [](AsyncWebServerRequest* request) {
        String json = getTempString();
        request->send(200, "application/json", json);
    });

    // TODO: could send values also chunked and without json (but needs three
    // endpoints then?)
    // https://stackoverflow.com/questions/61559745/espasyncwebserver-serve-large-array-from-ram
    server.on("/timeseries", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncResponseStream* response = request->beginResponseStream("application/json");

        // set capacity of json doc for history structure
        DynamicJsonDocument doc(JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(HISTORY_LENGTH) * 3);

        // for each value in mem history array, add json array element
        JsonArray currentTemps = doc.createNestedArray("currentTemps");
        JsonArray targetTemps = doc.createNestedArray("targetTemps");
        JsonArray heaterPowers = doc.createNestedArray("heaterPowers");

        // go through history values backwards starting from currentIndex and
        // wrap around beginning to include valueCount many values
        for (int i = mod(historyCurrentIndex - historyValueCount, HISTORY_LENGTH); i != mod(historyCurrentIndex, HISTORY_LENGTH); i = mod(i + 1, HISTORY_LENGTH)) {
            currentTemps.add(round2(tempHistory[0][i]));
            targetTemps.add(round2(tempHistory[1][i]));
            heaterPowers.add(round2(tempHistory[2][i]));
        }

        serializeJson(doc, *response);
        request->send(response);
    });

    server.on("/wifireset", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "WiFi settings are being reset. Rebooting...");

        // Defer slightly so the response gets sent before reboot
        delay(1000);

        wiFiReset();
    });

    server.on("/download/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!LittleFS.exists("/config.json")) {
            request->send(404, "text/plain", "Config file not found");
            return;
        }

        File configFile = LittleFS.open("/config.json", "r");

        if (!configFile) {
            request->send(500, "text/plain", "Failed to open config file");
            return;
        }

        AsyncWebServerResponse* response = request->beginResponse(LittleFS, "/config.json", "application/json", true);

        response->addHeader("Content-Disposition", "attachment; filename=\"config.json\"");

        request->send(response);
        configFile.close();
    });

    server.onNotFound([](AsyncWebServerRequest* request) { request->send(404, "text/plain", "Not found"); });

    // set up event handler for temperature messages
    events.onConnect([](AsyncEventSourceClient* client) {
        if (client->lastId()) {
            LOGF(DEBUG, "Reconnected, last message ID was: %u", client->lastId());
        }

        client->send("hello", nullptr, millis(), 10000);
    });

    server.addHandler(&events);

    // serve static files
    LittleFS.begin();
    server.serveStatic("/css", LittleFS, "/css/", "max-age=604800"); // cache for one week
    server.serveStatic("/js", LittleFS, "/js/", "max-age=604800");
    server.serveStatic("/img", LittleFS, "/img/", "max-age=604800"); // cache for one week
    server.serveStatic("/webfonts", LittleFS, "/webfonts/", "max-age=604800");
    server.serveStatic("/manifest.json", LittleFS, "/manifest.json", "max-age=604800");
    server.serveStatic("/", LittleFS, "/html/", "max-age=604800").setDefaultFile("index.html").setTemplateProcessor(staticProcessor);

    server.begin();

    LOG(INFO, ("Server started at " + WiFi.localIP().toString()).c_str());
}

// skip counter so we don't keep a value every second
inline int skippedValues = 0;
#define SECONDS_TO_SKIP 2

inline void sendTempEvent(double currentTemp, double targetTemp, double heaterPower) {
    curTemp = currentTemp;
    tTemp = targetTemp;
    hPower = heaterPower;

    // save all values in memory to show history
    if (skippedValues > 0 && skippedValues % SECONDS_TO_SKIP == 0) {
        // use array and int value for start index (round robin)
        // one record (3 float values == 12 bytes) every three seconds, for half
        // an hour -> 7.2kB of static memory
        tempHistory[0][historyCurrentIndex] = (float)currentTemp;
        tempHistory[1][historyCurrentIndex] = (float)targetTemp;
        tempHistory[2][historyCurrentIndex] = (float)heaterPower;
        historyCurrentIndex = (historyCurrentIndex + 1) % HISTORY_LENGTH;
        historyValueCount = min(HISTORY_LENGTH - 1, historyValueCount + 1);
        skippedValues = 0;
    }
    else {
        skippedValues++;
    }

    events.send("ping", nullptr, millis());
    events.send(getTempString().c_str(), "new_temps", millis());
}
