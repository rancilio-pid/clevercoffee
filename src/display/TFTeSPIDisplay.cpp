#include "../userConfig.h"

#if (DISPLAY_HARDWARE == 4)

#include "TFTeSPIDisplay.h"
#include <float.h>

TFTeSPIDisplay::TFTeSPIDisplay(TFT_eSPI& tftInstance) : tft(tftInstance) {
}

void TFTeSPIDisplay::init(Rotation rotation) {
    tft.init();
    tft.fillScreen(TFT_BLACK);

    if (rotation == Rotation::R90)
        tft.setRotation(1);
    else if (rotation == Rotation::R180)
        tft.setRotation(2);
    else if (rotation == Rotation::R270)
        tft.setRotation(3);
    else 
        tft.setRotation(0);

    initViews();
}

void TFTeSPIDisplay::initViews() {
    int width = getWidth();
    int height = getHeight();
    int margin = 5;
    int maxBootLogoHeight = 131;

    int statusIconHeight = 56;
    int statusIconWidth = 56;

    int iconHeight  = 45;

    int numberOfStatusIcons = 3;

    Point upperLeft = Point(0, 0);

    bootLogo = Viewport(upperLeft, width, maxBootLogoHeight);
    bootMessage = Viewport(0, bootLogo.getLowerLeft().Y + margin, width, height - bootLogo.getHeight() - margin);
    
    //actionImage = Viewport(upperLeft, width / 2, (height / 2));
    actionImage = Viewport(upperLeft, width / 2, (height / 4));
    temperature = Viewport(width / 2, upperLeft.Y, width / 2, (height / 2));

    statusMessage = Viewport(0, actionImage.getHeight()*2 + margin, getWidth(), getHeight() - actionImage.getHeight()*2 - margin - statusIconHeight);
    statusIcons = Viewport(Point(0, height - statusIconHeight), statusIconWidth * numberOfStatusIcons, statusIconHeight);
    profileIcon = Viewport(Point(width - statusIconWidth, height - statusIconHeight), statusIconWidth, statusIconHeight);

    softwareUpdate = Viewport(upperLeft, getWidth(), getHeight()); // fullscreen

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

void TFTeSPIDisplay::clearBuffer() {
   tft.fillScreen(TFT_BLACK);
}

void TFTeSPIDisplay::sendBuffer() {
    // No need to send a buffer for this library
}

void TFTeSPIDisplay::setPowerSave(uint32_t is_enabled) {
    // this is used to turn of display in screensaver mode, TODO check is there is anything similar in this library
    //tft.enableSleep(is_enabled);
}

void TFTeSPIDisplay::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap) {
   tft.drawXBitmap(x, y, bitmap, w, h, TFT_WHITE);  // Draw bitmap
}

void TFTeSPIDisplay::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap) {
    tft.setSwapBytes(true); // swap the byte order for pushImage() - corrects endianness
    tft.pushImage(x,y,w,h,bitmap);
    tft.setSwapBytes(false);
}

void TFTeSPIDisplay::drawImage(uint16_t x, uint16_t y, tImage bitmap) {
    tft.setSwapBytes(true); // swap the byte order for pushImage() - corrects endianness
    tft.pushImage(x, y, bitmap.width, bitmap.height, bitmap.data);
    tft.setSwapBytes(false);
}

void TFTeSPIDisplay::setFont(FontType fontType) {
    if (fontType == FontType::Small) {
       tft.setFreeFont(&FreeSans9pt7b);
    } else if (fontType == FontType::Normal) {
       tft.setFreeFont(&FreeSans12pt7b);
    } else if (fontType == FontType::Big) {
       tft.setFreeFont(&FreeSans18pt7b);
    } else if (fontType == FontType::OpenIconicArrow) {
         tft.setFreeFont(&FreeMono12pt7b);
    } else if (fontType == FontType::OpenIconicEmbedded) {
        tft.setFreeFont(&FreeMono12pt7b);
    } else if (fontType == FontType::OpenIconicThing) {
        tft.setFreeFont(&FreeMono12pt7b);
    } else if (fontType == FontType::OpenIconicOther) {
        tft.setFreeFont(&FreeMono12pt7b);
    } else if (fontType == FontType::H12Pixel) {
        tft.setFreeFont(&FreeSans12pt7b);
    } else if (fontType == FontType::H15Pixel) {
        tft.setFreeFont(&FreeSans18pt7b);
    } else if (fontType == FontType::fup17) {
        tft.setFreeFont(&FreeSans18pt7b);
    } else if (fontType == FontType::fup25) {
        tft.setFreeFont(&FreeSans24pt7b);
    }    
}

void TFTeSPIDisplay::setCursor(int16_t x, int16_t y) {
    tft.setCursor(x, y);  // Set the cursor position
}

void TFTeSPIDisplay::print(float data, int digits) {
    tft.drawFloat(data, digits, tft.getCursorX(), tft.getCursorY());  // Print float data
}

void TFTeSPIDisplay::print(char c) {
    tft.drawChar(c, tft.getCursorX(), tft.getCursorY());  // Print a character
}

