#pragma once
#include "PuppyBotRGBClassifier.h"
#include "PuppyBotRGBDisplay.h"

namespace PuppyBotRGBDetail {
    template<class Display> class ColorDisplay {
        static constexpr uint16_t background = rgb565(7, 24, 32);
        static constexpr uint16_t panel = rgb565(16, 42, 54);
        static constexpr uint16_t text = rgb565(245, 248, 250);
        Display &screen;
        RGBColorResult previous[2];
        bool painted[2] = {false, false};
        int width = 0, height = 0;

        static uint16_t color(RGBColor value) {
            switch (value) {
                case RGBColor::Red: return rgb565(255, 99, 120);
                case RGBColor::Green: return rgb565(101, 222, 152);
                case RGBColor::Blue: return rgb565(118, 170, 255);
                case RGBColor::Yellow: return rgb565(255, 207, 74);
                case RGBColor::White: return text;
                case RGBColor::Black: return rgb565(180, 190, 200);
                default: return rgb565(156, 178, 191);
            }
        }
        int cardTop(int index) const { return 22 + index * (cardHeight() + 6); }
        int cardHeight() const { return (height - 34) / 2; }
        void label(int x, int y, const char *value, uint16_t foreground, uint16_t base, int size = 1) {
            screen.setTextSize(size); screen.setTextColor(foreground, base);
            screen.setCursor(x, y); screen.print(value);
        }
        const char *status(RGBColorStatus value) const {
            if (width < 150) {
                if (value == RGBColorStatus::Unknown) return "No close match";
                if (value == RGBColorStatus::Ambiguous) return "Colors too close";
            }
            return rgbColorStatusName(value);
        }
        void result(int index, const RGBColorResult &value) {
            const int top = cardTop(index);
            if (!painted[index] || previous[index].color != value.color) {
                screen.fillRect(12, top + 17, width - 24, 16, panel);
                label(12, top + 17, rgbColorName(value.color), color(value.color), panel, 2);
            }
            if (!painted[index] || previous[index].status != value.status) {
                screen.fillRect(12, top + 35, width - 24, 8, panel);
                label(12, top + 35, status(value.status), text, panel);
            }
            previous[index] = value; painted[index] = true;
        }
    public:
        explicit ColorDisplay(Display &display) : screen(display) {}
        void invalidate() { width = 0; }
        void show(const RGBColorResult &rgb0, const RGBColorResult &rgb1) {
            if (width != screen.width() || height != screen.height()) {
                width = screen.width(); height = screen.height();
                painted[0] = painted[1] = false;
                screen.setFont(nullptr); screen.setTextWrap(false);
                screen.fillScreen(background);
                label(6, 6, "Calibrated colors", text, background);
                for (int index = 0; index < 2; ++index) {
                    const int top = cardTop(index);
                    screen.fillRoundRect(6, top, width - 12, cardHeight(), 4, panel);
                    label(12, top + 4, index == 0 ? "RGB0" : "RGB1",
                          index == 0 ? rgb565(73, 216, 238) : rgb565(255, 200, 117), panel);
                }
            }
            result(0, rgb0); result(1, rgb1);
        }
    };
    static ColorDisplay<Adafruit_ST7735> colorDisplay(tft_);
    static bool colorDisplayStarted = false;
    static uint32_t colorDisplayUpdatedAt = 0;
}

// TFT/calibration owner core only. Reads the RGB cache; no I2C, flash access or Serial output.
// Default refresh is 100ms; unchanged fields draw nothing.
inline void showRGBColor(const RGBColorOptions &rgb0 = RGBColorOptions{},
                         const RGBColorOptions &rgb1 = RGBColorOptions{}, uint32_t refreshMs = 100) {
    using namespace PuppyBotRGBDetail;
    const uint32_t now = millis();
    if (colorDisplayStarted && uint32_t(now - colorDisplayUpdatedAt) < refreshMs) return;
    colorDisplayStarted = true; colorDisplayUpdatedAt = now;
    const auto snapshot = getRGBSnapshot();
    colorDisplay.show(classifyRGB(RGB0, snapshot.bus0, rgb0), classifyRGB(RGB1, snapshot.bus1, rgb1));
}
inline void beginRGBColorDisplay() {
    PuppyBotRGBDetail::colorDisplay.invalidate();
    PuppyBotRGBDetail::colorDisplayStarted = false;
    showRGBColor();
}
