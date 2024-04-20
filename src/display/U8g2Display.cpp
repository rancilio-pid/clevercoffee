#include "U8g2Display.h"
#include "../userConfig.h"
// #include "debugSerial.h"
#include <float.h>

U8g2Display::U8g2Display(U8G2& u8g2Instance) : u8g2(u8g2Instance) {}

void U8g2Display::init(Rotation rotation) {
#ifdef ESP32
#if (DISPLAY_HARDWARE == 3)
  u8g2.setBusClock(600000);
#else
  u8g2.setBusClock(2000000);
#endif
#endif
  u8g2.begin();
  prepare();
  //u8g2.setFlipMode(0); // 0=Off (Default), 1=180 degree clockwise rotation
  if (rotation == Rotation::R90)
    u8g2.setDisplayRotation(U8G2_R1);
  else if (rotation == Rotation::R180)
    u8g2.setDisplayRotation(U8G2_R2);
  else if (rotation == Rotation::R270)
    u8g2.setDisplayRotation(U8G2_R3);
  else 
    u8g2.setDisplayRotation(U8G2_R0);

  u8g2.clear();

  u8g2.setBitmapMode(1);  // Enable transparent bitmap mode

  initViews();    
}

void U8g2Display::initViews() {
    int iconWidth  = 45;
    int iconHeight  = 45;
    int maxBootLogoHeight = 45;
    int margin = 2;
    int statusIconHeight = 9;
    int statusIconWidth = 9;
    int statusMessageY = iconHeight - 5;
    int height = 37; 

    bootLogo = Viewport(0, 0, getWidth(), maxBootLogoHeight);
    bootMessage = Viewport(0, maxBootLogoHeight + margin, getWidth(), getHeight() - maxBootLogoHeight - margin);

    actionImage = Viewport(0, 0, iconWidth, iconHeight);
    temperature = Viewport(iconWidth + margin, 0, getWidth() - iconWidth - margin, iconHeight);

    statusMessage = Viewport(0, statusMessageY, getWidth(), getHeight() - iconHeight - statusIconHeight);
    statusIcons = Viewport(0, getHeight() - statusIconHeight, getWidth() - statusIconWidth, statusIconHeight);
    profileIcon = Viewport(getWidth() - statusIconWidth - 1, getHeight() - statusIconHeight, statusIconWidth, statusIconHeight);

    softwareUpdate = Viewport(0, 0, getWidth(), getHeight()); // fullscreen

    areaMap = {
        {Area::BootLogo, this->bootLogo},
        {Area::BootMessage, this->bootMessage},
        {Area::ActionImage, this->actionImage},
        {Area::StatusMessage, this->statusMessage},
        {Area::Temperature, this->temperature},
        {Area::StatusIcons, this->statusIcons},
        {Area::ProfileIcon, this->profileIcon},
        {Area::SoftwareUpdate, this->softwareUpdate},
    };        
}

void U8g2Display::prepare(void) {
  u8g2.setFont(u8g2_font_profont11_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
  u8g2.setPowerSave(0);
}

void U8g2Display::clearBuffer() {
    u8g2.clearBuffer();
}

void U8g2Display::setPowerSave(uint32_t is_enabled) {
    if (is_enabled) {
        u8g2.setPowerSave(1);  // Enable power save
    } else {
        u8g2.setPowerSave(0);  // Disable power save
    }
}

void U8g2Display::clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    u8g2.setDrawColor(0);
    u8g2.drawBox(x, y, w, h);
    //u8g2.sendBuffer();
    u8g2.setDrawColor(1);
    //u8g2.sendBuffer();
}

void U8g2Display::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap) {
    u8g2.drawXBMP(x, y, w, h, bitmap);
    //u8g2.sendBuffer();
}

void U8g2Display::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap) {
    //u8g2.drawXBMP(x, y, w, h, bitmap); // TODO: show "not supported" image, like a "x"
}

void U8g2Display::drawImage(uint16_t x, uint16_t y, tImage bitmap) {
    drawImage(x, y, bitmap.width, bitmap.height, bitmap.data);
}

void U8g2Display::setFont(FontType fontType) {
    if (fontType == FontType::Small) {
        u8g2.setFont(u8g2_font_profont10_tf);
    } else if (fontType == FontType::Normal) {
        u8g2.setFont(u8g2_font_profont11_tf);
    } else if (fontType == FontType::Big) {
        u8g2.setFont(u8g2_font_profont22_tf);
    } else if (fontType == FontType::OpenIconicArrow) {
        u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t); // 8x8
    } else if (fontType == FontType::OpenIconicArrow2x) {
        u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t); // 8x8
    } else if (fontType == FontType::OpenIconicEmbedded) {
        u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
    } else if (fontType == FontType::OpenIconicThing) {
        u8g2.setFont(u8g2_font_open_iconic_thing_1x_t);
    } else if (fontType == FontType::OpenIconicOther) {
        u8g2.setFont(u8g2_font_open_iconic_other_1x_t);
    } else if (fontType == FontType::H12Pixel) {
        u8g2.setFont(u8g2_font_profont12_tf);
    } else if (fontType == FontType::H15Pixel) {
        u8g2.setFont(u8g2_font_profont15_tf);
    } else if (fontType == FontType::fup17) {
        u8g2.setFont(u8g2_font_fub17_tf);
    } else if (fontType == FontType::fup25) {
        u8g2.setFont(u8g2_font_fub25_tf);
    } else if (fontType == FontType::fup35) {
        u8g2.setFont(u8g2_font_fub35_tf);
    }
}

