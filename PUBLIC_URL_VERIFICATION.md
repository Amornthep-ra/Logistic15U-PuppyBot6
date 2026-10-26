# Public URL verification — 2026-10-21

Result: **PASS**. The public repository and installation URLs are available.
Tested commit: `04f744ba6c5f4cc762fdc73113f2c21cda7561ff`.

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

The Arduino CLI configuration, data directory, downloads and sketchbook were
isolated under `.arduino-test/public-url/`. The existing Arduino-Pico 6.2.0 core
and builtin tools were reused via filesystem junctions. The custom board and
libraries were installed fresh. No existing Logistic15U board installation or
existing user libraries were used in this test.

Final build: **103,440 bytes program storage** / 4,186,112 and **11,048 bytes
static RAM** / 524,288. These are the size results from this public-download
build; the earlier local build reported 103,360 bytes. No firmware was uploaded.

## Installation URLs

Add both URLs to Arduino IDE Preferences → Additional Boards Manager URLs:

```text
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json
```

Install Arduino-Pico 6.2.0 first, then Logistic15U-PuppyBot6 0.1.0.
Install the [Library ZIP](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.1.0.zip)
using Sketch → Include Library → Add .ZIP Library, plus the dependency versions
listed in README/library.properties. The custom library is distributed as a ZIP;
it is not registered in the central Arduino Library Manager.

## Download evidence

- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json — HTTP 200, 1110 bytes, SHA-256 `f90a7328549ebd6a9fadb42a5956cd4ff2e39bb7c1782544d6e8e4c78e5aa426`
- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/manifest.json — HTTP 200, 2926 bytes, SHA-256 `28d18be536a744c148cba00b6cf90ec8d903d856e04b7f96fb977f8dc20a820e`
- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.1.0.zip — HTTP 200, 21223 bytes, SHA-256 `1872003287dae3f4f12e20e6727978430ba81923ad1f1c40043e051e3b412ebb`
- https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/logistic15u-rp2040-0.1.0.zip — HTTP 200, 11597 bytes, SHA-256 `f65db16b352b1aa291c5afa14e0b803bc46f6963a3a9685a1d5a686318ab33cb`
- https://codeload.github.com/Amornthep-ra/Logistic15U-PuppyBot6/zip/refs/heads/main — HTTP 200, 93999 bytes, SHA-256 `0007ed9ccafcb26560637ac94f7dacd997548ed0d29cfa856153acfe23159134`

Local evidence:

- `.arduino-test/public-url/install-public.log`
- `.arduino-test/public-url/url-results.json`
- `.arduino-test/public-url/arduino-cli.json`
- `.arduino-test/public-url/source/Logistic15U-PuppyBot6-main/`

This local report supersedes the pre-publication status text in the README,
PUBLISHING and VERIFICATION files at the tested commit. It has not been committed
or pushed to GitHub.
