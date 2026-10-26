#pragma once
#include "PuppyBotRGBCalibration.h"
#include <math.h>

enum class RGBColorStatus : uint8_t {
    Matched, NoSample, Offline, Stale, IncompleteCalibration, Unknown, Ambiguous, InvalidChannel,
    LowSignal, Saturated, InvalidOptions
};

struct RGBColorOptions {
    float maxDistance = 0.035f;   // Same starting tolerance as High-PuppyBot6.
    float separationRatio = 0.70f;
    uint32_t maxAgeMs = 250;       // Cache freshness policy, not a color threshold. Zero disables it.
};

struct RGBColorResult {
    RGBColor color = RGBColor::Unknown;
    RGBColorStatus status = RGBColorStatus::NoSample;
    uint8_t candidates = 0;
    bool matched() const { return status == RGBColorStatus::Matched; }
};

inline const char *rgbColorStatusName(RGBColorStatus status) {
    switch (status) {
        case RGBColorStatus::Matched: return "Matched";
        case RGBColorStatus::NoSample: return "No samples";
        case RGBColorStatus::Offline: return "Sensor offline";
        case RGBColorStatus::Stale: return "Sample too old";
        case RGBColorStatus::IncompleteCalibration: return "Need 6 colors";
        case RGBColorStatus::Unknown: return "No close reference";
        case RGBColorStatus::Ambiguous: return "Colors too similar";
        case RGBColorStatus::InvalidChannel: return "Invalid channel";
        case RGBColorStatus::LowSignal: return "Too little light";
        case RGBColorStatus::Saturated: return "Light saturated";
        case RGBColorStatus::InvalidOptions: return "Invalid tolerance";
    }
    return "Invalid status";
}

// High-PuppyBot6 method: normalized RGB nearest reference, distance + runner-up rejection.
// Clear gates signal validity only. References come from this sensor's captured raw means.
// Use on the same core that owns calibration/load/save. RGB cache reads can cross cores.
inline RGBColorResult classifyRGB(const RGBCalibration &references, RGBChannel channel,
                                 const RGBReading &reading, const RGBColorOptions &options = RGBColorOptions{}) {
    RGBColorResult result;
    if (!PuppyBotRGBDetail::valid(channel)) {
        result.status = RGBColorStatus::InvalidChannel;
        return result;
    }
    if (!reading.sampled) return result;
    if (!reading.connected) { result.status = RGBColorStatus::Offline; return result; }
    if (options.maxAgeMs && uint32_t(millis() - reading.sampledAt) >= options.maxAgeMs) {
        result.status = RGBColorStatus::Stale;
        return result;
    }
    if (!isfinite(options.maxDistance) || !isfinite(options.separationRatio) ||
        options.maxDistance < 0.001f || options.maxDistance > 1.0f ||
        options.separationRatio < 0.01f || options.separationRatio > 0.99f) {
        result.status = RGBColorStatus::InvalidOptions;
        return result;
    }
    const uint32_t sum = uint32_t(reading.rawRed) + reading.rawGreen + reading.rawBlue;
    if (!sum || reading.clear < PuppyBotRGBSettings::minimumClear) {
        result.status = RGBColorStatus::LowSignal;
        return result;
    }
    if (reading.rawRed >= PuppyBotRGBSettings::saturation || reading.rawGreen >= PuppyBotRGBSettings::saturation ||
        reading.rawBlue >= PuppyBotRGBSettings::saturation || reading.clear >= PuppyBotRGBSettings::saturation) {
        result.status = RGBColorStatus::Saturated;
        return result;
    }
    if (references.completed(channel) != 6) {
        result.status = RGBColorStatus::IncompleteCalibration;
        return result;
    }
    const float r = float(reading.rawRed) / sum, g = float(reading.rawGreen) / sum, b = float(reading.rawBlue) / sum;
    float best = 10.0f, second = 10.0f;
    RGBColor candidate = RGBColor::Unknown;
    for (unsigned index = 0; index < 6; ++index) {
        const auto color = RGBColor(index);
        const auto ref = references.get(channel, color).mean;
        const float total = float(ref.red) + ref.green + ref.blue;
        const float dr = r - ref.red / total, dg = g - ref.green / total, db = b - ref.blue / total;
        const float distance = dr * dr + dg * dg + db * db;
        if (distance < best) {
            second = best; best = distance; candidate = color;
        } else if (distance < second) {
            second = distance;
        }
        if (distance <= options.maxDistance * options.maxDistance) ++result.candidates;
    }
    if (best > options.maxDistance * options.maxDistance) result.status = RGBColorStatus::Unknown;
    else if (best >= second * options.separationRatio * options.separationRatio) result.status = RGBColorStatus::Ambiguous;
    else {
        result.color = candidate;
        result.status = RGBColorStatus::Matched;
    }
    return result;
}

// These overloads use the calibration loaded/captured in this program. They do not load flash or read I2C.
inline RGBColorResult classifyRGB(RGBChannel channel, const RGBReading &reading,
                                 const RGBColorOptions &options = RGBColorOptions{}) {
    return classifyRGB(PuppyBotRGBDetail::calibration, channel, reading, options);
}
inline RGBColorResult classifyRGB(RGBChannel channel, const RGBColorOptions &options = RGBColorOptions{}) {
    return classifyRGB(channel, getRGB(channel), options);
}
inline RGBColor getRGBColor(RGBChannel channel, const RGBColorOptions &options = RGBColorOptions{}) {
    return classifyRGB(channel, options).color;
}
