#pragma once
#include <Adafruit_ST7735.h>
#include <stdio.h>
#include <string.h>
#include "PuppyBotButtons.h"
#include "PuppyBotEncoders.h"
#include "PuppyBotRGBMode.h"
#include "PuppyBotLineCalibrationMenu.h"

// Included by puppybot.h after the board's ADC and MUX readers are defined.
namespace PuppyBotWaitDetail {
    enum class Page : uint8_t { Menu, Adc, Mux, Motion, Calibrate, CalibrateLine };
    struct Menu {
        Page page = Page::Menu;
        uint8_t selection = 0;
        bool handle(PuppyBotInput::ButtonEvent event) {
            using PuppyBotInput::ButtonEvent;
            if (event == ButtonEvent::Next) {
                if (page == Page::Menu) selection = (selection + 1) % 6;
                else if (page != Page::Calibrate && page != Page::CalibrateLine) {
                    page = Page(uint8_t(page) % 3 + 1);
                    selection = uint8_t(page);
                }
            } else if (event == ButtonEvent::Select) {
                if (page == Page::Menu) {
                    if (selection == 0) return true;
                    page = Page(selection);
                } else if (page != Page::Calibrate && page != Page::CalibrateLine) page = Page::Menu; // Exit returns to the selected menu item.
            }
            return false;
        }
    };
    struct Readings {
        int adc[16] = {}, front[8] = {}, back[8] = {};
        float angles[4] = {};
        int encoders[2] = {};
    };
    inline void sample(Page page, Readings &values) {
        if (page == Page::Adc) {
            for (int i = 0; i < 16; ++i) values.adc[i] = ADC(i);
        } else if (page == Page::Mux) {
            for (int i = 0; i < 8; ++i) {
                values.front[i] = PuppyBotAdcDetail::readMuxFront(i, 8);
                values.back[i] = PuppyBotAdcDetail::readMuxBack8(i);
            }
        } else if (page == Page::Motion) {
            values.angles[0] = pitch_loop1; values.angles[1] = roll_loop1;
            values.angles[2] = yaw_loop1; values.angles[3] = Con_yaw_loop1;
            values.encoders[0] = PuppyBotIMU::get_pulse_Encoder(1);
            values.encoders[1] = PuppyBotIMU::get_pulse_Encoder(2);
        }
    }

