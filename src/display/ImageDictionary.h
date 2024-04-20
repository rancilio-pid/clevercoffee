#pragma once

#ifndef IMAGE_DICTIONARY_h
#define IMAGE_DICTIONARY_h

#include <map>
#include "status_images.h"  // Include the enum
#include "../userConfig.h"
#include <pgmspace.h>
#include "tImage.h"

#if DISPLAY_HARDWARE == 4

//using ImageDataType = uint16_t;

#ifdef MACHINE_TYPE_GAGGIA
#include "icons/icon_gaggia_color.h"
#elif defined(MACHINE_TYPE_ECM)
#include "icons/icon_ecm_color.h"
#elif defined(MACHINE_TYPE_RANCILIO)
#include "icons/icon_rancilio_color.h"
#else
#include "icons/icon_generic_color_.h"
#endif
#include "icons/icon_shared_color.h"
#include "icons/icon_color.h"

#else // DISPLAY_HARDWARE

//using ImageDataType = const unsigned char;
#ifdef MACHINE_TYPE_GAGGIA
#include "icons/icon_gaggia.h"
#elif defined(MACHINE_TYPE_ECM)
#include "icons/icon_ecm.h"
#elif defined(MACHINE_TYPE_RANCILIO)
#include "icons/icon_rancilio.h"
#else
#include "icons/icon_generic.h"
#endif

#if (ENABLE_BIG_STATUS_ICONS)
#include "icons/icon_big.h"
#else
#include "icons/icon.h"
#endif

#include "icons/icon_shared.h"
#endif // DISPLAY_HARDWARE

#if (ICON_COLLECTION == 2)
#include "icons/icon_winter.h"
#elif (ICON_COLLECTION == 1)
#include "icons/icon_smiley.h"
#else
#include "icons/icon_simple.h" // also used as placeholder for ICON_COLLECTION==3
#endif

class ImageDictionary {
public:
     ImageDictionary();

     const tImage getLogo(StatusImage);
     const unsigned char* getImage(StatusImage);
     const unsigned char* getImageRotated(StatusImage);
     const tImage getStatusIcon(StatusIcon);
     const tImage getProfileIcon(ProfileIcon);

// add some pointers to make it compile without "ok status icons"
#if DISPLAY_HARDWARE < 4 
     const ImageDataType* wifi_ok_bits = wifi_not_ok_bits;
     const ImageDataType* blynk_ok_bits = blynk_not_ok_bits;
     const ImageDataType* mqtt_ok_bits = mqtt_not_ok_bits;
#endif

private:
     std::map<StatusImage, tImage> logoDictionary;
     std::map<StatusImage, const unsigned char*> imageDictionary;
     std::map<StatusImage, const unsigned char*> imageRotatedDictionary;
     std::map<StatusIcon, tImage> statusIconDictionary;
     std::map<ProfileIcon, tImage> profileIconDictionary;
};

#endif // IMAGE_DICTIONARY_h