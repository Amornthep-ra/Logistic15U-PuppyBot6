#pragma once
#include "PuppyBotEncoderLinePD.h"
#include <Adafruit_ST7735.h>
#include <stdio.h>
#include <string.h>

struct LinePDMonitorReading {
    LinePDTelemetry pd;
    bool preview = true, targetsValid = true, rate1Valid = false, rate2Valid = false;
    float target1 = 0, target2 = 0, actual1 = 0, actual2 = 0, pwm1 = 0, pwm2 = 0;
    WheelSpeedStatus wheelStatus = WheelSpeedStatus::NotConfigured;
};

namespace PuppyBotLineMonitorDetail {
    template<class Display> class Screen {
        Display &display_;
        char wanted_[13][26] = {}, painted_[13][26] = {};
        uint16_t colors_[13] = {}, paintedColors_[13][25] = {};
        unsigned next_ = 0, columns_ = 0;
        static constexpr uint16_t background = 0x0845, panel = 0x192A;
        static constexpr uint16_t cyan = 0x4EDF, green = 0x4F30, amber = 0xFE68, red = 0xF986;
        void row(unsigned n, const char *text, uint16_t color = 0xFFFF, bool center = false) {
            const unsigned size = strlen(text) < columns_ ? unsigned(strlen(text)) : columns_;
            memset(wanted_[n], ' ', columns_); wanted_[n][columns_] = 0;
            memcpy(wanted_[n] + (center ? (columns_ - size) / 2 : 0), text, size); colors_[n] = color;
        }
        static void number(char *out, size_t size, float v, unsigned width, unsigned decimals) {
            if (!isfinite(v)) { snprintf(out, size, "--"); return; }
            char full[48]; snprintf(full, sizeof(full), "%.*f", int(decimals), double(v));
            snprintf(out, size, "%s", strlen(full) <= width ? full : "RANGE");
        }
    public:
        explicit Screen(Display &display) : display_(display) {}
        void begin() {
            display_.setFont(nullptr); display_.setTextSize(1); display_.setTextWrap(false);
            display_.fillScreen(background);
            columns_ = unsigned((display_.width() - 8) / 6); if (columns_ > 25) columns_ = 25;
            memset(wanted_, 0, sizeof(wanted_)); memset(painted_, 0, sizeof(painted_));
            memset(paintedColors_, 0, sizeof(paintedColors_)); next_ = 0;
        }
        // One opaque 6x8 glyph per call. No row/full-screen erase while driving.
        bool drawPending() {
            for (unsigned i = 0; i < columns_ * 13; ++i) {
                const unsigned cell = (next_ + i) % (columns_ * 13), r = cell / columns_, c = cell % columns_;
                if (wanted_[r][c] == painted_[r][c] && colors_[r] == paintedColors_[r][c]) continue;
                const char text[] = {wanted_[r][c], 0};
                display_.setTextColor(colors_[r], (r == 0 || r == 6) ? panel : background);
                display_.setCursor(4 + int(c) * 6, 4 + int(r) * 9); display_.print(text);
                painted_[r][c] = wanted_[r][c]; paintedColors_[r][c] = colors_[r]; next_ = (cell + 1) % (columns_ * 13);
                return true;
            }
            return false;
        }
        void show(const LinePDMonitorReading &v, const LinePDOptions &options, bool deferred = false) {
            char text[64], a[12], b[12];
            row(0, v.pd.back ? "LINE PD / BACK" : "LINE PD / FRONT", cyan, true);
            const bool fault = v.wheelStatus != WheelSpeedStatus::NotConfigured && v.wheelStatus != WheelSpeedStatus::Ready &&
                               v.wheelStatus != WheelSpeedStatus::Running && v.wheelStatus != WheelSpeedStatus::Stopped;
            row(1, fault ? "STOP / PREVIEW" : v.preview ? (v.wheelStatus == WheelSpeedStatus::Running ? "WHEEL PI / PREVIEW" : "PREVIEW - NO DRIVE") :
                   v.pd.source == LinePDSource::Encoder ? "RUN / ENCODER + PD" : "RUN / LEGACY PD", fault ? red : v.preview ? amber : green, true);
            if (v.pd.valid) snprintf(text, sizeof(text), "Pos:%d Err:%+.0f", v.pd.position, double(v.pd.error));
            else snprintf(text, sizeof(text), "Pos:-- Err:--");
            row(2, text);
            const unsigned length = columns_ < 23 ? columns_ : 23;
            memset(text, '-', length); text[length] = 0;
            if (length >= 3) {
                text[0] = '['; text[length-1] = ']'; text[length/2] = '|';
                if (v.pd.valid) {
                    const int pos = v.pd.position < 0 ? 0 : v.pd.position > 700 ? 700 : v.pd.position;
                    text[1 + unsigned(pos) * (length - 3) / 700] = '#';
                }
            }
            row(3, text, v.pd.lineSeen ? green : amber, true);
            number(a, sizeof(a), v.pd.pTerm, 6, 2); number(b, sizeof(b), v.pd.dTerm, 6, 2);
            snprintf(text, sizeof(text), "P:%s D:%s", v.pd.valid ? a : "--", v.pd.valid ? b : "--"); row(4, text, cyan);
            number(a, sizeof(a), v.pd.correction, 6, 1);
            snprintf(text, sizeof(text), "Turn:%s %s", v.pd.valid ? a : "--", v.pd.limited ? "LIMIT" : ""); row(5, text, v.pd.limited ? amber : 0xFFFF);
            row(6, "ticks/s TARGET ACT", cyan, true);
            number(a, sizeof(a), v.target1, 6, 0); number(b, sizeof(b), v.actual1, 6, 0);
            snprintf(text, sizeof(text), "M1 %6s %6s", v.targetsValid ? a : "--", v.rate1Valid ? b : "--"); row(7, text, green);
            number(a, sizeof(a), v.target2, 6, 0); number(b, sizeof(b), v.actual2, 6, 0);
            snprintf(text, sizeof(text), "M2 %6s %6s", v.targetsValid ? a : "--", v.rate2Valid ? b : "--"); row(8, text, green);
            snprintf(text, sizeof(text), "PWM1:%+.0f PWM2:%+.0f", double(v.pwm1), double(v.pwm2)); row(9, text);
            number(a, sizeof(a), v.pd.dRaw, 5, 1); number(b, sizeof(b), v.pd.dFiltered, 5, 1);
            snprintf(text, sizeof(text), "dE:%s F:%s", v.pd.valid ? a : "--", v.pd.valid ? b : "--"); row(10, text);
            snprintf(text, sizeof(text), "dt:%lu F:%.0f C:%.0f", static_cast<unsigned long>(v.pd.dtMs),
                     double(options.derivativeFilterMs), double(options.maxCorrection)); row(11, text, amber);
            row(12, fault ? PuppyBotDrive::wheelSpeedStatusName(v.wheelStatus) : !v.pd.valid ? "LINE CAL REQUIRED" :
                    !v.pd.lineSeen ? "NO LINE DETECTED" : v.wheelStatus == WheelSpeedStatus::NotConfigured ? "ENCODER NOT SET" : "LINE SEEN", fault ? red : 0x9CD3, true);
            if (!deferred) while (drawPending()) {}
        }
    };

