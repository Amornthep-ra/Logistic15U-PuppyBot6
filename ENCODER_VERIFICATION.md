# Encoder update — local 0.1.1-dev, 2026-10-21

The encoder API now lives in `PuppyBotEncoders.h`, included by `PuppyBotIMU.h`.
The original `PuppyBotIMU` namespace, channel numbers, pins and public API names
are retained. The combined header still exposes the API as before.

- Removed writes that set the other encoder's pins to OUTPUT.
- Begin each selected encoder once, including when read/reset initializes it.
- Added `init_encoders()` to begin both counters.
- Reset only the selected encoder; invalid channel IDs no longer select encoder 2.
- Installed matching development files in `Documents/Arduino/libraries/Logistic15U-PuppyBot6`.
- Kept published 0.1.0 archives and the user's main sketch unchanged. No GitHub upload.

## Verification

Host test command: `python tests/encoders/run.py`.

Eight scenarios pass: both initialization orders, lazy initialization, repeated
initialization, independent resets and continued counting, reset before init,
invalid IDs, and combined initialization. A baseline made from the original
encoder declarations and functions reproduced five failures before the fix.
These tests execute the actual API header with a fake driver; they do not prove
electrical or PIO timing behavior on a physical board.

Arduino CLI 1.5.1, Arduino-Pico 6.2.0, `logistic15u:rp2040:logistic15u`:

| Sketch using the installed development library | Result | Flash | Static RAM |
|---|---|---:|---:|
| EncoderDual | PASS | 53,552 bytes | 9,776 bytes |
| User's Logistic15U-PuppyBot6.ino | PASS | 103,360 bytes | 11,048 bytes |

Logs: `.arduino-test/compile-encoder-dual.log` and
`.arduino-test/compile-user-encoder.log`. Previous installed files were copied
to `.arduino-test/encoder-installed-backup` before the update.

Next physical check is the EncoderDual example: turn both encoders and send `1`
or `2` over Serial to reset each counter independently. The example does not
initialize motor or servo outputs. GPIO 17/18 and 19/20 are inherited source
values, not new wiring measurements. No encoder pulses-per-revolution value
was added. Call encoder init/read/reset from one CPU core. No firmware uploaded.
