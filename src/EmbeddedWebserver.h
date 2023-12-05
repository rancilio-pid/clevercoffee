/**
 * @file EmbeddedWebserver.h
 *
 * @brief Embedded webserver
 *
 */

#pragma once

#include <Arduino.h>


#include <WiFi.h>
#include <AsyncTCP.h>
#include "FS.h"

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "LittleFS.h"
#include <functional>


enum EditableKind {
    kInteger,
    kUInt8,
    kDouble,
    kDoubletime,
    kCString
};

struct editable_t {
    String displayName;
    boolean hasHelpText;
    String helpText;
    EditableKind type;
    int section;                   // parameter section number
    int position;
    std::function<bool()> show;    // method that determines if we show this parameter (in the web interface)
    int minValue;
    int maxValue;
    void *ptr;                     // TODO: there must be a tidier way to do this? could we use c++ templates?
};

AsyncWebServer server(80);
AsyncEventSource events("/events");

double curTemp = 0.0;
double tTemp = 0.0;
double hPower = 0.0;

#define HISTORY_LENGTH 600    //30 mins of values (20 vals/min * 60 min) = 600 (7,2kb)

static float tempHistory[3][HISTORY_LENGTH] = {0};
int historyCurrentIndex = 0;
int historyValueCount = 0;

void serverSetup();
void setEepromWriteFcn(int (*fcnPtr)(void));

// editable vars are specified in main.cpp
#define EDITABLE_VARS_LEN 29
extern std::map<String, editable_t> editableVars;


// EEPROM
int (*writeToEeprom)(void) = NULL;

void setEepromWriteFcn(int (*fcnPtr)(void)) {
    writeToEeprom = fcnPtr;
}

uint8_t flipUintValue(uint8_t value) {
    return (value + 3) % 2;
}

String getTempString() {
    StaticJsonDocument<96> doc;

    doc["currentTemp"] = curTemp;
    doc["targetTemp"] = tTemp;
    doc["heaterPower"] = hPower;

    String jsonTemps;
    serializeJson(doc, jsonTemps);

    return jsonTemps;
}

