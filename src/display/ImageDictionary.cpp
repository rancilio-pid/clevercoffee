// ImageDictionary.cpp

#include "ImageDictionary.h"
#include <Arduino.h>

ImageDictionary::ImageDictionary() {

const tImage logo = { logo_bits, logo_width, logo_width, IMAGE_DATA_SIZE };
//const tImage generic_logo = { generic_logo_bits, logo_width, logo_width, IMAGE_DATA_SIZE };
    logoDictionary = {
           {StatusImage::MachineLogo, logo},
       //    {StatusImage::GenericLogo, generic_logo}
    };

    imageDictionary = {
        {StatusImage::Coldstart, coldstart_bits},
        {StatusImage::Brewing, brewing_bits},
        {StatusImage::BrewReady, brew_ready_bits},
        {StatusImage::BrewAcceptable, brew_acceptable_bits},
        {StatusImage::Steam, steam_bits},
        {StatusImage::OuterZone, outer_zone_bits},
        {StatusImage::Clean, clean_bits},
        {StatusImage::Menu, menu_bits},
        {StatusImage::SoftwareUpdate, update_bits}
    };

    imageRotatedDictionary = {
        {StatusImage::Coldstart, coldstart_rotate_bits},
        {StatusImage::Brewing, brewing_rotate_bits},
        {StatusImage::BrewReady, brew_ready_rotate_bits},
        {StatusImage::BrewAcceptable, brew_acceptable_rotate_bits},
        {StatusImage::Steam, steam_rotate_bits},
        {StatusImage::OuterZone, outer_zone_rotate_bits},
        {StatusImage::Clean, clean_rotate_bits},
        {StatusImage::Menu, menu_rotate_bits}
    };

const tImage profile_1 = { profile_1_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
const tImage profile_2 = { profile_2_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
const tImage profile_3 = { profile_3_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };

    profileIconDictionary = {
        {ProfileIcon::Profile_1, profile_1},
        {ProfileIcon::Profile_2, profile_2},
        {ProfileIcon::Profile_3, profile_3}
    };

const tImage wifi_not_ok = { wifi_not_ok_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
const tImage blynk_not_ok = { blynk_not_ok_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
const tImage mqtt_not_ok = { mqtt_not_ok_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
const tImage wifi_ok = { wifi_ok_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
const tImage blynk_ok = { blynk_ok_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };
const tImage mqtt_ok = { mqtt_ok_bits, status_icon_width, status_icon_height, IMAGE_DATA_SIZE };

    statusIconDictionary = {
        {StatusIcon::Wifi_Not_Ok, wifi_not_ok},
        {StatusIcon::Blynk_Not_Ok, blynk_not_ok},
        {StatusIcon::Mqtt_Not_Ok, mqtt_not_ok},
        {StatusIcon::Wifi_Ok, wifi_ok},
        {StatusIcon::Blynk_Ok, blynk_ok},
        {StatusIcon::Mqtt_Ok, mqtt_ok}
    };   
}

const tImage ImageDictionary::getLogo(StatusImage image) {
    return logoDictionary[image];
}

const unsigned char* ImageDictionary::getImage(StatusImage image) {
    return imageDictionary[image];
}

const unsigned char* ImageDictionary::getImageRotated(StatusImage image){
    return imageRotatedDictionary[image];
}

const tImage ImageDictionary::getStatusIcon(StatusIcon icon) {
    return statusIconDictionary[icon];
}

const tImage ImageDictionary::getProfileIcon(ProfileIcon icon) {
    return profileIconDictionary[icon];
}
