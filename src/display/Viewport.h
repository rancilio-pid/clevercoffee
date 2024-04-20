#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "Point.h"
#include <Arduino.h>

class Viewport {
public:
    Viewport();
    Viewport(Point startingPoint, int width, int height);
    Viewport(int x, int y, int width, int height);
    int getHeight();
    int getWidth();

    Point getUpperLeft();
    Point getUpperRight();

    Point getLowerLeft();
    Point getLowerRight();

    String getDebugString();

private:
    Point startPoint;
    int width;
    int height;
};

#endif