void U8g2Display::setCursor(int16_t x, int16_t y) {
    u8g2.setCursor(x, y);
}

void U8g2Display::print(float data, int digits) {
    u8g2.print(data, digits);
}

void U8g2Display::print(char c) {
    u8g2.print(c);
}

void U8g2Display::print(const char* c) {
    u8g2.print(c);
}

void U8g2Display::println(const String &s) {
    u8g2.println(s);
}

void U8g2Display::drawGlyph(uint8_t x, uint8_t y, uint8_t encoding) {
    u8g2.drawGlyph(x, y, encoding);
}

void U8g2Display::sendBuffer() {
    u8g2.sendBuffer();
}

int U8g2Display::getWidth() {
    return u8g2.getDisplayWidth();
}

int U8g2Display::getHeight() {
    return u8g2.getDisplayHeight();
}

void U8g2Display::printCentered(const char* c, uint16_t y) {
    int textWidth = u8g2.getStrWidth(c);
    int centerX = (u8g2.getWidth() - textWidth) / 2;
    u8g2.setCursor(centerX, y);
    u8g2.print(c);
    //u8g2.sendBuffer();
}

void U8g2Display::printCentered(const char* line1, const char* line2, uint16_t y) {
    int leading = 0; // set to 0, no idea anymore why I introduced this (maybe because of size of non gaggia logos)
    
    int textWidth = u8g2.getStrWidth(line1);
    int centerX = (u8g2.getWidth() - textWidth) / 2;
    u8g2.setCursor(centerX, y);
    u8g2.print(line1);

    textWidth = u8g2.getStrWidth(line2);
    centerX = (u8g2.getWidth() - textWidth) / 2;
    int yPos = y + u8g2.getMaxCharHeight() + leading;
    u8g2.setCursor(centerX, yPos);
    u8g2.print(line2);

    //u8g2.sendBuffer();
}

void U8g2Display::printRightAligned(const char* c, uint16_t y) {
    int posX = u8g2.getWidth() - u8g2.getStrWidth(c);
    u8g2.setCursor(posX, y);
    u8g2.print(c);
    //u8g2.sendBuffer();
}

int U8g2Display::getStringWidth(int lenght, FontType font) {
    setFont(font);
    return getStringWidth(lenght);
}

int U8g2Display::getStringWidth(int lenght) {
    int charWidth = u8g2.getUTF8Width("0");
    return charWidth * lenght;
}

void U8g2Display::printRightAligned(float data, unsigned int digits, uint16_t y) {
    int dataDigits = getDataDigits(data);
    int numWidth = getStringWidth(dataDigits + 1 + digits); // Include decimal point

    int posX = u8g2.getWidth() - numWidth;
    u8g2.setCursor(posX, y);
    u8g2.print(data, digits);
    //u8g2.sendBuffer();
}

void U8g2Display::printTemperatures(float input, float setPoint, bool steaming) {
    int symbolWidth = 11;
    int yDeltaSymbol = 6; // for placing the symbol on correct height
    int lineHeight = 16;
    int xDelta = 12; // fixes problem with calculation correct x coordiante

    Viewport view = getView(Area::Temperature);

    int dataDigits = getDataDigits(input);
    int tempWidth = getStringWidth(dataDigits + 2, FontType::Big); // Include decimal point and decimal place
    int temp2Width = getStringWidth(2, FontType::Small); // degree symbol and C

    int x = view.getUpperRight().X - tempWidth - temp2Width - xDelta;
    int y = view.getUpperLeft().Y;
    setFont(FontType::Big);
    u8g2.setCursor(x, y);
    u8g2.print(input, 1);
    setFont(FontType::Small);
    u8g2.print((char)176);
    u8g2.println("C");
    setFont(FontType::OpenIconicEmbedded);
    u8g2.drawGlyph(x - symbolWidth, y + yDeltaSymbol, 0x0046); // 11 width of symbol + margin?

    // if (Input <= *activeSetPoint + 5 || activeState == State::SteamMode) { //only show setpoint if we are not steaming
    if (!steaming) {
        setFont(FontType::Big);
        u8g2.setCursor(x, y + lineHeight);
        u8g2.print(setPoint, 1);
        setFont(FontType::Small);
        u8g2.print((char)176);
        u8g2.println("C");
        setFont(FontType::OpenIconicOther);
        u8g2.drawGlyph(x - symbolWidth, y + lineHeight + yDeltaSymbol, 0x047);  // small circle in circle, target symbol
  }
}