    static bool enabled = false, busy = false, sampled = false, wasPreview = true, rateBaseline = false;
    static uint32_t at = 0, rateAt = 0;
    static int32_t count1 = 0, count2 = 0;
    static int previewSpeed = 30;
    static float previewKp = 0, previewKd = 0;
    static LineSide side = LineSide::Front;
    static PuppyBotLinePDDetail::Controller preview;
    static LinePDOptions previewOptions;
    static LinePDMonitorReading reading;
    static Screen<Adafruit_ST7735> screen(tft_);
    inline float rate(int32_t value, int32_t previous, int sign, uint32_t dt) {
        const uint32_t delta = uint32_t(value) - uint32_t(previous);
        const int64_t ticks = delta <= 0x7fffffffu ? int64_t(delta) : int64_t(delta) - 0x100000000LL;
        return float(ticks * sign) * 1000 / dt;
    }
}

inline void updateLinePDMonitor();
inline bool isLinePDMonitorActive() { return PuppyBotLineMonitorDetail::enabled; }
// Enable after wait_SW1 and before a movement. PREVIEW never commands motors.
inline bool beginLinePDMonitor(int previewSpeed, float kp, float kd, LineSide side = LineSide::Front) {
    using namespace PuppyBotLineMonitorDetail;
    if (getLinePDTelemetry().running || getWheelSpeedTelemetry().status == WheelSpeedStatus::Running ||
        previewSpeed < 0 || previewSpeed > 100 || !isfinite(kp) || !isfinite(kd) || kp < 0 || kd < 0 || unsigned(side) > 1) return false;
    PuppyBotLineMonitorDetail::previewSpeed = previewSpeed; previewKp = kp; previewKd = kd;
    PuppyBotLineMonitorDetail::side = side; sampled = false; rateBaseline = false; wasPreview = true;
    previewOptions = getLinePDOptions(); preview.reset(); reading = LinePDMonitorReading{};
    screen.begin(); enabled = true; PuppyBotLinePDDetail::monitorService = updateLinePDMonitor;
    return true;
}
inline void endLinePDMonitor() {
    PuppyBotLineMonitorDetail::enabled = false;
    PuppyBotLinePDDetail::monitorService = nullptr;
}
inline LinePDMonitorReading getLinePDMonitorReading() { return PuppyBotLineMonitorDetail::reading; }

