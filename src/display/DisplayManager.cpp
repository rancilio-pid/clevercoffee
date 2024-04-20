#include "DisplayManager.h"
#include "../userConfig.h"
#include "../hardware/pinmapping.h"

#if (DISPLAY_HARDWARE == 4)
#define TFT_eSPI_DISPLAY
#include "TFTeSPIDisplay.h"
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>
#elif (DISPLAY_HARDWARE == 1) || (DISPLAY_HARDWARE == 2) || (DISPLAY_HARDWARE == 3)
#define U8G2_DISPLAY
#include "U8g2Display.h"
#else
#define NO_DISPLAY
#endif

#if (DISPLAY_HARDWARE == 3)
// 23-MOSI 18-CLK
#define OLED_CS             5
#define OLED_DC             2
#include <SPI.h>
#endif

#if (DISPLAY_HARDWARE == 0)
#define NO_DISPLAY
#include "DummyDisplay.h"
#endif

#if (DISPLAY_HARDWARE == 1)
  U8G2_SH1106_128X64_NONAME_F_HW_I2C displayInstance(U8G2_R0, U8X8_PIN_NONE, PIN_I2CSCL, PIN_I2CSDA); // e.g. 1.3"
#elif (DISPLAY_HARDWARE == 2)
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C displayInstance(U8G2_R0, U8X8_PIN_NONE, PIN_I2CSCL, PIN_I2CSDA); // e.g. 0.96"
#elif (DISPLAY_HARDWARE == 3)
  U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI displayInstance(U8G2_R0, OLED_CS, OLED_DC, /* reset=*/U8X8_PIN_NONE); // e.g. 1.3"
#elif (DISPLAY_HARDWARE == 4)
  TFT_eSPI displayInstance = TFT_eSPI();
#endif

DisplayManager::DisplayManager() {
#ifdef U8G2_DISPLAY
    display = new U8g2Display(displayInstance);
#endif    

#ifdef TFT_eSPI_DISPLAY
    display = new TFTeSPIDisplay(displayInstance);
#endif

#ifdef NO_DISPLAY
    display = new DummyDisplay();
#endif
}

void DisplayManager::init(Rotation rotation) {
    display->init(rotation);
}

void DisplayManager::clearBuffer() {
    display->clearBuffer();
}

void DisplayManager::setPowerSave(uint32_t is_enabled) {
    display->setPowerSave(is_enabled);
}

void DisplayManager::clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    display->clearRect(x, y, w, h);
};

void DisplayManager::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap) {
    display->drawImage(x, y, w, h, bitmap);
}

void DisplayManager::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap) {
    display->drawImage(x, y, w, h, bitmap);
}

void DisplayManager::drawImage(uint16_t x, uint16_t y, tImage bitmap) {
    display->drawImage(x, y, bitmap);
}

void DisplayManager::setFont(FontType fontType) {
    display->setFont(fontType);
}

void DisplayManager::setCursor(int16_t x, int16_t y) {
    display->setCursor(x, y);
}

void DisplayManager::print(float data, int digits) {
    display->print(data, digits);
}

void DisplayManager::print(char c) {
    display->print(c);
}

void DisplayManager::print(const char* c) {
    display->print(c);
}

void DisplayManager::println(const String &s) {
    display->println(s);
}

void DisplayManager::drawGlyph(uint8_t x, uint8_t y, uint8_t encoding) {
    display->drawGlyph(x, y, encoding);
}

void DisplayManager::sendBuffer() {
    display->sendBuffer();
}

int DisplayManager::getWidth() {
    return display->getWidth();
}

int DisplayManager::getHeight() {
    return display->getHeight();
}

void DisplayManager::printCentered(const char* c, uint16_t y) {
    display->printCentered(c, y);
}

void DisplayManager::printCentered(const char* line1, const char* line2, uint16_t y) {
    display->printCentered(line1, line2, y);
}

void DisplayManager::printRightAligned(const char* c, uint16_t y) {
    display->printRightAligned(c, y);
}

void DisplayManager::printRightAligned(float data, unsigned int digits, uint16_t y) {
    display->printRightAligned(data, digits, y);
}

void DisplayManager::printTemperatures(float t1, float t2, bool steaming) {
    display->printTemperatures(t1, t2, steaming);
}

void DisplayManager::printBrewingInfo(unsigned long totalBrewTime, unsigned long brewTimer, unsigned int activeBrewTimeEndDetection, bool scaleEnabled, int currentWeight, float activeScaleSensorWeightSetPoint) {
    display->printBrewingInfo(totalBrewTime, brewTimer, activeBrewTimeEndDetection, scaleEnabled, currentWeight, activeScaleSensorWeightSetPoint);
}

void DisplayManager::printCountdown(unsigned int timer) {
    display->printCountdown(timer);
}

Viewport DisplayManager::getView(Area area) {
    return display->getView(area);
}

void DisplayManager::clearView(Area area) {
    display->clearView(area);
}

void DisplayManager::fillView(Area area, uint32_t color) {
    display->fillView(area, color);
}

void DisplayManager::drawImageCentered(Area area, int w, int h, const uint8_t *bitmap) {
    display->drawImageCentered(area, w, h, bitmap);
}

void DisplayManager::drawImageCentered(Area area, int w, int h, const uint16_t *bitmap) {
    display->drawImageCentered(area, w, h, bitmap);
}

void DisplayManager::printCentered(Area area, const char* c) {
    display->printCentered(area, c);
}

void DisplayManager::printCentered(Area area, const char* line1, const char* line2) {
    display->printCentered(area, line1, line2);
}

void DisplayManager::drawBorder(Area area) {
    display->drawBorder(area);
}

void DisplayManager::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    display->drawLine(x1, y1, x2, y2);
}

void DisplayManager::drawVLine(uint16_t x, uint16_t y, uint16_t h) {
    display->drawVLine(x, y, h);
}

void DisplayManager::drawHLine(uint16_t x, uint16_t y, uint16_t w) {
    display->drawHLine(x, y, w);
}

void DisplayManager::drawCircle(uint16_t x, uint16_t y, uint16_t rad) {
    display->drawCircle(x, y, rad);
}

void DisplayManager::drawDisc(uint16_t x, uint16_t y, uint16_t rad) {
    display->drawDisc(x, y, rad);
}

void DisplayManager::drawPixel(uint16_t x, uint16_t y) {
    display->drawPixel(x, y);
}

void DisplayManager::drawFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    display->drawFrame(x, y, w, h);
}