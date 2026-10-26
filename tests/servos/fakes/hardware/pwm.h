#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
using uint = unsigned;
struct pwm_config { float divider = 1; uint16_t top = 65535; };
inline pwm_config configurations[12];
inline int initCalls[12] = {}, enabledCalls[12] = {}, muxCalls = 0;
inline bool enabled[12] = {};
inline uint16_t levels[12][2] = {};
inline std::vector<std::pair<uint, uint16_t>> writes;
inline pwm_config pwm_get_default_config() { return {}; }
inline void pwm_config_set_clkdiv(pwm_config *c, float d) {
    if (d < 1 || d >= 256) throw std::runtime_error("invalid hardware divider");
    c->divider = d;
}
inline void pwm_config_set_wrap(pwm_config *c, uint16_t top) { c->top = top; }
inline uint pwm_gpio_to_slice_num(uint gpio) { return gpio < 32 ? ((gpio >> 1) & 7) : 8 + ((gpio >> 1) & 3); }
inline void pwm_init(uint slice, pwm_config *config, bool start) {
    ++initCalls[slice]; configurations[slice] = *config;
    levels[slice][0] = levels[slice][1] = 0; enabled[slice] = start;
}
constexpr int GPIO_FUNC_PWM = 4;
inline void gpio_set_function(int, int) { ++muxCalls; }
inline void pwm_set_enabled(uint slice, bool on) { ++enabledCalls[slice]; enabled[slice] = on; }
inline void pwm_set_gpio_level(uint pin, uint16_t level) {
    levels[pwm_gpio_to_slice_num(pin)][pin & 1] = level;
    writes.emplace_back(pin, level);
    if (writes.size() > 1000) throw std::runtime_error("movement did not terminate");
}
