#include <iostream>
#include <stdexcept>
#include "PuppyBotRGBMode.h"

TwoWire Wire, Wire1;
Adafruit_TCS34725 tcs, tcsB;
Adafruit_ST7735 tft_;
int TCS_R = 0, TCS_G = 0, TCS_B = 0, TCS_T = 0, TCS_L = 0;
int swpin = 27;

void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
void frame(RGBChannel channel, uint16_t red) {
    fakeMillis += 55;
    RGBReading sample;
    sample.rawRed = red; sample.rawGreen = 212; sample.rawBlue = 229; sample.clear = 1059;
    sample.connected = sample.sampled = true;
    sample.sampledAt = fakeMillis;
    sample.sequence = getRGB(channel).sequence + 1;
    PuppyBotRGBDetail::publish(channel, sample);
}
void button(bool select) {
    fakeMillis += 50; updateRGBMode(); // Initialize or re-arm after a capture.
    fakeMillis += 50;
    if (select) fakeSwitchValue = LOW; else fakeBootPressed = true;
    updateRGBMode(); fakeMillis += 40; updateRGBMode();
    fakeMillis += 50;
    fakeSwitchValue = HIGH; fakeBootPressed = false;
    updateRGBMode(); fakeMillis += 40; updateRGBMode();
}

int main(int argc, char **) {
    try {
        const bool saved = argc > 1;
        if (saved) {
            RGBCalibration seed;
            for (auto channel : {RGB0, RGB1}) {
                require(seed.start(channel, RGBColor::Red, 3), "seed capture");
                for (int i = 0; i < 3; ++i) { frame(channel, channel == RGB0 ? 758 : 900); seed.update(); }
            }
            require(PuppyBotRGBDetail::saveCalibration(seed, EEPROM), "seed flash");
        }
        const int originalCommits = EEPROM.commits;
        updateRGBMode();
        require(tft_.fullClears == 0 && fakeToneCalls.empty(), "update before begin is inert");
        beginRGBMode(RGB_MODE_SHOW);
        const auto readsBeforeShow = fakeBootReads;
        require(getRGBCalibration(RGB0, RGBColor::Red).samples == (saved ? 3 : 0), "Show loads saved RGB0 or leaves missing data empty");
        require(getRGBCalibration(RGB1, RGBColor::Red).mean.red == (saved ? 900 : 0), "Show loads independent RGB1 data");
        button(true); button(false);
        require(fakeBootReads == readsBeforeShow, "Show never reads BOOTSEL");
        require(fakeToneCalls.empty() && !getRGBCalibrationState().collecting(), "Show ignores SW1 calibration actions");
        tft_.operations.clear(); fakeMillis += 100; updateRGBMode();
        require(tft_.operations.empty(), "unchanged Show issues no drawing commands");

        beginRGBMode(RGB_MODE_CALIBRATE);
        button(true); // Select RGB0.
        button(true); // Start Red.
        require(getRGBCalibrationState().collecting(), "Calibrate handles SW1 selection and starts capture");
        for (int i = 0; i < 20; ++i) { frame(RGB0, 758); updateRGBMode(); }
        require(getRGBCalibration(RGB0, RGBColor::Red).samples == 20 &&
                getRGBCalibration(RGB0, RGBColor::Red).mean.red == 758, "mode update captures all fresh frames");
        require(!fakeToneCalls.empty() && EEPROM.commits == originalCommits, "capture beeps but never autosaves");
        for (int i = 0; i < 6; ++i) button(false); // Save all.
        button(true);
        require(EEPROM.commits == originalCommits + 1 && !getRGBCalibrationState().dirty(), "SW1 Save all commits explicitly");

        require(startRGBCalibration(RGB0, RGBColor::Red, 3), "recapture");
        for (int i = 0; i < 3; ++i) { frame(RGB0, 800); updateRGBMode(); }
        require(startRGBCalibration(RGB0, RGBColor::Green), "start incomplete capture before mode switch");
        beginRGBMode(RGB_MODE_SHOW);
        require(!getRGBCalibrationState().collecting() && getRGBCalibrationState().dirty(), "switch cancels incomplete capture and retains unsaved records");
        require(getRGBCalibration(RGB0, RGBColor::Red).mean.red == 800, "switch cannot reload old flash over completed RAM references");
        const auto tones = fakeToneCalls.size();
        button(true); button(false);
        require(fakeToneCalls.size() == tones && EEPROM.commits == originalCommits + 1, "Show stops SW1 capture and save actions");
        beginRGBMode(RGB_MODE_CALIBRATE);
        require(getRGBCalibration(RGB0, RGBColor::Red).mean.red == 800, "return to Calibrate retains references");
        require(Wire.probes == 0 && Wire1.probes == 0 && fakeDelayCalls == 0, "mode APIs never perform I2C reads or integration delays");
        std::cout << "PASS " << (saved ? "saved" : "empty") << " flash startup, SW1 capture/save, Show isolation, mode switches and no I2C/delay\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n'; return 1;
    }
}
