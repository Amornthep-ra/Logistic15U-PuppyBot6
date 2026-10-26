#pragma once
#include "PuppyBotRGBCalibrationStorage.h"
#include "PuppyBotBuzzer.h"

extern int swpin;  // The original board's SW1 mapping, defined by puppybot.h.

enum class RGBCalibrationFocus : uint8_t { Sensor, Color };
enum class RGBCalibrationAction : uint8_t { None, Save, Exit };

namespace PuppyBotRGBDetail {
    using CalibrationButton = PuppyBotInput::ClickButton;
    using CalibrationInputs = PuppyBotInput::MenuButtons;

    class CalibrationMenu {
        RGBCalibrationFocus focus_ = RGBCalibrationFocus::Sensor;
        uint8_t choice_ = 0; // Six colors, Save all, Sensor menu, optional Exit.
        uint8_t sensorChoice_ = 0;
        bool allowExit_ = false;
        enum class Notice : uint8_t { None, Saved, NoSamples, SaveFailed };
        Notice notice_ = Notice::None;
        RGBChannel observedChannel_ = RGB0;
        RGBColor observedColor_ = RGBColor::Red;
    public:
        void begin(const RGBCalibration &data, bool allowExit = false) {
            allowExit_ = allowExit; sensorChoice_ = uint8_t(data.channel());
            focus_ = RGBCalibrationFocus::Sensor;
            choice_ = uint8_t(data.color()); notice_ = Notice::None;
            observedChannel_ = data.channel(); observedColor_ = data.color();
        }
        RGBCalibrationFocus focus() const { return focus_; }
        uint8_t choice() const { return choice_; }
        uint8_t sensorChoice() const { return sensorChoice_; }
        bool exitEnabled() const { return allowExit_; }
        int highlightedColor() const {
            return focus_ == RGBCalibrationFocus::Color && choice_ < 6 ? int(choice_) : -1;
        }
        void synchronize(const RGBCalibration &data) {
            // Serial selections remain usable alongside the button menu.
            if (data.channel() != observedChannel_ || data.color() != observedColor_) {
                choice_ = uint8_t(data.color()); notice_ = Notice::None;
                sensorChoice_ = uint8_t(data.channel());
                observedChannel_ = data.channel(); observedColor_ = data.color();
            }
        }
        RGBCalibrationAction handle(RGBButtonEvent event, RGBCalibration &data) {
            synchronize(data);
            if (event == RGBButtonEvent::None || data.collecting()) return RGBCalibrationAction::None;
            notice_ = Notice::None;
            if (event == RGBButtonEvent::Next) {
                if (focus_ == RGBCalibrationFocus::Sensor) {
                    sensorChoice_ = (sensorChoice_ + 1) % (allowExit_ ? 4 : 2);
                    if (sensorChoice_ < 2) data.select(RGBChannel(sensorChoice_), RGBColor::Red);
                } else {
                    choice_ = (choice_ + 1) % (allowExit_ ? 9 : 8);
                    if (choice_ < 6) data.select(data.channel(), RGBColor(choice_));
                }
            } else if (focus_ == RGBCalibrationFocus::Sensor && sensorChoice_ == 3) {
                return RGBCalibrationAction::Exit;
            } else if (focus_ == RGBCalibrationFocus::Sensor && sensorChoice_ == 2) {
                if (!data.completed(RGB0) && !data.completed(RGB1)) notice_ = Notice::NoSamples;
                else return RGBCalibrationAction::Save;
            } else if (focus_ == RGBCalibrationFocus::Sensor) {
                focus_ = RGBCalibrationFocus::Color;
                choice_ = uint8_t(data.color());
                data.select(data.channel(), data.color());
            } else if (choice_ < 6) {
                data.start(data.channel(), RGBColor(choice_));
            } else if (choice_ == 6) {
                if (!data.completed(RGB0) && !data.completed(RGB1)) notice_ = Notice::NoSamples;
                else return RGBCalibrationAction::Save;
            } else if (choice_ == 8) {
                return RGBCalibrationAction::Exit;
            } else {
                sensorChoice_ = uint8_t(data.channel());
                focus_ = RGBCalibrationFocus::Sensor;
                data.select(data.channel(), data.color());
            }
            observedChannel_ = data.channel(); observedColor_ = data.color();
            return RGBCalibrationAction::None;
        }
        void saved(bool success) { notice_ = success ? Notice::Saved : Notice::SaveFailed; }
        const char *message(const RGBCalibration &data) const {
            if (notice_ == Notice::Saved && !data.dirty() && data.storage() == RGBStorageState::Saved) return "Saved to flash";
            if (notice_ == Notice::NoSamples && !data.completed(RGB0) && !data.completed(RGB1)) return "No samples to save";
            if (notice_ == Notice::SaveFailed && data.storage() == RGBStorageState::Failed) return "Save failed: retry";
            if (allowExit_ && ((focus_ == RGBCalibrationFocus::Sensor && sensorChoice_ == 3) ||
                               (focus_ == RGBCalibrationFocus::Color && choice_ == 8)))
                return data.dirty() ? "Exit: RAM not saved" : "Exit to main menu";
            if (focus_ == RGBCalibrationFocus::Sensor && sensorChoice_ == 2) return "SW1: save flash";
            if (focus_ == RGBCalibrationFocus::Color && choice_ == 6) return "SW1: save flash";
            if (focus_ == RGBCalibrationFocus::Color && choice_ == 7) return "SW1: sensor menu";
            if (data.state() == RGBCaptureState::Offline) return "Offline: check RGB";
            if (data.state() == RGBCaptureState::TimedOut) return "No frames: retry";
            if (data.state() == RGBCaptureState::InvalidSignal) return data.rejectionMessage();
            if (data.state() == RGBCaptureState::Captured) return "Captured in RAM";
            if (focus_ == RGBCalibrationFocus::Sensor) return "Choose sensor";
            return "Place color, SW1";
        }
    };

