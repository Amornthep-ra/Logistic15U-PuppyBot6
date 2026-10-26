#pragma once
#include "PuppyBotRGB.h"
#include "PuppyBotRGBCalibrationFeedback.h"
#include <stddef.h>
#include <initializer_list>

enum class RGBColor : uint8_t { Red, Green, Blue, Yellow, White, Black, Unknown = 255 };
enum class RGBCaptureState : uint8_t { Ready, Collecting, Captured, Cancelled, Offline, TimedOut, InvalidSignal };
enum class RGBStorageState : uint8_t { RAM, Loaded, Saved, Invalid, Failed };

inline const char *rgbColorName(RGBColor color) {
    static const char *const names[] = {"Red", "Green", "Blue", "Yellow", "White", "Black"};
    if (color == RGBColor::Unknown) return "Unknown";
    return uint8_t(color) < 6 ? names[uint8_t(color)] : "Invalid";
}

struct RGBRawValues {
    uint16_t red = 0, green = 0, blue = 0, clear = 0;
    uint16_t at(unsigned component) const {
        return component == 0 ? red : component == 1 ? green : component == 2 ? blue : clear;
    }
    void set(unsigned component, uint16_t value) {
        if (component == 0) red = value;
        else if (component == 1) green = value;
        else if (component == 2) blue = value;
        else clear = value;
    }
};

struct RGBColorReference {
    uint16_t samples = 0;  // Zero means uncalibrated.
    RGBRawValues mean, minimum, maximum;
};

// Own this object, its display and storage on ONE core. Only cached RGB reads cross cores.
class RGBCalibration {
    RGBColorReference records_[2][6];
    RGBChannel channel_ = RGB0;
    RGBColor color_ = RGBColor::Red;
    RGBCaptureState state_ = RGBCaptureState::Ready;
    RGBStorageState storage_ = RGBStorageState::RAM;
    uint16_t target_ = 20, count_ = 0;
    uint32_t sequence_ = 0, lastFrameAt_ = 0, timeout_ = 2000;
    uint32_t sum_[4] = {};
    RGBRawValues minimum_, maximum_;
    RGBRawValues rejected_;
    bool dirty_ = false;

public:
    static bool valid(RGBChannel channel, RGBColor color) {
        return PuppyBotRGBDetail::valid(channel) && uint8_t(color) < 6;
    }
    RGBChannel channel() const { return channel_; }
    RGBColor color() const { return color_; }
    RGBCaptureState state() const { return state_; }
    RGBStorageState storage() const { return storage_; }
    uint16_t count() const { return count_; }
    uint16_t target() const { return target_; }
    bool dirty() const { return dirty_; }
    bool collecting() const { return state_ == RGBCaptureState::Collecting; }
    RGBRawValues rejectedReading() const { return rejected_; }
    const char *rejectionMessage() const {
        if (uint32_t(rejected_.red) + rejected_.green + rejected_.blue == 0) return "RGB zero: retry";
        if (rejected_.clear < PuppyBotRGBSettings::minimumClear) return "Clear <10: retry";
        return "Saturated: retry";
    }
    RGBColorReference get(RGBChannel channel, RGBColor color) const {
        return valid(channel, color) ? records_[channel][uint8_t(color)] : RGBColorReference{};
    }
    uint8_t completed(RGBChannel channel) const {
        if (!PuppyBotRGBDetail::valid(channel)) return 0;
        uint8_t count = 0;
        for (const auto &record : records_[channel]) if (record.samples) ++count;
        return count;
    }
    bool select(RGBChannel channel, RGBColor color) {
        if (collecting() || !valid(channel, color)) return false;
        channel_ = channel; color_ = color;
        state_ = RGBCaptureState::Ready;
        rejected_ = RGBRawValues{};
        count_ = 0;
        return true;
    }
    bool start(RGBChannel channel, RGBColor color, uint16_t samples = 20, uint32_t timeoutMs = 2000) {
        if (!valid(channel, color) || collecting() || samples == 0 || samples > 1000 ||
            timeoutMs == 0 || timeoutMs > 60000) return false;
        select(channel, color);
        target_ = samples; timeout_ = timeoutMs;
        sequence_ = getRGB(channel).sequence;  // Do not capture the frame from before the command.
        lastFrameAt_ = millis();
        for (unsigned i = 0; i < 4; ++i) {
            sum_[i] = 0; minimum_.set(i, 65535); maximum_.set(i, 0);
        }
        state_ = RGBCaptureState::Collecting;
        return true;
    }
    void cancel() {
        if (collecting()) state_ = RGBCaptureState::Cancelled;
    }
    void update() {
        if (!collecting()) return;
        const uint32_t now = millis();
        if (uint32_t(now - lastFrameAt_) >= timeout_) {
            state_ = RGBCaptureState::TimedOut;
            return;
        }
        const RGBReading reading = getRGB(channel_);
        if (!reading.sampled || reading.sequence == sequence_) return;
        sequence_ = reading.sequence;
        if (!reading.connected) { state_ = RGBCaptureState::Offline; return; }
        if (uint32_t(now - reading.sampledAt) >= timeout_) {
            state_ = RGBCaptureState::TimedOut;
            return;
        }
        lastFrameAt_ = now;
        const RGBRawValues raw{reading.rawRed, reading.rawGreen, reading.rawBlue, reading.clear};
        if (!PuppyBotRGBSettings::usable(raw.red, raw.green, raw.blue, raw.clear)) {
            rejected_ = raw;
            state_ = RGBCaptureState::InvalidSignal;
            return;  // Keep the previous reference; never save clipped or unlit colors.
        }
        for (unsigned i = 0; i < 4; ++i) {
            const uint16_t value = raw.at(i);
            sum_[i] += value;
            if (value < minimum_.at(i)) minimum_.set(i, value);
            if (value > maximum_.at(i)) maximum_.set(i, value);
        }
        if (++count_ != target_) return;
        RGBColorReference record;
        record.samples = count_; record.minimum = minimum_; record.maximum = maximum_;
        for (unsigned i = 0; i < 4; ++i)
            record.mean.set(i, uint16_t((sum_[i] + count_ / 2) / count_));
        if (!PuppyBotRGBSettings::usable(record.mean.red, record.mean.green, record.mean.blue, record.mean.clear)) {
            rejected_ = record.mean;
            state_ = RGBCaptureState::InvalidSignal;
            return;
        }
        records_[channel_][uint8_t(color_)] = record;
        state_ = RGBCaptureState::Captured;
        dirty_ = true; storage_ = RGBStorageState::RAM;
    }

