# Calibrate Line verification — 2026-10-22

## Update 0.1.20-dev — button responsiveness

A new regression (`python -X utf8 tests/line_calibration/responsive.py`) drives the actual menu using 60 ms BOOT/SW1 presses with an injected 12 ms cost per TFT transfer. Before the fix the replay failed with `FAIL menu failed to exit`; after cooperative rendering it passes both banks, exactly two Saves and Exit. The 12 ms cost is a test fixture, not a hardware timing measurement.

The TFT now queues changed rows and performs at most one row clear or one glyph transfer per loop, returning to input polling between operations. The line menu polls every 5 ms when the loop resumes; debounce/release semantics remain unchanged. Capture ignores extra button clicks instead of cancelling. It captures 16 samples, displays step 1/2 FLOOR and 2/2 LINE, and summarizes Min/Max/Gap. Saved 32-sample references remain valid (tested by decoding a legacy record). The quality rule is unchanged.

The line calibration suite, slow TFT regression and existing wait_SW1/RGB/Encoder integration pass. Source and installed files match. IMU headers and sketch setup1/loop1 were not edited. Arduino result: PASS, Arduino CLI exit 0; installed library 0.1.20-dev; flash 128,580 bytes and global RAM 14,832 bytes.


Version: 0.1.19-dev. Source library and the Arduino sketchbook installation were synchronized; hashes are recorded in `.arduino-test/line-calibration-tests/installed-manifest.json`. No firmware upload, motor run, Git commit, push or release publication was performed.

## Scope

Added menu item 6 (Calibrate Line), independent Front/Back Mux8 capture, all-channel and per-channel workflows, two-surface statistics and validation, explicit Save/Retry/Exit, persisted records and automatic loading inside wait_SW1. The TFT owner remains core 0. IMU initialization/update/offset code in setup1/loop1 is unchanged; executable sketch code is unchanged (only the wait_SW1 comment changed).

Fixed the legacy Min/Max extrema overwrite, normalization/position division-by-zero cases and centroid offset. Position, sum and boolean line detection now use the same checked normalization rules; sum still performs its own scan. No PD gains, timing law, Ramp, crossing counter or lost-line timeout changes were made. Saved line calibration configures the selected bank to Mux8 logical channels 0..7 and learns per-channel signal polarity. This does not verify physical left/right order.

## Repeatable host checks — PASS

Run from this repository:

```powershell
python -X utf8 tests/line_calibration/run.py
python -X utf8 tests/wait_sw1/run.py
python -X utf8 tests/imu_startup/run.py
```

- Production line calibration/controller: 32-sample captures, both acquisition methods, incomplete/noisy/identical surfaces rejected, both signal polarities, cancellation and retry.
- Flash doubles: independent front/back persistence, simulated restart, failed commit rollback, checksum/schema/bank/channel validation and preservation of RGB and every unrelated byte. No physical flash durability claim is made.
- Production function bodies extracted directly from puppybot.h: loaded references affect front/back position/sum/status; centered input returns the center; weak input/equal endpoints cannot divide by zero; legacy extrema remain correct after 0 or 1023 followed by 100.
- Real menu loop driven with debounced BOOT/SW1 events: both banks captured and explicitly saved, exactly two commits, motors stopped, Exit returns.
- TFT raster doubles using the GFX font: both 160x128 and 128x160 bounds; changed rows match a clean rendering; unchanged frames produce no drawing; no repeated full clears. Generated previews were visually inspected.
- Existing wait_SW1 integration: Start/Exit, RGB0/RGB1 capture/Save/Exit, actual ADC/Mux routing and independent Encoder readings pass.
- Existing IMU startup/buzzer/partial-display suite passes.

## Arduino compile

Board FQBN: `logistic15u:rp2040:logistic15u`, Arduino-Pico 6.2.0.
Sketch: `C:/Users/User/Desktop/PrinceBot Work/Example Code/Logistic15U-PuppyBot6/Logistic15U-PuppyBot6.ino`.
Compilation uses the library installed under `C:/Users/User/Documents/Arduino/libraries/Logistic15U-PuppyBot6`, verified in the build dependency file (no --library override in the final build).

PASS: 126,860 bytes flash (maximum 4,186,112); 14,152 bytes global RAM (maximum 524,288). Final compile exited 0.

## Physical verification still required

Upload the sketch, calibrate real Front and Back arrays, verify each channel’s Floor/Line/Gap and Saved status, power-cycle and verify reload, and check motor steering direction at low speed before line following. Confirm sensor height and lighting with the parcel load. Host tests and compilation cannot confirm sensor contrast, flash retention on the actual board, sound or physical driving accuracy. EEPROM.commit briefly pauses the other core as required by Arduino-Pico; the IMU task resumes afterward.
