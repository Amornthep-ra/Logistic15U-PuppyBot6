#pragma once
#include <stdint.h>
#include <math.h>

namespace PuppyBotDrive {
    struct WheelSpeedConfig {
        uint8_t encoder = 0;
        int8_t encoderSign = 0; // Measured sign of encoder change for motor(channel, positive).
        float referencePwm = 30;
        float referenceTicksPerSecond = 0; // Positive measured magnitude; never a guessed CPR.
        float kp = 0.20f, ki = 0.50f; // Initial tuning values, not measured robot gains.
        float pwmLimit = 100;
        bool valid() const {
            return (encoder == 1 || encoder == 2) && (encoderSign == 1 || encoderSign == -1) &&
                isfinite(referencePwm) && referencePwm > 0 && referencePwm <= 100 &&
                isfinite(referenceTicksPerSecond) && referenceTicksPerSecond > 0 &&
                isfinite(kp) && kp >= 0 && isfinite(ki) && ki >= 0 && (kp > 0 || ki > 0) &&
                isfinite(pwmLimit) && pwmLimit > 0 && pwmLimit <= 100 && referencePwm <= pwmLimit;
        }
    };
    struct WheelSpeedSettings {
        WheelSpeedConfig motor1, motor2;
        uint32_t samplePeriodMs = 20;
        uint32_t maxFeedbackGapMs = 250;
        uint32_t stallTimeoutMs = 1000;
        uint32_t directionGraceMs = 250;
        WheelSpeedSettings() { motor1.encoder = 1; motor2.encoder = 2; }
        bool valid() const {
            return motor1.valid() && motor2.valid() && motor1.encoder != motor2.encoder &&
                samplePeriodMs > 0 && samplePeriodMs <= 1000 && maxFeedbackGapMs > samplePeriodMs &&
                maxFeedbackGapMs <= 60000 && stallTimeoutMs > 0 && stallTimeoutMs <= 60000 &&
                directionGraceMs <= 60000;
        }
    };
    enum class WheelSpeedStatus : uint8_t {
        NotConfigured, Ready, Running, Stopped, InvalidCommand, FeedbackLate, EncoderDirection, EncoderStalled
    };
    inline const char *wheelSpeedStatusName(WheelSpeedStatus s) {
        switch (s) {
            case WheelSpeedStatus::NotConfigured: return "NOT CONFIGURED";
            case WheelSpeedStatus::Ready: return "READY";
            case WheelSpeedStatus::Running: return "RUNNING";
            case WheelSpeedStatus::Stopped: return "STOPPED";
            case WheelSpeedStatus::InvalidCommand: return "INVALID COMMAND";
            case WheelSpeedStatus::FeedbackLate: return "FEEDBACK LATE";
            case WheelSpeedStatus::EncoderDirection: return "ENCODER DIRECTION";
            case WheelSpeedStatus::EncoderStalled: return "ENCODER NO PULSES";
        }
        return "UNKNOWN";
    }
    struct WheelSpeedReading {
        int32_t count = 0;
        float targetTicksPerSecond = 0, ticksPerSecond = 0, pwm = 0, integral = 0;
    };
    struct WheelSpeedTelemetry {
        WheelSpeedReading motor1, motor2;
        WheelSpeedStatus status = WheelSpeedStatus::NotConfigured;
        uint32_t sampleMs = 0, samples = 0;
    };

