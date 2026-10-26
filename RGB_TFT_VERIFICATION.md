# RGB/TFT update — local 0.1.2-dev, 2026-10-21

Added the RGB reader/cache in `PuppyBotRGB.h` and the TFT monitor in
`PuppyBotRGBDisplay.h`, exposed by `Logistic15U_PuppyBot6.h`. Installed matching
development files in the Arduino IDE sketchbook. No commit/push or firmware upload.

## Behavior

- `beginRGB()` / `beginRGB(RGB0)` initialize both or one sensor.
- `readRGB(RGB0)` reads and caches one complete RGB tuple; `updateRGB()` reads both.
- `getRGB(RGB0)` / `getRGBSnapshot()` copy cached readings under a mutex.
- `showRGB()` displays both channels; `showRGB(RGB0)` / `(RGB1)` show one.
- `beginRGBDisplay()` draws the monitor again after switching from another page.
- The user's `.ino` now reads on core 1 and draws on core 0. Serial output,
  encoder reset commands and existing IMU updates remain integrated.
- Uses the original normalized scale and existing raw sensor values, integration
  time, gain and pins. No mission color classifier or calibration thresholds added.
- Missing devices display Offline and retry initialization at one-second intervals.
- Normal updates change only affected text fields, swatches and bar segments.
  The initial view, layout/rotation changes and explicit display reset redraw
  the full view. Unchanged readings issue no TFT drawing operations.

## Checks

`python tests/rgb/run.py`: PASS. Uses actual reader/display headers with host
sensor/Wire/mutex/TFT doubles and the externally installed GFX bitmap font.

- Single/both-channel routing and lazy/idempotent initialization.
- Complete samples, normalized values, raw values and old RGB1 cache compatibility.
- Disconnect, wrong device ID, reconnect and zero/very low light.
- Coherent cached RGB tuples with concurrent writer/reader threads.
- No draw operations on unchanged data (including timestamp-only changes).
- Partial updates stay in the changed card; shorter numbers leave no old pixels.
- Incremental rendering equals a fresh reference, including disconnect transitions.
- Both/single views and bounds at 160x128 landscape and 128x160 portrait.

Visually inspected host previews for both-channel landscape, single RGB1 and
both-channel portrait views. Preview corners are approximated by the host raster;
these are simulated images, not screenshots of physical TFT hardware.

Initial RGB-only TFT build: Arduino CLI 1.5.1, Arduino-Pico 6.2.0,
FQBN `logistic15u:rp2040:logistic15u`:

| Sketch using the installed development library | Result | Flash | Static RAM |
|---|---|---:|---:|
| User's Logistic15U-PuppyBot6.ino | PASS | 109,296 bytes | 11,224 bytes |
| RGBMonitor example | PASS | 82,152 bytes | 10,940 bytes |

### Clear extension

Clear was already read into `RGBReading.clear` in the same sensor call. The TFT
now shows a five-digit raw C/Clear field per channel, and the user's Serial
output is `RGB0(R,G,B,C)` / `RGB1(R,G,B,C)`. Clear remains unnormalized.

The host suite passes with independent raw Clear values of 0 and 65535, one
sensor read per RGB/C sample, a Clear-only dirty region, and 12345-to-9 digit
cleanup compared with a fresh raster. Updated landscape, portrait and single
views were visually inspected. Installed renderer and example guide match source.

The user's updated sketch compiles successfully with **109,560 bytes Flash**
and **11,224 bytes static RAM**. Log: `.arduino-test/compile-user-rgb-clear.log`.
Library remains local 0.1.2-dev; no firmware upload or GitHub publication.

Source and installed development library files match byte for byte. Published
0.1.0 ZIP checksums are unchanged. Whitespace check passed with CRLF allowed.

## Evidence and use

- `.arduino-test/rgb-tests/results.txt`
- `.arduino-test/rgb-tests/dual-landscape-4x.png`
- `.arduino-test/rgb-tests/single-rgb1-4x.png`
- `.arduino-test/rgb-tests/dual-portrait-4x.png`
- `.arduino-test/compile-user-rgb-tft.log`
- `.arduino-test/compile-rgb-monitor.log`
- Old installed metadata/header: `.arduino-test/rgb-installed-backup/`
- Previous sketch: `.arduino-test/sketch-backups/*before-rgb-library*.ino`

Read the API guide in `Logistic15U-PuppyBot6/examples/RGBMonitor/README.md`.
I2C initialization/reads need one owning core, and TFT calls need one owning
core. Cached getters can cross cores. Each channel contains its latest complete
tuple; the two sensors are read sequentially, not at the same instant.

Physical I2C/TFT wiring, colors, refresh behavior and multicore SDK mutex behavior
have not been checked on a connected board. Those require an on-board trial.
