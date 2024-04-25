#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "Point.h"
#include <Arduino.h>

class Viewport {
public:
    Viewport();
    Viewport(Point startingPoint, int width, int height);
    Viewport(u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height);
    int getHeight();
    int getWidth();

    Point getUpperLeft();
    Point getUpperRight();

    Point getLowerLeft();
    Point getLowerRight();

    String getDebugString();

private:
    Point startPoint;
    u_int16_t width;
    u_int16_t height;
};

#endif