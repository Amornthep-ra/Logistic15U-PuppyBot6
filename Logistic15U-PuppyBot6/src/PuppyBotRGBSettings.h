#pragma once
#include <Adafruit_TCS34725.h>

// The same optical configuration and validity limits as High-PuppyBot6.
namespace PuppyBotRGBSettings {
    constexpr uint8_t integration = TCS34725_INTEGRATIONTIME_50MS;
    constexpr auto gain = TCS34725_GAIN_4X;
    constexpr uint16_t minimumClear = 10;
    constexpr uint16_t saturation = (256u - integration) * 1024u;
    // 21 integration steps; allow 2.56 ms/step plus timer rounding.
    constexpr uint32_t sampleIntervalMs = 55;
    inline bool usable(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
        return uint32_t(r) + g + b > 0 && c >= minimumClear &&
               r < saturation && g < saturation && b < saturation && c < saturation;
    }
}