    template<class Display> class Screen {
        Display &display;
        int page_ = -1, width_ = 0, height_ = 0;
        int selection_ = -1;
        char fields_[16][16] = {};
        static constexpr uint16_t background = 0x0845, panel = 0x192A;
        static constexpr uint16_t text = 0xFFFF, cyan = 0x4EDF, amber = 0xFEA8, muted = 0x9CD3;
        void label(int x, int y, const char *value, uint16_t color, uint16_t base = background) {
            display.setTextColor(color, base); display.setCursor(x, y); display.print(value);
        }
        void centered(int x, int y, int width, const char *value, uint16_t color, uint16_t base = background) {
            char clipped[28]; snprintf(clipped, sizeof(clipped), "%.*s", width / 6, value);
            label(x + (width - int(strlen(clipped)) * 6) / 2, y, clipped, color, base);
        }
        void field(int index, int x, int y, int chars, const char *value, uint16_t color) {
            char clipped[16]; snprintf(clipped, sizeof(clipped), "%.*s", chars, value);
            if (strcmp(fields_[index], clipped) == 0) return;
            display.fillRect(x, y, chars * 6, 8, panel);
            label(x, y, clipped, color, panel);
            snprintf(fields_[index], sizeof(fields_[index]), "%s", clipped);
        }
    public:
        explicit Screen(Display &display) : display(display) {}
        void invalidate() { page_ = -1; }
        void show(const Menu &menu, const Readings &values) {
            if (menu.page == Page::Calibrate || menu.page == Page::CalibrateLine) return;
            if (page_ != int(menu.page) || width_ != display.width() || height_ != display.height()) {
                page_ = int(menu.page); width_ = display.width(); height_ = display.height();
                selection_ = -1; memset(fields_, 0, sizeof(fields_));
                display.setFont(nullptr); display.setTextWrap(false); display.setTextSize(1);
                display.fillScreen(background);
                const char *titles[] = {"Robot menu", "ADC A0-A15", "MUX 8 Front / Back", "IMU / Encoder"};
                centered(4, 6, width_ - 8, titles[page_], cyan);
                centered(4, height_ - 9, width_ - 8, menu.page == Page::Menu ? "BOOT next SW1 OK" : "BOOT next SW1 Exit", muted);
                if (menu.page != Page::Menu) {
                    display.fillRoundRect(4, 19, width_ - 8, height_ - 32, 3, panel);
                    if (menu.page == Page::Mux) {
                        label(8, 23, "Front", cyan, panel);
                        label(width_ / 2 + 4, 23, "Back", amber, panel);
                    } else if (menu.page == Page::Motion) {
                        const char *labels[] = {"Pitch", "Roll", "Yaw", "Yaw+", "ENC1", "ENC2"};
                        for (int i = 0; i < 6; ++i) label(8, 25 + 13 * i, labels[i], i < 4 ? cyan : amber, panel);
                    }
                }
            }
            if (menu.page == Page::Menu) {
                if (selection_ == menu.selection) return;
                const char *items[] = {"1. Start", "2. ADC A0-A15", width_ >= 150 ? "3. Mux 8 Front/Back" : "3. Mux 8 F/B", "4. IMU + ENC1/2", "5. Calibrate RGB", "6. Calibrate Line"};
                const uint16_t colors[] = {0x4F30, 0x2E7F, 0xFE68, 0xB43F, 0xFBF4, 0xA7EC};
                const int step = (height_ - 35) / 6;
                for (int i = 0; i < 6; ++i) {
                    if (selection_ >= 0 && i != selection_ && i != menu.selection) continue;
                    const bool selected = i == menu.selection;
                    const uint16_t base = selected ? colors[i] : panel;
                    const int y = 20 + step * i, h = step - 2;
                    display.fillRoundRect(6, y, width_ - 12, h, 3, base);
                    centered(8, y + (h - 8) / 2, width_ - 16, items[i], selected ? background : colors[i], base);
                }
                selection_ = menu.selection;
                return;
            }
            char value[24];
            if (menu.page == Page::Adc || menu.page == Page::Mux) {
                for (int i = 0; i < 16; ++i) {
                    const int column = i / 8, row = i % 8;
                    if (menu.page == Page::Adc) snprintf(value, sizeof(value), "A%02d:%4d", i, values.adc[i]);
                    else snprintf(value, sizeof(value), "%c%d:%4d", column ? 'B' : 'F', row,
                                  column ? values.back[row] : values.front[row]);
                    field(i, 8 + column * (width_ / 2 - 4), (menu.page == Page::Mux ? 36 : 26) + row * 9,
                          (width_ / 2 - 10) / 6, value, column ? amber : text);
                }
            } else {
                for (int i = 0; i < 6; ++i) {
                    if (i < 4) snprintf(value, sizeof(value), "%.1f", double(values.angles[i]));
                    else snprintf(value, sizeof(value), "%d", values.encoders[i - 4]);
                    if (strlen(value) > 11) snprintf(value, sizeof(value), "out of range");
                    field(i, width_ - 74, 25 + 13 * i, 11, value, text);
                }
            }
        }
    };
}

// Blocking start gate on the TFT core. setup1/loop1 must already be released.
inline void wait_SW1() {
    using namespace PuppyBotWaitDetail;
    pinMode(swpin, INPUT_PULLUP);
    PuppyBotInput::MenuButtons buttons;
    PuppyBotRGBDetail::CalibrationFeedback feedback;
    loadLineCalibration(LineSide::Front);
    loadLineCalibration(LineSide::Back);
    Menu menu;
    Readings readings;
    Screen<Adafruit_ST7735> screen(tft_);
    uint32_t polledAt = millis() - 20, sampledAt = millis();
    Page sampledPage = Page::Menu;
    while (true) {
        const uint32_t now = millis();
        feedback.update(now);
        if (menu.page == Page::Calibrate) {
            updateRGBMode();
            if (PuppyBotRGBDetail::calibrationExitRequested) {
                endRGBMode();
                feedback.play(PuppyBotInput::ButtonEvent::Select, now);
                menu.page = Page::Menu;
                buttons.reset(); polledAt = now;
                screen.invalidate();
            }
            delay(1);
            continue;
        }
        if (uint32_t(now - polledAt) >= 20) {
            polledAt = now;
            const auto event = buttons.update(bool(BOOTSEL), digitalRead(swpin) == LOW, now);
            if (event != PuppyBotInput::ButtonEvent::None) feedback.play(event, now);
            if (menu.handle(event)) {
                delay(80); feedback.stop();
                return;
            }
        }
        if (menu.page == Page::Calibrate) {
            feedback.stop();
            beginRGBMode(RGB_MODE_CALIBRATE, true);
            PuppyBotRGBDetail::calibrationFeedback.play(PuppyBotInput::ButtonEvent::Select, now);
            continue;
        }
        if (menu.page == Page::CalibrateLine) {
            feedback.stop();
            PuppyBotLineDetail::runMenu();
            menu.page = Page::Menu;
            buttons.reset(); polledAt = millis();
            screen.invalidate();
        }
        if (menu.page != sampledPage || uint32_t(now - sampledAt) >= 100) {
            sample(menu.page, readings);
            sampledPage = menu.page; sampledAt = now;
        }
        screen.show(menu, readings);
        delay(1);
    }
}
