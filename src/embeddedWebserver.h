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
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>

#include "LittleFS.h"
#include "Config.h"

inline AsyncWebServer server(80);
inline AsyncEventSource events("/events");

inline double curTemp = 0.0;
inline double tTemp = 0.0;
inline double hPower = 0.0;

#define HISTORY_LENGTH 600 // 30 mins of values (20 vals/min * 60 min) = 600 (7,2kb)

static float tempHistory[3][HISTORY_LENGTH] = {};
inline int historyCurrentIndex = 0;
inline int historyValueCount = 0;

void serverSetup();

inline bool authenticate(AsyncWebServerRequest* request) {
    if (!Config::getInstance().get<bool>("system.auth.enabled")) {
        return true;
    }

    const auto clientIP = request->client()->remoteIP().toString();
    const auto requestedPath = request->url();
    const auto username = Config::getInstance().get<String>("system.auth.username");
    const auto password = Config::getInstance().get<String>("system.auth.password");

    if (request->authenticate(username.c_str(), password.c_str())) {
        LOGF(DEBUG, "Web auth OK: %s -> %s", clientIP.c_str(), requestedPath.c_str());

        return true;
    }

    if (request->hasHeader("Authorization")) {
        LOGF(WARNING, "Web auth FAIL: %s -> %s (wrong credentials)", clientIP.c_str(), requestedPath.c_str());
    }
    else {
        LOGF(DEBUG, "Web auth required: %s -> %s", clientIP.c_str(), requestedPath.c_str());
    }

    return false;
}

inline String getTempString() {
    JsonDocument doc;

    doc["currentTemp"] = curTemp;
    doc["targetTemp"] = tTemp;
    doc["heaterPower"] = hPower;

    String jsonTemps;
    serializeJson(doc, jsonTemps);

    return jsonTemps;
}

inline String getValue(const String& varName) {
    try {
        auto& config = Config::getInstance();

        if (!config.hasParameter(varName)) {
            return "(unknown variable " + varName + ")";
        }

        // Try different types and return formatted value
        bool boolValue;
        if (config.tryGet(varName, boolValue)) {
            return String(boolValue);
        }

        int intValue;
        if (config.tryGet(varName, intValue)) {
            return String(intValue);
        }

        uint8_t uint8Value;
        if (config.tryGet(varName, uint8Value)) {
            return String(uint8Value);
        }

        double doubleValue;
        if (config.tryGet(varName, doubleValue)) {
            return String(doubleValue, 2);
        }

        float floatValue;
        if (config.tryGet(varName, floatValue)) {
            return String(floatValue, 2);
        }

        String stringValue;
        if (config.tryGet(varName, stringValue)) {
            return stringValue;
        }

        return "(unknown type for " + varName + ")";
    } catch (const std::exception& e) {
        return "(error reading " + varName + ")";
    }
}

inline void paramToJson(const String& name, const ParamDef& param, JsonVariant doc) {
    JsonObject jsonObj = doc.to<JsonObject>();
    param.toJson(jsonObj, name);
}

inline String getHeader(const String& varName) {
    static const std::unordered_map<std::string, const char*> headers = {
        {"FONTAWESOME", R"(<link href="/css/fontawesome-6.2.1.min.css" rel="stylesheet">)"}, {"BOOTSTRAP", R"(<link href="/css/bootstrap-5.2.3.min.css" rel="stylesheet">)"},
        {"BOOTSTRAP_BUNDLE", "<script src=\"/js/bootstrap.bundle.5.2.3.min.js\"></script>"}, {"VUEJS", "<script src=\"/js/vue.3.2.47.min.js\"></script>"},
        {"VUE_NUMBER_INPUT", "<script src=\"/js/vue-number-input.min.js\"></script>"},       {"UPLOT", R"(<script src="/js/uPlot.1.6.28.min.js"></script><link rel="stylesheet" href="/css/uPlot.min.css">)"}};

    const auto it = headers.find(varName.c_str());
    return it != headers.end() ? String(it->second) : String("");
}

