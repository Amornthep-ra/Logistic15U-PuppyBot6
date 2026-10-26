#pragma once
#include <stdint.h>
#include <math.h>

struct LinePDOptions {
    float derivativeFilterMs = 60; // Software starting values; tune on the real course.
    float maxCorrection = 20;      // Signed motor-command percentage points.
    bool allowWheelReverse = false;
    bool valid() const {
        return isfinite(derivativeFilterMs) && derivativeFilterMs >= 0 && derivativeFilterMs <= 2000 &&
               isfinite(maxCorrection) && maxCorrection >= 0 && maxCorrection <= 100;
    }
    bool same(const LinePDOptions &v) const {
        return derivativeFilterMs == v.derivativeFilterMs && maxCorrection == v.maxCorrection &&
               allowWheelReverse == v.allowWheelReverse;
    }
};
enum class LinePDSource : uint8_t { None, Legacy, Encoder, Preview };
struct LinePDTelemetry {
    bool valid = false, running = false, back = false, limited = false, lineSeen = false;
    int position = 350;
    float error = 0, pTerm = 0, dRaw = 0, dFiltered = 0, dTerm = 0;
    float correctionRaw = 0, correction = 0, command1 = 0, command2 = 0;
    uint32_t sampledAt = 0, dtMs = 0;
    LinePDSource source = LinePDSource::None;
};

namespace PuppyBotLinePDDetail {
    class Controller {
        bool initialized_ = false, back_ = false;
        float previous_ = 0, filtered_ = 0;
        uint32_t at_ = 0;
        LinePDTelemetry value_;
        static float clamp(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
    public:
        void reset() { initialized_ = false; filtered_ = 0; }
        bool step(const LinePDOptions &options, bool back, int position, int center, int speed,
                  float kp, float kd, uint32_t now, uint32_t nominalMs, bool seen) {
            value_ = LinePDTelemetry{};
            if (!options.valid() || speed < -100 || speed > 100 || nominalMs == 0 ||
                !isfinite(kp) || !isfinite(kd) || kp < 0 || kd < 0) { reset(); return false; }
            const float error = float(position - center);
            const uint32_t dt = now - at_;
            const bool continuous = initialized_ && back == back_ && dt > 0;
            const float raw = continuous ? (error - previous_) * nominalMs / dt : 0;
            const float alpha = options.derivativeFilterMs == 0 ? 1 :
                                float(dt) / (options.derivativeFilterMs + dt);
            filtered_ = continuous ? filtered_ + alpha * (raw - filtered_) : 0;
            const float p = kp * error, d = kd * filtered_, correction = p + d;
            if (!isfinite(correction)) { reset(); return false; }
            float limit = options.maxCorrection;
            const float magnitude = speed < 0 ? float(-speed) : float(speed);
            if (!options.allowWheelReverse && limit > magnitude) limit = magnitude;
            const float bounded = clamp(correction, -limit, limit);
            float c1 = back ? -(speed - bounded) : speed + bounded;
            float c2 = back ? -(speed + bounded) : speed - bounded;
            const float peak = fabsf(c1) > fabsf(c2) ? fabsf(c1) : fabsf(c2);
            if (peak > 100) { c1 *= 100 / peak; c2 *= 100 / peak; }
            if (speed == 0) c1 = c2 = 0;
            value_.valid = true; value_.back = back; value_.lineSeen = seen;
            value_.position = position; value_.error = error;
            value_.pTerm = p; value_.dRaw = raw; value_.dFiltered = filtered_; value_.dTerm = d;
            value_.correctionRaw = correction; value_.correction = bounded;
            value_.limited = bounded != correction || peak > 100;
            value_.command1 = c1; value_.command2 = c2; value_.sampledAt = now;
            value_.dtMs = continuous ? dt : 0;
            previous_ = error; at_ = now; back_ = back; initialized_ = true;
            return true;
        }
        LinePDTelemetry telemetry() const { return value_; }
    };
    static LinePDOptions options;
    static Controller legacyFront, legacyBack, encoder;
    static LinePDTelemetry latest;
    static void (*monitorService)() = nullptr;
    inline void publish(LinePDTelemetry v, LinePDSource source) {
        v.running = true; v.source = source; latest = v;
    }
}
inline LinePDOptions getLinePDOptions() { return PuppyBotLinePDDetail::options; }
inline LinePDTelemetry getLinePDTelemetry() { return PuppyBotLinePDDetail::latest; }
inline void resetLinePD() {
    using namespace PuppyBotLinePDDetail;
    legacyFront.reset(); legacyBack.reset(); encoder.reset(); latest.running = false;
}
// Configure before a movement segment. Does not command motors or reset encoders.
inline bool setLinePDOptions(const LinePDOptions &options) {
    if (!options.valid()) return false;
    PuppyBotLinePDDetail::options = options; resetLinePD(); return true;
}
