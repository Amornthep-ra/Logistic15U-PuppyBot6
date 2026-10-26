#pragma once
#include "Wire.h"
#include <cstdint>
constexpr uint8_t TCS34725_ADDRESS = 0x29;
constexpr uint8_t TCS34725_COMMAND_BIT = 0x80, TCS34725_ID = 0x12, TCS34725_ENABLE = 0,
    TCS34725_ENABLE_PON = 1, TCS34725_ENABLE_AEN = 2, TCS34725_ATIME = 1, TCS34725_CONTROL = 15,
    TCS34725_STATUS = 0x13, TCS34725_STATUS_AVALID = 1, TCS34725_CDATAL = 0x14,
    TCS34725_INTEGRATIONTIME_50MS = 0xEB, TCS34725_GAIN_4X = 1;
struct Adafruit_TCS34725 {
    uint16_t r = 100, g = 200, b = 300;
    uint16_t clearValue = 4200;
    bool validId = true;
    int starts = 0, reads = 0;
    TwoWire *bound = nullptr;
    void setIntegrationTime(uint8_t) {}
    void setGain(uint8_t) {}
    bool begin(uint8_t address, TwoWire *bus) {
        ++starts; bound = bus;
        return address == 0x29 && validId && bus->connected;
    }
    void getRawData(uint16_t *red, uint16_t *green, uint16_t *blue, uint16_t *clear) {
        ++reads; *red = r; *green = g; *blue = b; *clear = clearValue;
    }
    uint16_t calculateColorTemperature(uint16_t, uint16_t, uint16_t) { return 5000; }
    uint16_t calculateLux(uint16_t, uint16_t, uint16_t) { return 60; }
};
