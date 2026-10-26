# Public URL verification — 0.2.0 — 2026-10-26

Result: **PASS**. The 0.2.0 board package and library install from the public URLs.
Tested commit: `4769eba9757907103beae5d96b69c15f1ec3915d` (`origin/main`).

| Check | Result |
|---|---|
| Anonymous HTTPS download of package index | HTTP 200, lists platforms 0.2.0 and 0.1.0 |
| Anonymous HTTPS download of Library ZIP 0.2.0 | HTTP 200 |
| Anonymous HTTPS download of Board ZIP 0.2.0 and 0.1.0 | HTTP 200 |
| Archive SHA-256 and size vs published `dist/manifest.json` | PASS |
| Board `logistic15u:rp2040@0.2.0` installed through the public package index | PASS |
| Library ZIP and all seven pinned dependencies installed into a fresh sketchbook | PASS |
| Installed library (52 files) and board (4 files) hashes vs public manifest | PASS, 0 mismatches |
| GitHub-downloaded `scripts/install.ps1` run end to end, compiles `main/main.ino` | PASS, exit 0 |
| Compile all 6 library examples and the user sketch against the installed 0.2.0 library | PASS, all exit 0 |

The repository was downloaded from `codeload.github.com` and its own
`scripts/install.ps1` was run with `-ConfigFile` pointing to an isolated Arduino
CLI configuration under `.arduino-test/public-url-0.2.0/` (data, downloads and
sketchbook). The existing Arduino-Pico 6.2.0 core and builtin tools were reused
via filesystem junctions; the Logistic15U board package, the library and every
dependency were installed fresh. Compiler output confirms the library was taken
from the isolated sketchbook (`Using library Logistic15U-PuppyBot6 at version 0.2.0`),
not from `Documents/Arduino/libraries`. Arduino CLI 1.5.1 from Arduino IDE 2.x.

## Build sizes (no firmware uploaded)

| Sketch | Program storage (of 4,186,112) |
|---|---|
| `main/main.ino` via install.ps1 | 104,304 bytes; static RAM 13,156 of 524,288 |
| OriginalMain | 104,304 bytes |
| EncoderDual | 53,552 bytes |
| RGBCalibration | 92,180 bytes |
| RGBClassify | 83,084 bytes |
| RGBMonitor | 83,520 bytes |
| WheelSpeedMeasure | 79,008 bytes |
| User sketch `Logistic15U-PuppyBot6` (outside the repository) | 128,132 bytes |

## Installation URLs

Add both URLs to Arduino IDE Preferences → Additional Boards Manager URLs:

```text
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json
```

Install Arduino-Pico 6.2.0 first, then Logistic15U-PuppyBot6 0.2.0.
Install the [Library ZIP](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.2.0.zip)
using Sketch → Include Library → Add .ZIP Library, plus the dependency versions
listed in README/library.properties. The library is distributed as a ZIP; it is
not registered in the central Arduino Library Manager.

## Download evidence

- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json — HTTP 200, 1864 bytes, SHA-256 `55be95448b475b43d2f4cfce364b8728f7a3729318e0795fd6dd4b3599a28019`
- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/manifest.json — HTTP 200, 10415 bytes, SHA-256 `b6adc52461bf3290cff9f74057dc956b733a9cf5ede597ae90b7f5728caddec3`
- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.2.0.zip — HTTP 200, 105490 bytes, SHA-256 `97038388e6f08688e35449664fa5aaf4b9112737cb615b997b3eeec8b8d08c16`
- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/logistic15u-rp2040-0.2.0.zip — HTTP 200, 11593 bytes, SHA-256 `3078daadb08ec0f71d7ef3570e6128877d50d185bcb46641b7659a6d73ac5d62`
- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/logistic15u-rp2040-0.1.0.zip — HTTP 200, 11597 bytes, SHA-256 `f65db16b352b1aa291c5afa14e0b803bc46f6963a3a9685a1d5a686318ab33cb`
- https://codeload.github.com/Amornthep-ra/Logistic15U-PuppyBot6/zip/refs/heads/main — HTTP 200, 827755 bytes, SHA-256 `75d3618b5aa2eb9c25f0bbce49d7896e8fd8f7fee668a12b32ad4ba25736ff62`

Local evidence (not in the repository; `.arduino-test/` is git-ignored):

- `.arduino-test/public-url-0.2.0/install-public.log`
- `.arduino-test/public-url-0.2.0/compile-all.log`
- `.arduino-test/public-url-0.2.0/url-results.json`
- `.arduino-test/public-url-0.2.0/arduino-cli.json`
- `.arduino-test/public-url-0.2.0/source/Logistic15U-PuppyBot6-main/`

Not covered: uploading firmware or running on robot hardware.

---

# Public URL verification — 0.1.0 — 2026-10-21

Result: **PASS**. Tested commit: `04f744ba6c5f4cc762fdc73113f2c21cda7561ff`.

| Check | Result |
|---|---|
| Anonymous HTTPS download of package index | HTTP 200 |
| Anonymous HTTPS download of Library ZIP | HTTP 200 |
| Anonymous HTTPS download of Board ZIP | HTTP 200 |
| Archive SHA-256 and size vs published manifest and local release | PASS |
| Board installation through the public package index | PASS |
| Library and all seven dependencies installed into a fresh sketchbook | PASS |
| Installed board and library file hashes vs public manifest | PASS |
| Run the GitHub-downloaded install.ps1 and compile downloaded main.ino | PASS, exit 0 |

Isolated under `.arduino-test/public-url/` with the Arduino-Pico 6.2.0 core and
builtin tools reused via junctions. Final build: 103,440 bytes program storage
and 11,048 bytes static RAM. No firmware was uploaded.

- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.1.0.zip — HTTP 200, 21223 bytes, SHA-256 `1872003287dae3f4f12e20e6727978430ba81923ad1f1c40043e051e3b412ebb`
- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/logistic15u-rp2040-0.1.0.zip — HTTP 200, 11597 bytes, SHA-256 `f65db16b352b1aa291c5afa14e0b803bc46f6963a3a9685a1d5a686318ab33cb`
