#pragma once
#include <cstdint>
#include <stdexcept>

constexpr int INPUT = 0;
constexpr int OUTPUT = 1;
namespace FakeEncoder {
inline int modes[48]{};
inline int counts[48]{};
inline int starts[48]{};
inline bool initialized[48]{};
inline void ticks(int pin, int delta) {
    if (initialized[pin] && modes[pin] == INPUT && modes[pin + 1] == INPUT)
        counts[pin] += delta;
}
}
inline void pinMode(int pin, int mode) { FakeEncoder::modes[pin] = mode; }

// Emulates the driver boundary, not the physical PIO quadrature decoder.
class PioEncoder {
    int pin_;
public:
    explicit PioEncoder(uint8_t pin) : pin_(pin) {}
    void begin() {
        pinMode(pin_, INPUT);
        pinMode(pin_ + 1, INPUT);
        FakeEncoder::initialized[pin_] = true;
        ++FakeEncoder::starts[pin_];
        FakeEncoder::counts[pin_] = 0;
    }
    int getCount() {
        if (!FakeEncoder::initialized[pin_]) throw std::runtime_error("read before begin");
        return FakeEncoder::counts[pin_];
    }
    void reset() {
        if (!FakeEncoder::initialized[pin_]) throw std::runtime_error("reset before begin");
        FakeEncoder::counts[pin_] = 0;
    }
};
