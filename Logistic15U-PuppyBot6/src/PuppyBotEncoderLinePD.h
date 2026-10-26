#pragma once
#include "PuppyBotWheelSpeedControl.h"
#include "PuppyBotLinePD.h"
#include "PuppyBotEncoders.h"

using WheelSpeedSettings = PuppyBotDrive::WheelSpeedSettings;
using WheelSpeedConfig = PuppyBotDrive::WheelSpeedConfig;
using WheelSpeedStatus = PuppyBotDrive::WheelSpeedStatus;
using WheelSpeedTelemetry = PuppyBotDrive::WheelSpeedTelemetry;

namespace PuppyBotEncoderDriveDetail {
    static PuppyBotDrive::WheelSpeedController controller;
    static WheelSpeedSettings settings;
}

// Use only on the motor/encoder owner core. IMU and RGB retain their own core.
inline bool beginWheelSpeedControl(const WheelSpeedSettings &settings) {
    using namespace PuppyBotEncoderDriveDetail;
    ao();
    PuppyBotEncoderDriveDetail::settings = settings;
    resetLinePD();
    const bool valid = settings.valid();
    const int32_t c1 = valid ? PuppyBotIMU::get_pulse_Encoder(settings.motor1.encoder) : 0;
    const int32_t c2 = valid ? PuppyBotIMU::get_pulse_Encoder(settings.motor2.encoder) : 0;
    return controller.begin(settings, c1, c2, millis());
}
inline bool restartWheelSpeedControl() {
    return beginWheelSpeedControl(PuppyBotEncoderDriveDetail::settings);
}
inline void stopWheelSpeedControl() {
    PuppyBotEncoderDriveDetail::controller.stop();
    resetLinePD();
    ao();
}
inline bool isWheelSpeedControlConfigured() { return PuppyBotEncoderDriveDetail::controller.configured(); }
inline bool isWheelSpeedControlReady() { return PuppyBotEncoderDriveDetail::controller.ready(); }
inline WheelSpeedTelemetry getWheelSpeedTelemetry() { return PuppyBotEncoderDriveDetail::controller.telemetry(); }

// Signed command percentages are converted to independent, measured pulse-rate targets.
inline bool updateWheelSpeedControl(float motor1Command, float motor2Command) {
    using namespace PuppyBotEncoderDriveDetail;
    const uint32_t now = millis();
    if (!controller.ready()) { ao(); return false; }
    const bool due = controller.due(now);
    const auto previous = controller.telemetry();
    const int32_t c1 = due ? PuppyBotIMU::get_pulse_Encoder(settings.motor1.encoder) : previous.motor1.count;
    const int32_t c2 = due ? PuppyBotIMU::get_pulse_Encoder(settings.motor2.encoder) : previous.motor2.count;
    if (!controller.update(now, c1, c2, motor1Command, motor2Command)) { ao(); return false; }
    if (due) {
        const auto values = controller.telemetry();
        motor(1, int(lroundf(values.motor1.pwm)));
        motor(2, int(lroundf(values.motor2.pwm)));
    }
    return true;
}

namespace PuppyBotEncoderDriveDetail {
    inline bool runLine(bool back, int speed, float kp, float kd) {
        if (!controller.ready()) { ao(); return false; }
        if (speed <= 0 || speed > 100 || !isfinite(kp) || !isfinite(kd) || kp < 0 || kd < 0) {
            controller.rejectCommand(); ao(); return false;
        }
        if (!hasLineCalibration(back ? LineSide::Back : LineSide::Front) ||
            (back ? Sensor_mode_B : Sensor_mode_F) != 1 || (back ? PID_NumPin_B : PID_NumPin) != 8) {
            stopWheelSpeedControl(); return false;
        }
        if (!controller.due(millis())) {
            if (PuppyBotLinePDDetail::monitorService) PuppyBotLinePDDetail::monitorService();
            delay(1); return true;
        }
        const int position = back ? readline_B() : readline();
        auto &pd = PuppyBotLinePDDetail::encoder;
        if (!pd.step(getLinePDOptions(), back, position, 350, speed, kp, kd, millis(),
                     settings.samplePeriodMs, (back ? state_on_Line_B : state_on_Line) != 0)) {
            controller.rejectCommand(); ao(); return false;
        }
        const auto values = pd.telemetry();
        const bool ok = updateWheelSpeedControl(values.command1, values.command2);
        if (ok) PuppyBotLinePDDetail::publish(values, LinePDSource::Encoder);
        if (PuppyBotLinePDDetail::monitorService) PuppyBotLinePDDetail::monitorService();
        delay(1);
        return ok;
    }
}

