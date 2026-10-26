#pragma once
#include <stdio.h>
#include <string.h>
#include "PuppyBotIMUStartup.h"
#include "PuppyBotButtons.h"
#include "PuppyBotBuzzer.h"

namespace PuppyBotIMUStartupDetail {
    using PuppyBotIMU::StartupState;
    inline bool failed(StartupState state) {
        return state == StartupState::InitFailed || state == StartupState::NoData ||
               state == StartupState::TimedOut;
    }

    template<class Display> class Screen {
        Display &display;
        int width_ = 0, height_ = 0, barWidth_ = 0;
        uint16_t barColor_ = 0;
        struct Field { char text[28] = {}; uint16_t color = 0; } fields_[5];
        static constexpr uint16_t background = 0x0845, panel = 0x192A;
        static constexpr uint16_t white = 0xFFFF, cyan = 0x4EDF, green = 0x4F30;
        static constexpr uint16_t amber = 0xFEA8, red = 0xFB2C;

        // Fixed centered character cells let opaque GFX text replace old glyphs
        // directly, including spaces after shorter messages. No blank frame first.
        void field(int index, int y, const char *text, uint16_t color, uint16_t base) {
            const int cells = (width_ - 16) / 6 < 27 ? (width_ - 16) / 6 : 27;
            const int length = int(strlen(text)) < cells ? int(strlen(text)) : cells;
            char padded[28];
            memset(padded, ' ', cells);
            memcpy(padded + (cells - length) / 2, text, length);
            padded[cells] = '\0';
            auto &previous = fields_[index];
            const auto dirty = [&](int at) {
                return previous.text[at] != padded[at] ||
                    (previous.color != color && padded[at] != ' ');
            };
            for (int at = 0; at < cells;) {
                if (!dirty(at)) { ++at; continue; }
                const int first = at++;
                while (at < cells && dirty(at)) ++at;
                char run[28]; memcpy(run, padded + first, at - first); run[at - first] = '\0';
                display.setTextColor(color, base);
                display.setCursor((width_ - cells * 6) / 2 + first * 6, y);
                display.print(run);
            }
            memcpy(previous.text, padded, cells + 1);
            previous.color = color;
        }
        void bar(int width, uint16_t color) {
            if (width < barWidth_) display.fillRect(14 + width, 81, barWidth_ - width, 5, background);
            if (width > 0 && color != barColor_) display.fillRect(14, 81, width, 5, color);
            else if (width > barWidth_) display.fillRect(14 + barWidth_, 81, width - barWidth_, 5, color);
            barWidth_ = width; barColor_ = color;
        }
    public:
        explicit Screen(Display &display) : display(display) {}
        void show(StartupState state, uint32_t elapsed, unsigned done, unsigned total, int error) {
            if (width_ != display.width() || height_ != display.height()) {
                width_ = display.width(); height_ = display.height();
                for (auto &cached : fields_) cached = Field{};
                barWidth_ = 0; barColor_ = background;
                display.setFont(nullptr); display.setTextSize(1); display.setTextWrap(false);
                display.fillScreen(background);
                display.setTextColor(cyan, background);
                display.setCursor((width_ - 11 * 6) / 2, 7); display.print("IMU STARTUP");
                display.fillRoundRect(6, 24, width_ - 12, 70, 4, panel);
                display.fillRect(14, 81, width_ - 28, 5, background);
            }
            const bool problem = failed(state);
            const bool ready = state == StartupState::Ready;
            const bool disabled = state == StartupState::Disabled;
            const uint16_t accent = problem ? red : ready ? green : amber;
            const char *titles[] = {"WAITING FOR IMU", "CONNECTING", "CALIBRATING IMU",
                "IMU READY", "IMU DISABLED", "IMU NOT READY", "NO IMU DATA", "IMU TIMEOUT"};
            field(0, 31, titles[int(state)], accent, panel);
            const char *hint = ready ? "Yaw reference saved" : disabled ? "IMU not used" :
                problem ? "Check sensor / wiring" : "Keep robot still";
            field(1, 47, hint, white, panel);
            field(2, height_ - 26, problem ? "Restart or press SW1" : "Please wait...", white, background);
            field(3, height_ - 12, problem ? "Continue without IMU" : "", amber, background);
            const uint64_t progress = total ? uint64_t(done) * 100 / total : 0;
            const unsigned percent = ready || progress > 100 ? 100 : unsigned(progress);
            char value[28];
            if (state == StartupState::InitFailed) snprintf(value, sizeof(value), "Init error: %d", error);
            else if (state == StartupState::Calibrating) snprintf(value, sizeof(value), "%lu.%lus   %u%%",
                (unsigned long)(elapsed / 1000), (unsigned long)(elapsed / 100 % 10), percent);
            else snprintf(value, sizeof(value), "%lu.%lus", (unsigned long)(elapsed / 1000),
                (unsigned long)(elapsed / 100 % 10));
            field(4, 64, value, white, panel);
            bar((width_ - 28) * int(percent) / 100, accent);
        }
    };

    // Core 0 only. A timeout leaves core 1 alone; never access its I2C from here.
    template<class Display> bool wait(Display &display, int switchPin, uint32_t timeoutMs) {
        PuppyBotInput::MenuButtons buttons;
        Screen<Display> screen(display);
        pinMode(switchPin, INPUT_PULLUP);
        const uint32_t started = millis();
        uint32_t changedAt = started;
        auto previous = StartupState::Waiting;
        bool toneStarted = false;
        if (timeoutMs == 0) timeoutMs = 15000;
        while (true) {
            const uint32_t now = millis();
            if (uint32_t(now - started) >= timeoutMs) PuppyBotIMU::startup.timeout();
            const auto state = PuppyBotIMU::getIMUStartupState();
            if (state != previous) {
                changedAt = now; previous = state; buttons.reset();
                // Asynchronous, once per result; deliberate IMU disable stays silent.
                if (state == StartupState::Ready || failed(state)) {
                    tone(PUPPYBOT_BUZZER_PIN, state == StartupState::Ready ? 2400 : 440,
                         state == StartupState::Ready ? 180 : 600);
                    toneStarted = true;
                }
            }
            screen.show(state, now - started, PuppyBotIMU::startup.completed.load(),
                PuppyBotIMU::startup.total.load(), PuppyBotIMU::startup.error.load());
            const auto event = buttons.update(false, digitalRead(switchPin) == LOW, now);
            if (failed(state) && event == PuppyBotInput::ButtonEvent::Select) {
                if (toneStarted) noTone(PUPPYBOT_BUZZER_PIN);
                return false;
            }
            if ((state == StartupState::Ready || state == StartupState::Disabled) &&
                uint32_t(now - changedAt) >= 700) {
                if (toneStarted) noTone(PUPPYBOT_BUZZER_PIN);
                return state == StartupState::Ready;
            }
            delay(10);
        }
    }
}

inline bool waitIMUCalibration(uint32_t timeoutMs = 15000) {
    return PuppyBotIMUStartupDetail::wait(tft_, swpin, timeoutMs);
}
