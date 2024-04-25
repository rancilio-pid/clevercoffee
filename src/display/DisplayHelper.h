#ifndef DISPLAY_HELPER_H
#define DISPLAY_HELPER_H

#include "status_images.h"
#include "ImageDictionary.h"
#include <map>
#include <utility>
#include "Arduino.h"

class DisplayHelper {
public:
    DisplayHelper();

    String StatusIconToString(StatusIcon icon);

    ProfileIcon getCurrentProfileIcon();
    void setCurrentProfileIcon(ProfileIcon icon);

    StatusImage getCurrentActionImage(bool rotated);
    void setCurrentActionImage(StatusImage image, bool rotated);

    StatusIcon getStatusIconAtIndex(uint8_t index);
    void setStatusIconAtIndex(uint8_t index, StatusIcon icon);

private:
     std::map<uint8_t, StatusIcon> currentIconMap;
     ProfileIcon currentProfileIcon;
     std::pair<bool, StatusImage> currentActionImage;
};

#endif