    // Pure controller. One owner calls begin/update/stop; no pins, flash, IMU or encoder resets.
    class WheelSpeedController {
        WheelSpeedSettings settings_;
        WheelSpeedTelemetry data_;
        uint32_t sampledAt_ = 0, pulseAt_[2] = {}, changedAt_[2] = {};
        int targetSign_[2] = {};
        static float clamp(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
        static int sign(float x) { return x > 0 ? 1 : x < 0 ? -1 : 0; }
        static int64_t countDelta(int32_t current, int32_t previous) {
            const uint32_t delta = uint32_t(current) - uint32_t(previous);
            return delta <= 0x7fffffffu ? int64_t(delta) : int64_t(delta) - 0x100000000LL;
        }
        bool fail(WheelSpeedStatus status) {
            data_.status = status;
            data_.motor1.pwm = data_.motor2.pwm = 0;
            data_.motor1.integral = data_.motor2.integral = 0;
            return false;
        }
        bool wheel(unsigned index, const WheelSpeedConfig &config, WheelSpeedReading &v,
                   uint32_t now, uint32_t dtMs, int32_t count, float command, bool first) {
            command = clamp(command, -100, 100);
            const int direction = sign(command);
            const int64_t delta = countDelta(count, v.count) * config.encoderSign;
            v.count = count;
            v.targetTicksPerSecond = command / config.referencePwm * config.referenceTicksPerSecond;
            if (!isfinite(v.targetTicksPerSecond)) return fail(WheelSpeedStatus::InvalidCommand);
            v.ticksPerSecond = first ? 0 : float(delta) * 1000.0f / dtMs;
            if (first || direction != targetSign_[index]) {
                targetSign_[index] = direction; changedAt_[index] = pulseAt_[index] = now;
                v.integral = 0;
            }
            if (direction == 0) {
                v.pwm = v.integral = 0; pulseAt_[index] = now;
                return true;
            }
            if (delta != 0) pulseAt_[index] = now;
            if (!first && v.ticksPerSecond * direction < 0 &&
                uint32_t(now - changedAt_[index]) >= settings_.directionGraceMs)
                return fail(WheelSpeedStatus::EncoderDirection);
            // Allow at least three expected pulse intervals at low target rates.
            const double pulseWindow = 3000.0 / fabs(double(v.targetTicksPerSecond));
            const uint32_t expectedWindow = pulseWindow > 60000 ? 60000 : uint32_t(ceil(pulseWindow));
            const uint32_t stallLimit = expectedWindow > settings_.stallTimeoutMs ? expectedWindow : settings_.stallTimeoutMs;
            if (!first && uint32_t(now - pulseAt_[index]) >= stallLimit)
                return fail(WheelSpeedStatus::EncoderStalled);
            const float lo = direction > 0 ? 0 : -config.pwmLimit;
            const float hi = direction > 0 ? config.pwmLimit : 0;
            if (first) { v.pwm = clamp(command, lo, hi); return true; }
            const float error = (v.targetTicksPerSecond - v.ticksPerSecond) /
                                config.referenceTicksPerSecond * config.referencePwm;
            if (!isfinite(error)) return fail(WheelSpeedStatus::InvalidCommand);
            const float proposedI = clamp(v.integral + config.ki * error * (dtMs / 1000.0f),
                                          -config.pwmLimit, config.pwmLimit);
            const float proposedOutput = command + config.kp * error + proposedI;
            if (!((proposedOutput > hi && error > 0) || (proposedOutput < lo && error < 0)))
                v.integral = proposedI;
            v.pwm = clamp(command + config.kp * error + v.integral, lo, hi);
            return true;
        }
    public:
        bool begin(const WheelSpeedSettings &settings, int32_t count1, int32_t count2, uint32_t now) {
            settings_ = settings; data_ = WheelSpeedTelemetry{};
            data_.motor1.count = count1; data_.motor2.count = count2;
            sampledAt_ = now;
            for (unsigned i = 0; i < 2; ++i) { pulseAt_[i] = changedAt_[i] = now; targetSign_[i] = 0; }
            if (!settings.valid()) return false;
            data_.status = WheelSpeedStatus::Ready;
            return true;
        }
        bool configured() const { return settings_.valid(); }
        bool ready() const { return data_.status == WheelSpeedStatus::Ready || data_.status == WheelSpeedStatus::Running; }
        bool due(uint32_t now) const {
            return data_.status == WheelSpeedStatus::Ready ||
                (data_.status == WheelSpeedStatus::Running && uint32_t(now - sampledAt_) >= settings_.samplePeriodMs);
        }
        bool update(uint32_t now, int32_t count1, int32_t count2, float command1, float command2) {
            if (!ready()) return false;
            if (!isfinite(command1) || !isfinite(command2)) return fail(WheelSpeedStatus::InvalidCommand);
            if (!due(now)) return true;
            const bool first = data_.status == WheelSpeedStatus::Ready;
            const uint32_t dt = now - sampledAt_;
            if (!first && dt > settings_.maxFeedbackGapMs) return fail(WheelSpeedStatus::FeedbackLate);
            if (!wheel(0, settings_.motor1, data_.motor1, now, dt, count1, command1, first)) return false;
            if (!wheel(1, settings_.motor2, data_.motor2, now, dt, count2, command2, first)) return false;
            data_.status = WheelSpeedStatus::Running; data_.sampleMs = first ? 0 : dt; ++data_.samples;
            sampledAt_ = now;
            return true;
        }
        void stop() {
            data_.motor1.pwm = data_.motor2.pwm = 0;
            data_.motor1.integral = data_.motor2.integral = 0;
            if (ready()) data_.status = WheelSpeedStatus::Stopped;
        }
        bool rejectCommand() { return fail(WheelSpeedStatus::InvalidCommand); }
        WheelSpeedTelemetry telemetry() const { return data_; }
        WheelSpeedSettings settings() const { return settings_; }
    };
}
