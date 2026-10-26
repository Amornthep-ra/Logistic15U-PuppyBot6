# Servo fixes - 0.1.14-dev

Production code now lives in Logistic15U-PuppyBot6/src/PuppyBotServos.h, included by puppybot.h. The original failing cases are recorded in .arduino-test/servo-review/results.txt. The diff against the immediate backup is .arduino-test/servo-tests/puppybot.diff.

## Corrections

- Clock-derived 50 Hz PWM: at configured 150 MHz, divider 150, TOP 19999, 20,000 us period. Pulse conversion uses the same divider. Other startup clock values are tested; live clock changes are unsupported.
- Zero output on startup, no guessed initial angle. Each shared slice initializes once; repeat setup does not interrupt its outputs.
- Original source mapping retained: software channels 0..5, GPIO34..39. No physical wiring inferred.
- Last-command angle and active flag are independent per channel. getServoLastAngle is not measured position. The obsolete global current_degree_servo is removed.
- Invalid channel/angle/step/delay rejected before movement. Angles must be 0..180; -1 is not detach. Invalid commands preserve the previous valid output.
- control_servo uses the explicitly supplied start angle. Both helpers always command the endpoint, including uneven steps, either direction, equal endpoints and very large steps. No zero/negative-step loops. Still blocking on the calling core, with delays between commands only.
- servoStop clears just the selected output compare, effective at PWM wrap; the shared slice and neighbor remain active. Sending the same angle again resumes output. Pulse stop is neither a power cut nor a guarantee of holding load.
- setServoPulseRange accepts individual RAM-only measured endpoints while stopped. Validation only enforces a positive ascending range shorter than the PWM period. Defaults 300..3000 us are inherited source values, not calibrated robot limits. A default 90-degree command now gives 1650 us rather than the old approximately 1375 us. No mechanical lift angles were added.

## Verification

- python tests/servos/run.py: PASS at test clocks 125, 133, 150, 250, 300 MHz and 133333333 Hz. Compiles the production header against recording clock/PWM calls. Covers timing, no startup motion, independent state, stop/resume with a paired channel, repeated setup, endpoints, invalid/extreme input, custom pulse ranges, and untouched motor slices. Test pulse ranges are synthetic, not recommendations.
- python tests/encoders/run.py: all eight cases PASS.
- python tests/wait_sw1/run.py: diagnostics and calibration integration PASS.
- User sketch compiled with installed 0.1.14-dev for logistic15u:rp2040:logistic15u: 120100 bytes flash, 13336 bytes global RAM. Log: .arduino-test/compile-user-servo-fix.log.
- All installed/source headers match. Existing source headers other than puppybot.h are byte-identical to backup. IMU, Encoder, RGB and the user .ino/RobotSetup.h are unchanged. puppybot.h has only Servo extraction/removal edits.

No board upload, physical pulse measurement, Servo movement, GitHub push or release ZIP update. Backup: .arduino-test/before-servo-fix/. Usage: Logistic15U-PuppyBot6/SERVO_API.md.
