#ifndef IDISPLAY_H
#define IDISPLAY_H

#include <map>

#include "Viewport.h"
#include "tImage.h"

enum class Rotation {
    R0,
    R90,
    R180,
    R270
};

enum class FontType {
    H12Pixel,
    H15Pixel,
    fup17, // h 31
    fup25, // h 46
    fup35, // h 46
    Small, // 10 H10Pixel,
    Normal, // 11 H11Pixel,
    Big, // 22, H22Pixel,
    OpenIconicArrow,
    OpenIconicArrow2x,
    OpenIconicEmbedded,
    OpenIconicThing,
    OpenIconicOther
};

enum class Area {
    // BootLogo = 0,
    // BootMessage = 1,
    // ActionImage = 2, // like icon for heating, brewing, cleaning, ....
    // StatusMessage = 3,
    // StatusIcons = 4,
    // ProfileIcon = 5,
    // Temperature = 6,
    // SoftwareUpdate = 7,
    // test for clever coffee minimal template
    Statusbar,
    Progressbar,
    Temperature,
    BrewTime
};

class IDisplay {
public:
    // generic functions
    virtual void init(Rotation rotation);
    virtual void clearBuffer();
    virtual void sendBuffer();	
    virtual void setPowerSave(uint32_t is_enabled);
    virtual void setFont(FontType fontType);
    virtual void setCursor(int16_t x, int16_t y);
    virtual int getWidth();
    virtual int getHeight();

    // generic text functions
    virtual void print(float data, int digits);
    virtual void print(char c);
    virtual void print(const char* c);
    virtual void println(const String &s);
    virtual void printCentered(const char* c, uint16_t y);
    virtual void printCentered(const char* line1, const char* line2, uint16_t y);
    virtual void printRightAligned(const char* c, uint16_t y);
    virtual void printRightAligned(const char* c, uint16_t y, FontType font);
    virtual void printRightAligned(float data, unsigned int digits, uint16_t y);

    // generic graphic functions
    virtual void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    virtual void drawVLine(uint16_t x, uint16_t y, uint16_t h);
    virtual void drawHLine(uint16_t x, uint16_t y, uint16_t w);
    virtual void drawCircle(uint16_t x, uint16_t y, uint16_t rad);
    virtual void drawDisc(uint16_t x, uint16_t y, uint16_t rad);
    virtual void drawPixel(uint16_t x, uint16_t y);
    virtual void drawFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    virtual void clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

    // generic image functions    
    virtual void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap);
    virtual void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap);
    virtual void drawImage(uint16_t x, uint16_t y, tImage bitmap);
    virtual void drawGlyph(uint8_t x, uint8_t y, uint8_t encoding);
  
    // specific functions 
    virtual void printTemperatures(float t1, float t2, bool steaming);
    virtual void printBrewingInfo(unsigned long totalBrewTime, unsigned long brewTimer, unsigned int activeBrewTimeEndDetection, bool scaleEnabled, int currentWeight, float activeScaleSensorWeightSetPoint); // TODO: we could use a struct instead of 6 paramters, eg. "brewInfoData"
    virtual void printCountdown(unsigned int timer);

    // functions for defined areas
    virtual Viewport getView(Area);
    virtual void clearView(Area);
    virtual void fillView(Area view, uint32_t color);

    virtual void drawImageCentered(Area, int, int, const uint8_t *bitmap);
    virtual void drawImageCentered(Area, int, int, const uint16_t *bitmap);

    virtual void printCentered(Area, const char* c);
    virtual void printCentered(Area, const char* line1, const char* line2);
    // virtual void printRightAligned(Area, const char* c);
    // virtual void printRightAligned(Area, float data, unsigned int digits);

    virtual void drawBorder(Area);
};

#endif // IDISPLAY_H