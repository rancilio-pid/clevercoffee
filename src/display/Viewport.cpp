#include "Viewport.h"

Viewport::Viewport() {
    this->width = 0;
    this->height = 0;
    this->startPoint = Point(0,0);
}

Viewport::Viewport(Point startPoint, int width, int height) {
    this->width = width;
    this->height = height;
    this->startPoint = startPoint;
}

Viewport::Viewport(int x, int y, int width, int height) : Viewport(Point(x, y), width, height) {
}

int Viewport::getHeight() {
    return height;
}

int Viewport::getWidth() {
    return width;
}

Point Viewport::getUpperLeft() {
    return startPoint;
}

Point Viewport::getUpperRight() {
    return Point(startPoint.X + width, startPoint.Y);
}

Point Viewport::getLowerLeft() {
    return Point(startPoint.X, startPoint.Y + height);
}

Point Viewport::getLowerRight() {
    return Point(startPoint.X + width, startPoint.Y + height);
}


String Viewport::getDebugString() {
    String output = "X/Y: " + String(startPoint.X) + " / " + String(startPoint.Y) + ", w=" + String(width) + ", h=" + String(height);
    //snprintf((char*)displaymessagetextBuffer, sizeof(displaymessagetextBuffer), "%s", displaymessagetext);
    return output;
}
