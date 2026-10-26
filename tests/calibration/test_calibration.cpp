#include <iostream>
#include <sstream>
#include <stdexcept>
#include "PuppyBotRGBCalibrationDisplay.h"

TwoWire Wire, Wire1;
Adafruit_TCS34725 tcs, tcsB;
Adafruit_ST7735 tft_;
int TCS_R = 0, TCS_G = 0, TCS_B = 0, TCS_T = 0, TCS_L = 0;
int swpin = 27;

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
void frame(RGBChannel channel, uint16_t r, uint16_t g, uint16_t b, uint16_t c, uint32_t advance = 50) {
    fakeMillis += advance;
    RGBReading reading;
    reading.rawRed = r; reading.rawGreen = g; reading.rawBlue = b; reading.clear = c;
    reading.connected = (channel == RGB0 ? Wire : Wire1).connected;
    reading.sampled = true; reading.sampledAt = fakeMillis;
    reading.sequence = getRGB(channel).sequence + 1;
    PuppyBotRGBDetail::publish(channel, reading);
}
void capture(RGBCalibration &data, RGBChannel channel, RGBColor color, uint16_t base) {
    require(data.start(channel, color, 3), "start capture");
    for (unsigned i = 0; i < 3; ++i) {
        frame(channel, base + i, base + i + 10, base + i + 20, base + i + 30);
        data.update();
    }
    require(data.state() == RGBCaptureState::Captured, "capture complete");
}
struct Output {
    std::string text;
    void println(const char *line) { text += line; text += '\n'; }
};

