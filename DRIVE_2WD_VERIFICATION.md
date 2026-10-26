# 2WD verification — 0.1.15-dev

Date: 2026-10-21. Scope: remove the 4WD drive path for the user's confirmed 2WD robot. No GitHub publication or board upload.

## Result

- Drive commands, IMU PID variants (including encoder-based movement), front/back line PID and group stops use M1/M2 only.
- Removed drive-mode selection, 4WD-specific line APIs and Mecanum movement APIs. Migration instructions are in `Logistic15U-PuppyBot6/DRIVE_2WD.md`.
- Explicit auxiliary motor outputs remain available; they are not driven or stopped by chassis group commands.
- Installed Arduino library source and version match the project library.

## Checks

- `python tests/drive_2wd/run.py`: PASS. Executes extracted production movement/driver code with simulated time, IMU, encoder and sensor inputs; rejects writes outside M1/M2 pins. Covers forward/reverse, pivots/spins, IMU PID variants, line PID and helpers, stop commands, unsupported lateral states, and the motor(7) percentage conversion. Also checks removed identifiers are absent from active headers.
- `python tests/wait_sw1/run.py`: PASS.
- `python tests/encoders/run.py`: PASS.
- Arduino CLI compile of the user's `Logistic15U-PuppyBot6.ino`, FQBN `logistic15u:rp2040:logistic15u`: PASS. Flash 120388 bytes; global RAM 13444 bytes. Log: `.arduino-test/compile-user-2wd.log`.
- Compared against `.arduino-test/before-2wd-only/library`: every header other than the four drive-related headers is byte-identical, including Encoder, RGB and Servo code. IMU header differs only by deletion of the drive-mode variable and setter; sensor initialization and yaw processing are unchanged.
- `RobotSetup.h` is unchanged. No edits were made to the user sketch by this task. A concurrent change to comment placement/blank lines in setup was retained.
- `git -c core.whitespace=cr-at-eol diff --check`: PASS.

These checks establish compilation and software motor routing, not physical wiring, direction or PID accuracy. No motor was run. Existing encoder PID timeout limitations and calibration requirements remain unchanged.
