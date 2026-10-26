# High-PuppyBot6 color method — local 0.1.7-dev

Historical verification for 0.1.7-dev. The eight-byte burst reader described below was replaced in 0.1.8-dev with High/Adafruit word reads. See [RGB diagnostics verification](RGB_DIAGNOSTICS_VERIFICATION.md) for the hardware symptom, current changes and remaining uncertainty.

Verified on 2026-10-21. Local changes only; no GitHub upload, package release or firmware upload.

## Behavior

- Both channels classify raw RGB proportions using the same squared Euclidean distance and runner-up separation as `High-PuppyBot6/include/HighRGBCheck.h::Check_Color`.
- Default maximum distance 0.035 and separation ratio 0.70. Clear gates signal validity only: minimum 10, saturation 21504 on any RGBC channel. No brightness-only white/black rule or invented reference colors.
- References are captured means for each channel's six colors. Existing SW1 navigation, 20-frame capture, sounds, TFT dirty updates, explicit Save/Load and CSV remain. Invalid light aborts capture without replacing the previous reference or playing completion audio.
- Both sensors use ATIME 0xEB (50.4 ms), gain 4x. Flash schema 2 is 328 bytes at 3072..3399, with ATIME/gain/minimum Clear metadata and CRC32. Old schema 1 and mismatched settings are rejected without writing Flash or replacing current RAM data. Recalibrate both sensors after upgrading; old optical settings are not rescaled.
- `RGBColorOptions.padding` is replaced by `maxDistance` and `separationRatio`. Existing simple getRGBColor/showRGBColor calls are unchanged. `candidates` counts references within the distance limit, not confidence.

## IMU scheduling

The installed Adafruit driver's `enable()` and `getRawData()` wait for integration. New RGB APIs instead use checked Wire transactions: device ID and configuration, scheduled 3 ms oscillator startup, ADC enable, then status and an eight-byte auto-increment RGBC read at intervals of at least 55 ms. Both sensors integrate concurrently. NACK, invalid ID, short read and stalled ADC paths recover without integration delays. Wire1 clock is not changed.

`updateRGB()` is now serviced each loop1 iteration after IMU processing. The user's IMU initialization, 300-round offset, cached pitch/roll/yaw/continuous yaw and reset-flag code were compared with the backup and preserved. PuppyBotIMU.h, PuppyBotTurnPID.h and PuppyBotEncoders.h also match that backup byte-for-byte.

Short I2C transactions still occupy the owning core. No actual IMU rate/jitter measurements or physical sensor tests were performed. Startup puppybot_setup() and legacy RGB functions still use the upstream driver; only the new begin/read/update RGB path has scheduled integration. Explicit EEPROM Save retains the core's existing brief pause of the other core; save while the robot is stopped.

Register and integration references: [TCS3472 manufacturer datasheet hosted by Adafruit](https://cdn-shop.adafruit.com/datasheets/TCS34725.pdf), plus the installed Adafruit_TCS34725.h/.cpp. Auto-increment command bit 5 reads successive register bytes; RGBC words are read low byte first.

## Checks

- `python tests/classifier/run.py`: 10,048 differential cases against the actual local High header's Check_Color, with distinct sensor profiles, all six reference colors, brightness-scaled and deterministic generated inputs. The reference code is extracted into ignored build output; not vendored into the library. Also checks invalid light, saturation, ambiguity, invalid tolerance, cached-only APIs, stale/wrapped time, v2 round trip, mismatched metadata and a correctly checksummed v1 record.
- `python tests/rgb/run.py`: separate buses, original pins, phased startup and reconnect, correct settings and burst order, cadence, no duplicate cached-frame sequencing, NACK/ID/short-read/stalled-ADC errors, clock wrap, cross-core cache consistency and TFT rendering. Zero delay calls and zero upstream begin/getRawData calls in the new reader. Host Wire doubles are used; this is not electrical validation.
- `python tests/calibration/run.py`: raw statistics, distinct-frame counting, invalid-light rejection, cancellation, disconnect/timeout, persistent full and partial profiles, CRC, unrelated EEPROM preservation, SW1 menus/debounce/hold/save, buzzer timing, TFT bounds/dirty drawing.
- `python tests/classifier/run_display.py`: cached-only TFT APIs, refresh timing, status/name-only redraw, no ghost pixels, landscape and portrait bounds.
- Source and installed library: all 30 files match. `git -c core.whitespace=cr-at-eol diff --check` passes.

## Arduino builds

Arduino CLI with the user's Arduino IDE configuration, FQBN `logistic15u:rp2040:logistic15u`, Arduino-Pico 6.2.0, installed development library 0.1.7-dev.

| Sketch | Flash | Global RAM | Log |
| --- | ---: | ---: | --- |
| User Logistic15U-PuppyBot6.ino with IMU | 107588 bytes | 12072 bytes | `.arduino-test/compile-user-high-rgb.log` |
| RGBCalibration example with SW1 and Save | 91004 bytes | 12916 bytes | `.arduino-test/compile-high-calibration.log` |

Pre-change backup: `.arduino-test/before-high-rgb-20261021-155646/` includes the library, user sketch and user README. Published dist/0.1.0 files were not rebuilt.