void U8g2Display::printBrewingInfo(unsigned long totalBrewTime, unsigned long brewTimer, unsigned int activeBrewTimeEndDetection, bool scaleEnabled, int currentWeight, float activeScaleSensorWeightSetPoint) {
      unsigned int align_right_left_value = getWidth() - 56 - 5;
      unsigned int align_right_right_value = getWidth() - 56 + 28;
      setFont(FontType::Big);
      setCursor(align_right_left_value, 3);
      if (brewTimer < 10000) print("0");
      // TODO: Use print(u8x8_u8toa(value, digits)) or print(u8x8_u16toa(value, digits)) to print numbers with constant width (numbers are prefixed with 0 if required).
      print(brewTimer / 1000);

      setFont(FontType::OpenIconicArrow);
      drawGlyph(align_right_right_value - 8, 3 + 6, 0x04e);
      setFont(FontType::Big);
      setCursor(align_right_right_value, 3);
      print(totalBrewTime / 1000);

      setFont(FontType::Small);
      println("s");

      if (scaleEnabled) {
        setFont(FontType::Big);
        setCursor(align_right_left_value, 20);
        int weight = (int) currentWeight;
        //if (weight <0) weight = 0;
        if (weight < 10) print("0");
        print(weight<0?0:weight, 0);

        setFont(FontType::OpenIconicArrow);
        drawGlyph(align_right_right_value - 8, 20 + 6, 0x04e);
        setFont(FontType::Big);
        setCursor(align_right_right_value, 20);
        print(activeScaleSensorWeightSetPoint, 0);

        setFont(FontType::Small);
        println("g");
      }

      setFont(FontType::OpenIconicThing);
      if (activeBrewTimeEndDetection == 0) {
        drawGlyph(align_right_left_value - 11, 3 + 6, 0x04f);
      } else {
        drawGlyph(align_right_left_value - 11, 20 + 6, 0x04f);
      }
}


void U8g2Display::printCountdown(unsigned int powerOffTimer) {
    // TODO: use own viewport for countdown??
    const unsigned int align_right_countdown_min = u8g2.getWidth() - 52;
    const unsigned int align_right_countdown_sec = u8g2.getWidth() - 52 + 20;
    static char line[30];

    setFont(FontType::OpenIconicEmbedded);
    u8g2.drawGlyph(align_right_countdown_min - 15, 37 + 6, 0x004e); // symbol for power button
    setFont(FontType::Big);
    u8g2.setCursor(align_right_countdown_min, 37);
    snprintf(line, sizeof(line), "%d", int(powerOffTimer / 60));
    u8g2.print(line);
    setFont(FontType::Small);
    u8g2.println("m");
    setFont(FontType::Big);
    u8g2.setCursor(align_right_countdown_sec, 37);
    snprintf(line, sizeof(line), "%02d", int(powerOffTimer % 60));
    u8g2.print(line);
    u8g2.setCursor(align_right_countdown_sec + 23, 37);
    setFont(FontType::Small);
    u8g2.println(" s");
}

void U8g2Display::fillView(Area area, uint32_t color) {
    Viewport view = getView(area);

    if (color != 0) {
        color = 1;
    }
    u8g2.setDrawColor(color);
    u8g2.drawBox(view.getUpperLeft().X, view.getUpperLeft().Y, view.getWidth(), view.getHeight());
    //u8g2.sendBuffer();
}

void U8g2Display::drawBorder(Area area) {
    Viewport view = getView(area);
    u8g2.drawBox(view.getUpperLeft().X, view.getUpperLeft().Y, view.getWidth(), view.getHeight());
    //u8g2.sendBuffer();
}

void U8g2Display::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    u8g2.drawLine(x1, y1, x2, y2);
}

void U8g2Display::drawVLine(uint16_t x, uint16_t y, uint16_t h) {
    u8g2.drawVLine(x, y, h);
}

void U8g2Display::drawHLine(uint16_t x, uint16_t y, uint16_t w) {
    u8g2.drawHLine(x, y, w);
}

void U8g2Display::drawCircle(uint16_t x, uint16_t y, uint16_t rad) {
    u8g2.drawCircle(x, y, rad);
}

void U8g2Display::drawDisc(uint16_t x, uint16_t y, uint16_t rad) {
    u8g2.drawDisc(x, y, rad);
}

void U8g2Display::drawPixel(uint16_t x, uint16_t y) {
    u8g2.drawPixel(x, y);
}

void U8g2Display::drawFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    u8g2.drawFrame(x, y, w, h);
}

 