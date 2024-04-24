#pragma once

#ifndef IMAGE_DICTIONARY_h
#define IMAGE_DICTIONARY_h

#include <map>
#include "status_images.h"  // Include the enum
#include "../userConfig.h"
#include <pgmspace.h>
#include "tImage.h"

#if DISPLAY_HARDWARE == 4

#ifdef MACHINE_TYPE_GAGGIA
#include "icons/icon_gaggia_color.h"
#elif defined(MACHINE_TYPE_ECM)
#include "icons/icon_ecm_color.h"
#elif defined(MACHINE_TYPE_RANCILIO)
#include "icons/icon_rancilio_color.h"
#else
#include "icons/icon_generic_color_.h"
#endif
#include "bitmaps_color.h"

#else // DISPLAY_HARDWARE != 4
#include "bitmaps.h"
#endif // DISPLAY_HARDWARE

class ImageDictionary {
public:
     ImageDictionary();

     const tImage getLogo(StatusImage);
     // const unsigned char* getImage(StatusImage);
     // const unsigned char* getImageRotated(StatusImage);
     const tImage getStatusIcon(StatusIcon);
     // const tImage getProfileIcon(ProfileIcon);

private:
     std::map<StatusImage, tImage> logoDictionary;
     // std::map<StatusImage, const unsigned char*> imageDictionary;
     // std::map<StatusImage, const unsigned char*> imageRotatedDictionary;
     std::map<StatusIcon, tImage> statusIconDictionary;
     // std::map<ProfileIcon, tImage> profileIconDictionary;
};

#endif // IMAGE_DICTIONARY_h