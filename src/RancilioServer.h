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

        String result = "<label class=\"form-label\" for=\"var";
        result += e.templateString;
        result += "\">";
        result += e.displayName;
        result += "</label><br />";

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


int readMinInt(String valName){
    int min = 0;
 
    Serial.println(valName);
    Serial.println("Unknown parameter");        

    return min;
}

int readMaxInt(String valName){
    int max = 0;
 
    Serial.println(valName);
    Serial.println("Unknown parameter");

    return max;
}

uint8_t readMinUint(String valName){
    uint8_t min = 0;

    if (valName == "PID_ON"){
        min = sysParaPidOn.getMin();
    } else { 
        Serial.println(valName);
        Serial.println("Unknown parameter");        
    }

    return min;
}

uint8_t readMaxUint(String valName){
    uint8_t max = 0;

    if (valName == "PID_ON"){
        max = sysParaPidOn.getMax();
    } else { 
        Serial.println(valName);
        Serial.println("Unknown parameter");        
    }

    return max;
}

double readMinDouble(String valName){
    double min = 0.0;

    if (valName == "START_KP"){
        min = sysParaPidKpStart.getMin();
    } else if (valName == "START_TN"){
        min = sysParaPidTnStart.getMin();
    } else if (valName == "PID_KP"){
        min = sysParaPidKpReg.getMin();
    } else if (valName == "PID_TN"){
        min = sysParaPidTnReg.getMin();
    } else if (valName == "PID_TV"){
        min = sysParaPidTvReg.getMin();
    } else if (valName == "STEAM_KP"){
        min = sysParaPidKpSteam.getMin();
    } else if (valName == "BREW_SET_POINT"){
        min = sysParaBrewSetPoint.getMin();
    } else if (valName == "BREW_TIME"){
        min = sysParaBrewTime.getMin();
    } else if (valName == "BREW_PREINFUSUINPAUSE"){
        min = sysParaPreInfPause.getMin();
    } else if (valName == "BREW_PREINFUSION"){
        min = sysParaPreInfTime.getMin();
    } else if (valName == "SCALE_WEIGHTSETPOINT"){
        min = sysParaWeightSetPoint.getMin();
    } else if (valName == "PID_BD_TN"){
        min = sysParaPidTnBd.getMin();
    } else if (valName == "PID_BD_KP"){
        min = sysParaPidKpBd.getMin();
    } else if (valName == "PID_BD_TN"){
        min = sysParaPidTnBd.getMin();
    } else if (valName == "PID_BD_TV"){
        min = sysParaPidTvBd.getMin();
    } else if (valName == "PID_BD_TIMER"){
        min = sysParaBrewSwTimer.getMin();
    } else if (valName == "PID_BD_BREWBOARDER"){
        min = sysParaBrewThresh.getMin();
    } else { 
        Serial.println(valName);
        Serial.println("Unknown parameter");        
    }
    return min;
}

double readMaxDouble(String valName){
    double max = 0.0;

    if (valName == "START_KP"){
        max = sysParaPidKpStart.getMax();
    } else if (valName == "START_TN"){
        max = sysParaPidTnStart.getMax();
    } else if (valName == "PID_KP"){
        max = sysParaPidKpReg.getMax();
    } else if (valName == "PID_TN"){
        max = sysParaPidTnReg.getMax();
    } else if (valName == "PID_TV"){
        max = sysParaPidTvReg.getMax();
    } else if (valName == "STEAM_KP"){
        max = sysParaPidKpSteam.getMax();
    } else if (valName == "BREW_SET_POINT"){
        max = sysParaBrewSetPoint.getMax();
    } else if (valName == "BREW_TIME"){
        max = sysParaBrewTime.getMax();
    } else if (valName == "BREW_PREINFUSUINPAUSE"){
        max = sysParaPreInfPause.getMax();
    } else if (valName == "BREW_PREINFUSION"){
        max = sysParaPreInfTime.getMax();
    } else if (valName == "SCALE_WEIGHTSETPOINT"){
        max = sysParaWeightSetPoint.getMax();
    } else if (valName == "PID_BD_TN"){
        max = sysParaPidTnBd.getMax();
    } else if (valName == "PID_BD_KP"){
        max = sysParaPidKpBd.getMax();
    } else if (valName == "PID_BD_TN"){
        max = sysParaPidTnBd.getMax();
    } else if (valName == "PID_BD_TV"){
        max = sysParaPidTvBd.getMax();
    } else if (valName == "PID_BD_TIMER"){
        max = sysParaBrewSwTimer.getMax();
    } else if (valName == "PID_BD_BREWBOARDER"){
        max = sysParaBrewThresh.getMax();
    } else { 
        Serial.println(valName);
        Serial.println("Unknown parameter");        
    }


    return max;
}

