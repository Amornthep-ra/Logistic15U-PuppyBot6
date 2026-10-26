#pragma once
#include "PuppyBotRGBCalibration.h"
#include <EEPROM.h>
#include <stdio.h>

namespace PuppyBotRGBDetail {
    // Reserve bytes 3072..3399 of the core's 4096-byte emulated EEPROM (schema 2).
    // The complete sector is loaded, preserving bytes outside our record on commit.
    constexpr size_t calibrationOffset = 3072;
    template<class Storage> bool loadCalibration(RGBCalibration &data, Storage &store) {
        if (data.collecting()) return false;
        store.begin(4096);
        uint8_t bytes[RGBCalibration::encodedSize];
        for (size_t i = 0; i < sizeof(bytes); ++i) bytes[i] = store.read(calibrationOffset + i);
        const bool success = data.decode(bytes);
        data.storageResult(success, false);
        return success;  // Invalid/missing flash data never replaces the current RAM references.
    }
    template<class Storage> bool saveCalibration(RGBCalibration &data, Storage &store) {
        if (data.collecting() || (!data.completed(RGB0) && !data.completed(RGB1))) return false;
        if (store.length() != 4096) store.begin(4096);
        uint8_t bytes[RGBCalibration::encodedSize], previous[RGBCalibration::encodedSize];
        data.encode(bytes);
        for (size_t i = 0; i < sizeof(bytes); ++i) {
            previous[i] = store.read(calibrationOffset + i);
            store.write(calibrationOffset + i, bytes[i]);
        }
        const bool success = store.commit(); // Arduino-Pico pauses/resumes the other core internally.
        if (!success) {
            for (size_t i = 0; i < sizeof(bytes); ++i) store.write(calibrationOffset + i, previous[i]);
        }
        data.storageResult(success, true);
        return success;
    }
    template<class Output> void printCalibration(const RGBCalibration &data, Output &out) {
        out.println("sensor,color,samples,R,G,B,C,minR,minG,minB,minC,maxR,maxG,maxB,maxC");
        for (unsigned channel = 0; channel < 2; ++channel) for (unsigned color = 0; color < 6; ++color) {
            const auto record = data.get(RGBChannel(channel), RGBColor(color));
            // Missing records are omitted.
            if (!record.samples) continue;
            char line[150];
            snprintf(line, sizeof(line), "RGB%u,%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
                     channel, rgbColorName(RGBColor(color)), unsigned(record.samples),
                     unsigned(record.mean.red), unsigned(record.mean.green), unsigned(record.mean.blue), unsigned(record.mean.clear),
                     unsigned(record.minimum.red), unsigned(record.minimum.green), unsigned(record.minimum.blue), unsigned(record.minimum.clear),
                     unsigned(record.maximum.red), unsigned(record.maximum.green), unsigned(record.maximum.blue), unsigned(record.maximum.clear));
            out.println(line);
        }
    }
}

inline bool loadRGBCalibration() {
    return PuppyBotRGBDetail::loadCalibration(PuppyBotRGBDetail::calibration, EEPROM);
}
inline bool saveRGBCalibration() {
    return PuppyBotRGBDetail::saveCalibration(PuppyBotRGBDetail::calibration, EEPROM);
}
template<class Output> inline void printRGBCalibration(Output &out) {
    PuppyBotRGBDetail::printCalibration(PuppyBotRGBDetail::calibration, out);
}
