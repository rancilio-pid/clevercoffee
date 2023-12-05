#include "debugSerial.h"


void getCurrentTimeString(char *output) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    snprintf(output, 12, "[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

// Print to remote serial (e.g. using OTA Monitor Task ) if client is connected, otherwise use hardware serial
void debugPrintln(const char *message) {
    char time[12];
    getCurrentTimeString(time);

    Serial.print(time);
    Serial.println(message);
}

void debugPrint(const char *message) {
    char time[12];
    getCurrentTimeString(time);

    Serial.print(time);
    Serial.print(message);
}

size_t debugPrintf(const char *format, ...) {
    //reimplement printf method so that we can take dynamic list of parameters as printf does (we can't simply pass these on to existing printf methods it seems)

    va_list arg;
    va_start(arg, format);
    char temp[64];  //allocate a temp buffer
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);

    //if temp buffer was too short, create new one with enough room (inludes previous bytes)
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer) {
            return 0;
        }
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }

    //get time to prepend to message
    char time[12];
    getCurrentTimeString(time);

    len = Serial.print(time);
    len += Serial.print(buffer);

    if (buffer != temp) {
        delete[] buffer;
    }
    return len;
}