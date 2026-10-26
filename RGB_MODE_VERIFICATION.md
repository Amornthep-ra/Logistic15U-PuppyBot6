# RGB mode helpers, 0.1.10-dev

`beginRGBMode(RGB_MODE_SHOW)` loads saved references and starts the color display. `beginRGBMode(RGB_MODE_CALIBRATE)` starts the SW1 calibration menu. Both use `updateRGBMode()` on the TFT core. The sensor owner core still calls `beginRGB()` and `updateRGB()` alongside the unchanged IMU code.

Flash loads only on the first valid begin call. Later begin calls cancel an incomplete capture, stop calibration feedback, and retain completed RAM references. Only the explicit Save all action writes flash. Update before begin does nothing; Show does not process calibration button events.

The source library and installed Arduino library are identical. Only `library.properties`, the umbrella header and new `PuppyBotRGBMode.h` changed in the library. Comparison with `.arduino-test/before-rgb-mode/` confirms the user's setup1/loop1 text is unchanged, along with all existing sensor, calibration, classification, storage and IMU implementations.

Host validation passed:

- `python tests/rgb_mode/run.py`: empty and saved flash startup, independent sensor references, SW1 capture and explicit Save all, no automatic save, Show ignoring calibration actions, cancelled capture and preserved unsaved records across mode switches, and no I2C transactions or delay calls from the mode interface.
- `python tests/calibration/run.py`: existing sampling, rejection, persistence, SW1, buzzer and partial TFT redraw behavior.
- `python tests/classifier/run_display.py`: existing color display, cached reads, refresh, stale feedback and partial redraw behavior.

Arduino compilation uses `logistic15u:rp2040:logistic15u` with installed 0.1.10-dev. The user sketch is kept in Show mode. The Calibrate build uses a copy at `.arduino-test/RGBModeCalibrate/RGBModeCalibrate.ino` with only the two mode-selection comments switched. Logs are `.arduino-test/compile-user-rgb-mode-show.log` and `.arduino-test/compile-user-rgb-mode-calibrate.log`.

Both compilations passed: Show uses 115468 bytes flash and 13180 bytes global RAM; Calibrate uses 115524 bytes flash and 13180 bytes global RAM. `git -c core.whitespace=cr-at-eol diff --check` passes with the repository's Windows line endings.

No firmware upload, GitHub push or release ZIP rebuild was performed. Physical SW1/TFT operation still needs validation on the robot.
