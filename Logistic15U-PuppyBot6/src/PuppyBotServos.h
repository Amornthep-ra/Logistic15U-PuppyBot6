#pragma once
#include <Arduino.h>
#include <hardware/clocks.h>
#include <hardware/pwm.h>

#ifndef SERVO_COUNT
#define SERVO_COUNT 6
#endif

// Mapping from the original PuppyBot6 source, not inferred robot wiring.
static const uint8_t servoPins[SERVO_COUNT] = {34, 35, 36, 37, 38, 39};

namespace PuppyBotServoDetail {
    constexpr uint32_t frequencyHz = 50;
    constexpr int periodUs = 20000;
    // Retain the original source's pulse endpoints; these are NOT calibrated limits.
    constexpr int defaultMinUs = 300, defaultMaxUs = 3000;
    struct Channel {
        int minimumUs = defaultMinUs, maximumUs = defaultMaxUs;
        int lastAngle = -1; // Last command, never a measurement of shaft position.
        bool active = false;
    };
    static Channel channels[SERVO_COUNT];
    static bool initialized = false;
    static uint32_t clockHz = 0, divider = 0, periodTicks = 0;

    inline bool validChannel(int channel) { return channel >= 0 && channel < SERVO_COUNT; }
    inline bool validAngle(int angle) { return angle >= 0 && angle <= 180; }
    inline uint32_t clockDivider(uint32_t hz) {
        const uint32_t requested = (uint64_t(hz) + 999999) / 1000000;
        return requested > 255 ? 255 : requested;
    }
    inline uint16_t pulseLevel(int microseconds, uint32_t hz, uint32_t div) {
        if (!div) return 0;
        const uint64_t denominator = uint64_t(div) * 1000000;
        return uint16_t((uint64_t(microseconds) * hz + denominator / 2) / denominator);
    }
}

// Initialize each shared slice once, with both outputs low. Repeating is harmless.
inline bool setupServos() {
    using namespace PuppyBotServoDetail;
    if (initialized) return true;
    const uint32_t hz = clock_get_hz(clk_sys), div = clockDivider(hz);
    if (!div) return false;
    const uint32_t ticks = (uint64_t(hz) + div * frequencyHz / 2) / (div * frequencyHz);
    if (ticks < 2 || ticks > 65536) return false;
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, float(div));
    pwm_config_set_wrap(&config, uint16_t(ticks - 1));
    uint32_t slices = 0;
    for (int channel = 0; channel < SERVO_COUNT; ++channel) {
        const uint slice = pwm_gpio_to_slice_num(servoPins[channel]);
        const uint32_t bit = 1u << slice;
        if (!(slices & bit)) {
            pwm_init(slice, &config, false); // Reset both compare registers to zero.
            slices |= bit;
        }
        gpio_set_function(servoPins[channel], GPIO_FUNC_PWM);
    }
    for (int channel = 0; channel < SERVO_COUNT; ++channel) {
        const uint slice = pwm_gpio_to_slice_num(servoPins[channel]);
        if (slices & (1u << slice)) {
            pwm_set_enabled(slice, true);
            slices &= ~(1u << slice);
        }
    }
    clockHz = hz; divider = div; periodTicks = ticks; initialized = true;
    return true;
}

// Compatibility helper using the original pulse range. Invalid angles return zero.
inline uint16_t angleToLevel(int angle) {
    using namespace PuppyBotServoDetail;
    if (!validAngle(angle)) return 0;
    const uint32_t hz = clock_get_hz(clk_sys), div = clockDivider(hz);
    return pulseLevel(defaultMinUs + (defaultMaxUs - defaultMinUs) * angle / 180, hz, div);
}

// Set measured endpoints while stopped. Does not save to flash or move the servo.
inline bool setServoPulseRange(int channel, int minimumUs, int maximumUs) {
    using namespace PuppyBotServoDetail;
    if (!validChannel(channel) || minimumUs <= 0 || maximumUs <= minimumUs ||
        maximumUs >= periodUs || channels[channel].active) return false;
    channels[channel].minimumUs = minimumUs;
    channels[channel].maximumUs = maximumUs;
    channels[channel].lastAngle = -1; // The old command used a different mapping.
    return true;
}

inline bool servoRun(int channel, int angle) {
    using namespace PuppyBotServoDetail;
    if (!validChannel(channel) || !validAngle(angle) || !setupServos()) return false;
    Channel &state = channels[channel];
    const int us = state.minimumUs + (state.maximumUs - state.minimumUs) * angle / 180;
    const uint16_t level = pulseLevel(us, clockHz, divider);
    if (!level || level >= periodTicks) return false;
    pwm_set_gpio_level(servoPins[channel], level);
    state.lastAngle = angle; state.active = true;
    return true;
}

// Stop only this output, never the shared PWM slice. Takes effect at PWM wrap.
inline bool servoStop(int channel) {
    using namespace PuppyBotServoDetail;
    if (!validChannel(channel)) return false;
    if (initialized) pwm_set_gpio_level(servoPins[channel], 0);
    channels[channel].active = false;
    return true;
}

inline int getServoLastAngle(int channel) {
    return PuppyBotServoDetail::validChannel(channel) ? PuppyBotServoDetail::channels[channel].lastAngle : -1;
}
inline bool isServoActive(int channel) {
    return PuppyBotServoDetail::validChannel(channel) && PuppyBotServoDetail::channels[channel].active;
}

// Blocking movement on the calling core. startAngle is explicitly supplied by caller.
inline bool servoSweep(int channel, int startAngle, int endAngle, int step, int delayMs) {
    using namespace PuppyBotServoDetail;
    if (!validChannel(channel) || !validAngle(startAngle) || !validAngle(endAngle) || step <= 0 || delayMs < 0)
        return false;
    int angle = startAngle;
    while (true) {
        if (!servoRun(channel, angle)) return false;
        if (angle == endAngle) return true;
        if (delayMs) delay(delayMs);
        const int remaining = angle < endAngle ? endAngle - angle : angle - endAngle;
        const int increment = step < remaining ? step : remaining;
        angle += angle < endAngle ? increment : -increment;
    }
}

inline bool control_servo(int channel, int startAngle, int targetAngle, int delayMs) {
    return servoSweep(channel, startAngle, targetAngle, 1, delayMs);
}
