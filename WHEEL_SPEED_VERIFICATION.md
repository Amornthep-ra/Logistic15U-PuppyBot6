# Encoder wheel speed + line PD verification — 2026-10-22

## User measurement update — 2026-10-22

The user supplied M1 sign=-1, M2 sign=+1, both 4667.0 ticks/s; positive motor commands move both wheels forward. Battery during that measurement was 9.98–10 V. Applied these measurements to the user RobotSetup.h with referencePwm=30, matching the WheelSpeedMeasure example. Feedback now passes the configuration gate instead of using the missing-data fallback after the main sketch is uploaded.

The actual RobotSetup settings function was compiled with the production controller and replayed using a simulated M1-decreasing/M2-increasing pulse trace; the controller stays Running and interprets both as forward motion. This validates sign handling, not real loaded performance. Main sketch including IMU/setup1/loop1 and latest user PD values (30 / 0.05 / 1.40 / 400 / 10000) is byte-for-byte unchanged. README records the measured profile. No library defaults were changed to robot-specific measurements.

Arduino compile using the installed library: PASS, 131748 bytes flash / 15356 bytes global RAM. No firmware upload or GitHub publication performed. On-ground PI/PD tuning remains to be tested.


Version 0.1.21-dev. Implemented independent per-wheel PI speed feedback, measured pulse-rate reference settings, Front/Back line adapters, explicit Ready/Running/Stopped/fault status and telemetry. User confirmed Encoder 1 pairs with M1 and Encoder 2 with M2. No encoder signs, pulse rates, CPR/PPR, gearing or wheel sizes were supplied as robot facts.

## Runtime integration and deliberate missing-data behavior

RobotSetup.h holds the measured settings. encoderSign and referenceTicksPerSecond remain 0 for both wheels until measured, so feedback is **not active on the actual robot yet**. referencePwm=30 is the existing sketch command value, not an encoder calibration. Inner PI gains 0.20/0.50 and 20 ms sample timing are initial software tuning settings, not verified robot gains. The sketch's user-edited line settings 30 / 0.05 / 1.00 / 500 / 10000 are retained.

The existing Front-until-cross command shows LINE PD ONLY / ENCODER NOT SET when unconfigured. Once configured it uses ENCODER + LINE PD for the whole job. A fault while using encoder feedback stops both wheels without falling back mid-job. Direct Run_PID_Encoder / Run_PID_B_Encoder APIs reject unconfigured feedback instead of silently using the legacy controller. Stop/restart reset controller history without resetting encoder counters.

WheelSpeedMeasure is a separate, operator-triggered example to obtain sign and ticks/s with the wheels lifted. It runs one selected motor at +30% for a 250 ms warmup and approximately 1000 ms timed measurement, with SW1 cancellation, then stops. No physical measurement or example upload was performed. It measures a no-load starting reference, not confirmed loaded speed.

## Tests — PASS

Run `python -X utf8 tests/wheel_speed/run.py` from the repository. It tests real production headers and extracts the actual user Front-until-cross function from the sibling sketch directory:

- Independent motor targets with deliberately different pulse scales and polarities; no equality constraint between counters.
- Actual count deltas and actual dt, reverse motion, zero targets, explicit stop, re-baseline without resetting encoders, and first-step D behavior.
- Integral anti-windup, output limits, command validation, missing measured settings and invalid encoder assignment.
- Feedback watchdog, missing encoder pulses, wrong direction and latched faults stopping both outputs.
- Counter wrap and millis wrap.
- Deterministic simulated load reduction: M1 recovers toward its 300 ticks/s target while M2 retains its independent 450 ticks/s target. Fixture data only; not a real robot performance measurement.
- Actual user helper: visible unconfigured fallback, encoder-enabled crossing/timeout stop and encoder fault without silent fallback.

Existing `python -X utf8 tests/encoders/run.py` (eight scenarios) and `python -X utf8 tests/imu_startup/run.py` also pass.

## Source protection and compile

SHA-256 comparisons against `.arduino-test/before-wheel-speed-control/protected.json` confirm PuppyBotIMU.h, PuppyBotTurnPID.h, IMU startup/display, PuppyBotEncoders.h, PuppyBotServos.h and puppybot.h were not changed. User sketch loop/setup1/loop1 and existing Servo actions are unchanged. Source and installed copies match (`.arduino-test/wheel-speed-tests/installed-manifest.json`).

Arduino CLI with FQBN logistic15u:rp2040:logistic15u, Arduino-Pico 6.2.0:

| Sketch | Result | Flash bytes | Global RAM bytes |
|---|---|---:|---:|
| User Logistic15U-PuppyBot6.ino, installed library | PASS | 131748 | 15356 |
| WheelSpeedMeasure, installed library | PASS | 78824 | 11704 |

No motor/servo test on hardware, firmware upload, GitHub push or release publication was performed. IMU/RGB stay on the original core; encoder/motor control uses core 0 and performs no I2C, Flash writes or TFT redraw inside its feedback updates. Physical encoder measurement, direction/traction checks and line/PI retuning remain required before claiming an improvement on the real robot.
