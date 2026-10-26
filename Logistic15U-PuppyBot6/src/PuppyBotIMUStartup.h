#pragma once
#include <atomic>
#include <stdint.h>

namespace PuppyBotIMU {
    enum class StartupState : uint8_t {
        Waiting, Initializing, Calibrating, Ready, Disabled, InitFailed, NoData, TimedOut
    };

    // Only this atomic snapshot crosses cores; the IMU and I2C stay on core 1.
    struct StartupStatus {
        std::atomic<StartupState> state{StartupState::Waiting};
        std::atomic<unsigned> completed{0}, total{0}, samples{0};
        std::atomic<int> error{0};

        bool start(bool enabled) {
            auto expected = StartupState::Waiting;
            return state.compare_exchange_strong(expected,
                enabled ? StartupState::Initializing : StartupState::Disabled) && enabled;
        }
        bool initialized(bool ok, int code) {
            error.store(code);
            if (!ok) {
                auto expected = StartupState::Initializing;
                state.compare_exchange_strong(expected, StartupState::InitFailed);
                return false;
            }
            return state.load() == StartupState::Initializing;
        }
        bool calibrate(unsigned rounds) {
            completed.store(0); samples.store(0); total.store(rounds);
            auto expected = StartupState::Initializing;
            return state.compare_exchange_strong(expected, StartupState::Calibrating);
        }
        bool finish(bool freshData) {
            auto expected = StartupState::Calibrating;
            return state.compare_exchange_strong(expected,
                freshData ? StartupState::Ready : StartupState::NoData) && freshData;
        }
        void timeout() {
            auto current = state.load();
            while (current == StartupState::Waiting || current == StartupState::Initializing ||
                   current == StartupState::Calibrating) {
                if (state.compare_exchange_weak(current, StartupState::TimedOut)) return;
            }
        }
    };
    inline StartupStatus startup;
    inline StartupState getIMUStartupState() { return startup.state.load(); }
    inline bool isIMUCalibrated() { return getIMUStartupState() == StartupState::Ready; }
}
