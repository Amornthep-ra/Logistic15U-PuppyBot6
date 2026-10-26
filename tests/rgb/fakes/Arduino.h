#pragma once
#include <cstdint>
#include <vector>
// ArduinoCore-API defines this macro; omitting it hides name collisions in host tests.
inline uint16_t makeWord(uint16_t value) { return value; }
inline uint16_t makeWord(uint8_t high, uint8_t low) { return uint16_t(high) << 8 | low; }
#define word(...) makeWord(__VA_ARGS__)
inline uint32_t fakeMillis = 0;
inline uint32_t millis() { return fakeMillis; }
inline unsigned fakeDelayCalls = 0;
inline void (*fakeDelayHook)() = nullptr;
inline void delay(unsigned long ms) { ++fakeDelayCalls; fakeMillis += ms; if (fakeDelayHook) fakeDelayHook(); }
constexpr int LOW = 0, HIGH = 1, INPUT_PULLUP = 2;
inline int fakeSwitchValue = HIGH, fakeSwitchPin = -1, fakeSwitchMode = -1;
inline bool fakeBootPressed = false;
inline unsigned fakeBootReads = 0;
inline bool (*fakeBootHook)() = nullptr;
inline int (*fakeDigitalReadHook)(int) = nullptr;
struct FakeBootsel {
    operator bool() const { ++fakeBootReads; return fakeBootHook ? fakeBootHook() : fakeBootPressed; }
};
inline FakeBootsel BOOTSEL;
inline void pinMode(int pin, int mode) { fakeSwitchPin = pin; fakeSwitchMode = mode; }
inline int digitalRead(int pin) { fakeSwitchPin = pin; return fakeDigitalReadHook ? fakeDigitalReadHook(pin) : fakeSwitchValue; }
struct FakeToneCall { uint8_t pin; unsigned frequency; unsigned long duration; };
inline std::vector<FakeToneCall> fakeToneCalls;
inline std::vector<uint8_t> fakeToneStops;
inline void tone(uint8_t pin, unsigned frequency, unsigned long duration) {
    fakeToneCalls.push_back({pin, frequency, duration});
}
inline void noTone(uint8_t pin) { fakeToneStops.push_back(pin); }
