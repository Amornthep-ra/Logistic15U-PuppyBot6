# Changes for Arduino IDE 0.1.0

- Added library metadata, `Logistic15U_PuppyBot6.h`, an example and a thin board package referencing Arduino-Pico 6.2.0.
- Resolve `Adafruit_ST7789.h` before `Adafruit_ST7735.h` so a clean dependency scan selects the combined 1.11.0 display library even when the old ST7735-only library is also installed. Old cached dependency choices may require a clean rebuild.
- `main/main.ino` uses the original setup/loop/setup1/loop1 code with the combined include. `reference/original-main/main/main.ino` is unmodified.
- Replaced the private InvenSense SDK with the separately installed SparkFun ICM-20948 library 1.3.2. No vendor SDK or DMP firmware is copied into this distribution.
- `initIMU()` now returns success/failure instead of blocking forever on failure. Added `isIMUReady()`, `hasIMUSample()`, `getIMUSampleTimeMs()` and `getIMUStatus()`.
- The adapter enables Quat6 game rotation DMP output at the driver's maximum rate (55 Hz per SparkFun's example), instead of the old driver's requested 200 Hz. Wire1 now runs at 400 kHz rather than 1 MHz.
- The board passes `ICM_20948_USE_DMP` globally and includes `string.h` for C sources to support SparkFun 1.3.2 on the C23 compiler. No downloaded dependency is patched.
- Only valid Quat6 packets update cached angles. Empty FIFO reads leave the previous values intact.
- Preserved the old wrapper's quaternion index order and existing pitch/roll swaps and signs. Physical orientation and drift are unverified; repeat calibration after this driver change.

## Limits

The original API is implemented in headers with global state. Include it only in the main translation unit. Multiple `.ino` tabs become one translation unit; including the API in multiple `.cpp` files is not supported in this initial port. Existing two-core shared state and blocking routines are preserved.

`main` is a compatibility example. `puppybot_setup()` initializes servo outputs, and core 1 initializes IMU and color hardware. Compilation does not confirm wiring, servo endpoints, encoder pulses, physical flash or runtime orientation. No firmware was uploaded during verification.

Some files from the original copy stage moved to local-only `reference/legacy-imu`, and `PuppyBotIMU.h` was ported. Use `dist/manifest.json` for current distributed hashes.

## References

- [Arduino platform references](https://docs.arduino.cc/arduino-cli/platform-specification/)
- [Arduino package index format](https://docs.arduino.cc/arduino-cli/package_index_json-specification/)
- [Arduino library format](https://docs.arduino.cc/arduino-cli/library-specification/)
- [SparkFun ICM-20948 source and examples](https://github.com/sparkfun/SparkFun_ICM-20948_ArduinoLibrary)
