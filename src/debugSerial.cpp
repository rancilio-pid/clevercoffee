/**
 * @file debugSerial.cpp
 *
 * @brief TODO
 *
 */

#include "debugSerial.h"

// server for monitor connections
WiFiServer SerialServer(23);
WiFiClient RemoteSerial;

void startRemoteSerialServer() {
    SerialServer.begin();
}

void checkForRemoteSerialClients() {
    if (SerialServer.hasClient()) {
        // If we are already connected to another client, then reject the new connection. 
        // Otherwise accept the connection.
        if (RemoteSerial.connected()) {
            debugPrintln("Serial Server Connection rejected");
            SerialServer.available().stop();
        }
        else {
            debugPrintln("Serial Server Connection accepted");
            RemoteSerial = SerialServer.available();
        }
    }
}

void getCurrentTimeString(char *output) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    snprintf(output, 12, "[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

// Print to remote serial (e.g. using OTA Monitor Task ) if client is connected, 
// otherwise use hardware serial
void debugPrintln(const char *message) {
    char time[12];
    getCurrentTimeString(time);

    if (RemoteSerial.connected()) {
        RemoteSerial.print(time);
        RemoteSerial.println(message);
    }
    else {
        Serial.print(time);
        Serial.println(message);
    }
}

// Print to remote serial (e.g. using OTA Monitor Task ) if client is connected, 
// otherwise use hardware serial
void debugPrintln(const String& message) {
    debugPrintln(message.c_str());
}

void debugPrint(const char *message) {
    char time[12];
    getCurrentTimeString(time);

    if (RemoteSerial.connected()) {
        RemoteSerial.print(time);
        RemoteSerial.print(message);
    }
    else {
        Serial.print(time);
        Serial.print(message);
    }
}

void debugPrint(const String& message) {
    debugPrint(message.c_str());
}

size_t debugPrintf(const char *format, ...) {
    // reimplement printf method so that we can take dynamic list of parameters as printf does 
    // (we can't simply pass these on to existing printf methods it seems)

    va_list arg;
    va_start(arg, format);
    char temp[64];  // allocate a temp buffer
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);

    // if temp buffer was too short, create new one with enough room (inludes previous bytes)
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];

        if (!buffer) {
            return 0;
        }

        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }

    // get time to prepend to message
    char time[12];
    getCurrentTimeString(time);

    if (RemoteSerial.connected()) {
        len = RemoteSerial.print(time);
        len += RemoteSerial.print(buffer);
    }
    else {
        len = Serial.print(time);
        len += Serial.print(buffer);
    }

    if (buffer != temp) {
        delete[] buffer;
    }

    return len;
}
