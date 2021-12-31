#include "RancilioServer.h"

AsyncWebServer server(80);

String generateForm() {
    // TODO: That's a lot of allocation. We should find a better way. Maybe there's a simple template engine?
  #if defined(ESP8266)
   timer1_disable();
   #elif defined(ESP32) // ESP32
   timerAlarmDisable(timer);
   #else
   #error("not supported MCU");
   #endif
    
    String result = "<html><body><h2>ranciliopid</h2><p>Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.</p><form action=\"/post\" method=\"post\">";
    int i = 0;
    for (editable_t e : editableVars) {
        result += "<label for=\"var";
        result += i;
        result += "\">";
        result += e.displayName;
        result += "</label><br />";

        String currVal;
        switch (e.type) {
            case kDouble:
                result += "<input type=\"number\" step=\"0.01\"";
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
        result += i;
        result += "\" name=\"var";
        result += i;
        result += "\" value=\"";
        result += currVal;
        result += "\"><br />";

        i++;
    }
    result += "<input type=\"submit\"></form></body></html>";
    return result;
}

void serverSetup() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", generateForm());
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
        int params = request->params();
        String m = "Got ";
        m += params;
        m += " request parameters: <br />";
        for(int i = 0 ; i < params; i++) {
            AsyncWebParameter* p = request->getParam(i);
            int paramNum = atoi(p->name().substring(3).c_str());
            editable_t thisEdit = editableVars[paramNum];

            m += "Setting ";
            m += thisEdit.displayName;
            m += " to ";
            m += p->value();

            if (thisEdit.type == kInteger) {
                int newVal = atoi(p->value().c_str());
                *(int *)thisEdit.ptr = newVal;
                m += ", it is now: ";
                m += *(int *)thisEdit.ptr;
            } else if (thisEdit.type == kDouble) {
                float newVal = atof(p->value().c_str());
                *(double *)thisEdit.ptr = newVal;
                m += ", it is now: ";
                m += *(double *)thisEdit.ptr;
            }
            m += "<br />";
        }

        request->send(200, "text/html", m);
      #if defined(ESP8266)
   //timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
   timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
   #elif defined(ESP32) // ESP32
   timerAlarmEnable(timer);
   #else
   #error("not supported MCU");
   #endif
 
    });

    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println("Server started at " + WiFi.localIP().toString());
}
