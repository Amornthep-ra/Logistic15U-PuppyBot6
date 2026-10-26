#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <initializer_list>

enum class LineSide : uint8_t { Front, Back };

namespace PuppyBotLineDetail {
    constexpr unsigned channels = 8;
    constexpr unsigned captureSamples = 16;
    constexpr uint32_t samplePeriodMs = 10;
    struct Surface {
        uint16_t count = 0, mean = 0, low = 0, high = 0;
    };
    struct Record {
        Surface floor[channels], line[channels];
        bool channelValid(unsigned i) const {
            if (i >= channels) return false;
            const Surface &f = floor[i], &l = line[i];
            // Continue accepting the saved 32-sample records from 0.1.19-dev.
            if ((f.count != captureSamples && f.count != 32) ||
                (l.count != captureSamples && l.count != 32)) return false;
            if (f.low > f.mean || f.mean > f.high || l.low > l.mean || l.mean > l.high) return false;
            const int gap = f.mean > l.mean ? f.mean - l.mean : l.mean - f.mean;
            // Separation must exceed measured peak-to-peak noise, plus quantization.
            // This is a conservative quality screen, not a measured accuracy guarantee.
            return gap > 2 * (int(f.high - f.low) + int(l.high - l.low) + 1);
        }
        bool valid() const {
            for (unsigned i = 0; i < channels; ++i) if (!channelValid(i)) return false;
            return true;
        }
        int normalized(unsigned i, int raw) const {
            if (!channelValid(i)) return 0;
            const int span = int(line[i].mean) - int(floor[i].mean);
            const int value = int((int64_t(raw) - floor[i].mean) * 100 / span);
            return value < 0 ? 0 : value > 100 ? 100 : value;
        }
    };
    struct Accumulator {
        uint32_t total = 0;
        Surface value;
        bool add(int raw) {
            if (raw < 0 || raw > 65535 || value.count >= captureSamples) return false;
            if (!value.count) value.low = value.high = uint16_t(raw);
            if (raw < value.low) value.low = uint16_t(raw);
            if (raw > value.high) value.high = uint16_t(raw);
            total += raw;
            ++value.count;
            value.mean = uint16_t((total + value.count / 2) / value.count);
            return true;
        }
    };

    enum class Stage : uint8_t { Side, Method, Capture, Review };
    enum class Action : uint8_t { None, Exit, Save };
    class Session {
        Accumulator accum_[channels];
    public:
        Stage stage = Stage::Side;
        LineSide side = LineSide::Front;
        unsigned selection = 0, surface = 0, channel = 0, samples = 0;
        bool single = false, collecting = false;
        Record draft;
        const char *notice = "Choose sensor bank";
        unsigned itemCount() const {
            return stage == Stage::Side || stage == Stage::Method || stage == Stage::Review ? 3 : 2;
        }
        void next() { if (!collecting) selection = (selection + 1) % itemCount(); }
        Action select() {
            if (collecting) {
                return Action::None; // Extra clicks cannot cancel or restart a capture.
            }
            if (stage == Stage::Side) {
                if (selection == 2) return Action::Exit;
                side = LineSide(selection); stage = Stage::Method; selection = 0;
                notice = "One surface per step";
            } else if (stage == Stage::Method) {
                if (selection == 2) { stage = Stage::Side; selection = unsigned(side); notice = "Choose sensor bank"; }
                else {
                    single = selection == 1;
                    restart();
                }
            } else if (stage == Stage::Capture) {
                if (selection == 1) { stage = Stage::Side; selection = unsigned(side); draft = Record{}; notice = "Choose sensor bank"; }
                else {
                    for (auto &a : accum_) a = Accumulator{};
                    samples = 0; collecting = true; notice = "Hold on surface";
                }
            } else {
                if (selection == 0) {
                    if (draft.valid()) return Action::Save;
                    notice = "Invalid: Retry";
                } else if (selection == 1) restart();
                else { stage = Stage::Side; selection = unsigned(side); draft = Record{}; notice = "Choose sensor bank"; }
            }
            return Action::None;
        }
        void restart() {
            stage = Stage::Capture; selection = surface = channel = samples = 0;
            collecting = false; draft = Record{}; notice = "On FLOOR: press SW1";
        }
        bool sample(const int raw[channels]) {
            if (!collecting) return false;
            for (unsigned i = 0; i < channels; ++i) {
                if (single && i != channel) continue;
                if (!accum_[i].add(raw[i])) {
                    collecting = false; notice = "Read invalid: Retry"; return false;
                }
            }
            if (++samples < captureSamples) return false;
            for (unsigned i = 0; i < channels; ++i) {
                if (single && i != channel) continue;
                (surface ? draft.line[i] : draft.floor[i]) = accum_[i].value;
            }
            collecting = false;
            if (single && ++channel < channels) { notice = "Place next channel"; return true; }
            channel = 0;
            if (!surface) { surface = 1; notice = "On LINE: press SW1"; }
            else { stage = Stage::Review; notice = draft.valid() ? "Ready to Save" : "Invalid: Retry"; }
            return true;
        }
        void saved(bool ok) { notice = ok ? "Saved on board" : "Save failed: Retry"; }
    };

