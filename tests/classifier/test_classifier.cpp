#include <iostream>
#include <stdexcept>
#include <limits>
#include "PuppyBotRGBClassifier.h"
#include "PuppyBotRGBCalibrationStorage.h"
namespace HighReference {
#include "HighRGBClassifierReference.h"
}
TwoWire Wire, Wire1;
Adafruit_TCS34725 tcs, tcsB;
int TCS_R = 0, TCS_G = 0, TCS_B = 0, TCS_T = 0, TCS_L = 0;
void require(bool value, const char *why) { if (!value) throw std::runtime_error(why); }
RGBReading reading(RGBRawValues raw) {
    RGBReading result; result.connected = result.sampled = true;
    result.rawRed = raw.red; result.rawGreen = raw.green; result.rawBlue = raw.blue; result.clear = raw.clear;
    result.sampledAt = fakeMillis; return result;
}
void frame(RGBChannel ch, RGBRawValues raw) {
    fakeMillis += 55;
    auto sample = reading(raw); sample.sequence = getRGB(ch).sequence + 1;
    PuppyBotRGBDetail::publish(ch, sample);
}
void train(RGBCalibration &data, RGBChannel ch, RGBColor color, RGBRawValues raw) {
    require(data.start(ch, color), "capture starts");
    for (unsigned i = 0; i < 20; ++i) { frame(ch, raw); data.update(); }
    require(data.get(ch, color).samples == 20, "real calibration path collects references");
}
void highProfile(const RGBCalibration &data, RGBChannel ch) {
    for (unsigned i = 0; i < 6; ++i) {
        auto r = data.get(ch, RGBColor(i)).mean;
        require(HighReference::HighRGB_SetReference(HighReference::RGBColor(i + 1), r.red, r.green, r.blue, r.clear),
                "High accepts the same captured reference");
    }
}
RGBColor expected(RGBRawValues raw) {
    HighReference::RGBReading value;
    value.red = raw.red; value.green = raw.green; value.blue = raw.blue; value.clear = raw.clear;
    value.valid = uint32_t(raw.red) + raw.green + raw.blue > 0 && raw.clear >= HighReference::RGB_MIN_CLEAR &&
        raw.red < HighReference::RGB_SATURATION && raw.green < HighReference::RGB_SATURATION &&
        raw.blue < HighReference::RGB_SATURATION && raw.clear < HighReference::RGB_SATURATION;
    const auto result = HighReference::Check_Color(value);
    return result == HighReference::RGB_UNKNOWN ? RGBColor::Unknown : RGBColor(unsigned(result) - 1);
}
int main() {
    try {
        // Fixture values from High's generated sketch, not installed robot defaults.
        const RGBRawValues centers[6] = {{758,212,229,1059},{356,743,444,1511},{94,200,334,595},
            {1202,1069,449,2644},{1086,1368,1292,3646},{73,84,76,219}};
        RGBCalibration data;
        for (unsigned ch = 0; ch < 2; ++ch) for (unsigned k = 0; k < 6; ++k)
            train(data, RGBChannel(ch), RGBColor(k), centers[(k + ch) % 6]);
        unsigned comparisons = 0;
        for (RGBChannel ch : {RGB0, RGB1}) {
            highProfile(data, ch);
            for (unsigned k = 0; k < 6; ++k) {
                auto raw = data.get(ch, RGBColor(k)).mean;
                require(classifyRGB(data, ch, reading(raw)).color == RGBColor(k), "all six exact references on both channels");
                for (float scale : {0.5f, 1.0f, 1.5f, 2.0f}) {
                    RGBRawValues scaled{uint16_t(raw.red*scale),uint16_t(raw.green*scale),uint16_t(raw.blue*scale),uint16_t(raw.clear*scale)};
                    require(classifyRGB(data, ch, reading(scaled)).color == expected(scaled), "brightness-scaled result matches actual High code");
                    ++comparisons;
                }
            }
            uint32_t random = 0x12345678;
            auto next = [&]() { random = random * 1664525u + 1013904223u; return uint16_t((random >> 8) % 23000); };
            for (unsigned n = 0; n < 5000; ++n) {
                RGBRawValues raw{next(),next(),next(),next()};
                require(classifyRGB(data, ch, reading(raw)).color == expected(raw), "differential sample differs from High");
                ++comparisons;
            }
        }
        std::cout << "PASS " << comparisons << " differential cases against actual High-PuppyBot6 Check_Color, both sensor profiles\n";
        auto raw = centers[0]; raw.clear = 10;
        require(classifyRGB(data, RGB0, reading(raw)).color == RGBColor::Red, "Clear gates signal only, not brightness matching");
        raw.clear = 9; require(classifyRGB(data, RGB0, reading(raw)).status == RGBColorStatus::LowSignal, "minimum Clear boundary");
        for (unsigned i=0;i<4;++i) {
            raw=centers[0]; raw.set(i,21504);
            require(classifyRGB(data,RGB0,reading(raw)).status == RGBColorStatus::Saturated, "every saturated component rejected");
        }
        require(classifyRGB(data,RGB0,reading({0,0,0,30})).status == RGBColorStatus::LowSignal, "zero RGB sum rejected");
        RGBCalibration overlap = data;
        train(overlap,RGB0,RGBColor::White,centers[5]);
        require(classifyRGB(overlap,RGB0,reading(centers[5])).status == RGBColorStatus::Ambiguous, "tied white/black cannot be forced");
        RGBColorOptions options;
        options.maxDistance = std::numeric_limits<float>::quiet_NaN();
        require(classifyRGB(data,RGB0,reading(centers[0]),options).status == RGBColorStatus::InvalidOptions, "NaN tolerance rejected");
        options = {}; options.separationRatio = 1;
        require(classifyRGB(data,RGB0,reading(centers[0]),options).status == RGBColorStatus::InvalidOptions, "invalid separation rejected");
        std::cout << "PASS low-light/saturation checks, Clear validity-only behavior, ambiguous colors and tolerance validation\n";
        auto old = reading(centers[0]); old.sampledAt -= 250;
        require(classifyRGB(data,RGB0,old).status == RGBColorStatus::Stale, "stale cache rejected");
        options={}; options.maxAgeMs=0;
        require(classifyRGB(data,RGB0,old,options).matched(), "explicit disabled age check");
        fakeMillis=20; old.sampledAt=0xFFFFFFF0u;
        require(classifyRGB(data,RGB0,old).matched(), "clock wrap freshness");
        auto offline=reading(centers[0]);offline.connected=false;
        require(classifyRGB(data,RGB0,offline).status==RGBColorStatus::Offline, "offline cache rejected");
        require(classifyRGB(data,RGB0,RGBReading{}).status==RGBColorStatus::NoSample, "unread cache rejected");
        require(getRGBColor(RGBChannel(9))==RGBColor::Unknown, "invalid channel");
        RGBCalibration partial;
        train(partial,RGB0,RGBColor::Red,centers[0]);
        require(classifyRGB(partial,RGB0,reading(centers[0])).status==RGBColorStatus::IncompleteCalibration, "all six required per selected channel");
        require(PuppyBotRGBDetail::saveCalibration(data,EEPROM) && loadRGBCalibration(), "v2 saved references reload");
        frame(RGB0,centers[0]);frame(RGB1,centers[3]);
        const int probes=Wire.probes+Wire1.probes, commits=EEPROM.commits;
        require(getRGBColor(RGB0)==RGBColor::Red && getRGBColor(RGB1)==RGBColor::Blue, "independent cached APIs");
        require(probes==Wire.probes+Wire1.probes && commits==EEPROM.commits, "classifier performs no I2C or flash writes");
        uint8_t bytes[RGBCalibration::encodedSize]; data.encode(bytes);
        auto fixCRC=[&]() { auto crc=RGBCalibration::checksum(bytes,sizeof(bytes)-4); for(unsigned i=0;i<4;++i) bytes[sizeof(bytes)-4+i]=uint8_t(crc>>(i*8)); };
        for(unsigned field : {4u,8u,9u,10u}) {
            data.encode(bytes); bytes[field]^=1; fixCRC();
            require(!partial.decode(bytes) && partial.completed(RGB0)==1, "old schema or wrong optical settings rejected without replacing RAM");
        }
        // Build a real old 324-byte record with its own valid CRC, plus four unrelated bytes.
        data.encode(bytes);
        uint8_t oldBytes[RGBCalibration::encodedSize]{};
        const uint8_t oldHeader[8]={'L','1','5','C',1,2,6,0};
        for(unsigned i=0;i<8;++i) oldBytes[i]=oldHeader[i];
        for(unsigned i=0;i<312;++i) oldBytes[8+i]=bytes[12+i];
        auto crc=RGBCalibration::checksum(oldBytes,320);
        for(unsigned i=0;i<4;++i) oldBytes[320+i]=uint8_t(crc>>(i*8));
        require(!partial.decode(oldBytes), "valid v1 record cannot silently use 2.4ms calibration at 50ms");
        std::cout << "PASS cache policies, independent sensors, v2 persistence, optical metadata and rejection of old flash data\n";
        return 0;
    } catch(const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
