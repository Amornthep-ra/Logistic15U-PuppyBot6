#pragma once
#include <Arduino.h>
#include <Adafruit_TCS34725.h>
#include <Wire.h>
#include <pico/mutex.h>
#include "PuppyBotRGBSettings.h"

// These devices and compatibility caches are defined by puppybot.h.
extern Adafruit_TCS34725 tcs;
extern Adafruit_TCS34725 tcsB;
extern int TCS_R, TCS_G, TCS_B, TCS_T, TCS_L;

enum RGBChannel : uint8_t { RGB0 = 0, RGB1 = 1 };

struct RGBReading {
    uint16_t red = 0, green = 0, blue = 0;
    uint16_t rawRed = 0, rawGreen = 0, rawBlue = 0, clear = 0;
    uint16_t colorTemperature = 0, lux = 0;
    bool connected = false;
    bool sampled = false;
    uint32_t sampledAt = 0;
    uint32_t sequence = 0;  // Distinguishes fresh frames, even within the same millisecond.
};

struct RGBSnapshot {
    RGBReading bus0;
    RGBReading bus1;
};

namespace PuppyBotRGBDetail {
    struct SensorState {
        bool initialized = false;
        bool attempted = false;
        bool powering = false;
        uint32_t lastAttempt = 0;
        uint32_t phaseAt = 0;
        uint32_t sequence = 0;
    };
    static SensorState sensors[2];
    static RGBSnapshot cache;
    auto_init_mutex(cacheMutex);

    inline bool valid(RGBChannel channel) { return channel == RGB0 || channel == RGB1; }
    inline TwoWire &wire(RGBChannel channel) { return channel == RGB0 ? Wire : Wire1; }
    inline Adafruit_TCS34725 &sensor(RGBChannel channel) { return channel == RGB0 ? tcs : tcsB; }

    inline bool writeRegister(TwoWire &bus, uint8_t reg, uint8_t value) {
        bus.beginTransmission(TCS34725_ADDRESS);
        bus.write(uint8_t(TCS34725_COMMAND_BIT | reg));
        bus.write(value);
        return bus.endTransmission() == 0;
    }
    inline bool readRegisters(TwoWire &bus, uint8_t reg, uint8_t *bytes, uint8_t count) {
        bus.beginTransmission(TCS34725_ADDRESS);
        // Match High-PuppyBot6/Adafruit read8/read16 command bytes exactly.
        bus.write(uint8_t(TCS34725_COMMAND_BIT | reg));
        if (bus.endTransmission(false) != 0) return false;
        if (bus.requestFrom(uint8_t(TCS34725_ADDRESS), size_t(count), true) != count) {
            while (bus.available()) bus.read();
            return false;
        }
        for (unsigned i = 0; i < count; ++i) bytes[i] = uint8_t(bus.read());
        return true;
    }
    inline bool readRawRegisters(TwoWire &bus, uint8_t (&bytes)[8]) {
        // Same C/R/G/B word reads as getRawData(), without its integration delay.
        for (uint8_t offset = 0; offset < 8; offset += 2)
            if (!readRegisters(bus, uint8_t(TCS34725_CDATAL + offset), bytes + offset, 2)) return false;
        return true;
    }

    inline void publish(RGBChannel channel, const RGBReading &reading) {
        mutex_enter_blocking(&cacheMutex);
        if (channel == RGB0) cache.bus0 = reading;
        else {
            cache.bus1 = reading;
            TCS_R = reading.red;
            TCS_G = reading.green;
            TCS_B = reading.blue;
            TCS_T = reading.colorTemperature;
            TCS_L = reading.lux;
        }
        mutex_exit(&cacheMutex);
    }
}

inline RGBReading getRGB(RGBChannel channel);

// One I2C owner core. Begin schedules power-up; first sample arrives on later updates.
// No integration-time delay, including reconnect. Existing Wire1 clock/IMU setup is preserved.
inline bool beginRGB(RGBChannel channel) {
    using namespace PuppyBotRGBDetail;
    if (!valid(channel)) return false;
    SensorState &state = sensors[channel];
    if (state.initialized) return true;
    TwoWire &bus = wire(channel);
    // Pin assignments are inherited from the original PuppyBot library.
    bus.setSDA(channel == RGB0 ? 4 : 6);
    bus.setSCL(channel == RGB0 ? 5 : 7);
    bus.begin();
    state.lastAttempt = millis();
    state.attempted = true;
    uint8_t id = 0;
    state.initialized = readRegisters(bus, TCS34725_ID, &id, 1) &&
        (id == 0x44 || id == 0x4D || id == 0x10) &&
        writeRegister(bus, TCS34725_ENABLE, 0) &&
        writeRegister(bus, TCS34725_ATIME, PuppyBotRGBSettings::integration) &&
        writeRegister(bus, TCS34725_CONTROL, PuppyBotRGBSettings::gain) &&
        writeRegister(bus, TCS34725_ENABLE, TCS34725_ENABLE_PON);
    state.powering = state.initialized;
    state.phaseAt = millis();
    return state.initialized;
}

