#include <iostream>
#include <stdexcept>
#include "PuppyBotRGBColorDisplay.h"

TwoWire Wire, Wire1;
Adafruit_TCS34725 tcs, tcsB;
Adafruit_ST7735 tft_;
int TCS_R = 0, TCS_G = 0, TCS_B = 0, TCS_T = 0, TCS_L = 0;

void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

int main(int argc, char **argv) {
    try {
        require(argc == 2, "output folder required");
        const std::string output = argv[1];
        using PuppyBotRGBDetail::ColorDisplay;
        RGBColorResult rgb0{RGBColor::Red, RGBColorStatus::Matched, 1};
        RGBColorResult rgb1{RGBColor::Blue, RGBColorStatus::Matched, 1};
        Adafruit_ST7735 screen;
        ColorDisplay<Adafruit_ST7735> display(screen);
        display.show(rgb0, rgb1); screen.save(output + "/color-display-red-blue.ppm");
        screen.operations.clear(); display.show(rgb0, rgb1);
        require(screen.operations.empty(), "same results must not redraw");
        rgb0 = {RGBColor::Unknown, RGBColorStatus::Ambiguous, 2};
        display.show(rgb0, rgb1);
        for (const auto &op : screen.operations)
            require(op.x >= 12 && op.x + op.w <= 148 && op.y >= 39 && op.y + op.h <= 65,
                    "one changed sensor may only redraw its text fields");
        Adafruit_ST7735 fresh;
        ColorDisplay<Adafruit_ST7735> freshDisplay(fresh);
        freshDisplay.show(rgb0, rgb1);
        require(screen.pixels == fresh.pixels, "dirty result frame must equal fresh frame");
        rgb0 = {RGBColor::Red, RGBColorStatus::Matched, 1};
        display.show(rgb0, rgb1); freshDisplay.invalidate(); freshDisplay.show(rgb0, rgb1);
        require(screen.pixels == fresh.pixels && screen.fullClears == 1, "shorter color/status names leave no ghost text or full clear");
        screen.operations.clear(); rgb1 = {RGBColor::Unknown, RGBColorStatus::Offline, 0};
        display.show(rgb0, rgb1);
        for (const auto &op : screen.operations)
            require(op.y >= 92 && op.y + op.h <= 118, "RGB1 updates cannot overwrite RGB0");
        screen.save(output + "/color-display-offline.ppm");
        Adafruit_ST7735 portrait(128, 160);
        ColorDisplay<Adafruit_ST7735> portraitDisplay(portrait);
        portraitDisplay.show({RGBColor::Unknown, RGBColorStatus::Unknown, 0},
                             {RGBColor::Unknown, RGBColorStatus::Ambiguous, 2});
        portrait.save(output + "/color-display-portrait.ppm");
        std::cout << "PASS reusable TFT view, no-op redraw, per-sensor fields, shrinking text and landscape/portrait bounds\n";

        fakeMillis = 100;
        beginRGBColorDisplay();
        tft_.operations.clear();
        for (RGBChannel ch : {RGB0, RGB1}) {
            RGBReading sample; sample.connected = sample.sampled = true;
            sample.rawRed = 100; sample.rawGreen = 200; sample.rawBlue = 300; sample.clear = 600;
            sample.sampledAt = fakeMillis; PuppyBotRGBDetail::publish(ch, sample);
        }
        const int reads0 = tcs.reads, reads1 = tcsB.reads;
        const int probes0 = Wire.probes, probes1 = Wire1.probes;
        fakeMillis = 199; showRGBColor();
        require(tft_.operations.empty(), "default wrapper throttles updates until 100ms");
        fakeMillis = 200; showRGBColor();
        require(!tft_.operations.empty(), "wrapper updates cached status after 100ms");
        require(reads0 == tcs.reads && reads1 == tcsB.reads && probes0 == Wire.probes && probes1 == Wire1.probes,
                "display wrapper cannot read I2C");
        tft_.operations.clear(); fakeMillis = 300; showRGBColor();
        require(tft_.operations.empty(), "unchanged cached results draw nothing even after refresh interval");
        fakeMillis = 350; showRGBColor(RGBColorOptions{}, RGBColorOptions{}, 0);
        require(!tft_.operations.empty(), "zero interval permits immediate stale-status update");
        const int oldClears = tft_.fullClears;
        beginRGBColorDisplay();
        require(tft_.fullClears == oldClears + 1, "begin invalidates the page when returning from another screen");
        fakeMillis = 0xFFFFFFF0u; beginRGBColorDisplay(); tft_.operations.clear();
        fakeMillis = 0x54u; for (RGBChannel ch : {RGB0, RGB1}) {
            RGBReading sample; sample.connected = sample.sampled = true;
            sample.rawRed = 100; sample.rawGreen = 200; sample.rawBlue = 300; sample.clear = 600;
            sample.sampledAt = fakeMillis; PuppyBotRGBDetail::publish(ch, sample);
        } showRGBColor();
        require(!tft_.operations.empty(), "refresh works across millis wrap");
        std::cout << "PASS begin/show APIs, cached-only reads, configurable refresh, stale feedback and clock wrap\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n'; return 1;
    }
}
