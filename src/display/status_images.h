#pragma once

#ifndef status_images_h
#define status_images_h

enum class StatusImage {
    None = 0,
    Coldstart = 1,
    Brewing = 2,
    BrewReady = 3,
    BrewAcceptable = 4,
    Steam = 5,
    OuterZone = 6,
    Clean = 7,
    Menu = 8,
    SoftwareUpdate = 9,
    MachineLogo = 10,
    GenericLogo = 11,
    Test = 12
};

enum class StatusIcon {
    None = 0,
    Wifi_Not_Ok = 1,
    Mqtt_Not_Ok = 3,
    Wifi_Ok = 4,
    Mqtt_Ok = 6,
    Water_Empty = 7
};

enum class ProfileIcon {
    None = 0,
    Profile_1 = 1, // this needs to be one, so we can use the profile integer to access the correct profile
    Profile_2 = 2,
    Profile_3 = 3
};

#endif