#include "DisplayBase.h"
#include "float.h"

Viewport DisplayBase::getView(Area area) {
    return areaMap[area];
}

void DisplayBase::clearView(Area area) {
    Viewport view = getView(area);
    clearRect(view.getUpperLeft().X, view.getUpperLeft().Y, view.getWidth(), view.getHeight());
}

void DisplayBase::drawImageCentered(Area area, int width, int height, const uint8_t *bitmap) {
    Viewport view = getView(area);
    int xDelta = (view.getWidth() - width) / 2;
    drawImage(view.getUpperLeft().X + xDelta, view.getUpperLeft().Y, width, height, bitmap);
}

void DisplayBase::drawImageCentered(Area area, int width, int height, const uint16_t *bitmap) {
    Viewport view = getView(area);
    int xDelta = (view.getWidth() - width) / 2;
    drawImage(view.getUpperLeft().X + xDelta, view.getUpperLeft().Y, width, height, bitmap);
}

void DisplayBase::printCentered(Area area, const char* line1) {
    Viewport view = getView(area);
    printCentered(line1, view.getUpperLeft().Y);
}

void DisplayBase::printCentered(Area area, const char* line1, const char* line2) {
    Viewport view = getView(area);
    printCentered(line1, line2, view.getUpperLeft().Y);
}

int DisplayBase::getDataDigits(float data) {
    int dataDigits = 0;
    if (data - 100 > -FLT_EPSILON) {
        dataDigits = 3;
    } else {
        dataDigits = 2;
    }
    return dataDigits;
}
