#pragma once
#include "PuppyBotLineCalibration.h"
#include <EEPROM.h>

// Included after the existing MUX readers and PID configuration globals.
namespace PuppyBotLineDetail {
    static Record active[2];
    static bool loaded[2] = {false, false};
    inline void apply(LineSide side, const Record &record) {
        const bool back = side == LineSide::Back;
        active[unsigned(side)] = record; loaded[unsigned(side)] = true;
        (back ? Sensor_mode_B : Sensor_mode_F) = 1;
        (back ? PID_NumPin_B : PID_NumPin) = channels;
        int *pins = back ? PID_SetupPin_B : PID_SetupPin;
        int *low = back ? PID_Min_B : PID_Min, *high = back ? PID_Max_B : PID_Max;
        for (unsigned i = 0; i < channels; ++i) {
            pins[i] = i; // Same logical channel numbering as the existing MUX monitor.
            const int f = record.floor[i].mean, l = record.line[i].mean;
            low[i] = f < l ? f : l; high[i] = f > l ? f : l;
        }
        (back ? Back_color : Front_color) = record.line[0].mean > record.floor[0].mean ? 1 : 0;
        (back ? previous_error_B : previous_error) = 0;
        (back ? _lastPosition_B : _lastPosition) = (channels - 1) * 100 / 2;
    }
    inline int normalized(bool back, unsigned index, int raw) {
        const unsigned count = back ? PID_NumPin_B : PID_NumPin;
        if (index >= count || index >= 16) return 0;
        const int pin = (back ? PID_SetupPin_B : PID_SetupPin)[index];
        const int lo = (back ? PID_Min_B : PID_Min)[index];
        const int hi = (back ? PID_Max_B : PID_Max)[index];
        const Record &r = active[back ? 1 : 0];
        if (loaded[back ? 1 : 0] && count == channels && (back ? Sensor_mode_B : Sensor_mode_F) == 1 &&
            pin >= 0 && pin < int(channels)) {
            const int f = r.floor[pin].mean, l = r.line[pin].mean;
            // Manual legacy calibration remains usable if the caller replaces these endpoints.
            if (lo == (f < l ? f : l) && hi == (f > l ? f : l)) return r.normalized(pin, raw);
        }
        if (hi <= lo) return 0;
        int value = int((int64_t(raw) - lo) * 100 / (hi - lo));
        if ((back ? Back_color : Front_color) == 0) value = 100 - value;
        return value < 0 ? 0 : value > 100 ? 100 : value;
    }
}

inline bool hasLineCalibration(LineSide side) {
    return unsigned(side) < 2 && PuppyBotLineDetail::loaded[unsigned(side)];
}
// Core 0 / TFT owner only, after puppybot_setup(). wait_SW1 calls this automatically.
inline bool loadLineCalibration(LineSide side) {
    PuppyBotLineDetail::Record record;
    if (!PuppyBotLineDetail::load(EEPROM, side, record)) return false;
    PuppyBotLineDetail::apply(side, record);
    return true;
}