void TFTeSPIDisplay::print(const char* c) {
    tft.drawString(c, tft.getCursorX(), tft.getCursorY());  // Print a string
}

void TFTeSPIDisplay::println(const String &s) {
    tft.drawString(s, tft.getCursorX(), tft.getCursorY());  // Print a string followed by a newline
}

void TFTeSPIDisplay::drawGlyph(uint8_t x, uint8_t y, uint8_t encoding) {
    // Implement drawing a glyph using tft, if applicable
}

void TFTeSPIDisplay::clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    tft.fillRect(x, y, w, h, TFT_BLACK);
}

int TFTeSPIDisplay::getWidth() {
    return tft.width();
}

int TFTeSPIDisplay::getHeight() {
    return tft.height();
}

void TFTeSPIDisplay::printCentered(const char* c, uint16_t y) {
    int posX = tft.width() / 2; // Calculate the center X position
    tft.setTextDatum(TC_DATUM);
    tft.drawString(c, posX, y);  // Print a string
}

void TFTeSPIDisplay::printCentered(const char* line1, const char* line2, uint16_t y) {
    int posX = tft.width() / 2;
    tft.setTextDatum(TC_DATUM);
    tft.drawString(line1, posX, y);
    tft.drawString(line2, posX, y + tft.fontHeight());
}

void TFTeSPIDisplay::printRightAligned(const char* c, uint16_t y) {
    int posX = tft.width() - tft.textWidth(c);
    tft.drawString(c, posX, y);
}

void TFTeSPIDisplay::printRightAligned(float data, unsigned int digits, uint16_t y) {
    int dataDigits = getDataDigits(data);
    int charWidth = tft.textWidth("0"); // Width of a single character
    int numWidth = charWidth * (dataDigits + 1 + digits); // Include decimal point
    int posX = tft.width() - numWidth;
    tft.drawFloat(data, digits, posX, y);
}


void TFTeSPIDisplay::fillView(Area area, uint32_t color) {
    Viewport view = getView(area);
    tft.fillRect(view.getUpperLeft().X, view.getUpperLeft().Y, view.getWidth(), view.getHeight(), color);
}

void TFTeSPIDisplay::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    tft.drawLine(x1, y1, x2, y2, TFT_WHITE);
}

void TFTeSPIDisplay::drawVLine(uint16_t x, uint16_t y, uint16_t h) {
    tft.drawFastVLine(x, y, h, TFT_WHITE);
}

void TFTeSPIDisplay::drawHLine(uint16_t x, uint16_t y, uint16_t w) {
    tft.drawFastHLine(x, y, w, TFT_WHITE);
}

void TFTeSPIDisplay::drawCircle(uint16_t x, uint16_t y, uint16_t rad) {
    tft.drawCircle(x, y, rad, TFT_WHITE);
}

void TFTeSPIDisplay::drawDisc(uint16_t x, uint16_t y, uint16_t rad) {
    tft.drawCircle(x, y, rad, TFT_WHITE);
}

void TFTeSPIDisplay::drawPixel(uint16_t x, uint16_t y) {
    tft.drawPixel(x, y, TFT_WHITE);
}

void TFTeSPIDisplay::drawFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    //TODO tft->drawFrame(x, y, w, h);
    // draw h times a border each time 1px smaller (or bigger??)
}


void TFTeSPIDisplay::printTemperatures(float input, float setPoint, bool steaming) {
    int xpos = 120;
    int ypos = 10;
    int radius = 60;

    // Draw analogue meter
    ringMeter(input, 0, setPoint, xpos, ypos, radius, "`C", RED2RED);
}

void TFTeSPIDisplay::printBrewingInfo(unsigned long totalBrewTime, unsigned long brewTimer, unsigned int activeBrewTimeEndDetection, bool scaleEnabled, int currentWeight, float activeScaleSensorWeightSetPoint) {

    Viewport actionImageView = getView(Area::ActionImage);
    // for brewinfo we can use the actionImage viewport as reference, because currently we still use the old images (45x45) and have a lot of space 
    Viewport brewInfo = Viewport(actionImageView.getLowerLeft().X, actionImageView.getLowerLeft().Y + 5, actionImageView.getWidth(), actionImageView.getHeight());

    unsigned int align_right_left_value = brewInfo.getWidth() - 56 - 5;
    unsigned int align_right_right_value = brewInfo.getWidth() - 56 + 28;
    setFont(FontType::Big);
    setCursor(align_right_left_value, brewInfo.getUpperRight().Y + 3);
    if (brewTimer < 10000) {
        print("0");
    }
    print(brewTimer / 1000);

    setFont(FontType::OpenIconicArrow);
    drawGlyph(align_right_right_value - 8, brewInfo.getUpperRight().Y + 3 + 6, 0x04e);
    setFont(FontType::Big);
    setCursor(align_right_right_value, brewInfo.getUpperRight().Y + 3);
    print(totalBrewTime / 1000);

    setFont(FontType::Small);
    println("s");

    if (scaleEnabled) {
        setFont(FontType::Big);
        setCursor(align_right_left_value, brewInfo.getUpperRight().Y + 20);
        int weight = (int) currentWeight;
        //if (weight <0) weight = 0;
        if (weight < 10) {
            print("0");
        }
        print(weight<0?0:weight, 0);

        setFont(FontType::OpenIconicArrow);
        drawGlyph(align_right_right_value - 8, brewInfo.getUpperRight().Y + 20 + 6, 0x04e);
        setFont(FontType::Big);
        setCursor(align_right_right_value, brewInfo.getUpperRight().Y + 20);
        print(activeScaleSensorWeightSetPoint, 0);

        setFont(FontType::Small);
        println("g");
    }

    setFont(FontType::OpenIconicThing);
    if (activeBrewTimeEndDetection == 0) {
        drawGlyph(align_right_left_value - 11, brewInfo.getUpperRight().Y + 3 + 6, 0x04f);
    } else {
        drawGlyph(align_right_left_value - 11, brewInfo.getUpperRight().Y + 20 + 6, 0x04f);
    }
}

