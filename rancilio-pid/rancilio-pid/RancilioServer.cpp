#include "RancilioServer.h"
#include "userConfig.h"

#ifdef ESP32
#include "FS.h"
#include "SPIFFS.h"
#endif

AsyncWebServer server(80);

int (*writeToEeprom)(void) = NULL;
void setEepromWriteFcn(int (*fcnPtr)(void)) {
    writeToEeprom = fcnPtr;
}

String generateForm(String varName) {
    // Yikes, we don't have std::map available
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

    #ifdef ESP32
    timerAlarmDisable(timer);
    #endif
    File file = SPIFFS.open("/html_fragments/" + varLower + ".htm", "r");
    if(file) {
        String ret = file.readString();
        file.close();
        return ret;
    }
    #ifdef ESP32
    timerAlarmEnable(timer);
    #endif

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
                } else if (e.type == kDouble) {
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

        request->send(200, "text/html", m);
        if(writeToEeprom) {
            if (writeToEeprom() == 0) {
                Serial.println("successfully wrote EEPROM");
            } else {
                Serial.println("EEPROM write failed");
            }
        }
    });


    #ifdef ESP32
    timerAlarmDisable(timer);
    #endif
    SPIFFS.begin();
    server.serveStatic("/css", SPIFFS, "/css/");
    server.serveStatic("/", SPIFFS, "/html/").setTemplateProcessor(staticProcessor);
    #ifdef ESP32
    timerAlarmEnable(timer);
    #endif

    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println("Server started at " + WiFi.localIP().toString());
}
