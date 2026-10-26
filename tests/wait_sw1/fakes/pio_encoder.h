#pragma once
#include <cstdint>
namespace FakeEncoder {
    inline int counts[48] = {}, starts[48] = {}, resets[48] = {};
}
class PioEncoder {
    int pin;
public:
    explicit PioEncoder(uint8_t pin) : pin(pin) {}
    void begin() { ++FakeEncoder::starts[pin]; FakeEncoder::counts[pin] = 0; }
    int getCount() { return FakeEncoder::counts[pin]; }
    void reset() { ++FakeEncoder::resets[pin]; FakeEncoder::counts[pin] = 0; }
};