inline void updateLinePDMonitor() {
    using namespace PuppyBotLineMonitorDetail;
    if (!enabled || busy) return;
    busy = true;
    const uint32_t now = millis();
    if (!sampled || uint32_t(now - at) >= 100) {
        sampled = true; at = now;
        const auto wheels = getWheelSpeedTelemetry();
        const auto latest = getLinePDTelemetry();
        const bool live = latest.valid && latest.running && uint32_t(now - latest.sampledAt) <= 250 &&
            (latest.source != LinePDSource::Encoder || wheels.status == WheelSpeedStatus::Running);
        reading = LinePDMonitorReading{}; reading.preview = !live; reading.wheelStatus = wheels.status;
        const auto options = getLinePDOptions();
        if (live) reading.pd = latest;
        else {
            if (!wasPreview || !previewOptions.same(options)) { preview.reset(); previewOptions = options; }
            const bool back = side == LineSide::Back;
            const bool calibrated = hasLineCalibration(side) && (back ? Sensor_mode_B : Sensor_mode_F) == 1 &&
                                    (back ? PID_NumPin_B : PID_NumPin) == 8;
            if (calibrated) {
                const int pos = back ? readline_B() : readline();
                preview.step(options, back, pos, 350, previewSpeed, previewKp, previewKd, now,
                    PuppyBotEncoderDriveDetail::settings.samplePeriodMs, (back ? state_on_Line_B : state_on_Line) != 0);
                reading.pd = preview.telemetry(); reading.pd.source = LinePDSource::Preview;
            } else { preview.reset(); reading.pd.back = back; }
        }
        wasPreview = !live;
        const auto config = PuppyBotEncoderDriveDetail::settings;
        const int32_t current1 = PuppyBotIMU::get_pulse_Encoder(config.motor1.encoder);
        const int32_t current2 = PuppyBotIMU::get_pulse_Encoder(config.motor2.encoder);
        const uint32_t dt = now - rateAt;
        if (wheels.status == WheelSpeedStatus::Running) {
            reading.target1 = wheels.motor1.targetTicksPerSecond; reading.target2 = wheels.motor2.targetTicksPerSecond;
            reading.actual1 = wheels.motor1.ticksPerSecond; reading.actual2 = wheels.motor2.ticksPerSecond;
            reading.rate1Valid = reading.rate2Valid = true;
            reading.pwm1 = wheels.motor1.pwm; reading.pwm2 = wheels.motor2.pwm;
        } else {
            reading.rate1Valid = rateBaseline && dt > 0 && (config.motor1.encoderSign == 1 || config.motor1.encoderSign == -1);
            reading.rate2Valid = rateBaseline && dt > 0 && (config.motor2.encoderSign == 1 || config.motor2.encoderSign == -1);
            if (reading.rate1Valid) reading.actual1 = rate(current1, count1, config.motor1.encoderSign, dt);
            if (reading.rate2Valid) reading.actual2 = rate(current2, count2, config.motor2.encoderSign, dt);
            if (live && latest.source == LinePDSource::Legacy) {
                reading.targetsValid = false; reading.pwm1 = latest.command1; reading.pwm2 = latest.command2;
            }
        }
        count1 = current1; count2 = current2; rateAt = now; rateBaseline = true;
        screen.show(reading, options, true);
    }
    screen.drawPending(); busy = false;
}
