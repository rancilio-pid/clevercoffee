#include "DisplayHelper.h"

DisplayHelper::DisplayHelper() {
    currentProfileIcon = ProfileIcon::None;
    currentActionImage = std::make_pair(false, StatusImage::None);

    currentIconMap = {
        {0, StatusIcon::None},
        {1, StatusIcon::None},
        {2, StatusIcon::None}
    };
}

StatusIcon DisplayHelper::getStatusIconAtIndex(uint8_t index) {
    return currentIconMap[index];
}

void DisplayHelper::setStatusIconAtIndex(uint8_t index, StatusIcon icon) {
    currentIconMap[index] = icon;
}

ProfileIcon DisplayHelper::getCurrentProfileIcon() {
    return currentProfileIcon;
}

void DisplayHelper::setCurrentProfileIcon(ProfileIcon icon) {
    currentProfileIcon = icon;
}

StatusImage DisplayHelper::getCurrentActionImage(bool rotated) {
    if (currentActionImage.first == rotated) {
        return currentActionImage.second;
    }
    return StatusImage::None;
}

void DisplayHelper::setCurrentActionImage(StatusImage image, bool rotated) {
    currentActionImage.first = rotated;
    currentActionImage.second = image;
}

String DisplayHelper::StatusIconToString(StatusIcon icon) {
    switch (icon) {
        case StatusIcon::None: return "None";
        case StatusIcon::Wifi_Not_Ok: return "Wifi_Not_Ok";
        case StatusIcon::Mqtt_Not_Ok: return "Mqtt_Not_Ok";
        case StatusIcon::Wifi_Ok: return "Wifi_Ok";
        case StatusIcon::Mqtt_Ok: return "Mqtt_Ok";
        default: return "Invalid value";
    }
}

String DisplayHelper::StateToString(State state) {
  switch(state) {
    case State::Undefined: return "Undefined";
    case State::ColdStart: return "ColdStart";
    case State::StabilizeTemperature: return "StabilizeTemperature";
    case State::InnerZoneDetected: return "InnerZoneDetected";
    case State::BrewDetected: return "BrewDetected";
    case State::OuterZoneDetected: return "OuterZoneDetected";
    case State::SteamMode: return "SteamMode";
    case State::SleepMode: return "SleepMode";
    case State::CleanMode: return "CleanMode";
    default: return "Unknown";
  }
}
