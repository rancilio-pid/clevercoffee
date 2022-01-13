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
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "userConfig.h"


enum EditableKind {
    kInteger,
    kDouble,
    kDoubletime,
    kCString,
};

struct editable_t {
    String templateString;
    String displayName;
    EditableKind type;
    void *ptr;  // TODO: there must be a tidier way to do this? could we use c++ templates?
};

AsyncWebServer server(80);
AsyncEventSource events("/events");

double curTemp = 0.0;
double tTemp = 0.0;

void serverSetup();
void setEepromWriteFcn(int (*fcnPtr)(void));
void setBlynkWriteFcn(int (*fcnPtr)(void));


// We define these in the ino file
extern std::vector<editable_t> editableVars;


int (*writeToEeprom)(void) = NULL;

void setEepromWriteFcn(int (*fcnPtr)(void)) {
    writeToEeprom = fcnPtr;
}

int (*writeToBlynk)(void) = NULL;

void setBlynkWriteFcn(int (*fcnPtr)(void)) {
    writeToBlynk = fcnPtr;
}

String getTempString() {
    StaticJsonDocument<96> doc;

    doc["currentTemp"] = curTemp;
    doc["targetTemp"] = tTemp;

    String jsonTemps;

    serializeJson(doc, jsonTemps);

    return jsonTemps;
}

String generateForm(String varName) {
    // Yikes, we don't have std::map available

    // ms to s

    for (editable_t e : editableVars) {
        if (e.templateString != varName) {
            continue;
        }
        String result = "<label for=\"var";
        result += e.templateString;
        result += "\">";
        result += e.displayName;
        result += "</label><br />";

        String currVal;
        switch (e.type) {
            case kDouble:
                result += "<input type=\"number\" step=\"1\"";
                currVal = String(*(double *)e.ptr);
                break;
            case kDoubletime:
                result += "<input type=\"number\" step=\"1\"";
                currVal = String(*(double *)e.ptr/1000);
                break;    
            case kInteger:
                result += "<input type=\"number\" step=\"1\"";
                currVal = String(*(int *)e.ptr);
                break;
            default:
                result += "<input type=\"text\"";
                currVal = (const char *)e.ptr;
        }

        result += " id=\"var";
        result += e.templateString;
        result += "\" name=\"var";
        result += e.templateString;
        result += "\" value=\"";
        result += currVal;
        result += "\"><br />";

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
            case kCString:
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
    // Send a POST request to <IP>/post with a form field message set to <message>
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
        if(writeToEeprom)
        {
            if (writeToEeprom() == 0)
            {
                Serial.println("successfully wrote EEPROM");
            } else {
                Serial.println("EEPROM write failed");
            }
        }
        // Write to Blynk
        writeToBlynk();
    });

    SPIFFS.begin();
    server.serveStatic("/css", SPIFFS, "/css/");
    server.serveStatic("/js", SPIFFS, "/js/");
    server.serveStatic("/", SPIFFS, "/html/").setTemplateProcessor(staticProcessor);

    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });

    server.on("/temperatures", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = getTempString();
        request->send(200, "application/json", json);
        json = String();
    });

    events.onConnect([](AsyncEventSourceClient *client){
        if(client->lastId()) {
            Serial.printf("Reconnected, last message ID was: %u\n", client->lastId());
        }
        
        client->send("hello", NULL, millis(), 10000);
    });
    
    server.addHandler(&events);

    server.begin();
    Serial.println("Server started at " + WiFi.localIP().toString());
}


void sendTempEvent(float currentTemp, float targetTemp) {
    curTemp = currentTemp; 
    tTemp = targetTemp;

    events.send("ping", NULL, millis());
    events.send(getTempString().c_str(), "new_temps", millis());
}

#endif
