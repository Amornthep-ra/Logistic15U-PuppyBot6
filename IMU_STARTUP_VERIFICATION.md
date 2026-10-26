# IMU startup verification — 0.1.16-dev

Date: 2026-10-21. Local library and user sketch only. No firmware upload or GitHub publication.

## Behavior

Core 1 initializes the IMU and performs the existing 300-round yaw reference procedure. It publishes atomic startup status/progress. Core 0 owns the TFT startup screen, waits before the existing lift_down() and wait_SW1(), and handles SW1 acknowledgement on failure. USE_IMU=false skips IMU access. Existing runtime quaternion/yaw, encoder, RGB and servo implementations were not changed.

The calibration procedure now rejects absent, non-finite or stale data instead of reporting success just because the rounds elapsed. Freshness limit is 250 ms at completion; the UI watchdog defaults to 15000 ms. These are software availability limits, not measured sensor accuracy or a stillness detector. Successful startup does not guarantee gyro drift performance.

## Validation

- `python tests/imu_startup/run.py`: PASS. Runs extracted production init/calibration functions and the actual startup display/wait implementation. Covers connected startup, absent device, intentionally disabled IMU with no I2C touches, no frames, stale frames, invalid yaw, zero rounds, timeout during calibration, late worker completion, acknowledgement on SW1 release, rejection of a previously held button, auto-continue for ready/disabled, and millis rollover.
- Display tests: all eight startup states at 160x128 and 128x160, no out-of-bounds drawing, unchanged frames do not redraw, elapsed/progress changes do not clear the full screen. Raster previews generated in `.arduino-test/imu-startup-tests/`; landscape calibrating and failure previews inspected visually.
- `python tests/wait_sw1/run.py`: PASS, including RGB0/RGB1 calibration, Save/Exit and menu input ownership.
- `python tests/drive_2wd/run.py`: PASS.
- `python tests/encoders/run.py`: PASS for all eight scenarios.
- Arduino CLI compiled the actual user sketch successfully for `logistic15u:rp2040:logistic15u`. Compiler output: `.arduino-test/compile-imu-startup.log`.
- Project source headers and installed Arduino library headers/version match byte-for-byte. IMU runtime methods from updateIMU() onward match the pre-change backup exactly.
- `git -c core.whitespace=cr-at-eol diff --check`: PASS.

## Limits

No physical sensor, unplugged-wire or TFT test was performed. A core-0 timeout cannot terminate a core-1 I2C call that is physically stuck; RGB on that core may remain unavailable until restart. The failed state remains latched after timeout so a late completion cannot unexpectedly re-enable IMU use. The user sketch guards its IMU loop with isIMUCalibrated(); future mission code must also check this before using IMU-dependent movement. Angles displayed elsewhere are not valid when IMU startup was skipped or failed.

## Buzzer addition — 0.1.17-dev

IMU READY plays one 2400 Hz / 180 ms tone; InitFailed, NoData and TimedOut play one 440 Hz / 600 ms tone on the existing PUPPYBOT_BUZZER_PIN. Disabled mode remains silent. tone() is asynchronous; noTone() releases the buzzer before returning to the menu. No sketch changes or IMU calculation changes were needed.

`python tests/imu_startup/run.py`: PASS, including exactly one correct tone for each terminal result, no tone before Ready, silent Disabled, no repeated alert during a long error wait, ready before the screen opens, and SW1 acknowledgement at 290 ms while a 600 ms alert is scheduled, with explicit sound cleanup. Existing startup and TFT checks pass. Installed/project headers and library version match. `git -c core.whitespace=cr-at-eol diff --check`: PASS.

Actual user sketch Arduino CLI compile: PASS (see `.arduino-test/compile-imu-buzzer.log`). No physical audio test, firmware upload or GitHub upload was performed.

Sketch uses 122044 bytes (2%) of program storage space. Maximum is 4186112 bytes.
Global variables use 13464 bytes (2%) of dynamic memory, leaving 510824 bytes for local variables. Maximum is 524288 bytes.

## Partial display updates — 0.1.18-dev

The startup screen now clears only on initial entry or display dimensions changing. Cached centered character cells replace only changed glyphs with opaque text, including spaces when text shortens. No blank row is drawn first. Progress updates draw only new/removed segments, or recolor the occupied segment when the result changes. Header and panel remain untouched across status changes. The core-0 wait, button and buzzer implementation is byte-for-byte unchanged from 0.1.17-dev.

`python tests/imu_startup/run.py`: PASS. In addition to the startup and buzzer checks, all 64 previous/next state combinations in each of two orientations match fresh-frame pixels, catching leftover text/footer/bar pixels. Tests assert one full clear across all states, glyph/bar-only transition operations, no redraw of unchanged hints, a single six-pixel glyph update for a changed timer digit, and only the new progress segment being drawn. Rendered calibration preview inspected. Project and installed library headers match. `git -c core.whitespace=cr-at-eol diff --check`: PASS.

Actual user sketch compiled successfully; log `.arduino-test/compile-imu-partial.log`. No firmware upload, physical TFT flicker test or GitHub publication performed.

Sketch uses 122348 bytes (2%) of program storage space. Maximum is 4186112 bytes.
Global variables use 13464 bytes (2%) of dynamic memory, leaving 510824 bytes for local variables. Maximum is 524288 bytes.
