/**
 * @file RancilioServer.h
 *
 * @brief Embedded webserver
 */

#ifndef rancilioserver_h
#define rancilioserver_h

#include <Arduino.h>

#ifdef ESP32
    #include <WiFi.h>
    #include <AsyncTCP.h>
    #include "FS.h"
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
    #define WEBSERVER_H
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "LittleFS.h"
#include <functional>

#include "userConfig.h"


enum EditableKind {
    kInteger,
    kUInt8,
    kDouble,
    kDoubletime,
    kCString
};

struct editable_t {
    String templateString;
    String displayName;
    boolean hasHelpText;
    String helpText;
    EditableKind type;
    int section;                   // parameter section number
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

#if defined(ESP8266)
    #define HISTORY_LENGTH 120    //10 mins of values
#elif defined(ESP32)
    #define HISTORY_LENGTH 720    //60 mins of values
#endif

static float tempHistory[3][HISTORY_LENGTH] = {0};
int historyCurrentIndex = 0;
int historyValueCount = 0;

void serverSetup();
void setEepromWriteFcn(int (*fcnPtr)(void));

// We define these in rancilio-pid.cpp
#define EDITABLE_VARS_LEN 28
extern std::vector<editable_t> editableVars;

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

//proper modulo function (% is remainder, so will return negatives)
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

String generateForm(String varName) {
    for (editable_t e : editableVars) {
        if (e.templateString != varName) {
            continue;
        }

        String result = F("<label class=\"form-label me-1\" for=\"var");
        result += e.templateString;
        result += F("\">");
        result += e.displayName;
        result += "</label>";
        if (!e.helpText.isEmpty()) {
            result += F("<a href=\"#\" role=\"button\" data-bs-toggle=\"popover\" data-bs-html=\"true\" data-bs-original-title=\"");
            result += e.helpText;
            result += F("\"><span class=\"fa-solid fa-circle-question\"></span></a></br>");
        } else {
            result += F("<br/>");
        }
        String currVal;

        switch (e.type) {
            case kDouble:
                result += F("<input class=\"form-control form-control-lg\" type=\"number\" step=\"0.1\"");
                currVal = *(double *)e.ptr;
                break;

            case kDoubletime:
                result += F("<input class=\"form-control form-control-lg\" type=\"number\" step=\"0.1\"");
                currVal = *(double *)e.ptr/1000;
                break;

            case kInteger:
                result += F("<input class=\"form-control form-control-lg\" type=\"number\" step=\"1\"");
                currVal = *(int *)e.ptr;
                break;

            case kUInt8:
            {
                uint8_t val = *(uint8_t *)e.ptr;

                result += F("<input type=\"hidden\" id=\"var");
                result += e.templateString;
                result += F("\" name=\"var");
                result += e.templateString + F("\" value=\"0\" />");
                result += F("<input type=\"checkbox\" class=\"form-check-input form-control-lg\" id=\"var");
                result += e.templateString;
                result += F("\" name=\"var");
                result += e.templateString;
                result += F("\" value=\"1\"");

                if (val) {
                    result += F(" checked=\"checked\"");
                }

                result += F("><br />");

                break;
            }

            default:
                result += F("<input class=\"form-control form-control-lg\" type=\"text\"");
                currVal = (const char *)e.ptr;
        }

        if (e.type != kUInt8) {
            result += F(" id=\"var");
            result += e.templateString;
            result += F("\" name=\"var");
            result += e.templateString;
            result += F("\" value=\"");
            result += currVal;
            result += F("\"><br />");
        }

        return result;
    }

    return "(unknown variable " + varName + ")";
}

String getValue(String varName) {
    // Yikes, we don't have std::map available
    for (editable_t e : editableVars) {
        if (e.templateString != varName) {
            continue;
        }

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
    }
    return "(unknown variable " + varName + ")";
}

//hash strings at compile time to use in switch statement
//(from https://stackoverflow.com/questions/2111667/compile-time-string-hashing)
constexpr unsigned int str2int(const char* str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

String getHeader(String varName) {
    //TODO: actually put the references libs on local file system again (only when using ESP32 which has more flash mem, but also make sure web server can handle this many concurrent requests (might crash, at least with ESP8266)
    switch (str2int(varName.c_str())) {
        case (str2int("FONTAWESOME")):
            #if defined(WEB_USE_LOCAL_LIBS) && WEB_USE_LOCAL_LIBS == 1
                return F("<link href=\"/css/fontawesome-6.1.2.min.css\" rel=\"stylesheet\">");
            #else
                return F("<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.1.2/css/all.min.css\">");
            #endif
        case (str2int("BOOTSTRAP")):
            #if defined(WEB_USE_LOCAL_LIBS) && WEB_USE_LOCAL_LIBS == 1
                return F("<link href=\"/css/bootstrap-5.1.3.min.css\" rel=\"stylesheet\">");
            #else
                return F("<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3\" crossorigin=\"anonymous\">");
            #endif
        case (str2int("BOOTSTRAP_BUNDLE")):
            #if defined(WEB_USE_LOCAL_LIBS) && WEB_USE_LOCAL_LIBS == 1
                return F("<script src=\"/js/bootstrap.bundle.5.1.3.min.js\" integrity=\"sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p\"></script>");
            #else
                return F("<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-ka7Sk0Gln4gmtz2MlQnikT1wXgYsOg+OMhuP+IlRH9sENBO0LRn5q+8nbTov4+1p\" crossorigin=\"anonymous\"></script>");
            #endif
        case (str2int("VUEJS")):
            #if defined(WEB_USE_LOCAL_LIBS) && WEB_USE_LOCAL_LIBS == 1
                return F("<script src=\"/js/vue.3.2.37.min.js\"></script>");
            #else
                return F("<script src=\"https://cdn.jsdelivr.net/npm/vue@3.2/dist/vue.global.prod.min.js\"></script>");
            #endif
        case (str2int("PLOTLY")):
            //file is too large for flash mem on ESP8288, replace with chart.js?
            //#if defined(WEB_USE_LOCAL_LIBS) && WEB_USE_LOCAL_LIBS == 1
            //    return F("<script src=\"/js/plotly-2.13.3.min.js\"></script>");
            //#else
                return F("<script src=\"https://cdn.plot.ly/plotly-basic-2.13.3.min.js\"></script>");
            //#endif
    }
    return "";
}

String staticProcessor(const String& var) {
    skipHeaterISR = true;

    //try replacing var for variables in editableVars
    if (var.startsWith("VAR_EDIT_")) {
        return generateForm(var.substring(9)); // cut off "VAR_EDIT_"
    } else if (var.startsWith("VAR_SHOW_")) {
        return getValue(var.substring(9)); // cut off "VAR_SHOW_"
    } else if (var.startsWith("VAR_HEADER_")) {
        return getHeader(var.substring(11)); // cut off "VAR_HEADER_"
    }

    //var didn't start with above names, try opening var as fragment file and use contents if it exists
    //TODO: this seems to consume too much heap in some cases, probably better to remove fragment loading and only use one SPA in the long term (or only support ESP32 which has more RAM)
    String varLower(var);
    varLower.toLowerCase();

    File file = LittleFS.open("/html_fragments/" + varLower + ".htm", "r");

    if (file && file.size()*2 < ESP.getFreeHeap()) {
        String ret = file.readString();
        file.close();
        return ret;
    } else {
        debugPrintf("Can't open file %s, not enough memory available\n", file ? file.name() : "");
    }
    
    skipHeaterISR = false;

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
        //stop writing to heater in ISR method (digitalWrite) as it causes crashes when called at the same time as flash is read or written
        skipHeaterISR = true;

        if (request->method() == 2) {   //returns values from WebRequestMethod enum -> 2 == HTTP_POST
            //update all given params and match var name in editableVars
            int params = request->params();
            String m = "Got ";
            m += params;
            m += " request parameters: <br />";

            for (int i = 0 ; i < params; i++) {
                AsyncWebParameter* p = request->getParam(i);
                String varName;
                if (p->name().startsWith("var")) {
                    varName = p->name().substring(3);
                } else {
                    varName = p->name();
                }

                for (editable_t e : editableVars) {
                    if (e.templateString != varName) {
                        continue;
                    }

                    m += "Setting ";
                    m += e.displayName;
                    m += " from ";

                    if (e.type == kInteger) {
                        m += *(int *)e.ptr;

                        int newVal = atoi(p->value().c_str());
                        *(int *)e.ptr = newVal;
                    } else if (e.type == kUInt8) {
                        m += *(uint8_t *)e.ptr;

                        *(uint8_t *)e.ptr = (uint8_t)atoi(p->value().c_str());
                    } else if (e.type == kDouble || e.type == kDoubletime) {
                        m += *(double *)e.ptr;

                        float newVal = atof(p->value().c_str());
                        *(double *)e.ptr = newVal;
                    }
                    //we don't need to set strings at the moment, needs testing also
                    /*} else if (e.type == kCString) {
                        m += String((char *)e.ptr);

                        String val = p->value();
                        static const char* newVal = new char[val.length() + 1]; //is this persistent or on stack?
                        val.toCharArray(newVal, val.length() + 1); 

                        if (e.ptr) {
                            delete[] ((char *)e.ptr);
                        }
                        e.ptr = (void *)newVal;
                    }*/

                    m += " to ";
                    m += p->value();

                    m += "<br/>";
                }
            }

            request->send(200, "text/html", m);

            // Write to EEPROM
            if (writeToEeprom) {
                if (writeToEeprom() == 0) {
                    debugPrintln("successfully wrote EEPROM");
                } else {
                    debugPrintln("EEPROM write failed");
                }
            }

            // Write the new values to Blynk and MQTT
            writeSysParamsToBlynk();
            writeSysParamsToMQTT();

        } else if (request->method() == 1) {  //WebRequestMethod enum -> HTTP_GET
            //return all params as json
            DynamicJsonDocument doc(JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(9+1) * EDITABLE_VARS_LEN);

            //get parameter id from frst parameter, e.g. /parameters?param=PID_ON
            int paramCount = request->params();
            String paramId = paramCount > 0 ? request->getParam(0)->value() : "";

            for (editable_t e : editableVars) {
                if (!paramId.isEmpty()) {
                    //we have a parameter to select a single var,
                    //skip vars until we find the one for the id in the request parameter
                    if (paramId != e.templateString) {
                        continue;
                    }                        
                }

                JsonObject paramObj = doc.createNestedObject();
                //set all parameter fields
                paramObj["type"] = e.type;
                paramObj["name"] = e.templateString;
                paramObj["displayName"] = e.displayName;
                paramObj["section"] = e.section;
                paramObj["hasHelpText"] = e.hasHelpText;
                paramObj["show"] = e.show();

                //set parameter value
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

                //we found the parameter, no need to search further
                if (!paramId.isEmpty()) {
                    break;
                }
            }

            if (doc.size() == 0) {
                request->send(404, "application/json", F("{ \"code\": 404, \"message\": \"Parameter not found\"}"));
                skipHeaterISR = false;
                return;
            }

            String paramsJson;
            serializeJson(doc, paramsJson);
            request->send(200, "application/json", paramsJson);
        }
        skipHeaterISR = false;
    });