    // A versioned, little-endian format; no compiler padding or native struct serialization.
    static constexpr size_t encodedSize = 12 + 2 * 6 * 26 + 4;
    static constexpr uint8_t header[12] = {'L', '1', '5', 'C', 2, 2, 6, 0,
        PuppyBotRGBSettings::integration, uint8_t(PuppyBotRGBSettings::gain),
        uint8_t(PuppyBotRGBSettings::minimumClear), uint8_t(PuppyBotRGBSettings::minimumClear >> 8)};
    static uint32_t checksum(const uint8_t *bytes, size_t size) {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < size; ++i) {
            crc ^= bytes[i];
            for (unsigned bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
        return ~crc;
    }
    void encode(uint8_t (&bytes)[encodedSize]) const {
        for (unsigned i = 0; i < sizeof(header); ++i) bytes[i] = header[i];
        size_t position = sizeof(header);
        const auto put = [&](uint16_t value) {
            bytes[position++] = uint8_t(value); bytes[position++] = uint8_t(value >> 8);
        };
        for (const auto &channel : records_) for (const auto &record : channel) {
            put(record.samples);
            for (const auto *values : {&record.mean, &record.minimum, &record.maximum})
                for (unsigned i = 0; i < 4; ++i) put(values->at(i));
        }
        const uint32_t crc = checksum(bytes, position);
        for (unsigned i = 0; i < 4; ++i) bytes[position++] = uint8_t(crc >> (8 * i));
    }
    bool decode(const uint8_t (&bytes)[encodedSize]) {
        if (collecting()) return false;
        uint32_t crc = 0;
        for (unsigned i = 0; i < 4; ++i) crc |= uint32_t(bytes[encodedSize - 4 + i]) << (8 * i);
        for (unsigned i = 0; i < sizeof(header); ++i) if (bytes[i] != header[i]) return false;
        if (checksum(bytes, encodedSize - 4) != crc) return false;
        RGBColorReference candidate[2][6];
        size_t position = sizeof(header);
        const auto take = [&]() {
            const uint16_t value = uint16_t(bytes[position]) | uint16_t(bytes[position + 1]) << 8;
            position += 2; return value;
        };
        for (auto &channel : candidate) for (auto &record : channel) {
            record.samples = take();
            if (record.samples > 1000) return false;
            for (auto *values : {&record.mean, &record.minimum, &record.maximum})
                for (unsigned i = 0; i < 4; ++i) values->set(i, take());
            for (unsigned i = 0; i < 4; ++i) {
                if (record.minimum.at(i) > record.mean.at(i) || record.mean.at(i) > record.maximum.at(i)) return false;
                if (!record.samples && record.maximum.at(i) != 0) return false;
            }
            if (record.samples &&
                (record.minimum.clear < PuppyBotRGBSettings::minimumClear ||
                 !PuppyBotRGBSettings::usable(record.mean.red, record.mean.green, record.mean.blue, record.mean.clear) ||
                 !PuppyBotRGBSettings::usable(record.maximum.red, record.maximum.green, record.maximum.blue, record.maximum.clear))) return false;
        }
        for (unsigned c = 0; c < 2; ++c) for (unsigned k = 0; k < 6; ++k) records_[c][k] = candidate[c][k];
        dirty_ = false; storage_ = RGBStorageState::Loaded;
        state_ = RGBCaptureState::Ready; count_ = 0;
        return true;
    }
    void storageResult(bool success, bool saving) {
        storage_ = success ? (saving ? RGBStorageState::Saved : RGBStorageState::Loaded) :
                            (saving ? RGBStorageState::Failed : RGBStorageState::Invalid);
        if (success && saving) dirty_ = false;
    }
};

namespace PuppyBotRGBDetail { static RGBCalibration calibration; }

inline bool selectRGBCalibration(RGBChannel channel, RGBColor color) {
    return PuppyBotRGBDetail::calibration.select(channel, color);
}
inline bool startRGBCalibration(RGBChannel channel, RGBColor color, uint16_t samples = 20) {
    return PuppyBotRGBDetail::calibration.start(channel, color, samples);
}
inline void cancelRGBCalibration() { PuppyBotRGBDetail::calibration.cancel(); }
inline void updateRGBCalibration() {
    using namespace PuppyBotRGBDetail;
    calibrationFeedback.update(millis());
    const bool wasCollecting = calibration.collecting();
    calibration.update();
    if (wasCollecting && calibration.state() == RGBCaptureState::Captured)
        calibrationFeedback.complete(millis());
}
inline const RGBCalibration &getRGBCalibrationState() { return PuppyBotRGBDetail::calibration; }
inline RGBColorReference getRGBCalibration(RGBChannel channel, RGBColor color) {
    return PuppyBotRGBDetail::calibration.get(channel, color);
}
