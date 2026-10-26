#pragma once
#include "PuppyBotRGB.h"
#include <Adafruit_ST7735.h>
#include <stdio.h>

extern Adafruit_ST7735 tft_;

namespace PuppyBotRGBDetail {
    constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
        return uint16_t((r & 0xF8) << 8) | uint16_t((g & 0xFC) << 3) | (b >> 3);
    }

    // Classic 5x7 glyphs keep the complete monitor readable on the 160x128 TFT.
    template<class Display> class Monitor {
        static constexpr uint16_t background = rgb565(7, 24, 32);
        static constexpr uint16_t panel = rgb565(16, 42, 54);
        static constexpr uint16_t border = rgb565(37, 71, 84);
        static constexpr uint16_t text = rgb565(245, 248, 250);
        static constexpr uint16_t muted = rgb565(156, 178, 191);
        static constexpr uint16_t track = rgb565(32, 62, 74);
        struct Box { int x, y, w, h; };
        Display &screen;
        int selection = -2, width = 0, height = 0;
        RGBReading previous[2];
        bool painted[2] = {false, false};

        static uint16_t accent(int channel) {
            return channel == 0 ? rgb565(73, 216, 238) : rgb565(255, 200, 117);
        }
        static uint16_t componentColor(int component) {
            return component == 0 ? rgb565(255, 99, 120) :
                   component == 1 ? rgb565(101, 222, 152) : rgb565(118, 170, 255);
        }
        static int status(const RGBReading &reading) {
            return !reading.sampled ? 0 : reading.connected ? 2 : 1;
        }
        static uint16_t value(const RGBReading &reading, int component) {
            return component == 0 ? reading.red : component == 1 ? reading.green : reading.blue;
        }
        static int barSize(const RGBReading &reading, int component, int available) {
            const uint32_t sum = uint32_t(reading.rawRed) + reading.rawGreen + reading.rawBlue;
            const uint16_t raw = component == 0 ? reading.rawRed : component == 1 ? reading.rawGreen : reading.rawBlue;
            return reading.connected && sum ? int(uint32_t(raw) * available / sum) : 0;
        }
        static uint16_t swatch(const RGBReading &reading) {
            if (!reading.connected) return track;
            uint16_t peak = reading.rawRed;
            if (reading.rawGreen > peak) peak = reading.rawGreen;
            if (reading.rawBlue > peak) peak = reading.rawBlue;
            if (!peak) return 0;
            return rgb565(uint32_t(reading.rawRed) * 255 / peak,
                          uint32_t(reading.rawGreen) * 255 / peak,
                          uint32_t(reading.rawBlue) * 255 / peak);
        }
        void label(int x, int y, const char *content, uint16_t color, int size = 1) {
            screen.setTextSize(size);
            screen.setTextColor(color, panel);
            screen.setCursor(x, y);
            screen.print(content);
        }
        Box card(int index) const {
            const int top = 27, available = height - top - 15;
            if (selection != -1) return {6, top, width - 12, available};
            if (width >= 150) {
                const int w = (width - 18) / 2;
                return {6 + index * (w + 6), top, w, available};
            }
            const int h = (available - 6) / 2;
            return {6, top + index * (h + 6), width - 12, h};
        }
        bool columns(const Box &box) const { return selection != -1 || box.h < 75; }
        bool large(const Box &box) const { return selection != -1 && box.h >= 80 && box.w >= 140; }
        Box numberBox(const Box &box, int component) const {
            if (columns(box)) {
                const int cell = (box.w - 12) / 3;
                const int size = large(box) ? 2 : 1;
                return {box.x + 6 + component * cell + (cell - 18 * size) / 2,
                        box.y + (large(box) ? 51 : 39), 18 * size, 8 * size};
            }
            return {box.x + box.w - 25, box.y + 30 + component * 11, 18, 8};
        }
        Box barBox(const Box &box, int component) const {
            if (columns(box)) {
                const int cell = (box.w - 12) / 3;
                return {box.x + 8 + component * cell, box.y + box.h - 6, cell - 4, 2};
            }
            return {box.x + 18, box.y + 32 + component * 11, box.w - 47, 4};
        }
        Box clearBox(const Box &box) const {
            const int y = selection != -1 ? box.y + box.h - 16 :
                          columns(box) ? box.y + 18 : box.y + 63;
            return {box.x + box.w - 37, y, 30, 8};
        }
        Box statusBox(const Box &box) const {
            if (selection != -1) return {box.x + 62, box.y + 7, 48, 8};
            if (columns(box)) return {box.x + 40, box.y + 6, 42, 8};
            return {box.x + 7, box.y + box.h - 12, box.w - 14, 8};
        }
        Box swatchBox(const Box &box) const {
            const int size = selection != -1 ? 18 : columns(box) ? 10 : 12;
            return {box.x + box.w - size - 7, box.y + 6, size, size};
        }
        void frame(const Box &box, int channel) {
            screen.fillRoundRect(box.x, box.y, box.w, box.h, 4, panel);
            screen.drawRoundRect(box.x, box.y, box.w, box.h, 4, border);
            screen.fillRect(box.x + 6, box.y, 24, 2, accent(channel));
            label(box.x + 7, box.y + 6, channel == 0 ? "RGB0" : "RGB1", accent(channel));
            label(box.x + 7, box.y + 18, channel == 0 ? "SDA0/SCL0" : "SDA1/SCL1", muted);
            const Box color = swatchBox(box);
            screen.drawRect(color.x - 1, color.y - 1, color.w + 2, color.h + 2, border);
            for (int c = 0; c < 3; ++c) {
                const char name[2] = {"RGB"[c], '\0'};
                const Box number = numberBox(box, c);
                if (columns(box)) {
                    label(number.x + (number.w - 6) / 2, box.y + (large(box) ? 38 : 28), name, componentColor(c));
                } else label(box.x + 7, number.y, name, componentColor(c));
                const Box bar = barBox(box, c);
                screen.fillRect(bar.x, bar.y, bar.w, bar.h, track);
            }
            const Box clear = clearBox(box);
            const int clearLabelX = selection == -1 && columns(box) ? box.x + box.w - 49 : box.x + 7;
            label(clearLabelX, clear.y, selection == -1 ? "C" : "Clear", muted);
        }
        void readings(const Box &box, int channel, const RGBReading &reading) {
            const RGBReading &old = previous[channel];
            const bool stateChanged = !painted[channel] || status(old) != status(reading);
            if (stateChanged) {
                const Box area = statusBox(box);
                screen.fillRect(area.x, area.y, area.w, area.h, panel);
                const int state = status(reading);
                label(area.x, area.y, state == 0 ? "Wait" : state == 1 ? "Offline" : "Ready",
                      state == 2 ? componentColor(1) : state == 1 ? componentColor(0) : muted);
            }
            for (int c = 0; c < 3; ++c) {
                if (stateChanged || value(old, c) != value(reading, c)) {
                    const Box area = numberBox(box, c);
                    char valueText[8];
                    if (reading.connected) snprintf(valueText, sizeof(valueText), "%3u", unsigned(value(reading, c)));
                    else snprintf(valueText, sizeof(valueText), "---");
                    screen.fillRect(area.x, area.y, area.w, area.h, panel);
                    label(area.x, area.y, valueText, reading.connected ? componentColor(c) : muted, large(box) ? 2 : 1);
                }
                const Box bar = barBox(box, c);
                const int before = painted[channel] ? barSize(old, c, bar.w) : 0;
                const int now = barSize(reading, c, bar.w);
                if (now > before) screen.fillRect(bar.x + before, bar.y, now - before, bar.h, componentColor(c));
                else if (now < before) screen.fillRect(bar.x + now, bar.y, before - now, bar.h, track);
            }
            if (stateChanged || old.clear != reading.clear) {
                const Box area = clearBox(box);
                char clearText[8];
                if (reading.connected) snprintf(clearText, sizeof(clearText), "%5u", unsigned(reading.clear));
                else snprintf(clearText, sizeof(clearText), "  ---");
                screen.fillRect(area.x, area.y, area.w, area.h, panel);
                label(area.x, area.y, clearText, reading.connected ? text : muted);
            }
            if (!painted[channel] || swatch(old) != swatch(reading)) {
                const Box area = swatchBox(box);
                screen.fillRect(area.x, area.y, area.w, area.h, swatch(reading));
            }
            previous[channel] = reading;
            painted[channel] = true;
        }
    public:
        explicit Monitor(Display &display) : screen(display) {}
        void invalidate() { selection = -2; }
        void show(const RGBSnapshot &snapshot, int selected = -1) {
            if (selected < -1 || selected > 1) return;
            const bool rebuild = selection != selected || width != screen.width() || height != screen.height();
            if (rebuild) {
                selection = selected;
                width = screen.width(); height = screen.height();
                painted[0] = painted[1] = false;
                screen.setFont(nullptr);
                screen.setTextWrap(false);
                screen.fillScreen(background);
                screen.setTextSize(1);
                screen.setTextColor(text, background);
                screen.setCursor(7, 7); screen.print("Color monitor");
                screen.drawFastHLine(6, 20, width - 12, border);
                screen.setTextColor(muted, background);
                screen.setCursor(7, height - 10); screen.print("RGB:x100 Clear:raw");
                if (selection == -1) { frame(card(0), 0); frame(card(1), 1); }
                else frame(card(0), selection);
            }
            if (selection == -1) {
                readings(card(0), 0, snapshot.bus0);
                readings(card(1), 1, snapshot.bus1);
            } else readings(card(0), selection, selection == 0 ? snapshot.bus0 : snapshot.bus1);
        }
    };
    static Monitor<Adafruit_ST7735> monitor(tft_);
}

// TFT calls belong to one core, normally setup()/loop(). No I2C reads here.
inline void showRGB() { PuppyBotRGBDetail::monitor.show(getRGBSnapshot()); }
inline void showRGB(RGBChannel channel) {
    if (PuppyBotRGBDetail::valid(channel)) PuppyBotRGBDetail::monitor.show(getRGBSnapshot(), channel);
}
inline void beginRGBDisplay() {
    PuppyBotRGBDetail::monitor.invalidate();
    showRGB();
}
inline void beginRGBDisplay(RGBChannel channel) {
    if (!PuppyBotRGBDetail::valid(channel)) return;
    PuppyBotRGBDetail::monitor.invalidate();
    showRGB(channel);
}
