# Logistic15U-PuppyBot6 0.2.0

Requires Earle Philhower Arduino-Pico `rp2040:rp2040@6.2.0` installed separately.
FQBN: `logistic15u:rp2040:logistic15u`.

This thin platform references the upstream core, generic_rp2350 variant and upload
tools. It does not redistribute the core or toolchain. `boards.txt` is derived
from the Arduino-Pico 6.2.0 Generic RP2350 profile (upstream license included).
RP2350B, ARM, 150 MHz and 4 MB flash are taken from PuppyBot6-New context.json.
These are source configuration values, not a physical flash probe.

The DMP flag is applied to all compilation units. The C-only `-include string.h`
supplies memcpy's declaration missing from SparkFun 1.3.2 DMP C source when used
with the upstream C23 compiler. No downloaded dependency is patched.

Use Arduino IDE 2.x with Arduino CLI >= 1.0.4 (tested CLI 1.5.1).
