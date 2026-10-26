#pragma once
#include "PuppyBotBuzzer.h"
#include "PuppyBotButtons.h"

using RGBButtonEvent = PuppyBotInput::ButtonEvent;

namespace PuppyBotRGBDetail {
    class CalibrationFeedback {
        enum class Phase : uint8_t { Idle, Single, CompleteFirst, CompleteGap, CompleteSecond };
        Phase phase_ = Phase::Idle;
        uint32_t startedAt_ = 0, duration_ = 0;
        void note(unsigned frequency, uint32_t duration, uint32_t now, Phase phase) {
            tone(PUPPYBOT_BUZZER_PIN, frequency, duration);
            startedAt_ = now; duration_ = duration; phase_ = phase;
        }
    public:
        void stop() {
            if (phase_ != Phase::Idle) noTone(PUPPYBOT_BUZZER_PIN);
            phase_ = Phase::Idle;
        }
        void update(uint32_t now) {
            if (phase_ == Phase::Idle || uint32_t(now - startedAt_) < duration_) return;
            if (phase_ == Phase::CompleteFirst) {
                noTone(PUPPYBOT_BUZZER_PIN);
                phase_ = Phase::CompleteGap; startedAt_ = now; duration_ = 50;
            } else if (phase_ == Phase::CompleteGap) {
                note(3200, 110, now, Phase::CompleteSecond);
            } else stop();
        }
        void play(RGBButtonEvent event, uint32_t now) {
            if (event == RGBButtonEvent::None) return;
            stop();
            note(event == RGBButtonEvent::Next ? 1400 : 2600,
                 event == RGBButtonEvent::Next ? 35 : 80, now, Phase::Single);
        }
        void complete(uint32_t now) {
            stop();
            note(2000, 70, now, Phase::CompleteFirst);
        }
    };
    static CalibrationFeedback calibrationFeedback;
}