int main(int argc, char **argv) {
    try {
        require(argc == 2, "output folder required");
        const std::string output = argv[1];
        RGBCalibration data;
        require(!PuppyBotRGBDetail::loadCalibration(data, EEPROM), "blank EEPROM must not invent references");
        require(!PuppyBotRGBDetail::saveCalibration(data, EEPROM) && EEPROM.commits == 0, "empty data cannot be saved");
        require(!data.start(RGBChannel(2), RGBColor::Red) && !data.start(RGB0, RGBColor(6)) &&
                !data.start(RGB0, RGBColor::Red, 0) && !data.start(RGB0, RGBColor::Red, 1001), "invalid capture requests");
        frame(RGB0, 999, 999, 999, 999);
        require(data.start(RGB0, RGBColor::Red, 3), "start selected sensor");
        data.update(); data.update();
        require(data.count() == 0, "cached frame preceding start must not count");
        require(!data.select(RGB1, RGBColor::Black) && !data.start(RGB1, RGBColor::Black), "capture selection is locked");
        require(!PuppyBotRGBDetail::saveCalibration(data, EEPROM) && !PuppyBotRGBDetail::loadCalibration(data, EEPROM), "no storage operations mid capture");
        frame(RGB1, 500, 500, 500, 500); data.update();
        require(data.count() == 0, "other sensor cannot advance capture");
        for (unsigned i = 0; i < 3; ++i) {
            frame(RGB0, 10 + i * 10, 20 + i * 10, 30 + i * 10, 4000 + i * 1000, 0);
            data.update(); data.update();
            require(data.count() == i + 1, "count one per new sequence even with identical timestamps");
        }
        const auto reference = data.get(RGB0, RGBColor::Red);
        require(reference.samples == 3 && reference.mean.red == 20 && reference.mean.green == 30 &&
                reference.mean.blue == 40 && reference.mean.clear == 5000 && reference.minimum.clear == 4000 &&
                reference.maximum.clear == 6000, "raw means/min/max, independent raw Clear");
        std::cout << "PASS raw RGBC statistics, distinct-frame counting, sensor isolation and capture guards\n";

        require(data.start(RGB0, RGBColor::Red, 3), "recapture");
        frame(RGB0, 10, 20, 30, 40); data.update(); data.cancel();
        require(data.state() == RGBCaptureState::Cancelled && data.get(RGB0, RGBColor::Red).mean.clear == 5000, "cancel must preserve old reference");
        data.start(RGB0, RGBColor::Red); Wire.connected = false;
        frame(RGB0, 1, 1, 1, 1); data.update();
        require(data.state() == RGBCaptureState::Offline && data.get(RGB0, RGBColor::Red).mean.red == 20, "disconnect preserves reference");
        Wire.connected = true; fakeMillis += 1001;
        frame(RGB0, 1, 1, 1, 1);
        data.start(RGB0, RGBColor::Red); fakeMillis += 2000; data.update();
        require(data.state() == RGBCaptureState::TimedOut && data.get(RGB0, RGBColor::Red).mean.red == 20, "stopped producer timeout");
        fakeMillis = 0xFFFFFFF0u;
        data.start(RGB0, RGBColor::Black, 1);
        frame(RGB0, 0, 0, 0, 0, 50); data.update();
        require(data.state() == RGBCaptureState::InvalidSignal && data.get(RGB0, RGBColor::Black).samples == 0,
                "zero signal must not become a black reference across clock wrap");
        require(std::string(data.rejectionMessage()) == "RGB zero: retry", "zero RGB reason");
        data.start(RGB0, RGBColor::Red, 1); frame(RGB0, 21504, 100, 100, 1000); data.update();
        require(data.state() == RGBCaptureState::InvalidSignal && data.get(RGB0, RGBColor::Red).mean.red == 20,
                "saturation preserves previous reference");
        require(std::string(data.rejectionMessage()) == "Saturated: retry", "saturation reason");
        data.start(RGB0, RGBColor::Red, 1); frame(RGB0, 2, 4, 6, 0); data.update();
        require(data.state() == RGBCaptureState::InvalidSignal &&
                std::string(data.rejectionMessage()) == "Clear <10: retry", "reported 2/4/6/0 case identifies Clear");
        require(data.rejectedReading().red == 2 && data.rejectedReading().green == 4 &&
                data.rejectedReading().blue == 6 && data.rejectedReading().clear == 0,
                "retain exact failed frame for TFT even after live values recover");
        Adafruit_ST7735 diagnosticScreen;
        PuppyBotRGBDetail::CalibrationDisplay<Adafruit_ST7735> diagnosticDisplay(diagnosticScreen);
        diagnosticDisplay.show(data, getRGB(RGB0));
        diagnosticScreen.save(output + "/calibration-clear-zero.ppm");
        diagnosticScreen.operations.clear();
        frame(RGB0, 500, 100, 150, 800); data.update();
        diagnosticDisplay.show(data, getRGB(RGB0));
        require(diagnosticScreen.operations.empty() && data.get(RGB0, RGBColor::Red).mean.red == 20,
                "live recovery cannot erase failed-frame evidence or replace saved reference");
        Adafruit_ST7735 diagnosticPortrait(128, 160);
        PuppyBotRGBDetail::CalibrationDisplay<Adafruit_ST7735> portraitDiagnosticDisplay(diagnosticPortrait);
        portraitDiagnosticDisplay.show(data, getRGB(RGB0));
        require(data.start(RGB0, RGBColor::Red, 1), "retry begins");
        diagnosticDisplay.show(data, getRGB(RGB0));
        Adafruit_ST7735 resetReference;
        PuppyBotRGBDetail::CalibrationDisplay<Adafruit_ST7735> resetDisplay(resetReference);
        resetDisplay.show(data, getRGB(RGB0));
        require(diagnosticScreen.pixels == resetReference.pixels, "retry restores LIVE without stale FAIL text/digits");
        data.cancel();
        std::cout << "PASS exact rejected frame and separate reasons, frozen TFT, portrait bounds and retry redraw\n";
        std::cout << "PASS cancellation/disconnect/timeout, invalid-light rejection and clock wrap\n";

        for (unsigned channel = 0; channel < 2; ++channel) for (unsigned color = 0; color < 6; ++color)
            capture(data, RGBChannel(channel), RGBColor(color), 100 * channel + 10 * color);
        require(data.completed(RGB0) == 6 && data.completed(RGB1) == 6 && data.dirty(), "12 independent references");
        EEPROM.buffer[0] = 0x23; EEPROM.buffer[4095] = 0xA7;
        require(PuppyBotRGBDetail::saveCalibration(data, EEPROM) && !data.dirty(), "explicit save");
        require(EEPROM.flash[0] == 0x23 && EEPROM.flash[4095] == 0xA7, "preserve unrelated EEPROM bytes");
        RGBCalibration restored;
        require(PuppyBotRGBDetail::loadCalibration(restored, EEPROM), "new object loads after restart");
        for (unsigned channel = 0; channel < 2; ++channel) for (unsigned color = 0; color < 6; ++color) {
            const auto r = restored.get(RGBChannel(channel), RGBColor(color));
            require(r.samples == 3 && r.mean.red == 100 * channel + 10 * color + 1 &&
                    r.mean.clear == 100 * channel + 10 * color + 31, "all 12 reference records round trip");
        }
        Output csv; PuppyBotRGBDetail::printCalibration(restored, csv);
        require(csv.text.find("RGB1,Black,3,151,161,171,181,150,160,170,180,152,162,172,182") != std::string::npos,
                "CSV actual raw means and ranges");
        require(std::count(csv.text.begin(), csv.text.end(), '\n') == 13, "CSV header plus 12 measured rows");
        capture(data, RGB0, RGBColor::Red, 777);
        EEPROM.fail = true;
        require(!PuppyBotRGBDetail::saveCalibration(data, EEPROM) && data.dirty() &&
                data.storage() == RGBStorageState::Failed, "failed write retains dirty references");
        EEPROM.fail = false;
        require(PuppyBotRGBDetail::loadCalibration(restored, EEPROM) && restored.get(RGB0, RGBColor::Red).mean.red == 1,
                "failed save does not replace committed data");
        EEPROM.flash[PuppyBotRGBDetail::calibrationOffset + 42] ^= 1;
        require(!PuppyBotRGBDetail::loadCalibration(restored, EEPROM) && restored.completed(RGB0) == 6 &&
                restored.get(RGB0, RGBColor::Red).mean.red == 1, "CRC rejects corruption without replacing RAM");
        uint8_t bytes[RGBCalibration::encodedSize]; data.encode(bytes); bytes[4] = 9;
        require(!restored.decode(bytes), "unsupported schema rejected");
        std::cout << "PASS 12-record persistent round trip, CSV, EEPROM preservation, failed writes, CRC and version guards\n";

        RGBCalibration partial, partialReload;
        EEPROMClass partialStore;
        require(partial.start(RGB1, RGBColor::Black), "default capture starts");
        for (unsigned i = 0; i < 20; ++i) {
            frame(RGB1, 5, 7, 6, 20); partial.update(); partial.update();
            if (i < 19) require(partial.get(RGB1, RGBColor::Black).samples == 0, "incomplete record stays uncalibrated");
        }
        require(partial.get(RGB1, RGBColor::Black).samples == 20 && partial.completed(RGB0) == 0 &&
                partial.completed(RGB1) == 1, "default 20 frames and single-sensor calibration");
        require(PuppyBotRGBDetail::saveCalibration(partial, partialStore) &&
                PuppyBotRGBDetail::loadCalibration(partialReload, partialStore), "partial references persist");
        require(partialReload.get(RGB1, RGBColor::Black).samples == 20 &&
                partialReload.get(RGB1, RGBColor::Black).mean.clear == 20 &&
                partialReload.get(RGB0, RGBColor::Black).samples == 0, "measured zero and missing data remain distinct after reload");
        std::cout << "PASS default 20-frame capture and persistent partial/single-sensor sets including valid dark samples\n";

        using PuppyBotRGBDetail::CalibrationDisplay;
        Adafruit_ST7735 screen;
        CalibrationDisplay<Adafruit_ST7735> display(screen);
        restored.select(RGB1, RGBColor::Yellow);
        RGBReading live = getRGB(RGB1);
        live.rawRed = 12345; live.rawGreen = 65535; live.rawBlue = 5; live.clear = 45678;
        display.show(restored, live); screen.save(output + "/calibration-rgb1-yellow.ppm");
        require(screen.fullClears == 1, "initial full draw");
        screen.operations.clear(); ++live.sampledAt; ++live.sequence;
        display.show(restored, live);
        require(screen.operations.empty(), "unchanged values must not redraw");
        live.clear = 9;
        display.show(restored, live);
        require(!screen.operations.empty(), "Clear changed must update");
        for (const auto &op : screen.operations)
            require(op.x >= 33 && op.x + op.w <= 63 && op.y >= 94 && op.y + op.h <= 102, "only changed raw Clear field redrawn");
        Adafruit_ST7735 fresh;
        CalibrationDisplay<Adafruit_ST7735> freshDisplay(fresh);
        freshDisplay.show(restored, live);
        require(screen.pixels == fresh.pixels, "shorter raw value leaves no ghost digits");
        restored.start(RGB1, RGBColor::Yellow, 20);
        for (unsigned i = 0; i < 7; ++i) { frame(RGB1, 10, 20, 30, 40); restored.update(); }
        display.show(restored, getRGB(RGB1)); screen.save(output + "/calibration-capturing.ppm");
        require(screen.fullClears == 1, "capture updates without screen clear");
        restored.cancel();
        restored.start(RGB1, RGBColor::Yellow); fakeMillis += 2000; restored.update();
        display.show(restored, live); screen.save(output + "/calibration-timeout.ppm");
        freshDisplay.invalidate(); freshDisplay.show(restored, live);
        require(screen.pixels == fresh.pixels, "status and progress updates leave no stale content");
        Adafruit_ST7735 portrait(128, 160);
        CalibrationDisplay<Adafruit_ST7735> portraitDisplay(portrait);
        portraitDisplay.show(restored, live); portrait.save(output + "/calibration-portrait.ppm");
        RGBCalibration empty;
        Adafruit_ST7735 waiting;
        CalibrationDisplay<Adafruit_ST7735> waitingDisplay(waiting);
        waitingDisplay.show(empty, RGBReading{}); waiting.save(output + "/calibration-empty.ppm");
        std::cout << "PASS TFT field-only redraw, pixel equivalence, capture/timeout/empty screens and landscape/portrait bounds\n";

        using PuppyBotRGBDetail::CalibrationButton;
        CalibrationButton button;
        const auto next = RGBButtonEvent::Next, select = RGBButtonEvent::Select;
        button.begin(false, 0);
        require(!button.update(true, 10) && !button.update(false, 20) &&
                !button.update(true, 25) && !button.update(false, 35) &&
                !button.update(false, 100), "contact bounce must not emit events");
        require(!button.update(true, 200) && !button.update(true, 230) &&
                !button.update(false, 400) && button.update(false, 430), "tap emits once after stable release");
        require(!button.update(false, 500), "released button must not repeat");
        button.update(true, 600); button.update(true, 630);
        require(!button.update(true, 1600) && !button.update(true, 3000), "holding never selects or repeats");
        button.update(false, 3010);
        require(button.update(false, 3040) && !button.update(false, 3070), "long press release emits exactly once");
        button.begin(true, 0);
        require(!button.update(true, 5000), "held at startup cannot select");
        button.update(false, 5010);
        require(!button.update(false, 5040), "startup release cannot select");
        button.begin(false, 0xFFFFFF00u);
        button.update(true, 0xFFFFFF10u); button.update(true, 0xFFFFFF30u);
        button.update(false, 5);
        require(button.update(false, 35), "release across millis wrap");
        PuppyBotRGBDetail::CalibrationInputs inputs;
        const auto none = RGBButtonEvent::None;
        require(inputs.update(true, true, 0) == none && inputs.update(true, true, 2000) == none &&
                inputs.update(false, false, 2040) == none, "startup held buttons cannot choose or save");
        inputs.update(true, false, 2100); inputs.update(true, false, 2140);
        inputs.update(false, false, 2200);
        require(inputs.update(false, false, 2240) == next, "BOOT click maps to Next");
        inputs.update(false, true, 2300); inputs.update(false, true, 2340);
        inputs.update(false, false, 2400);
        require(inputs.update(false, false, 2440) == select, "SW1 click maps to Select");
        inputs.update(true, false, 2500); inputs.update(true, false, 2540);
        require(inputs.update(true, true, 2560) == none && inputs.update(false, true, 2600) == none &&
                inputs.update(false, true, 2640) == none && inputs.update(false, false, 2700) == none &&
                inputs.update(false, false, 2740) == none, "overlapping presses cannot advance or confirm");
        std::cout << "PASS button bounce, release events, no hold repeats, startup hold and clock wrap\n";

        using PuppyBotRGBDetail::CalibrationMenu;
        RGBCalibration menuData;
        CalibrationMenu menu;
        menu.begin(menuData);
        require(menu.focus() == RGBCalibrationFocus::Sensor, "menu starts at sensor selection");
        Adafruit_ST7735 menuScreen;
        CalibrationDisplay<Adafruit_ST7735> menuDisplay(menuScreen);
        menuDisplay.show(menuData, getRGB(RGB0), &menu); menuScreen.save(output + "/sw1-sensor.ppm");
        menu.handle(next, menuData); menu.handle(select, menuData);
        require(menuData.channel() == RGB1 && menu.focus() == RGBCalibrationFocus::Color && menu.choice() == 0,
                "Next chooses sensor and Select enters color menu");
        menuDisplay.show(menuData, getRGB(RGB1), &menu); menuScreen.save(output + "/sw1-red-ready.ppm");
        for (unsigned color = 0; color < 6; ++color) {
            require(menu.choice() == color && menuData.color() == RGBColor(color), "next color selected");
            require(menu.handle(select, menuData) == RGBCalibrationAction::None && menuData.collecting(), "Select starts chosen color capture");
            menu.handle(next, menuData); menu.handle(select, menuData);
            require(menu.choice() == color && menuData.collecting(), "menu cannot change during capture");
            for (unsigned i = 0; i < 20; ++i) {
                frame(RGB1, 10 + color * 30 + i, 10 + color * 20, 10 + color * 10, 30 + color * 100 + i);
                menuData.update();
            }
            require(menuData.get(RGB1, RGBColor(color)).samples == 20, "button-selected color captured");
            menu.handle(next, menuData);
        }
        require(menu.choice() == 6 && std::string(menu.message(menuData)) == "SW1: save flash", "Save all follows six colors");
        menuDisplay.show(menuData, getRGB(RGB1), &menu); menuScreen.save(output + "/sw1-save-menu.ppm");
        require(menu.handle(select, menuData) == RGBCalibrationAction::Save, "Select Save all requests storage");
        EEPROMClass menuStore;
        menu.saved(PuppyBotRGBDetail::saveCalibration(menuData, menuStore));
        require(!menuData.dirty() && std::string(menu.message(menuData)) == "Saved to flash", "save acknowledged on TFT");
        menuDisplay.show(menuData, getRGB(RGB1), &menu); menuScreen.save(output + "/sw1-saved.ppm");
        capture(menuData, RGB1, RGBColor::Black, 200);
        menu.synchronize(menuData);
        require(menuData.dirty() && std::string(menu.message(menuData)) == "SW1: save flash",
                "Serial recapture of same color must clear stale Saved message");
        menu.handle(next, menuData);
        require(menu.choice() == 7, "Sensor menu follows Save all");
        menu.handle(select, menuData); menu.handle(next, menuData); menu.handle(select, menuData);
        require(menu.focus() == RGBCalibrationFocus::Color && menuData.channel() == RGB0 && menu.choice() == 0,
                "return to sensor menu and begin other sensor at Red");
        for (unsigned i = 0; i < 6; ++i) menu.handle(next, menuData);
        require(menu.choice() == 6, "reach Save all without capturing RGB0");
        menuData.storageResult(false, true); menu.saved(false);
        menuDisplay.show(menuData, getRGB(RGB0), &menu); menuScreen.save(output + "/sw1-save-failed.ppm");
        menu.handle(next, menuData);
        require(std::string(menu.message(menuData)) == "SW1: sensor menu", "navigation dismisses storage notice");
        menuData.select(RGB1, RGBColor::Green); menu.synchronize(menuData);
        require(menu.choice() == 1, "Serial selection synchronizes menu");
        CalibrationMenu emptyMenu; RGBCalibration noData;
        emptyMenu.begin(noData); emptyMenu.handle(select, noData);
        for (unsigned i = 0; i < 6; ++i) emptyMenu.handle(next, noData);
        require(emptyMenu.handle(select, noData) == RGBCalibrationAction::None &&
                std::string(emptyMenu.message(noData)) == "No samples to save", "empty save shows action message");
        Adafruit_ST7735 menuPortrait(128, 160);
        CalibrationDisplay<Adafruit_ST7735> menuPortraitDisplay(menuPortrait);
        menuPortraitDisplay.show(noData, RGBReading{}, &emptyMenu);
        menuPortrait.save(output + "/sw1-empty-portrait.ppm");
        require(menuScreen.fullClears == 1, "all SW1 menu transitions keep field-only drawing");
        Adafruit_ST7735 menuFresh;
        CalibrationDisplay<Adafruit_ST7735> menuFreshDisplay(menuFresh);
        menuDisplay.show(menuData, getRGB(RGB1), &menu);
        menuFreshDisplay.show(menuData, getRGB(RGB1), &menu);
        require(menuScreen.pixels == menuFresh.pixels, "button menu transitions match a fresh TFT frame");
        menuScreen.operations.clear(); menuDisplay.show(menuData, getRGB(RGB1), &menu);
        require(menuScreen.operations.empty(), "unchanged button menu must not redraw");
        fakeToneCalls.clear(); fakeToneStops.clear();
        fakeSwitchValue = HIGH; fakeBootPressed = false;
        const auto readsBeforeBegin = fakeBootReads;
        beginRGBCalibrationSW1();
        require(fakeBootReads == readsBeforeBegin, "begin during setup does not poll BOOTSEL");
        require(fakeSwitchPin == swpin && fakeSwitchMode == INPUT_PULLUP, "wrapper uses original swpin with pullup");
        updateRGBCalibrationSW1(); // Initialize button states after setup.
        const auto readsAfterPoll = fakeBootReads;
        for (int i = 0; i < 19; ++i) { ++fakeMillis; updateRGBCalibrationSW1(); }
        require(fakeBootReads == readsAfterPoll, "BOOTSEL is not read more often than every 20ms");
        ++fakeMillis; updateRGBCalibrationSW1();
        require(fakeBootReads == readsAfterPoll + 1, "BOOTSEL is read at the next scheduled poll");
        const auto releaseSwitch = [&]() {
            fakeMillis += 50; fakeSwitchValue = HIGH; updateRGBCalibrationSW1();
            fakeMillis += 40; updateRGBCalibrationSW1();
        };
        const auto pressSwitch = [&]() {
            fakeMillis += 50; fakeSwitchValue = LOW; updateRGBCalibrationSW1();
            fakeMillis += 40; updateRGBCalibrationSW1();
        };
        const auto tapSwitch = [&]() {
            fakeMillis += 50; updateRGBCalibrationSW1(); // Re-arm after a capture.
            fakeMillis += 50; fakeBootPressed = true; updateRGBCalibrationSW1();
            fakeMillis += 40; updateRGBCalibrationSW1();
            fakeMillis += 50; fakeBootPressed = false; updateRGBCalibrationSW1();
            fakeMillis += 40; updateRGBCalibrationSW1();
        };
        tapSwitch();
        require(getRGBCalibrationState().channel() == RGB1, "BOOT tap advances sensor selection");
        require(fakeToneCalls.size() == 1 && fakeToneCalls.back().pin == PUPPYBOT_BUZZER_PIN &&
                fakeToneCalls.back().frequency == 1400 && fakeToneCalls.back().duration == 35,
                "Next produces a short lower-pitched beep on the original buzzer pin");
        pressSwitch();
        fakeMillis += 2000; updateRGBCalibrationSW1();
        require(PuppyBotRGBDetail::calibrationMenu.focus() == RGBCalibrationFocus::Sensor &&
                fakeToneCalls.size() == 1, "holding SW1 does not select before release");
        releaseSwitch();
        require(fakeToneCalls.size() == 2 && fakeToneCalls.back().frequency == 2600 && fakeToneCalls.back().duration == 80 &&
                !fakeToneStops.empty() && fakeToneStops.front() == PUPPYBOT_BUZZER_PIN,
                "SW1 release selects with distinct higher-pitched feedback");
        require(PuppyBotRGBDetail::calibrationMenu.focus() == RGBCalibrationFocus::Color &&
                !getRGBCalibrationState().collecting(), "one SW1 click enters color menu only");
        pressSwitch(); releaseSwitch();
        require(getRGBCalibrationState().collecting(), "second SW1 click starts selected color capture");
        const auto beforeIgnoredPress = fakeToneCalls.size();
        const auto readsDuringCapture = fakeBootReads;
        tapSwitch();
        require(fakeBootReads == readsDuringCapture, "capture never polls BOOTSEL");
        require(fakeToneCalls.size() == beforeIgnoredPress, "ignored press during capture must not play navigation feedback");
        for (unsigned i = 0; i < 20; ++i) {
            frame(RGB1, 10, 20, 30, 40);
            updateRGBCalibrationSW1(); updateRGBCalibration();
        }
        require(getRGBCalibration(RGB1, RGBColor::Red).samples == 20, "SW1 wrapper captures complete reference");
        require(fakeToneCalls.back().frequency == 2000 && fakeToneCalls.back().duration == 70,
                "completed capture starts a distinct two-note success sound");
        const auto firstSuccessNote = fakeToneCalls.size();
        updateRGBCalibration(); updateRGBCalibration();
        require(fakeToneCalls.size() == firstSuccessNote, "repeated updates cannot restart completion sound");
        fakeMillis += 70; updateRGBCalibration();
        require(fakeToneCalls.size() == firstSuccessNote, "completion sound has a silent gap");
        fakeMillis += 49; updateRGBCalibration();
        require(fakeToneCalls.size() == firstSuccessNote, "gap lasts 50ms");
        fakeMillis += 1; updateRGBCalibration();
        require(fakeToneCalls.size() == firstSuccessNote + 1 && fakeToneCalls.back().frequency == 3200 &&
                fakeToneCalls.back().duration == 110, "completion sound finishes with a higher note");
        const auto stopsBeforeSuccessEnds = fakeToneStops.size();
        fakeMillis += 110; updateRGBCalibration();
        require(fakeToneStops.size() == stopsBeforeSuccessEnds + 1, "completion sound releases buzzer resources");
        const auto successNotes = fakeToneCalls.size();
        fakeMillis += 1000; updateRGBCalibration();
        require(fakeToneCalls.size() == successNotes, "completed state does not repeat success sound");
        startRGBCalibration(RGB1, RGBColor::Green); cancelRGBCalibration(); updateRGBCalibration();
        require(fakeToneCalls.size() == successNotes, "cancelled capture must not sound successful");
        startRGBCalibration(RGB1, RGBColor::Green); Wire1.connected = false;
        frame(RGB1, 0, 0, 0, 0); updateRGBCalibration();
        require(getRGBCalibrationState().state() == RGBCaptureState::Offline && fakeToneCalls.size() == successNotes,
                "offline capture must not sound successful");
        Wire1.connected = true; fakeMillis += 1001; frame(RGB1, 10, 20, 30, 40);
        startRGBCalibration(RGB1, RGBColor::Green); fakeMillis += 2000; updateRGBCalibration();
        require(getRGBCalibrationState().state() == RGBCaptureState::TimedOut && fakeToneCalls.size() == successNotes,
                "timed-out capture must not sound successful");
        selectRGBCalibration(RGB1, RGBColor::Red);
        for (unsigned i = 0; i < 6; ++i) tapSwitch();
        require(PuppyBotRGBDetail::calibrationMenu.choice() == 6, "SW1 wrapper reaches Save all");
        const int priorCommits = EEPROM.commits;
        pressSwitch(); fakeMillis += 2000; updateRGBCalibrationSW1(); releaseSwitch();
        require(EEPROM.commits == priorCommits + 1 && !getRGBCalibrationState().dirty() &&
                std::string(PuppyBotRGBDetail::calibrationMenu.message(getRGBCalibrationState())) == "Saved to flash",
                "SW1 wrapper saves partial set exactly once per release");
        std::cout << "PASS BOOT/SW1 sensor/color/capture/save/back flow, Serial coexistence, empty/failure feedback and dirty TFT rendering\n";
        std::cout << "PASS distinct Next/Select buzzer tones, one beep per accepted event, timed cleanup and silence for ignored events\n";
        std::cout << "PASS one two-note completion sound per successful capture, silent gap/cleanup, no success on cancel/offline/timeout\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