inline String staticProcessor(const String& var) {
    // try replacing var for variables in Config system
    if (var.startsWith("VAR_SHOW_")) {
        return getValue(var.substring(9)); // cut off "VAR_SHOW_"
    }

    if (var.startsWith("VAR_HEADER_")) {
        return getHeader(var.substring(11)); // cut off "VAR_HEADER_"
    }

    // var didn't start with above names, try opening var as fragment file and use contents if it exists
    // TODO: this seems to consume too much heap in some cases, probably better to remove fragment loading and only use one SPA in the long term (or only support ESP32 which has more RAM)
    String varLower(var);
    varLower.toLowerCase();

    if (File file = LittleFS.open("/html_fragments/" + varLower + ".html", "r")) {
        if (file.size() * 2 < ESP.getFreeHeap()) {
            String ret = file.readString();
            file.close();
            return ret;
        }

        LOGF(DEBUG, "Can't open file %s, not enough memory available", file.name());
    }
    else {
        LOGF(DEBUG, "Fragment %s not found", varLower.c_str());
    }

    // didn't find a value for the var, replace var with empty string
    return {};
}

inline void serverSetup() {
    server.on("/toggleSteam", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!authenticate(request)) {
            return request->requestAuthentication();
        }

        const bool steamMode = !steamON;
        setSteamMode(steamMode);

        LOGF(DEBUG, "Toggle steam mode: %s", steamON ? "on" : "off");

        request->redirect("/");
    });

    server.on("/togglePid", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!authenticate(request)) {
            return request->requestAuthentication();
        }

        LOGF(DEBUG, "/togglePid requested, method: %d", request->method());

        const bool currentPidState = Config::getInstance().get<bool>("pid.enabled");
        const bool newPidState = !currentPidState;
        Config::getInstance().set("pid.enabled", newPidState);

        pidON = newPidState;

        LOGF(DEBUG, "Toggle PID state: %d\n", newPidState);

        request->redirect("/");
    });

    server.on("/toggleBackflush", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!authenticate(request)) {
            return request->requestAuthentication();
        }

        backflushOn = !backflushOn;
        LOGF(DEBUG, "Toggle backflush mode: %s", backflushOn ? "on" : "off");

        request->redirect("/");
    });

    if (Config::getInstance().get<bool>("hardware.sensors.scale.enabled")) {
        server.on("/toggleTareScale", HTTP_POST, [](AsyncWebServerRequest* request) {
            if (!authenticate(request)) {
                return request->requestAuthentication();
            }

            scaleTareOn = !scaleTareOn;

            LOGF(DEBUG, "Toggle scale tare mode: %s", scaleTareOn ? "on" : "off");

            request->redirect("/");
        });

        server.on("/toggleScaleCalibration", HTTP_POST, [](AsyncWebServerRequest* request) {
            if (!authenticate(request)) {
                return request->requestAuthentication();
            }

            scaleCalibrationOn = !scaleCalibrationOn;

            LOGF(DEBUG, "Toggle scale calibration mode: %s", scaleCalibrationOn ? "on" : "off");

            request->redirect("/");
        });
    }

    server.on("/parameters", [](AsyncWebServerRequest* request) {
        if (!authenticate(request)) {
            return request->requestAuthentication();
        }

        if (request->method() == 1) { // HTTP_GET
            auto& config = Config::getInstance();
            const auto& parameters = config.getParameters();

            // Check for filter parameter
            String filterType = "";
            if (request->hasParam("filter")) {
                filterType = request->getParam("filter")->value();
            }

            AsyncJsonResponse* response = new AsyncJsonResponse(false);
            JsonArray array = response->getRoot().to<JsonArray>();

            int filteredParameterCount = 0;

            // Get parameters based on filter
            for (const auto& paramPair : parameters) {
                const std::string& paramName = paramPair.first;
                const ParamDef& param = paramPair.second;

                if (!param.showCondition()) continue;

                bool includeParam = false;

                if (filterType == "hardware") {
                    includeParam = param.section >= 11 && param.section <= 15;
                }
                else if (filterType == "behavior") {
                    includeParam = param.section >= 0 && param.section <= 9;
                }
                else if (filterType == "other") {
                    includeParam = param.section == 10;
                }
                else if (filterType == "all") {
                    includeParam = true;
                }
                else {
                    includeParam = param.section == 0 || param.section == 1 || param.section == 10;
                }

                if (includeParam) {
                    JsonObject paramObj = array.add<JsonObject>();
                    paramToJson(String(paramName.c_str()), param, paramObj);
                    filteredParameterCount++;
                }
            }

            LOGF(DEBUG, "/parameters returning %d parameters", filteredParameterCount);
            response->setLength();
            request->send(response);
        }
        else if (request->method() == 2) { // HTTP_POST
            auto& config = Config::getInstance();

            String responseMessage = "OK";
            bool hasErrors = false;

            const auto requestParams = request->params();

            for (auto i = 0u; i < requestParams; ++i) {
                if (auto* p = request->getParam(i); p && p->name().length() > 0 && p->value().length() > 0) {
                    const String& varName = p->name();
                    const String& value = p->value();

                    try {
                        if (!config.hasParameter(varName)) {
                            continue;
                        }

                        // Try to determine parameter type and set accordingly
                        bool boolValue;
                        if (config.tryGet(varName, boolValue)) {
                            bool newValue = (value == "true" || value == "1");
                            config.set(varName, newValue);
                        }
                        else {
                            String stringValue;
                            if (config.tryGet(varName, stringValue)) {
                                config.set(varName, value);
                            }
                            else {
                                // Try as numeric value
                                double newVal = std::stod(value.c_str());

                                // Try different numeric types
                                int intValue;
                                if (config.tryGet(varName, intValue)) {
                                    config.set(varName, static_cast<int>(newVal));
                                }
                                else {
                                    uint8_t uint8Value;
                                    if (config.tryGet(varName, uint8Value)) {
                                        config.set(varName, static_cast<uint8_t>(newVal));
                                    }
                                    else {
                                        float floatValue;
                                        if (config.tryGet(varName, floatValue)) {
                                            config.set(varName, static_cast<float>(newVal));
                                        }
                                        else {
                                            double doubleValue;
                                            if (config.tryGet(varName, doubleValue)) {
                                                config.set(varName, newVal);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        LOGF(INFO, "Parameter %s processing failed: %s", varName.c_str(), e.what());
                        hasErrors = true;
                    }
                }
            }

            // Config automatically saves to NVS, but we still need to trigger MQTT updates
            writeSysParamsToMQTT(true);

            AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", hasErrors ? "Partial Success" : "OK");
            response->addHeader("Connection", "close");
            request->send(response);
        }
        else {
            LOGF(ERROR, "Unsupported HTTP method %d for /parameters", request->method());
            AsyncWebServerResponse* response = request->beginResponse(405, "text/plain", "Method Not Allowed");
            response->addHeader("Connection", "close");
            request->send(response);
        }
    });

    server.on("/parameterHelp", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        auto* p = request->getParam(0);

        if (p == nullptr) {
            request->send(422, "text/plain", "parameter is missing");
            return;
        }

        const String& varValue = p->value();
        auto& config = Config::getInstance();

        if (!config.hasParameter(varValue)) {
            request->send(404, "application/json", "parameter not found");
            return;
        }

        const auto& parameters = config.getParameters();
        auto it = parameters.find(varValue.c_str());
        if (it != parameters.end()) {
            doc["name"] = varValue;
            doc["helpText"] = it->second.helpText;
        } else {
            doc["name"] = varValue;
            doc["helpText"] = "";
        }

        String helpJson;
        serializeJson(doc, helpJson);
        request->send(200, "application/json", helpJson);
    });

    server.on("/temperatures", HTTP_GET, [](AsyncWebServerRequest* request) {
        const String json = getTempString();
        request->send(200, "application/json", json);
    });

    // TODO: could send values also chunked and without json (but needs three
    // endpoints then?)
    // https://stackoverflow.com/questions/61559745/espasyncwebserver-serve-large-array-from-ram
    server.on("/timeseries", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncResponseStream* response = request->beginResponseStream("application/json");
        response->addHeader("Connection", "close"); // Force connection close

        JsonDocument doc;

        // for each value in mem history array, add json array element
        auto currentTemps = doc["currentTemps"].to<JsonArray>();
        auto targetTemps = doc["targetTemps"].to<JsonArray>();
        auto heaterPowers = doc["heaterPowers"].to<JsonArray>();

        // go through history values backwards starting from currentIndex and
        // wrap around beginning to include valueCount many values
        for (int i = mod(historyCurrentIndex - historyValueCount, HISTORY_LENGTH); i != mod(historyCurrentIndex, HISTORY_LENGTH); i = mod(i + 1, HISTORY_LENGTH)) {
            currentTemps.add(round2(tempHistory[0][i]));
            targetTemps.add(round2(tempHistory[1][i]));
            heaterPowers.add(round2(tempHistory[2][i]));
        }

        String out;
        out.reserve(measureJson(doc) + 16);
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    server.on("/wifireset", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!authenticate(request)) {
            return request->requestAuthentication();
        }

        request->send(200, "text/plain", "WiFi settings are being reset. Rebooting...");

        // Defer slightly so the response gets sent before reboot
        delay(1000);

        wiFiReset();
    });

    server.on("/download/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!authenticate(request)) {
            return request->requestAuthentication();
        }

        // Use the new config system to export current configuration
        String configJson = Config::getInstance().exportToJson();

        if (configJson.isEmpty()) {
            request->send(500, "text/plain", "Failed to export configuration");
            return;
        }

        // Send the JSON configuration
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", configJson);
        response->addHeader("Content-Disposition", "attachment; filename=\"config.json\"");
        request->send(response);
    });

    server.on(
        "/upload/config", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // This response will be set by the upload handler
        },
        [](AsyncWebServerRequest* request, const String& filename, const size_t index, const uint8_t* data, const size_t len, const bool final) {
            if (!authenticate(request)) {
                return request->requestAuthentication();
            }

            static String uploadBuffer;
            static size_t totalSize = 0;

            if (index == 0) {
                uploadBuffer = "";
                uploadBuffer.reserve(8192);
                totalSize = 0;
                LOGF(INFO, "Config upload started: %s", filename.c_str());
            }

            for (size_t i = 0; i < len; i++) {
                uploadBuffer += static_cast<char>(data[i]);
            }

            totalSize += len;

            if (final) {
                LOGF(INFO, "Config upload finished: %s, total size: %u bytes", filename.c_str(), totalSize);

                if (bool isValid = Config::getInstance().validateAndApplyFromJson(uploadBuffer)) {
                    LOG(INFO, "Configuration validated and applied successfully");

                    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", R"({"success": true, "message": "Configuration validated and applied successfully.", "restart": true})");

                    response->addHeader("Connection", "close");
                    request->send(response);
                }
                else {
                    LOG(ERROR, "Configuration validation failed - invalid data or out of range values");

                    AsyncWebServerResponse* response =
                        request->beginResponse(400, "application/json", R"({"success": false, "message": "Configuration validation failed. Please check that all parameter values are within valid ranges.", "restart": true})");

                    response->addHeader("Connection", "close");
                    request->send(response);
                }
            }
        });

    server.on("/restart", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!authenticate(request)) {
            return request->requestAuthentication();
        }

        request->send(200, "text/plain", "Restarting...");
        delay(100);
        ESP.restart();
    });

    server.on("/factoryreset", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!authenticate(request)) {
            return request->requestAuthentication();
        }

        // Use the new config system to reset all parameters to defaults
        Config::getInstance().resetAllToDefaults();

        request->send(200, "text/plain", "Factory reset completed. Restarting...");

        delay(100);
        ESP.restart();
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

inline void sendTempEvent(const double currentTemp, const double targetTemp, const double heaterPower) {
    curTemp = currentTemp;
    tTemp = targetTemp;
    hPower = heaterPower;

    // save all values in memory to show history
    if (skippedValues > 0 && skippedValues % SECONDS_TO_SKIP == 0) {
        // use array and int value for start index (round robin)
        // one record (3 float values == 12 bytes) every three seconds, for half
        // an hour -> 7.2kB of static memory
        tempHistory[0][historyCurrentIndex] = static_cast<float>(currentTemp);
        tempHistory[1][historyCurrentIndex] = static_cast<float>(targetTemp);
        tempHistory[2][historyCurrentIndex] = static_cast<float>(heaterPower);
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
