#pragma once

#ifndef IMAGE_TYPEDEF
#define IMAGE_TYPEDEF

#include "../userConfig.h"

#if DISPLAY_HARDWARE == 4
using ImageDataType = uint16_t;
#define IMAGE_DATA_SIZE 16
#else
using ImageDataType = const unsigned char;
#define IMAGE_DATA_SIZE 8
#endif

 typedef struct {
     const ImageDataType *data;
     uint16_t width;
     uint16_t height;
     uint8_t dataSize;
     } tImage;

#endif