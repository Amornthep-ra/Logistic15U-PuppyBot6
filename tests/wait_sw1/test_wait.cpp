#include <iostream>
#include <stdexcept>
#include <climits>
#include <Arduino.h>
#include <Adafruit_ST7735.h>
int swpin = 27;
float pitch_loop1 = 12.3f, roll_loop1 = -45.6f, yaw_loop1 = 270.5f, Con_yaw_loop1 = -720.5f;
Adafruit_ST7735 tft_;
std::vector<int> analogPins, adcChannels;
int gpioValues[48] = {};
constexpr int GPIO_OUT = 1;
void gpio_init(int) {}
void gpio_set_dir(int, int) {}
void gpio_put(int pin, int value) { gpioValues[pin] = value; }
void delayMicroseconds(int) {}
int constrain(int value, int minimum, int maximum) { return std::max(minimum, std::min(value, maximum)); }
int analogRead(int pin) { analogPins.push_back(pin); return 2000 + pin; }
struct Adc { int readADC(int channel) { adcChannels.push_back(channel); return 1000 + channel; } } adc;
int Sensor_mode_F = 0, Sensor_mode_B = 0, PID_NumPin = 3, PID_NumPin_B = 3;
int PID_SetupPin[16] = {}, PID_SetupPin_B[16] = {};
int PID_Min[16] = {}, PID_Max[16] = {}, PID_Min_B[16] = {}, PID_Max_B[16] = {};
int Front_color = 0, Back_color = 0;
float previous_error = 0, previous_error_B = 0;
uint32_t _lastPosition = 0, _lastPosition_B = 0;
void ao() {}
#include "ActualAdc.h"
#include "PuppyBotWaitSW1.h"
TwoWire Wire, Wire1;
Adafruit_TCS34725 tcs, tcsB;
int TCS_R=0,TCS_G=0,TCS_B=0,TCS_T=0,TCS_L=0;
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
bool between(uint32_t start) { return fakeMillis >= start && fakeMillis < start + 60; }
bool boots() {
    if (fakeMillis > 5000) throw std::runtime_error("wait did not return after Start");
    return between(100) || between(420) || between(740) || between(900) || between(1060) || between(1220);
}
int switches(int) { return between(260) || between(580) || between(1380) ? LOW : HIGH; }
std::vector<std::pair<uint32_t, bool>> script;
uint32_t scriptEnd = 0, lastFrame = 0;
std::string previewPath;
bool savedExitPreview = false;
bool scriptedButton(bool select) {
    if (fakeMillis > scriptEnd + 1000) throw std::runtime_error("calibration script did not return");
    for (const auto &event : script)
        if (event.second == select && between(event.first)) return true;
    return false;
}
bool scriptedBoot() { return scriptedButton(false); }
int scriptedSW1(int) { return scriptedButton(true) ? LOW : HIGH; }
void simulateCore1() {
    using namespace PuppyBotRGBDetail;
    if (uint32_t(fakeMillis - lastFrame) >= 55) {
        lastFrame = fakeMillis;
        for (auto channel : {RGB0, RGB1}) {
            RGBReading live;
            live.rawRed = channel == RGB0 ? 758 : 900;
            live.rawGreen = 212; live.rawBlue = 229; live.clear = 1059;
            live.connected = live.sampled = true; live.sampledAt = fakeMillis;
            live.sequence = getRGB(channel).sequence + 1;
            publish(channel, live);
        }
    }
    if (!savedExitPreview && rgbModeActive && calibrationMenu.choice() == 8 && calibration.dirty()) {
        tft_.save(previewPath + "/calibration-exit-dirty.ppm");
        savedExitPreview = true;
    }
}
void calibrationJourney(const std::string &out) {
    using namespace PuppyBotRGBDetail;
    fakeMillis = 0; lastFrame = 0; previewPath = out;
    uint32_t at = 100;
    const auto click = [&](bool select) { script.emplace_back(at, select); at += 180; };
    for (int i = 0; i < 4; ++i) click(false);
    click(true); // Main -> Calibrate.
    click(true); click(true); at += 1300; // RGB0 -> Red -> 20 fresh frames.
    for (int i = 0; i < 6; ++i) click(false);
    click(true); // Save RGB0.
    click(false); click(true); // Sensor menu.
    click(false); click(true); click(true); at += 1300; // RGB1 -> Red -> capture.
    for (int i = 0; i < 8; ++i) click(false);
    click(true); // Exit with unsaved RGB1, no implicit commit.
    click(true); // Re-enter, retaining RGB1 in RAM.
    click(false); click(true); // Sensor RGB1 -> Save all.
    click(false); click(true); // Exit.
    click(false); click(false); click(true); // Main -> Start (six items).
    scriptEnd = at;
    fakeBootHook = scriptedBoot; fakeDigitalReadHook = scriptedSW1; fakeDelayHook = simulateCore1;
    const int commits = EEPROM.commits;
    wait_SW1();
    fakeBootHook = nullptr; fakeDigitalReadHook = nullptr; fakeDelayHook = nullptr;
    require(fakeMillis >= at - 80 && fakeMillis < at + 200, "calibration Exit returns to menu; only final Start releases gate");
    require(EEPROM.commits == commits + 2, "only the two explicit Save actions commit flash");
    require(getRGBCalibration(RGB0, RGBColor::Red).samples == 20 &&
            getRGBCalibration(RGB1, RGBColor::Red).samples == 20, "both channels capture during blocking wait");
    RGBCalibration restored;
    require(loadCalibration(restored, EEPROM) && restored.get(RGB0, RGBColor::Red).mean.red == 758 &&
            restored.get(RGB1, RGBColor::Red).mean.red == 900, "Exit/re-entry preserves RAM and Save persists both sensors");
    const auto reads = fakeBootReads; const int clears = tft_.fullClears;
    updateRGBMode();
    require(!rgbModeActive && !calibrationSW1Enabled && reads == fakeBootReads && clears == tft_.fullClears,
            "leaving calibration disables its button and TFT owner");
    require(Wire.probes == 0 && Wire1.probes == 0, "menu never reads I2C on the TFT core");
    require(savedExitPreview, "unsaved Exit was displayed before returning to main");
    std::cout << "PASS blocking wait: RGB0/RGB1 fresh captures, explicit Save, dirty Exit/re-entry, persistence and exclusive screen/input ownership\n";
}
int main(int argc, char **argv) {
    try {
        require(argc == 2, "output folder required");
        const std::string out = argv[1];
        using namespace PuppyBotWaitDetail;
        using PuppyBotInput::ButtonEvent;
        Menu menu;
        require(menu.handle(ButtonEvent::Select), "Start is selected first");
        menu.handle(ButtonEvent::Next);
        require(!menu.handle(ButtonEvent::Select) && menu.page == Page::Adc, "second menu item opens ADC");
        require(!menu.handle(ButtonEvent::Next) && menu.page == Page::Mux, "BOOT advances detail page");
        require(!menu.handle(ButtonEvent::Select) && menu.page == Page::Menu && menu.selection == 2, "Exit retains matching menu selection");
        menu.handle(ButtonEvent::Next); menu.handle(ButtonEvent::Next);
        require(!menu.handle(ButtonEvent::Select) && menu.page == Page::Calibrate, "fifth item opens calibration without starting");
        require(!menu.handle(ButtonEvent::Next) && !menu.handle(ButtonEvent::Select) && menu.page == Page::Calibrate, "parent menu cannot consume calibration buttons");
        menu = Menu{};
        for (int i = 0; i < 6; ++i) menu.handle(ButtonEvent::Next);
        require(menu.selection == 0, "six-item main menu wraps");

        Readings values;
        sample(Page::Menu, values);
        require(analogPins.empty() && adcChannels.empty(), "menu does not scan sensors");
        sample(Page::Adc, values);
        require(adcChannels.size() == 8 && analogPins.size() == 8, "all 16 ADC channels sampled");
        for (int i = 0; i < 8; ++i) require(adcChannels[i] == i && analogPins[i] == 40+i, "ADC routing matches original code");
        Sensor_mode_F = 0; Sensor_mode_B = 2; PID_NumPin = 16;
        analogPins.clear(); sample(Page::Mux, values);
        require(analogPins.size() == 16 && Sensor_mode_F == 0 && Sensor_mode_B == 2 && PID_NumPin == 16, "MUX page reads both banks without changing line settings");
        for (int i = 0; i < 8; ++i) {
            PuppyBotAdcDetail::readMuxFront(i, 8);
            require((gpioValues[41] + 2*gpioValues[42] + 4*gpioValues[43]) == 7-i, "original front eight-channel mapping");
            PuppyBotAdcDetail::readMuxBack8(i);
            require((gpioValues[45] + 2*gpioValues[46] + 4*gpioValues[47]) == 7-i, "original back eight-channel mapping");
        }
        Sensor_mode_F = 1; PID_NumPin = 16; ADC_F(0);
        require(gpioValues[44] == 1 && gpioValues[43] == 1, "legacy front 16-channel path remains intact");
        Sensor_mode_B = 2; ADC_B(15);
        require(analogPins.back() == 45 && gpioValues[15] == 1 && gpioValues[16] == 1, "legacy back 16-channel path remains intact");
        PuppyBotIMU::init_encoders(); FakeEncoder::counts[17] = INT_MIN; FakeEncoder::counts[19] = INT_MAX;
        sample(Page::Motion, values);
        require(values.angles[0] == pitch_loop1 && values.angles[3] == Con_yaw_loop1 && values.encoders[0] == INT_MIN && values.encoders[1] == INT_MAX, "IMU cache and independent signed encoder counts");
        require(FakeEncoder::starts[17] == 1 && FakeEncoder::starts[19] == 1 && FakeEncoder::resets[17] == 0 && FakeEncoder::resets[19] == 0, "diagnostics never reset encoders");

        for (bool portrait : {false, true}) {
            values.adc[0] = 1000; values.encoders[0] = INT_MIN; values.angles[0] = 12.3f;
            Adafruit_ST7735 display(portrait ? 128 : 160, portrait ? 160 : 128);
            Screen<Adafruit_ST7735> screen(display);
            Menu state;
            for (int page = 0; page < 4; ++page) {
                state.page = Page(page); screen.show(state, values);
                display.save(out + (portrait ? "/portrait-" : "/landscape-") + std::to_string(page) + ".ppm");
                display.operations.clear(); screen.show(state, values);
                require(display.operations.empty(), "unchanged page has no redraw");
            }
            state.page = Page::Menu;
            for (int selected = 0; selected < 6; ++selected) {
                state.selection = selected; screen.show(state, values);
                Adafruit_ST7735 clean(display.width(), display.height()); Screen<Adafruit_ST7735> cleanScreen(clean);
                cleanScreen.show(state, values);
                require(display.pixels == clean.pixels, "menu highlight updates match clean frame");
                display.save(out + (portrait ? "/portrait-menu-" : "/landscape-menu-") + std::to_string(selected) + ".ppm");
            }
            // Render every new sensor/color action on both supported display orientations.
            RGBCalibration data; PuppyBotRGBDetail::CalibrationMenu rgbMenu;
            rgbMenu.begin(data, true);
            PuppyBotRGBDetail::CalibrationDisplay<Adafruit_ST7735> rgbScreen(display);
            for (int choice = 0; choice < 4; ++choice) {
                rgbScreen.show(data, RGBReading{}, &rgbMenu);
                require(rgbMenu.sensorChoice() == choice, "RGB0/RGB1/Save/Exit ordering");
                if (choice == 2) {
                    require(rgbMenu.handle(ButtonEvent::Select, data) == RGBCalibrationAction::None,
                            "new top-level Save rejects empty records");
                    rgbScreen.show(data, RGBReading{}, &rgbMenu);
                }
                if (choice == 3) require(rgbMenu.handle(ButtonEvent::Select, data) == RGBCalibrationAction::Exit,
                                         "Exit works even with disconnected/empty sensors");
                rgbMenu.handle(ButtonEvent::Next, data);
            }
            require(rgbMenu.sensorChoice() == 0, "sensor selector wraps");
            rgbMenu.handle(ButtonEvent::Select, data);
            for (int choice = 0; choice < 9; ++choice) {
                rgbScreen.show(data, RGBReading{}, &rgbMenu);
                Adafruit_ST7735 clean(display.width(), display.height());
                PuppyBotRGBDetail::CalibrationDisplay<Adafruit_ST7735> cleanScreen(clean);
                cleanScreen.show(data, RGBReading{}, &rgbMenu);
                require(display.pixels == clean.pixels, "calibration action updates match clean frame in both orientations");
                if (choice == 8) {
                    display.save(out + (portrait ? "/portrait-calibrate-exit.ppm" : "/landscape-calibrate-exit.ppm"));
                    require(rgbMenu.handle(ButtonEvent::Select, data) == RGBCalibrationAction::Exit, "Exit available directly from color list");
                }
                rgbMenu.handle(ButtonEvent::Next, data);
            }
            screen.invalidate();
            state.page = Page::Adc; screen.show(state, values);
            const int clears = display.fullClears;
            values.adc[0] = 7; display.operations.clear(); screen.show(state, values);
            require(!display.operations.empty() && display.fullClears == clears, "shrinking ADC updates without full clear");
            for (const auto &op : display.operations) require(op.x < display.width()/2 && op.y == 26 && op.h == 8, "only changed ADC field is repainted");
            Adafruit_ST7735 fresh(display.width(), display.height()); Screen<Adafruit_ST7735> reference(fresh); reference.show(state, values);
            require(display.pixels == fresh.pixels, "shrinking value leaves no stale digits");
            state.page = Page::Motion; screen.show(state, values); values.encoders[0] = -1; values.angles[0] = 0;
            screen.show(state, values); reference.show(state, values);
            require(display.pixels == fresh.pixels, "shrinking signed encoder and angle leave no stale digits");
        }
        fakeMillis = 0; fakeBootHook = boots; fakeDigitalReadHook = switches;
        wait_SW1();
        fakeBootHook = nullptr; fakeDigitalReadHook = nullptr;
        require(fakeMillis >= 1480 && fakeMillis < 1660, "full blocking menu returns only after explicit Start");
        require(fakeSwitchMode == INPUT_PULLUP, "SW1 remains an input after return");
        require(FakeEncoder::counts[17] == INT_MIN && FakeEncoder::counts[19] == INT_MAX, "revisiting diagnostics preserves counts");
        require(fakeBootReads <= fakeMillis / 20 + 1, "BOOT polling is limited to 20ms");
        calibrationJourney(out);
        std::cout << "PASS menu/Exit/Start flow, actual ADC/MUX routing, signed encoders without resets, partial TFT redraw, both orientations and blocking input integration\n";
    } catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
