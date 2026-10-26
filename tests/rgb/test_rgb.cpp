#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>
#include "PuppyBotRGBDisplay.h"

TwoWire Wire, Wire1;
Adafruit_TCS34725 tcs, tcsB;
Adafruit_ST7735 tft_;
int TCS_R = 0, TCS_G = 0, TCS_B = 0, TCS_T = 0, TCS_L = 0;

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char **argv) {
    try {
        require(argc == 2, "output directory required");
        const std::string output = argv[1];
        require(!getRGB(RGB0).sampled && !getRGB(RGB1).sampled, "initial cache must wait");
        Wire1.r = 300; Wire1.g = 100; Wire1.b = 200; Wire1.clearValue = 12345;
        require(!readRGB(RGB1).sampled, "begin schedules, never waits for ADC");
        require(Wire.probes == 0 && Wire1.sda == 6 && Wire1.scl == 7, "single bus isolation");
        require(Wire1.registers[1] == 0xEB && Wire1.registers[15] == 1 && Wire1.registers[0] == 1,
                "50.4ms 4x setup, oscillator enabled before ADC");
        fakeMillis = 2; readRGB(RGB1);
        require(Wire1.registers[0] == 1, "respect power-up interval");
        fakeMillis = 3; readRGB(RGB1);
        require(Wire1.registers[0] == 3 && !getRGB(RGB1).sampled, "enable ADC without waiting");
        fakeMillis = 57; readRGB(RGB1);
        require(!getRGB(RGB1).sampled, "no early/stale startup conversion");
        fakeMillis = 58; auto one = readRGB(RGB1);
        require(one.rawRed == 300 && one.rawGreen == 100 && one.rawBlue == 200 && one.clear == 12345,
                "Arduino word macro must not replace decoded RGBC with offsets 2/4/6/0");
        require(one.connected && one.red == 150 && one.green == 50 && one.blue == 100 && one.clear == 12345,
                "High/Adafruit C/R/G/B word reads give correct word/channel order");
        const int reads1 = Wire1.reads;
        for (int i = 0; i < 20; ++i) readRGB(RGB1);
        require(Wire1.reads == reads1 && getRGB(RGB1).sequence == one.sequence, "polling cannot count one conversion twice");
        require(beginRGB(), "begin both");
        const int probes0 = Wire.probes, probes1 = Wire1.probes;
        require(beginRGB() && Wire.probes == probes0 && Wire1.probes == probes1, "begin is idempotent");
        fakeMillis += 3; updateRGB(); fakeMillis += 55; updateRGB();
        require(getRGB(RGB0).connected && getRGB(RGB1).connected, "both channels progress independently");
        Wire.r = 758; Wire.g = 212; Wire.b = 229; Wire.clearValue = 1059;
        fakeMillis += 55; updateRGB();
        const auto front = getRGB(RGB0);
        const auto rear = getRGB(RGB1);
        require(front.rawRed == 758 && front.rawGreen == 212 && front.rawBlue == 229 && front.clear == 1059 &&
                rear.rawRed == 300 && rear.rawGreen == 100 && rear.rawBlue == 200 && rear.clear == 12345,
                "each bus preserves its own raw bytes under Arduino macros");
        require(tcs.starts == 0 && tcsB.starts == 0 && tcs.reads == 0 && tcsB.reads == 0,
                "reader must never call blocking Adafruit begin/getRawData");
        require(Wire.sda == 4 && Wire.scl == 5 && Wire1.sda == 6 && Wire1.scl == 7, "original pin mapping");
        require(TCS_R == 150 && TCS_G == 50 && TCS_B == 100, "legacy RGB1 cache preserved");
        require(!beginRGB(RGBChannel(3)) && !readRGB(RGBChannel(3)).sampled, "invalid channel");
        std::cout << "PASS independent buses, phased startup, 50ms/4x configuration, High word reads and fresh-frame gating\n";

        Wire1.connected = false; fakeMillis += 55;
        require(!readRGB(RGB1).connected, "disconnect status");
        const auto failedSequence = getRGB(RGB1).sequence;
        const auto failedProbes = Wire1.probes;
        fakeMillis += 999; readRGB(RGB1);
        require(Wire1.probes == failedProbes && getRGB(RGB1).sequence == failedSequence, "retry throttle");
        Wire1.connected = true; Wire1.validId = false; fakeMillis += 1;
        require(!readRGB(RGB1).connected, "wrong ID rejected on correct bus");
        Wire1.validId = true; fakeMillis += 1000; readRGB(RGB1);
        fakeMillis += 3; readRGB(RGB1); fakeMillis += 55;
        require(readRGB(RGB1).connected, "nonblocking reconnect");
        Wire.shortRead = 1; fakeMillis += 55;
        require(!readRGB(RGB0).connected, "partial RGBC transfer cannot become a valid color");
        Wire.shortRead = -1; fakeMillis += 1000; readRGB(RGB0);
        fakeMillis += 3; readRGB(RGB0); fakeMillis += 55; readRGB(RGB0);
        const auto sequence = getRGB(RGB0).sequence;
        Wire.adcReady = false; fakeMillis += 55; readRGB(RGB0);
        require(getRGB(RGB0).sequence == sequence, "invalid ADC cannot refresh the cached timestamp");
        fakeMillis += 200; require(!readRGB(RGB0).connected, "stalled ADC becomes offline and can retry");
        Wire.adcReady = true; fakeMillis += 1000; readRGB(RGB0);
        fakeMillis += 3; readRGB(RGB0); fakeMillis += 55; readRGB(RGB0);
        std::cout << "PASS offline/retry/ID/short-read/ADC timeout paths without integration waits\n";

        fakeMillis = 0xFFFFFFF0u; readRGB(RGB0); auto wrappedSequence = getRGB(RGB0).sequence;
        fakeMillis = 39; readRGB(RGB0);
        require(getRGB(RGB0).sequence == wrappedSequence + 1, "55ms cadence across clock wrap");
        Wire.r = Wire.g = Wire.b = 0; Wire.clearValue = 0; fakeMillis += 55;
        auto dark = readRGB(RGB0);
        require(dark.connected && dark.rawRed == 0 && dark.clear == 0 && dark.red == 0, "monitor still exposes unusable dark raw values");
        Wire.r = 100; Wire.g = 200; Wire.b = 300; fakeMillis += 55; readRGB(RGB0);
        std::atomic<bool> done{false};
        std::thread writer([&] {
            for (int i = 0; i < 2000; ++i) {
                Wire.r = i % 2 ? 300 : 100; Wire.g = i % 2 ? 100 : 200; Wire.b = i % 2 ? 200 : 300;
                fakeMillis += 55; readRGB(RGB0);
            }
            done.store(true);
        });
        bool coherent = true;
        for (int n = 0; !done.load() || n < 1000; ++n) {
            const auto cached = getRGB(RGB0);
            coherent &= (cached.red == 50 && cached.green == 100 && cached.blue == 150) ||
                        (cached.red == 150 && cached.green == 50 && cached.blue == 100);
        }
        writer.join();
        require(coherent, "cache cannot contain torn RGB data across cores");
        Wire.r = 42; Wire.g = 150; Wire.b = 88; Wire.clearValue = 12345;
        Wire1.r = 190; Wire1.g = 72; Wire1.b = 24; Wire1.clearValue = 65535;
        fakeMillis += 55; updateRGB();
        require(fakeDelayCalls == 0, "reader cannot add integration delay to IMU core, even during recovery");
        std::cout << "PASS wrap, raw dark values, coherent cross-core cache and zero delay calls\n";
        RGBSnapshot snapshot = getRGBSnapshot();
        Adafruit_ST7735 display;
        PuppyBotRGBDetail::Monitor<Adafruit_ST7735> monitor(display);
        monitor.show(snapshot);
        display.save(output + "/dual-landscape.ppm");
        display.operations.clear();
        ++snapshot.bus0.sampledAt; ++snapshot.bus1.sampledAt;
        monitor.show(snapshot);
        require(display.operations.empty(), "unchanged screen issued draw commands");

        snapshot.bus0.clear = 9;
        monitor.show(snapshot);
        require(display.fullClears == 1 && !display.operations.empty(), "Clear change did not update independently");
        for (const auto &op : display.operations)
            require(op.x >= 40 && op.x + op.w <= 70 && op.y == 90 && op.h == 8,
                    "Clear-only update changed another field");
        Adafruit_ST7735 clearReference;
        PuppyBotRGBDetail::Monitor<Adafruit_ST7735> clearFresh(clearReference);
        clearFresh.show(snapshot);
        require(display.pixels == clearReference.pixels, "shorter Clear value left stale digits");
        display.operations.clear(); monitor.show(snapshot);
        require(display.operations.empty(), "unchanged Clear redrew the display");
        std::cout << "PASS raw Clear range, same sensor read, dirty field and shrinking digits\n";

        snapshot.bus0.red = 9;
        monitor.show(snapshot);
        require(!display.operations.empty() && display.fullClears == 1, "value update cleared screen");
        for (const auto &op : display.operations)
            require(op.x >= 6 && op.x + op.w <= 77 && op.y >= 27 && op.y + op.h <= 113,
                    "RGB0 update touched another card or chrome");
        Adafruit_ST7735 reference;
        PuppyBotRGBDetail::Monitor<Adafruit_ST7735> fresh(reference);
        fresh.show(snapshot);
        require(display.pixels == reference.pixels, "shorter value left stale pixels");

        snapshot.bus0.connected = false;
        snapshot.bus0.red = snapshot.bus0.green = snapshot.bus0.blue = 0;
        display.operations.clear(); monitor.show(snapshot);
        require(display.fullClears == 1, "disconnect cleared the whole screen");
        for (const auto &op : display.operations)
            require(op.x >= 6 && op.x + op.w <= 77, "disconnect touched RGB1 card");
        Adafruit_ST7735 offlineReference;
        PuppyBotRGBDetail::Monitor<Adafruit_ST7735> offlineFresh(offlineReference);
        offlineFresh.show(snapshot);
        require(display.pixels == offlineReference.pixels, "disconnect left stale text or bars");
        display.save(output + "/offline.ppm");
        snapshot = getRGBSnapshot();
        monitor.show(snapshot, RGB1);
        require(display.fullClears == 2, "mode change should rebuild once");
        display.save(output + "/single-rgb1.ppm");
        display.operations.clear(); monitor.show(snapshot, RGB1);
        require(display.operations.empty(), "unchanged single view drew again");
        std::cout << "PASS no-op redraw, dirty fields, shrinking text, offline and single view\n";

        Adafruit_ST7735 portrait(128, 160);
        PuppyBotRGBDetail::Monitor<Adafruit_ST7735> portraitMonitor(portrait);
        portraitMonitor.show(snapshot);
        portrait.save(output + "/dual-portrait.ppm");
        Adafruit_ST7735 waiting;
        PuppyBotRGBDetail::Monitor<Adafruit_ST7735> waitingMonitor(waiting);
        waitingMonitor.show(RGBSnapshot{});
        waiting.save(output + "/waiting.ppm");
        std::cout << "PASS landscape/portrait bounds and waiting layout\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
