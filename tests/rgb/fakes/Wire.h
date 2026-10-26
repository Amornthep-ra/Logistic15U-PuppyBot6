#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
struct TwoWire {
    bool connected = true;
    int sda = -1, scl = -1, probes = 0;
    int reads = 0, shortRead = -1;
    bool validId = true, adcReady = true;
    uint16_t r = 100, g = 200, b = 300, clearValue = 4200;
    std::array<uint8_t, 32> registers{};
    std::vector<uint8_t> tx, rx;
    size_t cursor = 0;
    uint8_t pointer = 0;
    void setSDA(int pin) { sda = pin; }
    void setSCL(int pin) { scl = pin; }
    void begin() {}
    void beginTransmission(uint8_t address) { if (address == 0x29) ++probes; tx.clear(); }
    size_t write(uint8_t value) { tx.push_back(value); return 1; }
    int endTransmission(bool = true) {
        if (!connected) return 2;
        if (!tx.empty()) pointer = tx[0] & 0x1F;
        if (tx.size() == 2) registers[pointer] = tx[1];
        return 0;
    }
    size_t requestFrom(uint8_t, size_t count, bool) {
        rx.clear(); cursor = 0;
        if (!connected) return 0;
        if (pointer == 0x12) rx.push_back(validId ? 0x44 : 0);
        else if (pointer == 0x13) rx.push_back(adcReady ? 1 : 0);
        else if (pointer >= 0x14 && pointer <= 0x1A && !(pointer & 1)) {
            if (pointer == 0x14) ++reads;
            // Model a device that needs the established High/Adafruit word-read protocol.
            if (tx[0] != uint8_t(0x80 | pointer) || count != 2) return 0;
            const uint16_t words[] = {clearValue, r, g, b};
            const auto value = words[(pointer - 0x14) / 2];
            rx.push_back(uint8_t(value)); rx.push_back(uint8_t(value >> 8));
        }
        if (shortRead >= 0 && size_t(shortRead) < rx.size()) rx.resize(size_t(shortRead));
        return rx.size();
    }
    int available() { return int(rx.size() - cursor); }
    int read() { return cursor < rx.size() ? rx[cursor++] : -1; }
};
extern TwoWire Wire, Wire1;
