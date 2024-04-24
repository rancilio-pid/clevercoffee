// ImageDictionary.cpp

#include "ImageDictionary.h"
#include <Arduino.h>

ImageDictionary::ImageDictionary() {

    const tImage logo = { CleverCoffee_Logo, CleverCoffee_Logo_width, CleverCoffee_Logo_height, IMAGE_DATA_SIZE };
    logoDictionary = {
           {StatusImage::MachineLogo, logo},
    };

    // imageDictionary = {
    //     {StatusImage::Coldstart, coldstart_bits},
    //     {StatusImage::Brewing, brewing_bits},
    //     {StatusImage::BrewReady, brew_ready_bits},
    //     {StatusImage::BrewAcceptable, brew_acceptable_bits},
    //     {StatusImage::Steam, steam_bits},
    //     {StatusImage::OuterZone, outer_zone_bits},
    //     {StatusImage::Clean, clean_bits},
    //     {StatusImage::Menu, menu_bits},
    //     {StatusImage::SoftwareUpdate, update_bits}
    // };

    // imageRotatedDictionary = {
    //     {StatusImage::Coldstart, coldstart_rotate_bits},
    //     {StatusImage::Brewing, brewing_rotate_bits},
    //     {StatusImage::BrewReady, brew_ready_rotate_bits},
    //     {StatusImage::BrewAcceptable, brew_acceptable_rotate_bits},
    //     {StatusImage::Steam, steam_rotate_bits},
    //     {StatusImage::OuterZone, outer_zone_rotate_bits},
    //     {StatusImage::Clean, clean_rotate_bits},
    //     {StatusImage::Menu, menu_rotate_bits}
    // };

    const tImage wifi_not_ok = { Antenna_NOK_Icon, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
    const tImage mqtt_not_ok = { mqtt_nok_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
    const tImage wifi_ok = { Antenna_OK_Icon, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
    const tImage mqtt_ok = { mqtt_ok_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
    const tImage water_empty = { Water_Empty_Icon, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };

    statusIconDictionary = {
        {StatusIcon::Wifi_Not_Ok, wifi_not_ok},
        {StatusIcon::Mqtt_Not_Ok, mqtt_not_ok},
        {StatusIcon::Wifi_Ok, wifi_ok},
        {StatusIcon::Mqtt_Ok, mqtt_ok},
        {StatusIcon::Water_Empty, water_empty}
    };
}

const tImage ImageDictionary::getLogo(StatusImage image) {
    return logoDictionary[image];
}

// const unsigned char* ImageDictionary::getImage(StatusImage image) {
//     return imageDictionary[image];
// }

// const unsigned char* ImageDictionary::getImageRotated(StatusImage image){
//     return imageRotatedDictionary[image];
// }

const tImage ImageDictionary::getStatusIcon(StatusIcon icon) {
    return statusIconDictionary[icon];
}

// const tImage ImageDictionary::getProfileIcon(ProfileIcon icon) {
//     return profileIconDictionary[icon];
// }