    // Each bank has its own checked record. RGB occupies 3072..3399.
    constexpr size_t storageOffset = 3456, slotSize = 160, encodedSize = 140;
    static_assert(storageOffset + 2 * slotSize <= 4096, "Line records exceed EEPROM");
    inline uint32_t checksum(const uint8_t *bytes, size_t length) {
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < length; ++i) hash = (hash ^ bytes[i]) * 16777619u;
        return hash;
    }
    inline void put16(uint8_t *p, uint16_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); }
    inline uint16_t get16(const uint8_t *p) { return uint16_t(p[0]) | uint16_t(p[1]) << 8; }
    inline void encode(const Record &record, LineSide side, uint8_t *bytes) {
        memset(bytes, 0, encodedSize);
        memcpy(bytes, "LNC1", 4);
        bytes[4] = 1; bytes[5] = uint8_t(side); bytes[6] = channels; bytes[7] = 1; // Mux8 mapping revision
        size_t n = 8;
        for (unsigned s = 0; s < 2; ++s) for (unsigned i = 0; i < channels; ++i) {
            const auto &v = s ? record.line[i] : record.floor[i];
            for (auto x : {v.count, v.mean, v.low, v.high}) { put16(bytes + n, x); n += 2; }
        }
        const uint32_t crc = checksum(bytes, encodedSize - 4);
        for (unsigned i = 0; i < 4; ++i) bytes[encodedSize - 4 + i] = uint8_t(crc >> (8 * i));
    }
    inline bool decode(const uint8_t *bytes, LineSide side, Record &record) {
        if (memcmp(bytes, "LNC1", 4) || bytes[4] != 1 || bytes[5] != uint8_t(side) ||
            bytes[6] != channels || bytes[7] != 1) return false;
        uint32_t crc = 0;
        for (unsigned i = 0; i < 4; ++i) crc |= uint32_t(bytes[encodedSize - 4 + i]) << (8 * i);
        if (crc != checksum(bytes, encodedSize - 4)) return false;
        Record candidate;
        size_t n = 8;
        for (unsigned s = 0; s < 2; ++s) for (unsigned i = 0; i < channels; ++i) {
            auto &v = s ? candidate.line[i] : candidate.floor[i];
            v.count = get16(bytes + n); v.mean = get16(bytes + n + 2);
            v.low = get16(bytes + n + 4); v.high = get16(bytes + n + 6); n += 8;
        }
        if (!candidate.valid()) return false;
        record = candidate;
        return true;
    }
    template<class Store> bool load(Store &store, LineSide side, Record &record) {
        if (unsigned(side) > 1) return false;
        if (store.length() != 4096) store.begin(4096);
        uint8_t bytes[encodedSize];
        for (size_t i = 0; i < encodedSize; ++i) bytes[i] = store.read(storageOffset + unsigned(side) * slotSize + i);
        return decode(bytes, side, record);
    }
    template<class Store> bool save(Store &store, LineSide side, const Record &record) {
        if (unsigned(side) > 1 || !record.valid()) return false;
        if (store.length() != 4096) store.begin(4096);
        uint8_t bytes[encodedSize], previous[encodedSize];
        encode(record, side, bytes);
        const size_t offset = storageOffset + unsigned(side) * slotSize;
        for (size_t i = 0; i < encodedSize; ++i) {
            previous[i] = store.read(offset + i); store.write(offset + i, bytes[i]);
        }
        if (store.commit()) return true;
        for (size_t i = 0; i < encodedSize; ++i) store.write(offset + i, previous[i]);
        return false;
    }
}
