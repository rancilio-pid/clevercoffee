#include "DummyDisplay.h"

// this block is needed to avoid crash with no display 
Viewport dummy = Viewport(Point(), 0, 0);
std::map<Area, Viewport> areaMap = {
    //{Area::BootLogo, dummy},
};


DummyDisplay::DummyDisplay() {
}

void DummyDisplay::initViews() {
}

void DummyDisplay::init(Rotation rotation) {
}

void DummyDisplay::clearBuffer() {
}

void DummyDisplay::sendBuffer() {
}

void DummyDisplay::setPowerSave(uint32_t is_enabled) {
}

void DummyDisplay::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap) {
}

void DummyDisplay::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap) {
}

void DummyDisplay::drawImage(uint16_t x, uint16_t y, tImage bitmap) {
}

void DummyDisplay::setFont(FontType fontType) {
}

void DummyDisplay::setCursor(int16_t x, int16_t y) {
}

void DummyDisplay::print(float data, int digits) {
}

void DummyDisplay::print(char c) {
}

void DummyDisplay::print(const char* c) {
}

void DummyDisplay::println(const String &s) {
}

void DummyDisplay::drawGlyph(uint8_t x, uint8_t y, uint8_t encoding) {
}

void DummyDisplay::clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
}

int DummyDisplay::getWidth() {
    return 1;
}

int DummyDisplay::getHeight() {
    return 1;
}

void DummyDisplay::printCentered(const char* c, uint16_t y) {
}

void DummyDisplay::printCentered(const char* line1, const char* line2, uint16_t y) {
}

void DummyDisplay::printRightAligned(const char* c, uint16_t y) {
}

void DummyDisplay::printRightAligned(const char* c, uint16_t y, FontType font) {
}

void DummyDisplay::printRightAligned(float data, unsigned int digits, uint16_t y) {
}

void DummyDisplay::printTemperatures(float t1, float t2, bool steaming) {
}

void DummyDisplay::printBrewingInfo(unsigned long totalBrewTime, unsigned long brewTimer, unsigned int activeBrewTimeEndDetection, bool scaleEnabled, int currentWeight, float activeScaleSensorWeightSetPoint) {
}

void DummyDisplay::printCountdown(unsigned int timer) {
}

void DummyDisplay::fillView(Area area, uint32_t color) {
}

void DummyDisplay::drawBorder(Area area) {
}

void DummyDisplay::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
}

void DummyDisplay::drawVLine(uint16_t x, uint16_t y, uint16_t h) {
}

void DummyDisplay::drawHLine(uint16_t x, uint16_t y, uint16_t w) {
}

void DummyDisplay::drawCircle(uint16_t x, uint16_t y, uint16_t rad) {
}

void DummyDisplay::drawDisc(uint16_t x, uint16_t y, uint16_t rad) {
}

void DummyDisplay::drawPixel(uint16_t x, uint16_t y) {
}