    static CalibrationInputs calibrationInputs;
    static uint32_t calibrationButtonsPolledAt = 0;
    static bool calibrationButtonsPolled = false;
    static CalibrationMenu calibrationMenu;
    static bool calibrationSW1Enabled = false;
    static bool calibrationExitRequested = false;
}

// Call after puppybot_setup(), on the same core as the calibration/TFT/Serial APIs.
inline void beginRGBCalibrationSW1(bool allowExit = false) {
    PuppyBotRGBDetail::calibrationExitRequested = false;
    PuppyBotRGBDetail::calibrationFeedback.stop();
    pinMode(swpin, INPUT_PULLUP);
    // Do not read BOOTSEL during setup before the other core has started.
    PuppyBotRGBDetail::calibrationInputs.reset();
    PuppyBotRGBDetail::calibrationButtonsPolled = false;
    PuppyBotRGBDetail::calibrationMenu.begin(PuppyBotRGBDetail::calibration, allowExit);
    PuppyBotRGBDetail::calibrationSW1Enabled = true;
}
inline void updateRGBCalibrationSW1() {
    using namespace PuppyBotRGBDetail;
    if (!calibrationSW1Enabled) return;
    const uint32_t now = millis();
    calibrationFeedback.update(now);
    if (calibration.collecting()) {
        calibrationInputs.reset();
        return;  // No BOOTSEL polling while collecting frames.
    }
    // BOOTSEL briefly pauses the other core; sample at most once per 20 ms.
    if (calibrationButtonsPolled && uint32_t(now - calibrationButtonsPolledAt) < 20) return;
    calibrationButtonsPolled = true;
    calibrationButtonsPolledAt = now;
    const bool bootPressed = bool(BOOTSEL);
    const auto event = calibrationInputs.update(bootPressed, digitalRead(swpin) == LOW, now);
    const bool accepted = event != RGBButtonEvent::None;
    const auto action = calibrationMenu.handle(event, calibration);
    if (accepted) calibrationFeedback.play(event, now);
    if (action == RGBCalibrationAction::Save)
        calibrationMenu.saved(saveRGBCalibration());
    if (action == RGBCalibrationAction::Exit) calibrationExitRequested = true;
}