inline bool beginRGB() {
    const bool ready0 = beginRGB(RGB0);
    const bool ready1 = beginRGB(RGB1);
    return ready0 && ready1;
}

inline RGBReading readRGB(RGBChannel channel) {
    using namespace PuppyBotRGBDetail;
    if (!valid(channel)) return RGBReading{};
    SensorState &state = sensors[channel];
    if (!state.attempted || (!state.initialized && uint32_t(millis() - state.lastAttempt) >= 1000))
        beginRGB(channel);
    if (!state.initialized) {
        const auto cached = getRGB(channel);
        if (cached.sampled && !cached.connected) return cached;
    }
    TwoWire &bus = wire(channel);
    if (state.initialized) {
        if (state.powering) {
            if (uint32_t(millis() - state.phaseAt) < 3) return getRGB(channel);
            if (writeRegister(bus, TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN)) {
                state.powering = false;
                state.phaseAt = millis();
                return getRGB(channel);
            }
        } else if (uint32_t(millis() - state.phaseAt) < PuppyBotRGBSettings::sampleIntervalMs) {
            return getRGB(channel);
        } else {
            uint8_t status = 0, bytes[8];
            if (readRegisters(bus, TCS34725_STATUS, &status, 1)) {
                if (!(status & TCS34725_STATUS_AVALID)) {
                    // Keep the timestamp unchanged: consumers can detect a stalled ADC.
                    if (uint32_t(millis() - state.phaseAt) < 250) return getRGB(channel);
                } else if (readRawRegisters(bus, bytes)) {
                    RGBReading reading;
                    reading.connected = reading.sampled = true;
                    reading.sequence = ++state.sequence;
                    reading.sampledAt = millis();
                    const auto decodeRaw16 = [&](unsigned offset) { return uint16_t(bytes[offset] | uint16_t(bytes[offset + 1]) << 8); };
                    reading.clear = decodeRaw16(0); reading.rawRed = decodeRaw16(2);
                    reading.rawGreen = decodeRaw16(4); reading.rawBlue = decodeRaw16(6);
                    Adafruit_TCS34725 &device = sensor(channel);
                    // Preserve the existing integer-mean scale, including dark-reading protection.
                    const uint32_t sum = uint32_t(reading.rawRed) + reading.rawGreen + reading.rawBlue;
                    const uint32_t average = sum / 3;
                    if (average > 0) {
                        reading.red = uint16_t((reading.rawRed / float(average)) * 100);
                        reading.green = uint16_t((reading.rawGreen / float(average)) * 100);
                        reading.blue = uint16_t((reading.rawBlue / float(average)) * 100);
                    }
                    if (sum > 0) {
                        reading.colorTemperature = device.calculateColorTemperature(reading.rawRed, reading.rawGreen, reading.rawBlue);
                        reading.lux = device.calculateLux(reading.rawRed, reading.rawGreen, reading.rawBlue);
                    }
                    state.phaseAt = millis();
                    publish(channel, reading);
                    return reading;
                }
            }
        }
    }
    if (state.initialized) state.lastAttempt = millis();
    state.initialized = false;
    RGBReading reading;
    reading.sampled = true;
    reading.sequence = ++state.sequence;
    reading.sampledAt = millis();
    publish(channel, reading);
    return reading;
}

inline void updateRGB() {
    readRGB(RGB0);
    readRGB(RGB1);
}

inline RGBSnapshot getRGBSnapshot() {
    mutex_enter_blocking(&PuppyBotRGBDetail::cacheMutex);
    const RGBSnapshot snapshot = PuppyBotRGBDetail::cache;
    mutex_exit(&PuppyBotRGBDetail::cacheMutex);
    return snapshot;
}

inline RGBReading getRGB(RGBChannel channel) {
    if (!PuppyBotRGBDetail::valid(channel)) return RGBReading{};
    const RGBSnapshot snapshot = getRGBSnapshot();
    return channel == RGB0 ? snapshot.bus0 : snapshot.bus1;
}
