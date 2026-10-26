# Source, license and dependencies

Copyright (c) 2026 PrinceBot. This library is released under the MIT License;
see [LICENSE](LICENSE).

The PuppyBot API headers originate from PrinceBot's PuppyBot6-New board for
KBIDE, version 3.5.8, and are published here by PrinceBot under the same MIT
License.

The original InvenSense SDK, ICM20948wire1.h and HALwire1.h are **not distributed**.
PuppyBotImuDevice.h is an adapter to the separately installed SparkFun 9DoF IMU
Breakout - ICM 20948 - Arduino Library 1.3.2. SparkFun's own license and notices
apply to its separately downloaded library and DMP firmware.

Adafruit and PIO encoder dependencies are separately installed through Arduino
Library Manager under their own licenses. Arduino-Pico supplies the runtime,
Wire, SPI and Pico SDK. See library.properties for the exact tested dependency
versions.
