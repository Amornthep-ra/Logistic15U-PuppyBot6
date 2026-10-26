# Encoder API regression tests

Run `python tests/encoders/run.py` with a host `g++` on PATH.
The tests compile the actual `PuppyBotEncoders.h` against a fake `PioEncoder`.
They exercise both initialization orders, lazy initialization, repeated init,
independent resets with further counting, reset before init, invalid channel
IDs, and the two-channel convenience initializer.

The original functions reproduced failures for both initialization orders,
repeated initialization, reset before initialization and invalid IDs. The fixed
API passes all eight scenarios. The fake models pin direction and independent
counts; it does not simulate the PIO instruction program, electrical signals,
RP2350 timing or cross-core calls. Verify those with the EncoderDual example on
the real board. All encoder API calls should have one owning CPU core.
