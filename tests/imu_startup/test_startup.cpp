#include <Arduino.h>
#include <Adafruit_ST7735.h>
#include <cmath>
#include <iostream>
#include "PuppyBotIMUStartup.h"

int swpin = 27;
Adafruit_ST7735 tft_;
struct WireFake {
    int touches = 0;
    void setSDA(int) { ++touches; }
    void setSCL(int) { ++touches; }
    void setClock(int) { ++touches; }
    void begin() { ++touches; }
} Wire1;
struct SerialFake { void println(const char *) {} } Serial;
constexpr int I2C_ADDR = 0x69;
constexpr double PI = 3.141592653589793;
using std::isfinite;
namespace PuppyBotIMU {
    float read_Yaw = 0, offset_yaw = 0;
    struct Device {
        int calls = 0, begins = 0;
        bool connected = true, data = true, onlyFirst = false, invalid = false;
        bool begin(WireFake &, int) { ++begins; return connected; }
        int status() { return connected ? 0 : 2; }
        void update() { ++calls; }
        bool available() { return data && (!onlyFirst || calls == 1); }
        void clearAvailable() {}
        float x() { return invalid ? NAN : 1; }
        float y() { return 0; }
        float z() { return 0; }
        float w() { return 0; }
    } icm;
}
#include "ActualIMU.h"
#include "PuppyBotIMUStartupDisplay.h"
using namespace PuppyBotIMU;
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
void reset() {
    startup.state.store(StartupState::Waiting);
    startup.completed.store(0); startup.total.store(0); startup.samples.store(0); startup.error.store(0);
    icm = {}; Wire1.touches = 0; fakeMillis = 0; fakeDelayHook = nullptr;
    fakeDigitalReadHook = nullptr; fakeSwitchValue = HIGH; offset_yaw = 123;
    fakeToneCalls.clear(); fakeToneStops.clear();
}
void requireTone(unsigned frequency, unsigned long duration) {
    require(fakeToneCalls.size() == 1, "result tone must play exactly once");
    const auto &call = fakeToneCalls.front();
    require(call.pin == PUPPYBOT_BUZZER_PIN && call.frequency == frequency && call.duration == duration,
        "correct board buzzer, result pitch and duration");
    require(fakeToneStops.size() == 1 && fakeToneStops.front() == PUPPYBOT_BUZZER_PIN,
        "stop startup tone before returning ownership to menu");
}
void expire() { if (fakeMillis >= 50) startup.timeout(); }
int click(int) { return fakeMillis >= 200 && fakeMillis < 260 ? LOW : HIGH; }
int lateClick(int) { return fakeMillis >= 2000 && fakeMillis < 2060 ? LOW : HIGH; }
int heldThenClick(int) {
    return fakeMillis < 100 || (fakeMillis >= 200 && fakeMillis < 260) ? LOW : HIGH;
}
void coreReady() {
    if (fakeMillis <= 100) require(fakeToneCalls.empty(), "no completion tone while initializing/calibrating");
    if (fakeMillis == 20) require(startup.start(true), "start worker");
    if (fakeMillis == 30) require(startup.calibrate(300), "start calibration");
    if (fakeMillis == 100) require(startup.finish(true), "complete worker");
}
int main(int argc, char **argv) {
    try {
        require(argc == 2, "output directory");
        reset(); require(initIMU(), "connected init");
        require(updateIMU_fine_offset_Yaw(300), "fresh calibration ready");
        require(isIMUCalibrated() && fakeMillis == 3000 && startup.samples == 300, "full calibration window");
        require(offset_yaw == 180, "save original yaw formula/index order result");
        reset(); icm.connected = false;
        require(!initIMU() && getIMUStartupState() == StartupState::InitFailed, "missing device fails");
        require(startup.error == 2, "error code retained");
        reset(); require(!initIMU(false), "intentional no IMU");
        require(getIMUStartupState() == StartupState::Disabled && icm.begins == 0 && Wire1.touches == 0,
            "disabled performs no IMU I2C");
        for (int kind = 0; kind < 3; ++kind) {
            reset(); require(initIMU(), "init for no-data scenario");
            if (kind == 0) icm.data = false;
            if (kind == 1) icm.onlyFirst = true;
            if (kind == 2) icm.invalid = true;
            require(!updateIMU_fine_offset_Yaw(300), "absent, stale or nonfinite data must fail");
            require(getIMUStartupState() == StartupState::NoData && offset_yaw == 123, "no fabricated yaw reference");
        }
        reset(); require(initIMU(), "init before watchdog"); fakeDelayHook = expire;
        require(!updateIMU_fine_offset_Yaw(300) && fakeMillis == 50, "cancel calibration after timeout");
        require(getIMUStartupState() == StartupState::TimedOut && !startup.finish(true), "late success cannot overwrite timeout");
        reset(); startup.timeout(); require(!initIMU() && Wire1.touches == 0, "late worker cannot restart timed-out initialization");
        reset(); require(initIMU(), "init zero rounds");
        require(!updateIMU_fine_offset_Yaw(0), "zero rounds cannot report ready");

        reset(); fakeDelayHook = coreReady;
        require(waitIMUCalibration(500) && fakeMillis == 800, "show ready then automatically continue");
        requireTone(2400, 180);
        reset(); initIMU(false);
        require(!waitIMUCalibration() && fakeMillis == 700, "disabled exits without needing a press");
        require(fakeToneCalls.empty() && fakeToneStops.empty(), "deliberately disabled IMU is silent");
        reset(); icm.connected = false; initIMU(); fakeDigitalReadHook = heldThenClick;
        require(!waitIMUCalibration() && fakeMillis == 290, "held SW1 ignored; new click explicitly continues after failure");
        requireTone(440, 600); // SW1 works even before the scheduled tone finishes.
        reset(); fakeDigitalReadHook = click;
        require(!waitIMUCalibration(100) && fakeMillis == 290 && getIMUStartupState() == StartupState::TimedOut,
            "unresponsive worker times out; UI stays usable");
        requireTone(440, 600);
        reset(); startup.state.store(StartupState::NoData); fakeDigitalReadHook = click;
        require(!waitIMUCalibration() && fakeMillis == 290, "no-data also requires explicit acknowledgement");
        requireTone(440, 600);
        reset(); startup.state.store(StartupState::InitFailed); fakeDigitalReadHook = lateClick;
        require(!waitIMUCalibration() && fakeMillis == 2090, "error screen remains usable after sound ends");
        requireTone(440, 600); // Waiting on an error does not repeatedly trigger the buzzer.
        reset(); startup.state.store(StartupState::Ready);
        require(waitIMUCalibration() && fakeMillis == 700, "ready before UI opens still gets feedback");
        requireTone(2400, 180);
        reset(); initIMU(false); fakeMillis = UINT32_MAX - 200;
        require(!waitIMUCalibration() && fakeMillis == 499, "wait handles millis rollover");
        require(fakeBootReads == 0, "startup never polls BOOTSEL or its flash lockout");

        for (const auto &dims : {std::pair<int,int>{160,128}, {128,160}}) {
            Adafruit_ST7735 display(dims.first, dims.second);
            PuppyBotIMUStartupDetail::Screen<Adafruit_ST7735> screen(display);
            for (int state = 0; state <= int(StartupState::TimedOut); ++state) {
                screen.show(StartupState(state), 1500, 150, 300, 2);
                require(display.fullClears == 1, "status transitions never clear the whole screen");
                const auto count = display.operations.size(); const int clears = display.fullClears;
                screen.show(StartupState(state), 1500, 150, 300, 2);
                require(display.operations.size() == count, "identical status does not redraw");
                screen.show(StartupState(state), 1600, 160, 300, 2);
                require(display.fullClears == clears, "changed time/progress only redraws fields");
                display.save(std::string(argv[1]) + "/imu-" + std::to_string(dims.first) + "-" + std::to_string(state) + ".ppm");
            }
            // Every previous state must produce the same pixels as a fresh target frame:
            // this detects leftover long text, obsolete footer text and wrong bar colors.
            for (int from = 0; from <= int(StartupState::TimedOut); ++from) {
                for (int to = 0; to <= int(StartupState::TimedOut); ++to) {
                    Adafruit_ST7735 incremental(dims.first, dims.second), fresh(dims.first, dims.second);
                    PuppyBotIMUStartupDetail::Screen<Adafruit_ST7735> a(incremental), b(fresh);
                    a.show(StartupState(from), 99900, 270, 300, 123);
                    incremental.operations.clear();
                    a.show(StartupState(to), 100000, 30, 300, 2);
                    b.show(StartupState(to), 100000, 30, 300, 2);
                    require(incremental.pixels == fresh.pixels, "partial frame must match fresh frame with no stale pixels");
                    require(incremental.fullClears == 1, "only initial frame uses full clear");
                    for (const auto &op : incremental.operations)
                        require(op.h <= 8, "transition touches only glyph rows or progress strip");
                }
            }
            Adafruit_ST7735 changes(dims.first, dims.second);
            PuppyBotIMUStartupDetail::Screen<Adafruit_ST7735> changing(changes);
            changing.show(StartupState::Waiting, 1000, 0, 300, 0);
            changes.operations.clear();
            changing.show(StartupState::Initializing, 1000, 0, 300, 0);
            for (const auto &op : changes.operations) require(op.y == 31, "unchanged hint, footer and time are not redrawn");
            changing.show(StartupState::Calibrating, 1000, 150, 300, 0);
            changes.operations.clear();
            changing.show(StartupState::Calibrating, 1100, 150, 300, 0);
            require(changes.operations.size() == 1 && changes.operations[0].y == 64 && changes.operations[0].w == 6,
                "time digit update paints only one glyph and does not touch progress");
            changes.operations.clear();
            changing.show(StartupState::Calibrating, 1100, 153, 300, 0);
            const int oldWidth = (dims.first - 28) * 50 / 100;
            const int newWidth = (dims.first - 28) * 51 / 100;
            int barWrites = 0;
            for (const auto &op : changes.operations) if (op.y == 81) {
                ++barWrites;
                require(op.x == 14 + oldWidth && op.w == newWidth - oldWidth,
                    "progress growth draws only the new segment");
            }
            require(barWrites == 1, "progress is not cleared before growth");
        }
        std::cout << "PASS IMU startup and buzzer; TFT clears once, all state transitions match fresh pixels, unchanged fields untouched, digit-only timer updates and incremental progress growth\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
