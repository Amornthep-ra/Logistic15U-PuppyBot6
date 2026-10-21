# Verification — 2026-10-21

Environment: Windows, Arduino CLI 1.5.1, Arduino-Pico 6.2.0, board 0.1.0.
FQBN: `logistic15u:rp2040:logistic15u`.

| Check | Result |
|---|---|
| Compile main against source library and local platform | PASS |
| Install board from package index via localhost HTTP using CLI core install | PASS |
| Install library using CLI lib install --zip-path | PASS |
| Compile main using installed board + installed library, without --library override | PASS |
| Compile unchanged reference/original-main/main/main.ino | PASS |
| Validate ZIP contents, CRC, SHA-256, source equivalence and index metadata | PASS |
| Parse PowerShell installer | PASS (live public URL workflow not executed) |
| Clean compile with the real Arduino IDE configuration and Documents/Arduino libraries | PASS |
| Public URL download / GitHub publication | PENDING |
| Upload or physical robot tests | NOT PERFORMED |

All successful main builds report **103,360 bytes program storage** out of 4,186,112
and **11,048 bytes static RAM** out of 524,288. These figures come from the compiler's
size report, not runtime heap/stack measurements. Installed-package build output
contains `main.ino.elf`, `main.ino.bin` and `main.ino.uf2` in the local test directory.

The test server served copies of the exact distribution ZIPs on 127.0.0.1; only the
index's download URL was changed for local testing. The custom board was installed
under Arduino15/packages/logistic15u/hardware/rp2040/0.1.0. The final library ZIP was also extracted into Documents/Arduino/libraries,
and its files were checked against the release manifest. The manual test-board copy
was moved out of the sketchbook before this test so it could not shadow that package.

Local logs (excluded from public bundle):

- `.arduino-test/compile-main.log`
- `.arduino-test/compile-installed.log` (verbose dependency and compiler commands)
- `.arduino-test/compile-original.log`
- `.arduino-test/compile-ide-clean.log` (final package in the real IDE sketchbook)

Dependencies and exact versions: `DEPENDENCIES.json` and `library.properties`.
The local IDE sketchbook also contains two legacy ST7735 1.0.8 copies. The port now
resolves ST7789 first to select the combined 1.11.0 library; existing libraries
were preserved. A clean rebuild is needed if an earlier build cached the old choice.

The DMP C source failed initially due to missing string.h; the board's C flags now
supply that standard header. No warnings or errors remain in the successful main build.

## Distribution hashes

- `Logistic15U-PuppyBot6-0.1.0.zip`: 21223 bytes; `SHA-256:1872003287dae3f4f12e20e6727978430ba81923ad1f1c40043e051e3b412ebb`
- `logistic15u-rp2040-0.1.0.zip`: 11597 bytes; `SHA-256:f65db16b352b1aa291c5afa14e0b803bc46f6963a3a9685a1d5a686318ab33cb`

## Repeat checks

```powershell
python .\scripts\build_release.py
python .\scripts\verify_distribution.py
arduino-cli compile --clean --fqbn logistic15u:rp2040:logistic15u .\main
```

The first two commands need Python only. Compilation requires the installed board,
core and libraries. No Arduino core, toolchain, third-party library source, local
SDK, user credentials or build cache is included in the distribution ZIPs.
