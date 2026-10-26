#pragma once
#include "PuppyBotLineSensors.h"
#include "PuppyBotButtons.h"
#include "PuppyBotRGBCalibrationFeedback.h"
#include <stdio.h>

void ao();

namespace PuppyBotLineDetail {
    template<class Display> class Screen {
        Display &display_;
        struct Row {
            char text[28] = {};
            uint16_t color = 0, base = 0;
            bool centered = false;
            bool same(const Row &other) const {
                return color == other.color && base == other.base && centered == other.centered && !strcmp(text, other.text);
            }
        };
        Row wanted_[13], painted_[13], work_;
        unsigned nextRow_ = 0, glyph_ = 0;
        int painting_ = -1;
        static constexpr uint16_t background = 0x0845, panel = 0x192A;
        static constexpr uint16_t cyan = 0x4EDF, green = 0x4F30, amber = 0xFE68, red = 0xF986;
        void row(unsigned n, const char *text, uint16_t color = 0xFFFF, uint16_t base = background, bool centered = false) {
            Row &target = wanted_[n];
            snprintf(target.text, sizeof(target.text), "%.*s", (display_.width() - 8) / 6, text);
            target.color = color; target.base = base; target.centered = centered;
        }
    public:
        explicit Screen(Display &display) : display_(display) {}
        void begin() {
            display_.setFont(nullptr); display_.setTextSize(1); display_.setTextWrap(false);
            display_.fillScreen(background);
            for (unsigned i = 0; i < 13; ++i) wanted_[i] = painted_[i] = Row{};
            painting_ = -1; nextRow_ = 0;
        }
        // At most one row fill or one glyph transfer per call, with button polling between them.
        bool drawPending() {
            if (painting_ >= 0) {
                if (work_.text[glyph_]) {
                    display_.setTextColor(work_.color, work_.base);
                    const int x = work_.centered ? (display_.width() - int(strlen(work_.text)) * 6) / 2 : 4;
                    display_.setCursor(x + int(glyph_) * 6, 4 + painting_ * 9);
                    const char letter[] = {work_.text[glyph_++], 0};
                    display_.print(letter);
                }
                if (!work_.text[glyph_]) { painted_[painting_] = work_; painting_ = -1; }
                return true;
            }
            for (unsigned i = 0; i < 13; ++i) {
                const unsigned n = (nextRow_ + i) % 13;
                if (wanted_[n].same(painted_[n])) continue;
                work_ = wanted_[n]; painting_ = int(n); glyph_ = 0; nextRow_ = (n + 1) % 13;
                display_.fillRect(4, 4 + int(n) * 9, display_.width() - 8, 8, work_.base);
                return true;
            }
            return false;
        }
        void show(const Session &s, const int raw[channels], bool deferred = false) {
            char title[28], text[40];
            if (s.stage == Stage::Side) snprintf(title, sizeof(title), "Calibrate Line");
            else snprintf(title, sizeof(title), "Line Cal - %s", s.side == LineSide::Front ? "Front" : "Back");
            row(0, title, cyan, background, true);
            row(1, s.notice, amber, background, true);
            // Build each row once per frame; unchanged rows are never erased.
            if (s.stage == Stage::Side || s.stage == Stage::Method) {
                row(2, s.stage == Stage::Side ? "Select bank" : "Capture method", cyan, background, true);
                for (unsigned n = 3; n <= 10; ++n) {
                    if (n == 3 || n == 5 || n == 7) {
                        const unsigned choice = (n - 3) / 2;
                        if (s.stage == Stage::Side) {
                            if (choice < 2) snprintf(text, sizeof(text), "%s: %s", choice ? "Back" : "Front",
                                loaded[choice] ? "Saved" : "Not calibrated");
                            else snprintf(text, sizeof(text), "Exit");
                        } else snprintf(text, sizeof(text), "%s", choice == 0 ? "All 8 channels" : choice == 1 ? "One by one" : "Exit");
                        row(n, text, choice == s.selection ? background : cyan, choice == s.selection ? cyan : panel, true);
                    } else row(n, "");
                }
                row(11, "");
            } else if (s.stage == Stage::Capture) {
                if (s.collecting) snprintf(text, sizeof(text), "%u/2 %s %u/%u", s.surface + 1, s.surface ? "LINE" : "FLOOR", s.samples, captureSamples);
                else if (s.single) snprintf(text, sizeof(text), "%u/2 %s CH %u", s.surface + 1, s.surface ? "LINE" : "FLOOR", s.channel);
                else snprintf(text, sizeof(text), "%u/2 %s ALL 8", s.surface + 1, s.surface ? "LINE" : "FLOOR");
                row(2, text, cyan, background, true);
                for (unsigned i = 0; i < channels; ++i) {
                    const bool target = !s.single || s.channel == i;
                    snprintf(text, sizeof(text), "%c%u  %5d %s", s.side == LineSide::Front ? 'F' : 'B', i, raw[i], target ? "< target" : "");
                    row(i + 3, text, target ? green : 0x9CD3, panel);
                }
                row(11, s.collecting ? "Reading... wait" : s.selection == 0 ? ">Capture    Exit" : " Capture   >Exit", green, panel, true);
            } else {
                row(2, "CH Min  Max  Gap", cyan);
                for (unsigned i = 0; i < channels; ++i) {
                    const auto &f = s.draft.floor[i], &l = s.draft.line[i];
                    const unsigned gap = f.mean > l.mean ? f.mean - l.mean : l.mean - f.mean;
                    snprintf(text, sizeof(text), "%u %4u %4u %4u%c", i, unsigned(f.mean < l.mean ? f.mean : l.mean), unsigned(f.mean > l.mean ? f.mean : l.mean), gap, s.draft.channelValid(i) ? '+' : '!');
                    row(i + 3, text, s.draft.channelValid(i) ? green : red, panel);
                }
                const char *actions[] = {">Save  Retry  Exit", " Save >Retry  Exit", " Save  Retry >Exit"};
                row(11, actions[s.selection], s.draft.valid() ? green : amber, panel, true);
            }
            row(12, s.collecting ? "Keep sensor still" : "BOOT next SW1 OK", 0x9CD3, background, true);
            if (!deferred) while (drawPending()) {}
        }
    };

