#pragma once
#include "PuppyBotRGBCalibrationDisplay.h"
#include "PuppyBotRGBColorDisplay.h"

enum RGBMode : uint8_t { RGB_MODE_CALIBRATE, RGB_MODE_SHOW };

namespace PuppyBotRGBDetail {
    static RGBMode rgbMode = RGB_MODE_SHOW;
    static bool rgbModeStarted = false;
    static bool rgbModeActive = false;
}

// Call on the TFT core after puppybot_setup(). I2C stays in beginRGB/updateRGB.
// Load flash on first begin only; switching modes keeps completed RAM references.
inline void beginRGBMode(RGBMode mode, bool calibrationExit = false) {
    using namespace PuppyBotRGBDetail;
    if (mode != RGB_MODE_CALIBRATE && mode != RGB_MODE_SHOW) return;
    if (!rgbModeStarted) loadRGBCalibration();
    else cancelRGBCalibration();
    calibrationFeedback.stop();
    calibrationSW1Enabled = false;
    rgbMode = mode;
    rgbModeStarted = true;
    rgbModeActive = true;
    if (mode == RGB_MODE_CALIBRATE) {
        beginRGBCalibrationSW1(calibrationExit);
        calibrationDisplay.invalidate();
        showRGBCalibration();
    } else {
        beginRGBColorDisplay();
    }
}

// Stop this screen without saving or discarding completed RAM references.
inline void endRGBMode() {
    using namespace PuppyBotRGBDetail;
    cancelRGBCalibration();
    calibrationFeedback.stop();
    calibrationSW1Enabled = false;
    rgbModeActive = false;
}

// Call only on the TFT core. Saving remains an explicit SW1 menu action.
inline void updateRGBMode() {
    using namespace PuppyBotRGBDetail;
    if (!rgbModeActive) return;
    if (rgbMode == RGB_MODE_CALIBRATE) {
        updateRGBCalibrationSW1();
        updateRGBCalibration();
        showRGBCalibration();
    } else {
        showRGBColor();
    }
}
