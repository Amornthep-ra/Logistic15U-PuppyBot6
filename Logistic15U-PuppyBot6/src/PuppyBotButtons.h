#pragma once
#include <Arduino.h>

namespace PuppyBotInput {
    enum class ButtonEvent : uint8_t { None, Next, Select };
    class ClickButton {
        bool raw_ = false, stable_ = false, armed_ = true;
        uint32_t changedAt_ = 0;
    public:
        void begin(bool pressed, uint32_t now) {
            raw_ = stable_ = pressed;
            armed_ = !pressed;  // A button held at startup must be released first.
            changedAt_ = now;
        }
        bool update(bool pressed, uint32_t now) {
            if (raw_ != pressed) { raw_ = pressed; changedAt_ = now; }
            if (stable_ != raw_ && uint32_t(now - changedAt_) >= 30) {
                stable_ = raw_;
                if (!stable_) {
                    const bool emit = armed_;
                    armed_ = true;
                    return emit;
                }
            }
            return false;
        }
    };

    class MenuButtons {
        ClickButton boot_, sw1_;
        bool initialized_ = false, overlapping_ = false;
    public:
        void reset() { initialized_ = false; overlapping_ = false; }
        ButtonEvent update(bool boot, bool sw1, uint32_t now) {
            if (!initialized_) {
                boot_.begin(boot, now); sw1_.begin(sw1, now);
                initialized_ = true;
                return ButtonEvent::None;
            }
            // Overlapping presses are ignored until both buttons are released.
            if (boot && sw1) overlapping_ = true;
            if (overlapping_) {
                boot_.begin(boot, now); sw1_.begin(sw1, now);
                if (!boot && !sw1) overlapping_ = false;
                return ButtonEvent::None;
            }
            const bool next = boot_.update(boot, now);
            const bool select = sw1_.update(sw1, now);
            if (next && select) return ButtonEvent::None;
            return next ? ButtonEvent::Next : select ? ButtonEvent::Select : ButtonEvent::None;
        }
    };

}