    server.on("/parameterHelp", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(4096);

        AsyncWebParameter* p = request->getParam(0);
        const String& varValue = p->value();

        skipHeaterISR = true;

        for (editable_t e : editableVars) {
            if (e.templateString != varValue) {
                continue;
            }

            doc["name"] = e.templateString;
            doc["helpText"] = e.helpText;
            break;
        }

        String helpJson;
        serializeJson(doc, helpJson);
        request->send(200, "application/json", helpJson);

        skipHeaterISR = false;
    });

    server.on("/temperatures", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = getTempString();
        request->send(200, "application/json", json);
    });

    //TODO: could send values also chunked and without json (but needs three endpoints then?)
    //https://stackoverflow.com/questions/61559745/espasyncwebserver-serve-large-array-from-ram
    server.on("/timeseries", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");

        //set capacity of json doc for history structure
        DynamicJsonDocument doc(JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(HISTORY_LENGTH)*3);

        //for each value in mem history array, add json array element
        JsonArray currentTemps = doc.createNestedArray("currentTemps");
        JsonArray targetTemps = doc.createNestedArray("targetTemps");
        JsonArray heaterPowers = doc.createNestedArray("heaterPowers");
        
        //go through history values backwards starting from currentIndex and wrap around beginning
        //to include valueCount many values
        for (int i=mod(historyCurrentIndex-historyValueCount, HISTORY_LENGTH);
                 i!=mod(historyCurrentIndex, HISTORY_LENGTH);
                 i=mod(i+1, HISTORY_LENGTH))
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
    server.serveStatic("/css", LittleFS, "/css/", "max-age=604800");   //cache for one week
    server.serveStatic("/js", LittleFS, "/js/", "max-age=604800");
    server.serveStatic("/", LittleFS, "/html/", "max-age=604800").setTemplateProcessor(staticProcessor);

    server.begin();

    debugPrintln(("Server started at " + WiFi.localIP().toString()).c_str());
}

//skip counter so we don't keep a value every second
int skippedValues = 0;

void sendTempEvent(double currentTemp, double targetTemp, double heaterPower) {
    curTemp = currentTemp;
    tTemp = targetTemp;
    hPower = heaterPower;

    //save all values in memory to show history
    if (skippedValues > 0 && skippedValues % 4 == 0) {
        //use array and int value for start index (round robin)
        //one record (3 float values == 12 bytes) every five seconds, for half an hour -> 4.3kB of static memory
        tempHistory[0][historyCurrentIndex] = (float)currentTemp;
        tempHistory[1][historyCurrentIndex] = (float)targetTemp;
        tempHistory[2][historyCurrentIndex] = (float)heaterPower;
        historyCurrentIndex = (historyCurrentIndex+1) % HISTORY_LENGTH;
        historyValueCount = min(HISTORY_LENGTH-1, historyValueCount + 1);
        skippedValues = 0;
    } else {
        skippedValues++;
    }

    events.send("ping", NULL, millis());
    events.send(getTempString().c_str(), "new_temps", millis());
}

#endif
