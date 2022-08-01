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
    #include "SPIFFS.h"
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
    #define WEBSERVER_H
#endif

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "userConfig.h"


enum EditableKind {
    kInteger,
    kUInt8,
    kDouble,
    kDoubletime,
    kCString,
    rInteger,
    rCString,
};

struct editable_t {
    String templateString;
    String displayName;
    String helpText;
    EditableKind type;
    void *ptr;  // TODO: there must be a tidier way to do this? could we use c++ templates?
};

AsyncWebServer server(80);
AsyncEventSource events("/events");

double curTemp = 0.0;
double tTemp = 0.0;
double hPower = 0.0;

void serverSetup();
void setEepromWriteFcn(int (*fcnPtr)(void));

// We define these in the ino file
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

String generateForm(String varName) {
    for (editable_t e : editableVars) {
        if (e.templateString != varName) {
            continue;
        }

        String result = "<label class=\"form-label me-1\" for=\"var";
        result += e.templateString;
        result += "\">";
        result += e.displayName;
        result += "</label>";
        if (!e.helpText.isEmpty()) {
            result += "<a href=\"#\" role=\"button\" data-bs-toggle=\"popover\" data-bs-html=\"true\" data-bs-original-title=\"" +
                e.helpText + "\"><span class=\"fa fa-question-circle\"></span></a></br>";
        } else {
            result += "<br/>";
        }
        String currVal;

        switch (e.type) {
            case kDouble:
                result += "<input class=\"form-control form-control-lg\" type=\"number\" step=\"0.1\"";
                currVal = String(*(double *)e.ptr);
                break;

            case kDoubletime:
                result += "<input class=\"form-control form-control-lg\" type=\"number\" step=\"0.1\"";
                currVal = String(*(double *)e.ptr/1000);
                break;

            case kInteger:
                result += "<input class=\"form-control form-control-lg\" type=\"number\" step=\"1\"";
                currVal = String(*(int *)e.ptr);
                break;

            case kUInt8:
            {
                uint8_t val = *(uint8_t *)e.ptr;

                result += "<input type=\"hidden\" id=\"var" + e.templateString + "\" name=\"var" + e.templateString + "\" value=\"0\" />";
                result += "<input type=\"checkbox\" class=\"form-check-input form-control-lg\" id=\"var" + e.templateString + "\" name=\"var" + e.templateString + "\" value=\"1\"";

                if(val) {
                    result += " checked=\"checked\"";
                }

                result += "><br />";

                break;
            }

            default:
                result += "<input class=\"form-control form-control-lg\" type=\"text\"";
                currVal = (const char *)e.ptr;
        }

        if (e.type != kUInt8) {
            result += " id=\"var";
            result += e.templateString;
            result += "\" name=\"var";
            result += e.templateString;
            result += "\" value=\"";
            result += currVal;
            result += "\"><br />";
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
                break;
            case kDoubletime:
                return String(*(double *)e.ptr);
                break;
            case kInteger:
                return String(*(int *)e.ptr);
                break;
            case kUInt8:
                return String(*(uint8_t *)e.ptr);
                break;
            case kCString:
                return String((const char *)e.ptr);
            case rInteger :
                return String(*(int *)e.ptr);
                break;
             case rCString:
                return String((const char *)e.ptr);
            default:
                return "Unknown type";
                break;
        }
    }
    return "(unknown variable " + varName + ")";
}

String staticProcessor(const String& var) {
    if (var.startsWith("VAR_EDIT_")) {
        return generateForm(var.substring(9)); // cut off "VAR_EDIT_"
    } else if (var.startsWith("VAR_SHOW_")) {
        return getValue(var.substring(9)); // cut off "VAR_SHOW_"
    }

    String varLower(var);
    varLower.toLowerCase();

    File file = SPIFFS.open("/html_fragments/" + varLower + ".htm", "r");

    if(file) {
        String ret = file.readString();
        file.close();
        return ret;
    }

    return String();
}

void serverSetup() {
    server.on("/steam", HTTP_POST, [](AsyncWebServerRequest *request) {
        int steam = flipUintValue(SteamON);

        setSteamMode(steam);
        Serial.printf("Toggle steam mode: %i \n", steam);

        request->redirect("/");
    });

    server.on("/pidstatus", HTTP_POST, [](AsyncWebServerRequest *request) {
        int status = flipUintValue(pidON);

        setPidStatus(status);
        Serial.printf("Toggle PID controller status: %i \n", status);

        request->redirect("/");
    });

    server.on("/backflush", HTTP_POST, [](AsyncWebServerRequest *request) {
        int backflush = flipUintValue(backflushON);

        setBackflush(backflush);
        Serial.printf("Toggle Backflush %i \n", backflush);

        request->redirect("/");
    });

    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
        int params = request->params();
        String m = "Got ";
        m += params;
        m += " request parameters: <br />";

        for(int i = 0 ; i < params; i++) {
            AsyncWebParameter* p = request->getParam(i);
            String varName = p->name().substring(3);

            for (editable_t e : editableVars) {
                if (e.templateString != varName) {
                    continue;
                }

                m += "Setting ";
                m += e.displayName;
                m += " to ";
                m += p->value();

                if (e.type == kInteger) {
                    int newVal = atoi(p->value().c_str());
                    *(int *)e.ptr = newVal;
                    m += ", it is now: ";
                    m += *(int *)e.ptr;
                } else if (e.type == kUInt8) {
                    *(uint8_t *)e.ptr = (uint8_t)atoi(p->value().c_str());
                    m += ", it is now: ";
                    m += *(uint8_t *)e.ptr;
                } else if (e.type == kDouble ||e.type == kDoubletime) {
                    float newVal = atof(p->value().c_str());
                    *(double *)e.ptr = newVal;
                    m += ", it is now: ";
                    m += *(double *)e.ptr;
                } else if (e.type == kCString) {
                    // Hum, how do we do this?
                    m += ", unsupported for now.";
                }

                m += "<br />";
            }
        }
         // ms to s

        request->send(200, "text/html", m);

        // Write to EEPROM
        if (writeToEeprom) {
            if (writeToEeprom() == 0)
            {
                Serial.println("successfully wrote EEPROM");
            } else {
                Serial.println("EEPROM write failed");
            }
        }

        // Write to Blynk and MQTT the new values
        writeSysParamsToBlynk();
        writeSysParamsToMQTT();
    });

    SPIFFS.begin();
    server.serveStatic("/css", SPIFFS, "/css/", "max-age=604800");   //cache for one week
    server.serveStatic("/js", SPIFFS, "/js/", "max-age=604800");
    server.serveStatic("/", SPIFFS, "/html/", "max-age=0").setTemplateProcessor(staticProcessor);

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server.on("/temperatures", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = getTempString();
        request->send(200, "application/json", json);
        json = String();
    });

    events.onConnect([](AsyncEventSourceClient *client) {
        if(client->lastId()) {
            Serial.printf("Reconnected, last message ID was: %u\n", client->lastId());
        }

        client->send("hello", NULL, millis(), 10000);
    });

    server.addHandler(&events);

    server.begin();
    Serial.println("Server started at " + WiFi.localIP().toString());
}


void sendTempEvent(float currentTemp, float targetTemp, float heaterPower) {
    curTemp = currentTemp;
    tTemp = targetTemp;
    hPower = heaterPower;

    events.send("ping", NULL, millis());
    events.send(getTempString().c_str(), "new_temps", millis());
}

#endif
