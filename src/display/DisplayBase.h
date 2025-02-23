#ifndef DISPLAY_BASE_H
#define DISPLAY_BASE_H

#include "IDisplay.h" // Include your interface header

class DisplayBase : public IDisplay {
public:
    // To be implemented by derived classes
    virtual void init(Rotation rotation) = 0; 
    virtual void clearBuffer() = 0; 
    virtual void setPowerSave(uint32_t is_enabled) = 0; 
    virtual void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* bitmap) = 0; 
    virtual void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* bitmap) = 0; 
    virtual void drawImage(uint16_t x, uint16_t y, tImage bitmap) = 0; 
    virtual void setFont(FontType fontType) = 0; 
    virtual void setCursor(int16_t x, int16_t y) = 0; 
    virtual void print(float data, int digits) = 0; 
    virtual void print(char c) = 0; 
    virtual void print(const char* c) = 0; 
    virtual void println(const String &s) = 0;
    virtual void drawGlyph(uint8_t x, uint8_t y, uint8_t encoding) = 0; 
    virtual void sendBuffer() = 0; 
    virtual int getWidth() = 0; 
    virtual int getHeight() = 0; 
    virtual void clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) = 0; 

    virtual void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) = 0;
    virtual void drawVLine(uint16_t x, uint16_t y, uint16_t h) = 0;
    virtual void drawHLine(uint16_t x, uint16_t y, uint16_t w) = 0;
    virtual void drawCircle(uint16_t x, uint16_t y, uint16_t rad) = 0;
    virtual void drawDisc(uint16_t x, uint16_t y, uint16_t rad) = 0;
    virtual void drawPixel(uint16_t x, uint16_t y) = 0;
    virtual void drawFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h) = 0;

    virtual void printCentered(const char* c, uint16_t y) = 0; 
    virtual void printCentered(const char* line1, const char* line2, uint16_t y) = 0; 

    virtual void printRightAligned(const char* c, uint16_t y) = 0; 
    virtual void printRightAligned(const char* c, uint16_t y, FontType font) = 0; 
    virtual void printRightAligned(float data, unsigned int digits, uint16_t y) = 0; 
    virtual void printTemperatures(float t1, float t2, bool steaming) = 0; 
    virtual void printBrewingInfo(unsigned long totalBrewTime, unsigned long brewTimer, unsigned int activeBrewTimeEndDetection, bool scaleEnabled, int currentWeight, float activeScaleSensorWeightSetPoint) = 0;
    virtual void printCountdown(unsigned int timer) = 0;

    virtual void fillView(Area view, uint32_t color) = 0; 
    virtual void drawBorder(Area view) = 0;

    // implemented by baseclass, probably we could move more methods there
    virtual int getDataDigits(float data);
    virtual Viewport getView(Area area);
    virtual void clearView(Area area);
    virtual void drawImageCentered(Area area, int width, int height, const uint8_t *bitmap);
    virtual void drawImageCentered(Area area, int width, int height, const uint16_t *bitmap);
    virtual void printCentered(Area area, const char* line1);
    virtual void printCentered(Area area, const char* line1, const char* line2);

protected:
    virtual void initViews() = 0;

    std::map<Area, Viewport> areaMap;

    // Viewport bootLogo;
    // Viewport bootMessage;
    // Viewport actionImage;
    // Viewport statusIcons;
    // Viewport profileIcon;
    // Viewport temperature;
    // Viewport statusMessage;
    // Viewport softwareUpdate;

    Viewport statusbar;
    Viewport progressbar;
    Viewport temperature;
    Viewport brewTime;
};

#endif // DISPLAY_BASE_H
