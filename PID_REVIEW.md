# PID review — 2026-10-22

Reviewed local development library 0.1.18-dev, matching the installed Arduino library. Scope: existing heading, turning, ramp, encoder movement and line-follow helpers. No production code, sketch, tuning, firmware or GitHub changes. The requested new heading-ramp API has not been implemented; its stop criterion remains undecided.

## Findings, ordered by priority

1. **P1 — Encoder movement can run indefinitely when feedback stops.** `PuppyBotTurnPID.h:326` loops only on pulse delta. There is no timeout, no stall check and no error return. A disconnected/frozen encoder leaves motor output active and prevents reaching ao(). The selected encoder is a single channel; initialization of two channels does not change this. Add bounded duration/stall handling and an explicit result; do not guess pulses-per-distance.
2. **P1 — Heading movement does not validate IMU availability or freshness.** `PuppyBotTurnPID.h:166`, `:223`, `:328`, `:473` consume Con_yaw_loop1 without checking startup or a timestamp. Selecting SW1 to continue after missing/failed IMU, or losing IMU frames after startup, does not prevent these functions from driving using zero/stale heading. The sketch's isIMUCalibrated() only guards the sensor update loop. Gate movement on a synchronized valid/fresh sample and stop on feedback loss. Startup Ready alone is not a runtime freshness check.
3. **P1 — shortestAngle is incorrect for multi-turn continuous yaw.** `PuppyBotTurnPID.h:29-31` uses fmod(target-current+540,360)-180. fmod can remain negative, so current=720,target=0 yields -360 instead of 0; current=720,target=90 yields -270 instead of +90. turnPID may choose the long/wrong rotation and never enter tolerance before its 2.5 s timeout. Normalize robustly for either sign and any number of turns.
4. **P1 — Shared heading/reset state is unsynchronized across cores.** `puppybot.h:97-101` defines ordinary float/uint8_t globals. User `.ino:51` writes Con_yaw_loop1 on core 1 while blocking PID loops read it on core 0; reset flags go the other direction. This is a C++ data race. An aligned hardware float load is not a synchronization contract, and volatile alone would not fix it. Publish heading, validity, sample sequence/timestamp coherently through atomics or a synchronized snapshot. This was identified from code; no physical/optimized-build race manifestation is claimed.
5. **P2 — Ramp output has a forced minimum and Slow=1 does not implement a smooth slowdown.** `PuppyBotTurnPID.h:201-202` clamps both motor commands to 10..100 after computing a ramp down to zero. Even speedBase=0/error=0 commands 10 until ao(). Separately, `:245-254` computes remainingMs*0.01 in the last 500 ms, giving at most 5, which is clamped back to 10. With speedBase=60 and zero error, output changes 60 -> 10 -> stop, not a ramp. Separate a real zero command from any experimentally measured minimum running duty and scale deceleration from the requested speed and chosen interval.
6. **P2 — D term kicks at startup and is not tied to new IMU samples.** `PuppyBotTurnPID.h:154,162,172-173` starts previous error at zero and differentiates on a tight loop with a minimum dt of 1 ms. In the reproduced example error=10 degrees and Kd=1 saturates the first ramp output to M1=100/M2=10 while the ramp base is still near zero. These example gains are not tuning recommendations. New IMU samples arriving less often than PID iterations produce a spike then repeated zero derivatives; wrapped error crossing +/-180 also needs care. Initialize derivative history from the first valid sample, use a defined update interval/fresh sample timestamp and handle angular derivative continuity.
7. **P2 — Crossing helper counts the same intersection repeatedly.** `KB_PIDFollowHelpers.h:91-93` increments whenever the threshold remains high, with only delay(50). One continuously occupied intersection satisfies crosses=3 in three loop iterations. Count the entry edge and require exit/re-arm before counting the next crossing, with a deliberate hysteresis/debounce policy.
8. **P2 — Timed movements use wrap-unsafe end-time comparison.** `PuppyBotTurnPID.h:152,159` and `:219,222` compare millis()<start+duration. On the board's 32-bit millisecond clock, crossing rollover can skip a requested move. Use unsigned elapsed time comparison; validate durations rather than silently converting arbitrary float input. This is a source-level finding, not part of the native harness reproduction (host unsigned long width can differ).

## Reproduction evidence

Native C++ harness extracted the production PID functions and front line-follow implementation into `.arduino-test/pid-review/ActualPID.h` and `ActualLine.h`. Clock, sensor inputs and motor output recording were simulated. Source: `.arduino-test/pid-review/review.cpp`; results: `.arduino-test/pid-review/results.txt`. No real motor outputs were used.

shortestAngle(current=720,target=0): -360 (equivalent heading should be 0)
shortestAngle(current=720,target=90): -270 (shortest error should be +90)
Ramp with base=0 and zero error/gains: first motor speed=10, last before ao=10
Ramp startup, target error=10, Kd=1: first M1=100, M2=10 (startup derivative kick)
Slow=1, speed=60, error=0: distinct M1 commands=60 10 then ao
Frozen encoder: external simulation watchdog after 10000 simulated ms; last motor=30, ao called=0
Single continuously occupied crossing: requested 3, helper exited after 3 line updates in 159 simulated ms
No IMU initialized in harness: timed PID sends 28 motor commands, first speed=30

The frozen-encoder case was stopped by an external harness watchdog after 10000 simulated milliseconds; the production routine did not stop itself. No hardware calibration or physical tracking accuracy is claimed. Previous 2WD tests verified motor routing and ordinary completion, not closed-loop accuracy or all of these fault scenarios.

## Recommended order

Fix feedback availability/freshness, cross-core publication and bounded stopping first; then angle normalization; then PID update timing and ramp mixing; then crossing edge counting and rollover. Keep real motor polarity, yaw sign, minimum motor duty, PID gains and encoder distance conversion as hardware verification items. No new constants for those have been invented. The new heading-ramp API should build on these fixes after choosing time and/or encoder termination.