// proper modulo function (% is remainder, so will return negatives)
int mod(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

// rounds a number to 2 decimal places
// example: round(3.14159) -> 3.14
// (less characters when serialized to json)
double round2(double value) {
   return (int)(value * 100 + 0.5) / 100.0;
}

String getValue(String varName) {
    try {
        editable_t e = editableVars.at(varName);
        switch (e.type) {
            case kDouble:
                return String(*(double *)e.ptr);
            case kDoubletime:
                return String(*(double *)e.ptr);
            case kInteger:
                return String(*(int *)e.ptr);
            case kUInt8:
                return String(*(uint8_t *)e.ptr);
            case kCString:
                return String((char *)e.ptr);
            default:
                return F("Unknown type");
                break;
        }
    } catch (const std::out_of_range &exc) {
        return "(unknown variable " + varName + ")";
    }
}

void paramToJson(String name, editable_t &e, DynamicJsonDocument &doc) {
    JsonObject paramObj = doc.createNestedObject();
    paramObj["type"] = e.type;
    paramObj["name"] = name;
    paramObj["displayName"] = e.displayName;
    paramObj["section"] = e.section;
    paramObj["position"] = e.position;
    paramObj["hasHelpText"] = e.hasHelpText;
    paramObj["show"] = e.show();

    // set parameter value
    if (e.type == kInteger) {
        paramObj["value"] = *(int *)e.ptr;
    } else if (e.type == kUInt8) {
        paramObj["value"] = *(uint8_t *)e.ptr;
    } else if (e.type == kDouble || e.type == kDoubletime) {
        paramObj["value"] = round2(*(double *)e.ptr);
    } else if (e.type == kCString) {
        paramObj["value"] = (char *)e.ptr;
    }

    paramObj["min"] = e.minValue;
    paramObj["max"] = e.maxValue;
}

// Use libraries for the webinterface from the internet (0) or from the local filesystem (1). 0 has slightly faster load times
#define NOINTERNET 1

//hash strings at compile time to use in switch statement
//(from https://stackoverflow.com/questions/2111667/compile-time-string-hashing)
constexpr unsigned int str2int(const char* str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

String getHeader(String varName) {
    //TODO: actually put the references libs on local file system again (only when using ESP32 which has more flash mem,
    //but also make sure web server can handle this many concurrent requests (might crash)
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
                return F("<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3\" crossorigin=\"anonymous\">");
            #endif
        case (str2int("BOOTSTRAP_BUNDLE")):
            #if NOINTERNET == 1
                return F("<script src=\"/js/bootstrap.bundle.5.2.3.min.js\"></script>");
            #else
                return F("<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-kenU1KFdBIe4zVF0s0G1M5b4hcpxyD9F7jL+jjXkk+Q2h455rYXK/7HAuoJl+0I4\" crossorigin=\"anonymous\"></script>");
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
                return F("<script src=\"/js/uPlot.1.6.24.min.js\"></script><link rel=\"stylesheet\" href=\"/css/uPlot.min.css\">");
            #else
                return F("<script src=\"https://cdn.jsdelivr.net/npm/uplot@1.6.24/dist/uPlot.iife.min.js\"></script><link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/uplot@1.6.24/dist/uPlot.min.css\">");
            #endif
    }
    return "";
}

String staticProcessor(const String& var) {
    //try replacing var for variables in editableVars
    if (var.startsWith("VAR_SHOW_")) {
        return getValue(var.substring(9)); // cut off "VAR_SHOW_"
    } else if (var.startsWith("VAR_HEADER_")) {
        return getHeader(var.substring(11)); // cut off "VAR_HEADER_"
    }

    //var didn't start with above names, try opening var as fragment file and use contents if it exists
    //TODO: this seems to consume too much heap in some cases, probably better to remove fragment loading and only use one SPA in the long term (or only support ESP32 which has more RAM)
    String varLower(var);
    varLower.toLowerCase();

    File file = LittleFS.open("/html_fragments/" + varLower + ".html", "r");

    if (file) {
        if (file.size()*2 < ESP.getFreeHeap()) {
            String ret = file.readString();
            file.close();
            return ret;
        } else {
            debugPrintf("Can't open file %s, not enough memory available\n", file.name());
        }
    } else {
        debugPrintf("Fragment %s not found\n", varLower.c_str());
    }

    //didn't find a value for the var, replace var with empty string
    return String();
}

void serverSetup() {
    // set up dynamic routes (endpoints)

    server.on("/toggleSteam", HTTP_POST, [](AsyncWebServerRequest *request) {
        int steam = flipUintValue(steamON);

        setSteamMode(steam);
        debugPrintf("Toggle steam mode: %i \n", steam);

        request->redirect("/");
    });

    server.on("/togglePid", HTTP_POST, [](AsyncWebServerRequest *request) {
        debugPrintf("/togglePid requested, method: %d\n", request->method());
        int status = flipUintValue(pidON);

        setPidStatus(status);
        debugPrintf("Toggle PID state: %d\n", status);

        request->redirect("/");
    });

    server.on("/toggleBackflush", HTTP_POST, [](AsyncWebServerRequest *request) {
        int backflush = flipUintValue(backflushON);

        setBackflush(backflush);
        debugPrintf("Toggle backflush mode: %i \n", backflush);

        request->redirect("/");
    });

    server.on("/parameters", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest *request) {
        // Determine the size of the document to allocate based on the number
        // of parameters
        // GET = either
        int requestParams = request->params();
        int docLength = EDITABLE_VARS_LEN;
        if (request->method() == 1) {
            if (requestParams > 0) {
                docLength = std::min(requestParams, EDITABLE_VARS_LEN);
            }
        } else if (request->method() == 2) {
            docLength = std::min(requestParams, EDITABLE_VARS_LEN);
        }

        DynamicJsonDocument doc(JSON_ARRAY_SIZE(EDITABLE_VARS_LEN)  // array EDITABLE_VARS_LEN with parameters
            + JSON_OBJECT_SIZE(9) * EDITABLE_VARS_LEN               // object with 9 values per parameter
            + JSON_STRING_SIZE(25 + 30) * EDITABLE_VARS_LEN);       // string size for templateString and displayName

        if (request->method() == 2) {   // method() returns values from WebRequestMethod enum -> 2 == HTTP_POST
            // returns values from WebRequestMethod enum -> 2 == HTTP_POST
            // update all given params and match var name in editableVars

            for (int i = 0; i < requestParams; i++) {
                AsyncWebParameter* p = request->getParam(i);
                String varName;
                if (p->name().startsWith("var")) {
                    varName = p->name().substring(3);
                } else {
                    varName = p->name();
                }

                try {
                    editable_t e = editableVars.at(varName);
                    if (e.type == kInteger) {
                        int newVal = atoi(p->value().c_str());
                        *(int *)e.ptr = newVal;
                    } else if (e.type == kUInt8) {
                        *(uint8_t *)e.ptr =
                            (uint8_t)atoi(p->value().c_str());
                    } else if (e.type == kDouble || e.type == kDoubletime) {
                        float newVal = atof(p->value().c_str());
                        *(double *)e.ptr = newVal;
                    }
                    paramToJson(varName, e, doc);
                } catch (const std::out_of_range &exc) {
                    continue;
                }
            }

            String paramsJson;
            serializeJson(doc, paramsJson);
            request->send(200, "application/json", paramsJson);

            // Write to EEPROM
            if (writeToEeprom) {
                if (writeToEeprom() == 0) {
                    debugPrintln("successfully wrote EEPROM");
                } else {
                    debugPrintln("EEPROM write failed");
                }
            }

            // Write the new values to MQTT
            writeSysParamsToMQTT(true); // Continue on error

        } else if (request->method() == 1) {  // WebRequestMethod enum -> HTTP_GET
            // get parameter id from first parameter, e.g. /parameters?param=PID_ON
            int paramCount = request->params();
            String paramId = paramCount > 0 ? request->getParam(0)->value() : "";

            std::map<String, editable_t>::iterator it;
            if (!paramId.isEmpty()) {
                it = editableVars.find(paramId);
            } else {
                it = editableVars.begin();
            }

            for (; it != editableVars.end(); it++) {
                editable_t e = it->second;
                paramToJson(it->first, e, doc);

                if (!paramId.isEmpty()) {
                    break;
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

    server.on("/parameterHelp", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);

        AsyncWebParameter* p = request->getParam(0);
        if (p == NULL) {
            request->send(422, "text/plain", "parameter is missing");
            return;
        }
        const String& varValue = p->value();

        try {
            editable_t e = editableVars.at(varValue);
            doc["name"] = varValue;
            doc["helpText"] = e.helpText;
        } catch (const std::out_of_range &exc) {
            request->send(404, "application/json", "parameter not found");
            return;
        }

        String helpJson;
        serializeJson(doc, helpJson);
        request->send(200, "application/json", helpJson);
    });

    server.on("/temperatures", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = getTempString();
        request->send(200, "application/json", json);
    });

    // TODO: could send values also chunked and without json (but needs three
    // endpoints then?)
    // https://stackoverflow.com/questions/61559745/espasyncwebserver-serve-large-array-from-ram
    server.on("/timeseries", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");

        // set capacity of json doc for history structure
        DynamicJsonDocument doc(JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(HISTORY_LENGTH) * 3);

        // for each value in mem history array, add json array element
        JsonArray currentTemps = doc.createNestedArray("currentTemps");
        JsonArray targetTemps = doc.createNestedArray("targetTemps");
        JsonArray heaterPowers = doc.createNestedArray("heaterPowers");

        // go through history values backwards starting from currentIndex and
        // wrap around beginning to include valueCount many values
        for (int i = mod(historyCurrentIndex - historyValueCount, HISTORY_LENGTH);
            i != mod(historyCurrentIndex, HISTORY_LENGTH);
            i = mod(i + 1, HISTORY_LENGTH))
        {
            currentTemps.add(round2(tempHistory[0][i]));
            targetTemps.add(round2(tempHistory[1][i]));
            heaterPowers.add(round2(tempHistory[2][i]));
        }

        serializeJson(doc, *response);
        request->send(response);
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    // set up event handler for temperature messages
    events.onConnect([](AsyncEventSourceClient *client) {
        if (client->lastId()) {
            debugPrintf("Reconnected, last message ID was: %u\n", client->lastId());
        }

        client->send("hello", NULL, millis(), 10000);
    });

    server.addHandler(&events);

    // serve static files
    LittleFS.begin();
    server.serveStatic("/css", LittleFS, "/css/", "max-age=604800");  // cache for one week
    server.serveStatic("/js", LittleFS, "/js/", "max-age=604800");
    server.serveStatic("/img", LittleFS, "/img/", "max-age=604800");  // cache for one week
    server.serveStatic("/webfonts", LittleFS, "/webfonts/", "max-age=604800");
    server.serveStatic("/", LittleFS, "/html/", "max-age=604800").setDefaultFile("index.html").setTemplateProcessor(staticProcessor);

    server.begin();

    debugPrintln(("Server started at " + WiFi.localIP().toString()).c_str());
}

//skip counter so we don't keep a value every second
int skippedValues = 0;
#define SECONDS_TO_SKIP 2

void sendTempEvent(double currentTemp, double targetTemp, double heaterPower) {
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
    } else {
        skippedValues++;
    }

    events.send("ping", NULL, millis());
    events.send(getTempString().c_str(), "new_temps", millis());
}