template <typename T> 
T validateValue(const char* valType, String valName, T Value){

    // There has to be a better way to do this using 
    // typeid(T)==typeid(uint8_t),
    // but I could not get it to work!
    if (valType == "int") {
        int min_Val = readMinInt(valName);
        int max_Val = readMaxInt(valName);

        if (Value < min_Val) {
            Serial.println("requested set of value is lower than minimum limit");
            Serial.println("Value has been set to minium limit.");
            Value = min_Val;
        } else if (Value > max_Val){
            Serial.println("requested set of value is higher than maximum limit");
            Serial.println("Value has been set to maximum limit.");
            Value = max_Val;
        }
    } else if (valType == "uint8_t") {
        uint8_t min_Val = readMinUint(valName);
        uint8_t max_Val = readMaxUint(valName);

        if (Value < min_Val) {
            Serial.println("requested set of value is lower than minimum limit");
            Serial.println("Value has been set to minium limit.");
            Value = min_Val;
        } else if (Value > max_Val){
            Serial.println("requested set of value is higher than maximum limit");
            Serial.println("Value has been set to maximum limit.");
            Value = max_Val;
        }
    } else if (valType == "double") {
        double min_Val = readMinDouble(valName);
        double max_Val = readMaxDouble(valName);

        if (Value < min_Val) {
            Serial.println("requested set of value is lower than minimum limit");
            Serial.println("Value has been set to minium limit.");
            Value = min_Val;
        } else if (Value > max_Val){
            Serial.println("requested set of value is higher than maximum limit");
            Serial.println("Value has been set to maximum limit.");
            Value = max_Val;
        }
    } else if (valType == "string") {
        Serial.println("I am an string");
    }

    return Value;
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

                const char* newValType = "";

                if (e.type == kInteger) {
                    newValType = "int";
                    int newVal = atoi(p->value().c_str());
                    int validVal = validateValue(newValType, varName,  newVal);
                        if (validVal != newVal){
                        m += " cannot be done! Value outside limits (";
                        m +=  readMinInt(varName);
                        m += ",";
                        m += readMaxInt(varName);
                        m += "). Set to: ";
                    } else {
                    m += ", it is now: ";
                    }
                    *(int *)e.ptr = validVal;
                    m += *(int *)e.ptr;
                } else if (e.type == kUInt8) {
                    newValType = "uint8_t";
                    uint8_t newVal = atoi(p->value().c_str());
                    uint8_t validVal = validateValue(newValType, varName,  newVal);
                    if (validVal != newVal){
                        m += " cannot be done! Value outside limits (";
                        m +=  readMinUint(varName);
                        m += ",";
                        m += readMaxUint(varName);
                        m += "). Set to: ";
                    } else {
                    m += ", it is now: ";
                    }
                    *(uint8_t *)e.ptr = validVal;
                    m += *(uint8_t *)e.ptr;
                } else if (e.type == kDouble ||e.type == kDoubletime) {
                    newValType = "double";
                    double newVal = atof(p->value().c_str());
                    double validVal = validateValue(newValType, varName,  newVal);
                    *(double *)e.ptr = validVal;
                    if (validVal != newVal){
                        m += " cannot be done! Value outside limits (";
                        m +=  readMinDouble(varName);
                        m += ",";
                        m += readMaxDouble(varName);
                        m += "). Set to: ";
                    } else {
                    m += ", it is now: ";
                    }
                    m += *(double *)e.ptr;
                } else if (e.type == kCString) {
                    newValType = "string";
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
    server.serveStatic("/css", SPIFFS, "/css/");
    server.serveStatic("/js", SPIFFS, "/js/");
    server.serveStatic("/", SPIFFS, "/html/").setTemplateProcessor(staticProcessor);

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