void TFTeSPIDisplay::printCountdown(unsigned int powerOffTimer) {
    // TODO: use own viewport for countdown??
    // TODO: fix coordinates for this display
    const unsigned int align_right_countdown_min = getWidth() - 52;
    const unsigned int align_right_countdown_sec = getWidth() - 52 + 20;
    static char line[30];

    //setFont(FontType::OpenIconicEmbedded);
    //tft.drawGlyph(align_right_countdown_min - 15, 37 + 6, 0x004e); // symbol for power button
    setFont(FontType::Big);
    tft.setCursor(align_right_countdown_min, 37);
    snprintf(line, sizeof(line), "%d", int(powerOffTimer / 60));
    tft.print(line);
    setFont(FontType::Small);
    tft.println("m");
    setFont(FontType::Big);
    tft.setCursor(align_right_countdown_sec, 37);
    snprintf(line, sizeof(line), "%02d", int(powerOffTimer % 60));
    tft.print(line);
    tft.setCursor(align_right_countdown_sec + 23, 37);
    setFont(FontType::Small);
    tft.println(" s");
}

// #########################################################################
//  Draw the meter on the screen, returns x coord of righthand side
// #########################################################################
int TFTeSPIDisplay::ringMeter(float value, int vmin, int vmax, int x, int y, int r, const char *units, byte scheme)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  
  x += r; y += r;   // Calculate coords of centre of ring
  int w = r / 3;    // Width of outer ring is 1/4 of radius
  int angle = 150;  // Half the sweep angle of meter (300 degrees)
  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 6; // Segments are 3 degrees wide = 100 segments for 300 degrees
  byte inc = 12; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Variable to save "value" text colour from scheme and set default
  int colour = TFT_BLUE;
 
  // Draw colour blocks every inc degrees
  for (int i = -angle+inc/2; i < angle-inc/2; i += inc) {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      switch (scheme) {
        case 0: colour = TFT_RED; break; // Fixed colour
        case 1: colour = TFT_GREEN; break; // Fixed colour
        case 2: colour = TFT_BLUE; break; // Fixed colour
        case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
        case 4: colour = rainbow(map(i, -angle, angle, 70, 127)); break; // Green to red (high temperature etc)
        case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
        case 6: colour = rainbow(map(i, -angle, angle, 0, 63)); break; // Blue to green (water temperature etc)        
        default: colour = TFT_BLUE; break; // Fixed colour
      }
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      //text_colour = colour; // Save the last colour drawn
    } else { // Fill in blank segments
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREY);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREY);
    }
  }
  // Convert value to a string
  char buf[10];
  //byte len = 3; if (value > 999) len = 5;
  byte len = 5;
  dtostrf(value, len, 1, buf);
  buf[len] = ' '; buf[len+1] = 0; // Add blanking space and terminator, helps to centre text too!
  // Set the text colour to default
  tft.setTextSize(1);

  // if (value<vmin || value>vmax) {
  //   drawAlert(x,y+90,50,1);
  // }
  // else {
  //   drawAlert(x,y+90,50,0);
  // }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Uncomment next line to set the text colour to the last segment value!
  //tft.setTextColor(colour, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  // Print value, if the meter is large then use big font 8, othewise use 4
  if (r > 84) {
    tft.setTextPadding(55*3); // Allow for 3 digits each 55 pixels wide
    tft.drawString(buf, x, y, 8); // Value in middle
  } else {
    tft.setTextPadding(3 * 14); // Allow for 3 digits each 14 pixels wide
    tft.drawString(buf, x, y, 4); // Value in middle
  }
  tft.setTextSize(1);
  tft.setTextPadding(0);
  // Print units, if the meter is large then use big font 4, othewise use 2
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //if (r <= 84) tft.drawString(units, x, y + 15, 2); else // Units display 
  tft.drawString(units, x, y + 50, 4); // Units display

  // Calculate and return right hand side x coordinate
  return x + r;
}

unsigned int TFTeSPIDisplay::rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}

void TFTeSPIDisplay::drawBorder(Area area) {
    Viewport view = getView(area);
    tft.drawRect(view.getUpperLeft().X, view.getUpperLeft().Y, view.getWidth(), view.getHeight(), TFT_WHITE);
}

#endif
