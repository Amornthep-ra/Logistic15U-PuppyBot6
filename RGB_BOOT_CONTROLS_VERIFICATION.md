# BOOT/SW1 calibration controls, 0.1.11-dev

BOOT press/release advances the menu; SW1 press/release selects or confirms, including Save all. Both debounce press and release for 30 ms. Holding generates no events until release, then at most one event. A button held on entry must be released before use. Overlapping presses are ignored until both release.

The old beginRGBCalibrationSW1/updateRGBCalibrationSW1 names remain source-compatible but now service both buttons. The RGB mode API calls them as before. No changes were made to wait_SW1 or its variants.

BOOT uses the installed Arduino-Pico 6.2.0 BOOTSEL accessor, not a guessed GPIO. Its implementation temporarily disables interrupts and idles the other core. We avoid reading it during setup, poll at most once per 20 ms in the menu, skip polling while capturing frames, and never poll in Show mode. This reduces interference; it does not guarantee zero impact on IMU timing. Actual board BOOT wiring and response remain to be tested by the user.

Validation:

- `python tests/calibration/run.py` passed. Updated button tests cover debounce, held startup, long hold/release, timer wrap, correct button mapping, overlapping presses, no BOOTSEL reads in begin, 20 ms polling, no BOOTSEL reads during capture, separate navigation/selection tones, capture completion tones, explicit Save all, storage and partial TFT redraw.
- `python tests/rgb_mode/run.py` passed with empty and saved flash. Show never reads BOOTSEL; mode transitions retain completed records; capture and save still work through the public mode interface.
- Landscape and portrait TFT previews were visually inspected. The footer reads `BOOT next SW1 OK`; prompts now refer to SW1 rather than holding for one second.
- The user's actual calibration sketch compiled for `logistic15u:rp2040:logistic15u` using installed 0.1.11-dev: 115588 bytes flash and 13332 bytes global RAM. Log: `.arduino-test/compile-user-rgb-boot-controls.log`.
- Source and installed library files are identical. setup1/loop1 are unchanged against `.arduino-test/before-rgb-boot-controls/`. IMU, puppybot.h (including wait_SW1), RGB acquisition, thresholds, classifier, calibration capture and Flash storage files are byte-identical to the backup. Flash v2 is unchanged.

The user sketch is set to `RGB_MODE_CALIBRATE` for the hardware trial. No board upload, GitHub push or distribution ZIP rebuild was performed.
