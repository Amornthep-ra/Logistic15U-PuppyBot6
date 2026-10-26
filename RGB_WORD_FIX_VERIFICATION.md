# RGB raw decoding fix, 0.1.9-dev

2026-10-21. User uploaded 0.1.8-dev and still reported RGB0 Red, FAIL R=2 G=4 B=6 C=0. Thresholds must not be lowered.

## Confirmed cause

The failed premise was that the displayed values came from sensor bytes. `PuppyBotRGB.h` named its byte-decoding lambda `word`. Arduino-Pico 6.2.0 `ArduinoCore-API/api/Common.h` defines `#define word(...) makeWord(__VA_ARGS__)`; `Common.cpp` implements the one-argument `makeWord(uint16_t w)` as `return w`.

Replaying the actual sketch compiler command from `compile_commands.json` with `-E` confirms that the lambda declaration survives, but its calls become:

```cpp
reading.clear = makeWord(0); reading.rawRed = makeWord(2);
reading.rawGreen = makeWord(4); reading.rawBlue = makeWord(6);
```

Thus a successful transfer always published 2/4/6/0 regardless of its bytes. Evidence: `.arduino-test/rgb-before-macro-preprocessed.txt`. This explains both the exact numbers and why the previous I2C protocol change did not help. It does not establish the physical sensor's current readings.

High-PuppyBot6's Adafruit driver combines the two bytes directly in `read16`, so it has no such collision. Its C/R/G/B register order and command bytes match our current four-word reader. The scalar/buffer Wire writes both ultimately enqueue individual bytes; that distinction is not this defect.

## Change and regression checks

- Rename the decoding lambda to `decodeRaw16` and use it for all four raw channels on both buses. No I2C transaction, optical setting, validity threshold, classifier, calibration storage, SW1, TFT or IMU behavior was changed.
- Include the Arduino `word` macro and both `makeWord` overloads in the host Arduino double. Previously the host environment omitted this macro, hiding the production fault.
- Assert independent raw RGBC tuples for RGB0 and RGB1. Before the fix the strict test failed on the unused lambda; running without warnings-as-errors also failed the raw-value assertion. After the fix the normal strict test passes.
- All four suites pass: `tests/rgb/run.py`, `tests/calibration/run.py`, `tests/classifier/run.py`, `tests/classifier/run_display.py`. The classifier includes 10,048 comparisons against actual High-PuppyBot6 logic. These are host tests, not physical sensor measurements.
- Actual user sketch compiled successfully for `logistic15u:rp2040:logistic15u` with installed 0.1.9-dev: 113748 bytes flash and 13168 bytes global RAM. Log: `.arduino-test/compile-user-rgb-word-fix.log`. Replaying the real compiler preprocessor after the fix confirms all four calls remain `decodeRaw16`; evidence: `.arduino-test/rgb-after-macro-preprocessed.txt`.
- All files in the source and installed libraries are byte-identical. Comparison against the backup confirms only `src/PuppyBotRGB.h` and `library.properties` changed in the library; the user sketch is byte-identical.

Backup: `.arduino-test/before-rgb-word-macro-fix/`. Only the reader header and library version change in the library distribution. The user's SW1-calibration sketch remains unchanged, including setup1/loop1 and commented color display. No GitHub push or firmware upload by the agent.

Physical follow-up: upload the existing user sketch using installed 0.1.9-dev, retry RGB0 Red, then calibrate both channels and explicitly Save all. Clear must still pass the existing minimum of 10; rejected samples must not be accepted as calibration references.