    inline void runMenu() {
        ao();
        Session session;
        PuppyBotInput::MenuButtons buttons;
        PuppyBotRGBDetail::CalibrationFeedback feedback;
        Screen<Adafruit_ST7735> screen(tft_);
        screen.begin();
        int raw[channels] = {};
        uint32_t sampledAt = millis(), polledAt = millis(), drawnAt = millis() - 100;
        bool redraw = true;
        while (true) {
            const uint32_t now = millis();
            feedback.update(now);
            if (uint32_t(now - polledAt) >= 5) {
                polledAt = now;
                const auto event = buttons.update(bool(BOOTSEL), digitalRead(swpin) == LOW, now);
                if (event != PuppyBotInput::ButtonEvent::None && !session.collecting) {
                    feedback.play(event, now); redraw = true;
                    if (event == PuppyBotInput::ButtonEvent::Next) session.next();
                    else {
                        const Action action = session.select();
                        if (action == Action::Exit) { feedback.stop(); ao(); return; }
                        if (action == Action::Save) {
                            ao();
                            const bool ok = save(EEPROM, session.side, session.draft);
                            if (ok) { apply(session.side, session.draft); feedback.complete(now); }
                            else { feedback.stop(); tone(PUPPYBOT_BUZZER_PIN, 350, 180); }
                            session.saved(ok);
                        } else if (session.stage == Stage::Review && !session.draft.valid()) {
                            feedback.stop(); tone(PUPPYBOT_BUZZER_PIN, 350, 180);
                        }
                    }
                }
            }
            if (session.stage == Stage::Capture && uint32_t(now - sampledAt) >= samplePeriodMs) {
                sampledAt = now;
                for (unsigned i = 0; i < channels; ++i) raw[i] = session.side == LineSide::Front ?
                    PuppyBotAdcDetail::readMuxFront(i, channels) : PuppyBotAdcDetail::readMuxBack8(i);
                if (session.sample(raw)) {
                    buttons.reset(); redraw = true;
                    if (session.stage == Stage::Review && !session.draft.valid()) {
                        feedback.stop(); tone(PUPPYBOT_BUZZER_PIN, 350, 180);
                    } else feedback.complete(now);
                }
            }
            if (redraw || uint32_t(now - drawnAt) >= 100) {
                screen.show(session, raw, true); drawnAt = now; redraw = false;
            }
            screen.drawPending();
            delay(1);
        }
    }
}
