#pragma once
#include "PuppyBotRGBCalibrationStorage.h"
#include "PuppyBotRGBCalibrationControl.h"
#include "PuppyBotRGBDisplay.h"
#include <string.h>

namespace PuppyBotRGBDetail {
    template<class Display> class CalibrationDisplay {
        static constexpr uint16_t background = rgb565(7, 24, 32);
        static constexpr uint16_t panel = rgb565(16, 42, 54);
        static constexpr uint16_t text = rgb565(245, 248, 250);
        static constexpr uint16_t muted = rgb565(156, 178, 191);
        static constexpr uint16_t accent = rgb565(73, 216, 238);
        struct Field { char value[26] = {}; uint16_t color = 0; bool painted = false; };
        Field fields[14];
        Display &screen;
        int width = 0, height = 0;
        int previousChannel = -1, previousSelection = -1, previousMask = -1;
        static uint16_t color(unsigned index) {
            constexpr uint16_t colors[] = {rgb565(255, 99, 120), rgb565(101, 222, 152), rgb565(118, 170, 255),
                                           rgb565(255, 207, 74), rgb565(245, 248, 250), rgb565(45, 56, 68)};
            return colors[index];
        }
        void label(int x, int y, const char *value, uint16_t foreground, uint16_t base) {
            screen.setTextSize(1); screen.setTextColor(foreground, base);
            screen.setCursor(x, y); screen.print(value);
        }
        void field(unsigned index, int x, int y, int characters, const char *value,
                   uint16_t foreground, uint16_t base = panel) {
            char clipped[26];
            snprintf(clipped, sizeof(clipped), "%.*s", characters, value);
            value = clipped;
            Field &last = fields[index];
            if (last.painted && last.color == foreground && strcmp(last.value, value) == 0) return;
            screen.fillRect(x, y, characters * 6, 8, base);
            if (*value) label(x + (characters * 6 - int(strlen(value)) * 6) / 2, y, value, foreground, base);
            snprintf(last.value, sizeof(last.value), "%s", value);
            last.color = foreground; last.painted = true;
        }
        static const char *status(RGBCaptureState state) {
            switch (state) {
                case RGBCaptureState::Collecting: return "Hold color steady";
                case RGBCaptureState::Captured: return "Captured: s to save";
                case RGBCaptureState::Cancelled: return "Cancelled: c retry";
                case RGBCaptureState::Offline: return "Offline: check RGB";
                case RGBCaptureState::TimedOut: return "No frames: c retry";
                case RGBCaptureState::InvalidSignal: return "Light invalid: retry";
                default: return "Ready: c to capture";
            }
        }
    public:
        explicit CalibrationDisplay(Display &display) : screen(display) {}
        void invalidate() { width = 0; }
        void show(const RGBCalibration &data, const RGBReading &live, const CalibrationMenu *menu = nullptr) {
            if (screen.width() != width || screen.height() != height) {
                width = screen.width(); height = screen.height();
                for (auto &item : fields) item.painted = false;
                previousChannel = previousSelection = previousMask = -1;
                screen.setFont(nullptr); screen.setTextWrap(false);
                screen.fillScreen(background);
                label((width - 13 * 6) / 2, 5, "Calibrate RGB", accent, background);
                screen.fillRoundRect(6, 17, width - 12, 13, 3, panel);
                screen.fillRoundRect(6, 46, width - 12, 59, 4, panel);
                
                label(width - 54, 49, "MEAN", muted, panel);
                for (unsigned i = 0; i < 4; ++i) {
                    const char letters[2] = {"RGBC"[i], 0};
                    label(12, 61 + 11 * i, letters, i < 3 ? color(i) : muted, panel);
                }
            }
            char value[26];
            const char *storage = data.storage() == RGBStorageState::Failed ? "ERR" :
                data.dirty() ? "RAM*" : (data.storage() == RGBStorageState::Saved ||
                data.storage() == RGBStorageState::Loaded) ? "FLASH" : "RAM";
            field(0, 8, 49, 5, storage, data.dirty() ? color(3) : accent);
            snprintf(value, sizeof(value), "RGB%u  %-6s", unsigned(data.channel()), rgbColorName(data.color()));
            if (menu) {
                if (menu->focus() == RGBCalibrationFocus::Sensor) {
                    if (menu->sensorChoice() == 2) snprintf(value, sizeof(value), "Save all");
                    else if (menu->sensorChoice() == 3) snprintf(value, sizeof(value), "Exit");
                    else snprintf(value, sizeof(value), "Sensor: RGB%u", unsigned(data.channel()));
                }
                else if (menu->choice() == 6) snprintf(value, sizeof(value), "Save all");
                else if (menu->choice() == 7) snprintf(value, sizeof(value), "Sensor menu");
                else if (menu->choice() == 8) snprintf(value, sizeof(value), "Exit");
            }
            field(1, 10, 20, (width - 46) / 6, value, data.channel() == RGB0 ? accent : color(3));
            snprintf(value, sizeof(value), "%u/6", unsigned(data.completed(data.channel())));
            field(2, width - 30, 20, 3, value, text);
            int mask = 0;
            for (unsigned i = 0; i < 6; ++i)
                if (data.get(data.channel(), RGBColor(i)).samples) mask |= 1 << i;
            const int highlightedColor = menu ? menu->highlightedColor() : int(data.color());
            if (previousChannel != data.channel() || previousSelection != highlightedColor || previousMask != mask) {
                const int chipWidth = (width - 12 - 5 * 4) / 6;
                for (unsigned i = 0; i < 6; ++i) {
                    const int x = 6 + i * (chipWidth + 4);
                    screen.fillRect(x, 33, chipWidth, 10, color(i));
                    const char letter[2] = {"RGBYWK"[i], 0};
                    label(x + 2, 34, letter, i == 5 ? text : background, color(i));
                    if (mask & (1 << i)) label(x + chipWidth - 7, 34, "+", i == 5 ? text : background, color(i));
                    screen.drawFastHLine(x, 44, chipWidth, highlightedColor == int(i) ? accent : background);
                }
                previousChannel = data.channel(); previousSelection = highlightedColor; previousMask = mask;
            }
            const bool rejected = data.state() == RGBCaptureState::InvalidSignal;
            field(11, 39, 49, 4, rejected ? "FAIL" : "LIVE", rejected ? color(0) : muted);
            const RGBRawValues raw = rejected ? data.rejectedReading() :
                RGBRawValues{live.rawRed, live.rawGreen, live.rawBlue, live.clear};
            const auto reference = data.get(data.channel(), data.color());
            for (unsigned i = 0; i < 4; ++i) {
                if (rejected || (live.sampled && live.connected)) snprintf(value, sizeof(value), "%5u", unsigned(raw.at(i)));
                else snprintf(value, sizeof(value), "  ---");
                field(3 + i, 33, 61 + 11 * i, 5, value, text);
                if (reference.samples) snprintf(value, sizeof(value), "%5u", unsigned(reference.mean.at(i)));
                else snprintf(value, sizeof(value), "  ---");
                field(7 + i, width - 54, 61 + 11 * i, 5, value, accent);
            }
            const char *message = menu ? menu->message(data) : status(data.state());
            if (data.collecting()) {
                snprintf(value, sizeof(value), "Capturing %u/%u", unsigned(data.count()), unsigned(data.target()));
                message = value;
            }
            if (!data.collecting() && data.storage() == RGBStorageState::Saved && !menu) message = "Saved to flash";
            if (!data.collecting() && data.storage() == RGBStorageState::Failed && !menu) message = "Save failed: retry";
            if (rejected && !menu) message = data.rejectionMessage();
            field(12, 6, height - 20, (width - 12) / 6, message,
                  data.state() == RGBCaptureState::Offline || data.state() == RGBCaptureState::TimedOut ||
                  data.state() == RGBCaptureState::InvalidSignal ? color(0) : text,
                  background);
            field(13, 6, height - 8, (width - 12) / 6, menu ? "BOOT next SW1 OK" : "0/1 r g b y w k c", muted, background);
        }
    };
    static CalibrationDisplay<Adafruit_ST7735> calibrationDisplay(tft_);
}

inline void showRGBCalibration() {
    const auto &data = PuppyBotRGBDetail::calibration;
    PuppyBotRGBDetail::calibrationDisplay.show(data, getRGB(data.channel()),
        PuppyBotRGBDetail::calibrationSW1Enabled ? &PuppyBotRGBDetail::calibrationMenu : nullptr);
}
inline bool beginRGBCalibration() {
    const bool loaded = loadRGBCalibration();
    PuppyBotRGBDetail::calibrationDisplay.invalidate();
    showRGBCalibration();
    return loaded;
}
