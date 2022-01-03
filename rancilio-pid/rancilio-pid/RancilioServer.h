#ifndef rancilioserver_h
#define rancilioserver_h

#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

enum EditableKind {
    kInteger,
    kDouble,
    kCString,
};

struct editable_t {
    String templateString;
    String displayName;
    EditableKind type;
    void *ptr;  // TODO: there must be a tidier way to do this? could we use c++ templates?
};

void serverSetup();

// We define these in the ino file
extern std::vector<editable_t> editableVars;

#endif