// These new APIs use the configured control period; retune line Kp/Kd for this period.
// Run_PID/Run_PID_B use the shared PD filter/limit without wheel PI.
inline bool Run_PID_Encoder(int speed, float kp, float kd) {
    return PuppyBotEncoderDriveDetail::runLine(false, speed, kp, kd);
}
inline bool Run_PID_B_Encoder(int speed, float kp, float kd) {
    return PuppyBotEncoderDriveDetail::runLine(true, speed, kp, kd);
}

namespace PuppyBotEncoderDriveDetail {
    // Average absolute travel of both wheels; uint32 subtraction keeps counter wrap safe.
    inline int32_t travelPulses(int32_t start1, int32_t start2) {
        const int32_t d1 = int32_t(uint32_t(PuppyBotIMU::get_pulse_Encoder(1)) - uint32_t(start1));
        const int32_t d2 = int32_t(uint32_t(PuppyBotIMU::get_pulse_Encoder(2)) - uint32_t(start2));
        return int32_t((int64_t(abs(d1)) + int64_t(abs(d2))) / 2);
    }

    inline bool runLinePulses(bool back, int speed, float kp, float kd, long targetPulses,
                              unsigned long timeoutMs, uint32_t rampMs) {
        ao();
        if (!hasLineCalibration(back ? LineSide::Back : LineSide::Front) ||
            (back ? Sensor_mode_B : Sensor_mode_F) != 1 || (back ? PID_NumPin_B : PID_NumPin) != 8 ||
            speed <= 0 || speed > 100 || targetPulses <= 0 || timeoutMs == 0 ||
            !isfinite(kp) || !isfinite(kd) || kp < 0 || kd < 0) {
            Serial.println("Line pulses: check calibration/settings");
            return false;
        }
        // Mode is chosen once per run; an encoder fault stops instead of falling back to legacy PD.
        const bool encoderMode = isWheelSpeedControlConfigured();
        if (encoderMode) { if (!restartWheelSpeedControl()) return false; }
        else resetLinePD();
        const int32_t start1 = PuppyBotIMU::get_pulse_Encoder(1);
        const int32_t start2 = PuppyBotIMU::get_pulse_Encoder(2);
        const uint32_t startedAt = millis();
        while (true) {
            if (travelPulses(start1, start2) >= targetPulses) {
                if (encoderMode) stopWheelSpeedControl(); else ao();
                return true;
            }
            const uint32_t elapsed = uint32_t(millis() - startedAt);
            if (elapsed >= timeoutMs) break;
            const int runSpeed = rampMs != 0 && elapsed < rampMs
                ? int(uint64_t(speed) * elapsed / rampMs) : speed;
            if (runSpeed == 0) { delay(1); continue; }
            if (encoderMode) {
                if (!runLine(back, runSpeed, kp, kd)) {
                    stopWheelSpeedControl();
                    Serial.println("Line pulses: encoder fault STOP");
                    return false;
                }
            } else if (back) Run_PID_B(runSpeed, kp, kd);
            else Run_PID(runSpeed, kp, kd);
        }
        if (encoderMode) stopWheelSpeedControl(); else ao();
        Serial.println("Line pulses: timeout STOP");
        return false;
    }
}

// Follow the line until the average travel of both encoders reaches targetPulses, then stop.
// Uses Encoder + line PD when wheel speed control is configured, otherwise Run_PID/Run_PID_B.
// true = target reached; false = invalid settings, timeout or encoder fault (motors stopped).
// rampMs ramps the base speed 0 -> speed at start; 0 disables the ramp.
inline bool Run_PID_Pulses(int speed, float kp, float kd, long targetPulses,
                           unsigned long timeoutMs = 10000, uint32_t rampMs = 300) {
    return PuppyBotEncoderDriveDetail::runLinePulses(false, speed, kp, kd, targetPulses, timeoutMs, rampMs);
}
inline bool Run_PID_B_Pulses(int speed, float kp, float kd, long targetPulses,
                             unsigned long timeoutMs = 10000, uint32_t rampMs = 300) {
    return PuppyBotEncoderDriveDetail::runLinePulses(true, speed, kp, kd, targetPulses, timeoutMs, rampMs);
}
