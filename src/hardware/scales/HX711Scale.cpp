/**
 * @file HX711Scale.cpp
 * @brief HX711-based scale implementation
 */

#include "HX711Scale.h"

HX711Scale::HX711Scale(const int dataPin, const int clkPin, const float calibrationFactor) :
    loadCell1(new HX711_ADC(dataPin, clkPin)), loadCell2(nullptr), currentWeight(0.0), calibrationFactor1(calibrationFactor), calibrationFactor2(1.0), isDualCell(false), readSecondScale(false), weight1(0.0), weight2(0.0) {
}

HX711Scale::HX711Scale(int dataPin1, int dataPin2, int clkPin, float calibrationFactor1, float calibrationFactor2) :
    loadCell1(new HX711_ADC(dataPin1, clkPin)),
    loadCell2(new HX711_ADC(dataPin2, clkPin)),
    currentWeight(0.0),
    calibrationFactor1(calibrationFactor1),
    calibrationFactor2(calibrationFactor2),
    isDualCell(true),
    readSecondScale(false),
    weight1(0.0),
    weight2(0.0) {
}

HX711Scale::~HX711Scale() {
    delete loadCell1;
    delete loadCell2;
}

bool HX711Scale::init() {
    loadCell1->begin();

    if (isDualCell) {
        loadCell2->begin();
    }

    constexpr unsigned long stabilizingTime = 5000;
    constexpr boolean _tare = true;

    if (!isDualCell) {
        while (!loadCell1->startMultiple(stabilizingTime, _tare)) {
            // Wait for initialization
        }
    }
    else {
        byte loadCell1Ready = 0;
        byte loadCell2Ready = 0;

        while (loadCell1Ready + loadCell2Ready < 2) {
            if (!loadCell1Ready) {
                loadCell1Ready = loadCell1->startMultiple(stabilizingTime, _tare);
            }

            if (!loadCell2Ready) {
                loadCell2Ready = loadCell2->startMultiple(stabilizingTime, _tare);
            }
        }
    }

    if (loadCell1->getTareTimeoutFlag() || loadCell1->getSignalTimeoutFlag()) {
        return false;
    }

    if (isDualCell && (loadCell2->getTareTimeoutFlag() || loadCell2->getSignalTimeoutFlag())) {
        return false;
    }

    loadCell1->setCalFactor(calibrationFactor1);

    if (isDualCell) {
        loadCell2->setCalFactor(calibrationFactor2);
    }

    return true;
}

bool HX711Scale::update() {
    if (!isDualCell) {
        if (loadCell1->update()) {
            weight1 = loadCell1->getData();
            currentWeight = weight1;
            return true;
        }
    }
    else {
        if (!readSecondScale) {
            if (loadCell1->update()) {
                weight1 = loadCell1->getData();
                readSecondScale = true;
            }
        }
        else {
            if (loadCell2->update()) {
                weight2 = loadCell2->getData();
                currentWeight = weight1 + weight2;
                readSecondScale = false;
                return true;
            }
        }
    }

    return false;
}

float HX711Scale::getWeight() const {
    return currentWeight;
}

void HX711Scale::tare() {
    loadCell1->tare();
    loadCell1->setCalFactor(calibrationFactor1);

    if (isDualCell) {
        loadCell2->tare();
        loadCell2->setCalFactor(calibrationFactor2);
    }
}

void HX711Scale::setSamples(const int samples) {
    if (loadCell1) {
        loadCell1->setSamplesInUse(samples);
    }
    if (loadCell2) {
        loadCell2->setSamplesInUse(samples);
    }
}

float HX711Scale::getCalibrationFactor(const int cellNumber) const {
    return cellNumber == 1 ? calibrationFactor1 : calibrationFactor2;
}

void HX711Scale::setCalibrationFactor(const float factor, const int cellNumber) {
    if (cellNumber == 1) {
        calibrationFactor1 = factor;

        if (loadCell1) {
            loadCell1->setCalFactor(factor);
        }
    }
    else if (cellNumber == 2 && isDualCell) {
        calibrationFactor2 = factor;

        if (loadCell2) {
            loadCell2->setCalFactor(factor);
        }
    }
}

HX711_ADC* HX711Scale::getLoadCell(const int cellNumber) const {
    if (cellNumber == 1) {
        return loadCell1;
    }

    if (cellNumber == 2 && isDualCell) {
        return loadCell2;
    }

    return nullptr;
}
