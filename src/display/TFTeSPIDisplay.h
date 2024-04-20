#ifndef TFTeSPIDisplay_H
#define TFTeSPIDisplay_H

#include <TFT_eSPI.h>
#include <TFT_eWidget.h>
#include "DisplayBase.h"

// Meter colour schemes
#define RED2RED 0
#define GREEN2GREEN 1
#define BLUE2BLUE 2
#define BLUE2RED 3
#define GREEN2RED 4
#define RED2GREEN 5
#define BLUE2GREEN 6

class TFTeSPIDisplay : public DisplayBase {
public:
    TFTeSPIDisplay(TFT_eSPI& tftInstance);

    void init(Rotation rotation) override;
    void clearBuffer() override;
    void setPowerSave(uint32_t is_enabled) override;
    void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap) override;
    void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap) override;
    void drawImage(uint16_t x, uint16_t y, tImage bitmap) override;
    void setFont(FontType fontType) override;
    void setCursor(int16_t x, int16_t y) override;
    void print(float data, int digits) override;
    void print(char c) override;
    void print(const char* c) override;
    void println(const String &s) override;
    void drawGlyph(uint8_t x, uint8_t y, uint8_t encoding) override;
    void sendBuffer() override;
    int getWidth() override;
    int getHeight() override;
    void clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) override;
    void printCentered(const char* c, uint16_t y) override;
    void printCentered(const char* line1, const char* line2, uint16_t y) override;
    void printRightAligned(const char* c, uint16_t y) override;
    void printRightAligned(float data, unsigned int digits, uint16_t y) override;

    void printTemperatures(float t1, float t2, bool steaming) override;
    void printBrewingInfo(unsigned long totalBrewTime, unsigned long brewTimer, unsigned int activeBrewTimeEndDetection, bool scaleEnabled, int currentWeight, float activeScaleSensorWeightSetPoint) override;
    void printCountdown(unsigned int timer) override;

    void fillView(Area, uint32_t color) override;
    void drawBorder(Area) override;

    void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) override;
    void drawVLine(uint16_t x, uint16_t y, uint16_t h) override;
    void drawHLine(uint16_t x, uint16_t y, uint16_t w) override;
    void drawCircle(uint16_t x, uint16_t y, uint16_t rad) override;
    void drawDisc(uint16_t x, uint16_t y, uint16_t rad) override;
    void drawPixel(uint16_t x, uint16_t y) override;
    void drawFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h) override;


protected:
    void initViews() override;

private:
    TFT_eSPI& tft;
    int ringMeter(float value, int vmin, int vmax, int x, int y, int r, const char *units, byte scheme);
    unsigned int rainbow(byte value);
};

#endif // TFTeSPIDisplay_H
