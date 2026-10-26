#pragma once
#include <array>
#include <cstdint>
#include <algorithm>

class EEPROMClass {
    size_t size_ = 0;
public:
    std::array<uint8_t, 4096> flash, buffer;
    int commits = 0;
    bool fail = false;
    EEPROMClass() { flash.fill(255); buffer.fill(255); }
    void begin(size_t size) { size_ = size; buffer = flash; }
    size_t length() const { return size_; }
    uint8_t read(int address) const { return buffer.at(address); }
    void write(int address, uint8_t value) { buffer.at(address) = value; }
    bool commit() { ++commits; if (fail) return false; flash = buffer; return true; }
};
inline EEPROMClass EEPROM;
